# SFND 3D Object Tracking

Welcome to the final project of the camera course. By completing all the lessons, you now have a solid understanding of keypoint detectors, descriptors, and methods to match them between successive images. Also, you know how to detect objects in an image using the YOLO deep-learning framework. And finally, you know how to associate regions in a camera image with Lidar points in 3D space. Let's take a look at our program schematic to see what we already have accomplished and what's still missing.

<img src="images/course_code_structure.png" width="779" height="414" />

In this final project, you will implement the missing parts in the schematic. To do this, you will complete four major tasks: 
1. First, you will develop a way to match 3D objects over time by using keypoint correspondences. 
2. Second, you will compute the TTC based on Lidar measurements. 
3. You will then proceed to do the same using the camera, which requires to first associate keypoint matches to regions of interest and then to compute the TTC based on those matches. 
4. And lastly, you will conduct various tests with the framework. Your goal is to identify the most suitable detector/descriptor combination for TTC estimation and also to search for problems that can lead to faulty measurements by the camera or Lidar sensor. In the last course of this Nanodegree, you will learn about the Kalman filter, which is a great way to combine the two independent TTC measurements into an improved version which is much more reliable than a single sensor alone can be. But before we think about such things, let us focus on your final project in the camera course. 

## Dependencies for Running Locally
* cmake >= 2.8
  * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1 (Linux, Mac), 3.81 (Windows)
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* Git LFS
  * Weight files are handled using [LFS](https://git-lfs.github.com/)
* OpenCV >= 4.1
  * This must be compiled from source using the `-D OPENCV_ENABLE_NONFREE=ON` cmake flag for testing the SIFT and SURF detectors.
  * The OpenCV 4.1.0 source code can be found [here](https://github.com/opencv/opencv/tree/4.1.0)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools](https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory in the top level project directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./3D_object_tracking`.

## Reflection

### FP.5 Performance Evaluation 1
The TTC computation using Lidar information was stabler than the one using camera in my case. Taking the computation result between every 2 frames were not change too dramatically comparing to the result from camera, and there's no bad result in the sequence until frames after 40. The reason was that it fails frequently in the later frames while detecting the bounding box for the ego-front car. Therefore, under the condition of the bounding box were detected or selected correctly, using Lidar as active sensor to calculate TTC would be a better choice. (It'd be even better if we can compare the computation result with the ground truth for more accurate conclusion.)

### FP.6 Performance Evaluation 2
The FAST/BRIEF combination was the fastest pair of detector/descriptor that also generates relatively more keypoints, from the performance evalutation in midtern project. In the following table, we can see the TTC result from camera was relatively stable (left side of the table), comparing to an unreliable combination as HARRIS/ORB (right side of the table).

| Frame | Combination | TTC Lidar | TTC Camera | Combination | TTC Lidar | TTC Camera |
|---|---|---|---|---|---|---|
|0/1|FAST/BRIEF| 12.5156 | 6.74766 | HARRIS/ORB | 12.5156 | 10.9082 |
|1/2|FAST/BRIEF| 12.6142 | 9.19418 | HARRIS/ORB | 12.6142 | 0.0414423 |
|2/3|FAST/BRIEF| 14.091 | 12.0187 | HARRIS/ORB | 14.091 | -80.8525 |
|3/4|FAST/BRIEF| 16.6894 | 8.31341 | HARRIS/ORB | 16.6894 | 11.3951 |
|4/5|FAST/BRIEF| 15.9082 | 13.0535 | HARRIS/ORB | 15.9082 | 3.66418 |
|5/6|FAST/BRIEF| 12.6787 | 9.57154 | HARRIS/ORB | 12.6787 | 0.0409057 |
|6/7|FAST/BRIEF| 11.9844 | 5.1632 | HARRIS/ORB | 11.9844 | 10.8714 |
|7/8|FAST/BRIEF| 13.1241 | 11.3055 | HARRIS/ORB | 13.1241 | 0.0275273 |
|8/9|FAST/BRIEF| 13.0241 | 11.4644 | HARRIS/ORB | 13.0241 | 0.0566185 |
|9/10|FAST/BRIEF| 11.1746 | 6.65905 | HARRIS/ORB | 11.1746 | 0.102627 |
|10/11|FAST/BRIEF| 12.8086 | 11.2613 | HARRIS/ORB | 12.8086 | 0.0295838 |
|11/12|FAST/BRIEF| 8.95978 | 6.48223 | HARRIS/ORB | 8.95978 | 5.05076 |
|12/13|FAST/BRIEF| 9.96439 | 10.9806 | HARRIS/ORB | 9.96439 | 0.240059 |
|13/14|FAST/BRIEF| 9.59863 | 8.4418 | HARRIS/ORB | 9.59863 | 0.0814819 |
|14/15|FAST/BRIEF| 8.57352 | 9.25435 | HARRIS/ORB | 8.57352 | -14.4168 |
|15/16|FAST/BRIEF| 9.51617 | 9.99493 | HARRIS/ORB | 9.51617 | 5.81679 |
|16/17|FAST/BRIEF| 9.54658 | 7.36621 | HARRIS/ORB | 9.54658 | 0.0473595 |
|17/18|FAST/BRIEF| 8.3988 | 7.1991 | HARRIS/ORB | 8.3988 | 0.02294 |
