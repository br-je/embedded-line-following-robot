//Initialise library
#include <iostream>
#include "opencv_aee.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;
using namespace std;

//Check whether the images are being uploaded (Reference arrows)
bool checkImageUploaded(const Mat& img, const string& name) {
    if (img.empty()) {
        cerr << "Error: Image '" << name << "' not loaded." << endl;
        return false;
    }
    return true;
}

int main(void)
{
    //Setup the camera
    setupCamera(320, 240);

    //Load the reference images and convert to grayscale so that compareImages() works
    Mat forwardSymbol = readImage("/home/pi/Documents/OpenCV-Object-Identification/forwards.png");
    Mat leftSymbol    = readImage("/home/pi/Documents/OpenCV-Object-Identification/left.png");
    Mat rightSymbol   = readImage("/home/pi/Documents/OpenCV-Object-Identification/right.png");

    //Check if images are uploaded - also included at the top of code
    if (!checkImageUploaded(forwardSymbol, "forwards.png") ||
        !checkImageUploaded(leftSymbol, "left.png") ||
        !checkImageUploaded(rightSymbol, "right.png")) {
        return -1;
    }

    //Convert images to grayscale for compatibility
    cvtColor(forwardSymbol, forwardSymbol, COLOR_BGR2GRAY);
    inRange(forwardSymbol, Scalar(35, 68, 57), Scalar(73, 255, 255), forwardSymbol);
    cvtColor(leftSymbol,    leftSymbol,    COLOR_BGR2GRAY);
    inRange(leftSymbol, Scalar(35, 68, 57), Scalar(73, 255, 255), leftSymbol);
    cvtColor(rightSymbol,   rightSymbol,   COLOR_BGR2GRAY);
    inRange(rightSymbol, Scalar(35, 68, 57), Scalar(73, 255, 255), rightSymbol);

    //Equalize contrast
    equalizeHist(forwardSymbol, forwardSymbol);
    equalizeHist(leftSymbol, leftSymbol);
    equalizeHist(rightSymbol, rightSymbol);

    // Show the grayscale version of forwards.png in a new window
    imshow("Grayscale Forward Symbol", forwardSymbol);

    // Define the HSV threshold values for blue (FIND USING HSVPI TOOL).
    Scalar lowerBlue(40, 100, 40);
    Scalar upperBlue(140, 255, 255);

    //Define a matching threshold (percentage returned by compareImages)
    const float MATCH_THRESHOLD = 70;

    cout << "Symbol detector started. Press ESC in any window to exit." << endl;

    while (true)
    {
        //Capture and (if needed) rotate the frame.
        Mat frame = captureFrame();
        if(frame.empty())
            continue;
        Mat rotatedFrame;
        rotate(frame, rotatedFrame, ROTATE_180);

        //Convert frame to HSV
        Mat hsvFrame;
        cvtColor(rotatedFrame, hsvFrame, COLOR_BGR2HSV);

        //Create a mask from the Blue colour range
        Mat mask;
        inRange(hsvFrame, lowerBlue, upperBlue, mask);

        //Find contours in the mask (RETR_EXTERNAL to find the outer border)
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        //Draw the contours on the rotated frame
        drawContours(rotatedFrame, contours, -1, Scalar(0, 255, 0), 2);

        //Variable to hold result; by default no valid symbol is detected.
        string symbolResult = "No symbol detected";
        float forwardMatch = 0.0, leftMatch = 0.0, rightMatch = 0.0;

        //Declare transformedGray within scope !!!
        Mat transformedGray;

        //Process each contour to look for a quadrilateral we can check.
        for (size_t i = 0; i < contours.size(); i++)
        {
            // Skip small contours to reduce noise.
            if (contourArea(contours[i]) < 10)
                continue;

            // Approximate the contour to reduce vertices.
            vector<Point> approx;
            double peri = arcLength(contours[i], true);
            approxPolyDP(contours[i], approx, 0.02 * peri, true);

            // We require exactly 4 vertices for a valid symbol border.
            if (approx.size() == 4)
            {
                // Use transformPerspective() from opencv_aee to unwarp the image.
                // We use the dimensions of one reference image for the target size.
                Mat transformed = transformPerspective(approx, rotatedFrame, forwardSymbol.cols, forwardSymbol.rows);
                if (transformed.empty())
                    continue;

                // Convert transformed image to grayscale for comparison.
                cvtColor(transformed, transformedGray, COLOR_BGR2GRAY);
                inRange(transformed, Scalar(35, 68, 57), Scalar(73, 255, 255), transformed);

                // Resize transformedGray to match forwardSymbol dimensions
                resize(transformedGray, transformedGray, forwardSymbol.size());

                equalizeHist(transformedGray, transformedGray);
                // Compare the transformed image with each reference symbol.
                forwardMatch = compareImages(transformedGray, forwardSymbol);
                leftMatch    = compareImages(transformedGray, leftSymbol);
                rightMatch   = compareImages(transformedGray, rightSymbol);

                // Determine which one gives the best match above the threshold.
                if (forwardMatch >= MATCH_THRESHOLD && forwardMatch >= leftMatch && forwardMatch >= rightMatch)
                {
                    symbolResult = "Forward symbol detected";
                    break; // Found a match   exit the loop.
                }
                else if (leftMatch >= MATCH_THRESHOLD && leftMatch >= forwardMatch && leftMatch >= rightMatch)
                {
                    symbolResult = "Left symbol detected";
                    break;
                }
                else if (rightMatch >= MATCH_THRESHOLD && rightMatch >= forwardMatch && rightMatch >= leftMatch)
                {
                    symbolResult = "Right symbol detected";
                    break;
                }
            }
        }

        // Show the normal view
        imshow("Normal View", rotatedFrame);

        // Only show the transformed image if it is not empty
        if (!transformedGray.empty()) {
            imshow("Transformed Image", transformedGray);
        } else {
            cerr << "Transformed image is empty!" << endl;
        }

        // Print the result and match percentages to the terminal.
        cout << symbolResult << endl;
        cout << "Forward match: " << forwardMatch << "%" << endl;
        cout << "Left match: " << leftMatch << "%" << endl;
        cout << "Right match: " << rightMatch << "%" << endl;

        cout << "Transformed Image Size: " << transformedGray.size() << endl;
        cout << "Forward Symbol Size: " << forwardSymbol.size() << endl;

        //Wait 0.25 seconds between outputs for readability
        waitKey(250);

        // Wait briefly (100 ms) and exit on ESC key press (if a window is open).
        int key = waitKey(100);
        if (key == 27)   // 27 is the ASCII code for ESC
            break;
    }

    // Clean up.
    closeCV();

    return 0;
}
