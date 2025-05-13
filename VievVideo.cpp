#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
#include "players.h"
int previous[6] = { 0 };
int nextId = 1;
using namespace std;
using namespace cv;
int purpleAmount[6] = {0};
bool check = false;
Scalar bluemin = Scalar(90, 20, 40);
Scalar bluemax = Scalar(140, 255, 255);
Scalar purplemin = Scalar(0, 50, 105);
Scalar purplemax = Scalar(6, 190, 255);
Scalar orangemin = Scalar(6, 200, 200);
Scalar orangemax = Scalar(10, 255, 255);

vector <Point> blueCenter;
vector <Point> purpleCenter;
vector <Point> ballCenter;
vector <Player> players;

void locatePlayer(Mat img, Scalar low, Scalar high, Color color) {
    Mat mask;
    Mat purpleMask, orangeMask;
    inRange(img, low, high, mask);
    vector < vector < Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    imshow("mask", mask);
    // Position Tracking
    for (size_t i = 0; i < contours.size();++i) {
        Rect boundRect = boundingRect(contours[i]);
            //store the blue color center for the enemy team
            if (color == Color::Blue) {
                if (boundRect.width && boundRect.height > 2) {

                int px = boundRect.x + boundRect.width / 2;
                int py = boundRect.y + boundRect.height / 2;
                Point currentPlayerCenter(px, py);
                blueCenter.push_back(currentPlayerCenter);
                }

            }
            //store the purple color identifier center for id creation
            if (color == Color::Purple) {
                    int idx = boundRect.x + boundRect.width / 2;
                    int idy = boundRect.y + boundRect.height / 2;
                    Point currentPlayerIdCenter(idx, idy);
                    purpleCenter.push_back(currentPlayerIdCenter);
            }
            if (color == Color::Orange) {
                ballCenter.clear();
                int bdx = boundRect.x + boundRect.width / 2;
                int bdy = boundRect.y + boundRect.height / 2;
                Point currentBallCenter(bdx, bdy);
                ballCenter.push_back(currentBallCenter);
            }
            players.emplace_back(color, boundRect, boundRect.x + boundRect.width / 2, boundRect.y + boundRect.height / 2);
        }
}

void playerId(Mat img) {
    ofstream Positions("C:\\Users\\jbnlu\\Desktop\\positions.txt", ios::app);
    if (!Positions.is_open()) {
        cerr << "Failed to open file for writing." << endl;
        return; // Return early if file can't be opened
    }
    if (check == false) {
        for (size_t i = 0; i < blueCenter.size() && i < 6; ++i) {
            for (size_t j = 0; j < purpleCenter.size(); ++j) {
                double dist = norm(blueCenter[i] - purpleCenter[j]);
                if (dist < 10 && purpleAmount[i] == 0) {
                    // Assign unique ID
                    purpleAmount[i] = nextId;
                    nextId++;
                    cout << "id:" << purpleAmount[i] << ", " << blueCenter[i] << "," << endl;
                    Positions << "id:" << purpleAmount[i] << ", " << blueCenter[i] << "," << endl;
                    break; // no need to keep checking this one
                }
            }
        }
        check = true;
    }
if (check == true) {
        
        Positions << "Ball:" << ballCenter << endl;
        for (size_t i = 0; i < blueCenter.size() && i < 6; ++i) {
            Positions << "id:" << purpleAmount[i] << ", " << blueCenter[i] << "," << endl;
        }
        Positions.close();
    }
}

//Note that this will later be repurposed for trail creation
void drawPlayer(Mat img) {
    for (size_t i = 0; i < blueCenter.size(); ++i) {
        circle(img, blueCenter[i], 25, CV_RGB(255, 255, 255), 2);
    }

    if (!ballCenter.empty()) {
        circle(img, ballCenter[0], 10, CV_RGB(255, 255, 255), 2);
    }
    //if (!purpleCenter.empty()) {
      //  for (size_t j = 0; j < purpleCenter.size(); j++) {

          //  circle(img, purpleCenter[j], 10, CV_RGB(255, 100, 255, 2));
        //}
        
    //}
}


int main() {
    //clear the file if previously written in
    ofstream clearFile("C:\\Users\\jbnlu\\Desktop\\positions.txt");
    clearFile.close();
    //load the video file
    VideoCapture cap("C:\\Users\\jbnlu\\Pictures\\cut.MP4");
    
    // Check if file opened
    if (!cap.isOpened()) {
        cout << "Error opening video stream" << endl;
        return -1;
    }

    // default resolutions of the frame are obtained. Since default is system dependent.
    int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);
    // Define the codec and create VideoWriter object.The output is stored in specified file.
    VideoWriter video("C://Users//jbnlu//Desktop//Tracking_Bound.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, Size(frame_width, frame_height));

    while (1) {
        Mat frame;
        Mat copy;
        Mat mask;
        
        cap >> frame;
        //if frame doesnt exist, stop the program 
        if (frame.empty())
            break;

        frame.copyTo(copy);
        rectangle(frame, Point(0, 0), Point(1140, 70), (0, 0, 0), FILLED);
        rectangle(frame, Point(0, 640), Point(1140, 1800), (0, 0, 0), FILLED);
        cvtColor(frame, frame, COLOR_BGR2HSV);
        locatePlayer(frame, bluemin, bluemax, Color::Blue);
        locatePlayer(frame, purplemin, purplemax, Color::Purple);
        locatePlayer(frame, orangemin, orangemax, Color::Orange);
        playerId(frame);
        drawPlayer(copy);
        blueCenter.clear();
        purpleCenter.clear();
        //writes vieo file to earlier specified location.
        video.write(copy);
        // Display the resulting video
        imshow("Check", copy);
        // Press  ESC on keyboard to  exit
        char c = (char)waitKey(1);
        if (c == 27)
            break;
    }

    // When everything done, release the video capture and write object
    cap.release();
    video.release();

    // Closes all the frames
    destroyAllWindows();
    return 0;
}