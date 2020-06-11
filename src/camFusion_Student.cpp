
#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "camFusion.hpp"
#include "dataStructures.h"

using namespace std;

bool compareLidarPointDistance(LidarPoint a, LidarPoint b) { return a.x < b.x; }

// Create groups of Lidar points whose projection into the camera falls into the same bounding box
void clusterLidarWithROI(std::vector<BoundingBox> &boundingBoxes, std::vector<LidarPoint> &lidarPoints, float shrinkFactor, cv::Mat &P_rect_xx, cv::Mat &R_rect_xx, cv::Mat &RT)
{
    // loop over all Lidar points and associate them to a 2D bounding box
    cv::Mat X(4, 1, cv::DataType<double>::type);
    cv::Mat Y(3, 1, cv::DataType<double>::type);

    for (auto it1 = lidarPoints.begin(); it1 != lidarPoints.end(); ++it1)
    {
        // assemble vector for matrix-vector-multiplication
        X.at<double>(0, 0) = it1->x;
        X.at<double>(1, 0) = it1->y;
        X.at<double>(2, 0) = it1->z;
        X.at<double>(3, 0) = 1;

        // project Lidar point into camera
        Y = P_rect_xx * R_rect_xx * RT * X;
        cv::Point pt;
        pt.x = Y.at<double>(0, 0) / Y.at<double>(0, 2); // pixel coordinates
        pt.y = Y.at<double>(1, 0) / Y.at<double>(0, 2);

        vector<vector<BoundingBox>::iterator> enclosingBoxes; // pointers to all bounding boxes which enclose the current Lidar point
        for (vector<BoundingBox>::iterator it2 = boundingBoxes.begin(); it2 != boundingBoxes.end(); ++it2)
        {
            // shrink current bounding box slightly to avoid having too many outlier points around the edges
            cv::Rect smallerBox;
            smallerBox.x = (*it2).roi.x + shrinkFactor * (*it2).roi.width / 2.0;
            smallerBox.y = (*it2).roi.y + shrinkFactor * (*it2).roi.height / 2.0;
            smallerBox.width = (*it2).roi.width * (1 - shrinkFactor);
            smallerBox.height = (*it2).roi.height * (1 - shrinkFactor);

            // check wether point is within current bounding box
            if (smallerBox.contains(pt))
            {
                enclosingBoxes.push_back(it2);
            }

        } // eof loop over all bounding boxes

        // check wether point has been enclosed by one or by multiple boxes
        if (enclosingBoxes.size() == 1)
        { 
            // add Lidar point to bounding box
            enclosingBoxes[0]->lidarPoints.push_back(*it1);
        }

    } // eof loop over all Lidar points
}


void show3DObjects(std::vector<BoundingBox> &boundingBoxes, cv::Size worldSize, cv::Size imageSize, bool bWait)
{
    // create topview image
    cv::Mat topviewImg(imageSize, CV_8UC3, cv::Scalar(255, 255, 255));

    for(auto it1=boundingBoxes.begin(); it1!=boundingBoxes.end(); ++it1)
    {
        // create randomized color for current 3D object
        cv::RNG rng(it1->boxID);
        cv::Scalar currColor = cv::Scalar(rng.uniform(0,150), rng.uniform(0, 150), rng.uniform(0, 150));

        // plot Lidar points into top view image
        int top=1e8, left=1e8, bottom=0.0, right=0.0; 
        float xwmin=1e8, ywmin=1e8, ywmax=-1e8;
        for (auto it2 = it1->lidarPoints.begin(); it2 != it1->lidarPoints.end(); ++it2)
        {
            // world coordinates
            float xw = (*it2).x; // world position in m with x facing forward from sensor
            float yw = (*it2).y; // world position in m with y facing left from sensor
            xwmin = xwmin<xw ? xwmin : xw;
            ywmin = ywmin<yw ? ywmin : yw;
            ywmax = ywmax>yw ? ywmax : yw;

            // top-view coordinates
            int y = (-xw * imageSize.height / worldSize.height) + imageSize.height;
            int x = (-yw * imageSize.width / worldSize.width) + imageSize.width / 2;

            // find enclosing rectangle
            top = top<y ? top : y;
            left = left<x ? left : x;
            bottom = bottom>y ? bottom : y;
            right = right>x ? right : x;

            // draw individual point
            cv::circle(topviewImg, cv::Point(x, y), 4, currColor, -1);
        }

        // draw enclosing rectangle
        cv::rectangle(topviewImg, cv::Point(left, top), cv::Point(right, bottom),cv::Scalar(0,0,0), 2);

        // augment object with some key data
        char str1[200], str2[200];
        sprintf(str1, "id=%d, #pts=%d", it1->boxID, (int)it1->lidarPoints.size());
        putText(topviewImg, str1, cv::Point2f(left-250, bottom+50), cv::FONT_ITALIC, 2, currColor);
        sprintf(str2, "xmin=%2.2f m, yw=%2.2f m", xwmin, ywmax-ywmin);
        putText(topviewImg, str2, cv::Point2f(left-250, bottom+125), cv::FONT_ITALIC, 2, currColor);  
    }

    // plot distance markers
    float lineSpacing = 2.0; // gap between distance markers
    int nMarkers = floor(worldSize.height / lineSpacing);
    for (size_t i = 0; i < nMarkers; ++i)
    {
        int y = (-(i * lineSpacing) * imageSize.height / worldSize.height) + imageSize.height;
        cv::line(topviewImg, cv::Point(0, y), cv::Point(imageSize.width, y), cv::Scalar(255, 0, 0));
    }

    // display image
    string windowName = "3D Objects";
    cv::namedWindow(windowName, 1);
    cv::imshow(windowName, topviewImg);

    if(bWait)
    {
        cv::waitKey(0); // wait for key to be pressed
    }
}


// associate a given bounding box with the keypoints it contains
void clusterKptMatchesWithROI(BoundingBox &boundingBox, std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, std::vector<cv::DMatch> &kptMatches)
{
    double totalDistance = 0.0, mean, variance = 0.0, stddev, upperBound, lowerBound;
    std::vector<cv::DMatch> enclosedMatches, result;

    // Loop through the keypoints in roi and compute the sum of the x distances
    for (auto match : kptMatches)
    {
        int prevKeypointIdx = match.queryIdx;
        cv::Point prevKeypoint = kptsPrev.at(prevKeypointIdx).pt;
        if (boundingBox.roi.contains(prevKeypoint))
        {
            enclosedMatches.push_back(match);
            totalDistance += prevKeypoint.x;
        }
    }

    // Compute the mean and standard deviation
    mean = totalDistance / enclosedMatches.size();
    for (auto match : enclosedMatches)
    {
        int prevKeypointIdx = match.queryIdx;
        cv::Point prevKeypoint = kptsPrev.at(prevKeypointIdx).pt;
        variance += std::pow(prevKeypoint.x - mean, 2);
    }
    stddev = std::sqrt(variance / (enclosedMatches.size() - 1));
    upperBound = mean + 2 * stddev;
    lowerBound = mean - 2 * stddev;

    // Keep the keypoint that locates within 2 standard deviation
    for (auto match : enclosedMatches)
    {
        int prevKeypointIdx = match.queryIdx;
        cv::Point prevKeypoint = kptsPrev.at(prevKeypointIdx).pt;
        if (prevKeypoint.x >= lowerBound && prevKeypoint.x <= upperBound)
            result.push_back(match);
    }

    boundingBox.kptMatches = result;
}


// Compute time-to-collision (TTC) based on keypoint correspondences in successive images
void computeTTCCamera(std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, 
                      std::vector<cv::DMatch> kptMatches, double frameRate, double &TTC, cv::Mat *visImg)
{
    // stores the distance ratios for all keypoints between current and previous frame
    vector<double> distRatios; 
    for (auto it1 = kptMatches.begin(); it1 != kptMatches.end() - 1; ++it1)
    {
        // get current keypoint and its matched partner in the previous frame
        cv::KeyPoint kpOuterCurr = kptsCurr.at(it1->trainIdx);
        cv::KeyPoint kpOuterPrev = kptsPrev.at(it1->queryIdx);

        for (auto it2 = kptMatches.begin() + 1; it2 != kptMatches.end(); ++it2)
        {
            // minimum required distance
            double minDist = 100.0; 

            // get next keypoint and its matched partner in the prev. frame
            cv::KeyPoint kpInnerCurr = kptsCurr.at(it2->trainIdx);
            cv::KeyPoint kpInnerPrev = kptsPrev.at(it2->queryIdx);

            // compute distances and distance ratios
            double distCurr = cv::norm(kpOuterCurr.pt - kpInnerCurr.pt);
            double distPrev = cv::norm(kpOuterPrev.pt - kpInnerPrev.pt);

            if (distPrev > std::numeric_limits<double>::epsilon() && distCurr >= minDist)
            {
                // avoid division by zero
                double distRatio = distCurr / distPrev;
                distRatios.push_back(distRatio);
            }
        }
    }

    // only continue if list of distance ratios is not empty
    if (distRatios.size() == 0)
    {
        TTC = std::numeric_limits<double>::quiet_NaN();
        return;
    }

    double dT = 1 / frameRate;
    std::sort(distRatios.begin(), distRatios.end());
    int medianIdx = floor(distRatios.size() / 2.0);
    double medianDistRatio = medianIdx % 2 == 1 ? distRatios[medianIdx] : (distRatios[medianIdx] + distRatios[medianIdx + 1]) / 2.0;
    TTC = -dT / (1 - medianDistRatio);
}


void computeTTCLidar(std::vector<LidarPoint> &lidarPointsPrev,
                     std::vector<LidarPoint> &lidarPointsCurr, double frameRate, double &TTC)
{
    std::sort(lidarPointsPrev.begin(), lidarPointsPrev.end(), compareLidarPointDistance);
    std::sort(lidarPointsCurr.begin(), lidarPointsCurr.end(), compareLidarPointDistance);
    double prevMedianX = lidarPointsPrev[lidarPointsPrev.size() / 2].x;
    double currMedianX = lidarPointsCurr[lidarPointsCurr.size() / 2].x;
    double dT = 1.0 / frameRate;
    TTC = currMedianX * dT / (prevMedianX - currMedianX);
}


void matchBoundingBoxes(std::vector<cv::DMatch> &matches, std::map<int, int> &bbBestMatches, DataFrame &prevFrame, DataFrame &currFrame)
{
    for (auto prevBoundingBox : prevFrame.boundingBoxes)
    {
        vector<cv::DMatch> enclosedMatches;
        for (auto match : matches)
        {
            int prevKeypointIdx = match.queryIdx;
            cv::Point prevKeypoint = prevFrame.keypoints.at(prevKeypointIdx).pt;

            if (prevBoundingBox.roi.contains(prevKeypoint))
            {
                enclosedMatches.push_back(match);
            }
        }

        // Count the number of keypoints that enclosed in the current frame bounding box
        std::map<int, int> bbMatches;
        int maxMatchCount = 0, maxMatchBBId = -1;
        for (auto match : enclosedMatches)
        {
            int currKeypointIdx = match.trainIdx;
            cv::Point currKeypoint = currFrame.keypoints.at(currKeypointIdx).pt;
            for (auto currBoundingBox : currFrame.boundingBoxes)
            {
                if (currBoundingBox.roi.contains(currKeypoint))
                {
                    if (bbMatches.find(currBoundingBox.boxID) != bbMatches.end()) bbMatches[currBoundingBox.boxID] += 1;
                    else bbMatches[currBoundingBox.boxID] = 1;
                    if (maxMatchCount < bbMatches[currBoundingBox.boxID])
                    {
                        maxMatchCount = bbMatches[currBoundingBox.boxID];
                        maxMatchBBId = currBoundingBox.boxID;
                    }
                }
            }
        }

        // cout << "Best matches: prevId: " << prevBoundingBox.boxID << ", curId: " << maxMatchBBId << ", with matches: " << maxMatchCount << endl;

        if (maxMatchCount > 0)
        {
            bbBestMatches.insert({prevBoundingBox.boxID, maxMatchBBId});
        }
    }
}
