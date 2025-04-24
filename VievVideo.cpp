#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
#include "players.h"

using namespace std;
using namespace cv;
int purpleAmount[4] = {0};
bool check = false;
Scalar bluemin = cv::Scalar(102, 103, 102);
Scalar bluemax = cv::Scalar(125, 255, 255);
Scalar purplemin = cv::Scalar(138, 94, 105);
Scalar purplemax = cv::Scalar(175.95, 255, 255);


vector <Point> prior(4);
vector <Point> currentBlue(4);
vector <Point> blueCenter;
vector <Point> purpleCenter;
vector <Player> players;

void locatePlayer(Mat img, Scalar low, Scalar high, Color color) {
    Mat mask;
    inRange(img, low, high, mask);
    vector < vector < Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // Position Tracking
    for (size_t i = 0; i < contours.size();++i) {
        Rect boundRect = boundingRect(contours[i]);
            //store the blue color center for the enemy team
            if (color == Color::Blue) {
                int px = boundRect.x + boundRect.width / 2;
                int py = boundRect.y + boundRect.width / 2;
                Point currentPlayerCenter(px, py);
                blueCenter.push_back(currentPlayerCenter);
               

            }
            //store the purple color identifier center for id creation
            if (color == Color::Purple) {
                int idx = boundRect.x + boundRect.width / 2;
                int idy = boundRect.y + boundRect.width / 2;
                Point currentPlayerIdCenter(idx, idy);
                purpleCenter.push_back(currentPlayerIdCenter);
               
            }
            players.emplace_back(color, boundRect, boundRect.x + boundRect.width / 2, boundRect.y + boundRect.height / 2);
        }
}

void playerId(Mat img) {
    if (check == false) {
        for (size_t i = 0; i < blueCenter.size() && i < 4; ++i) {
            cout << prior << endl;
            for (size_t j = 0; j < purpleCenter.size(); ++j) {
                double dist = norm(blueCenter[i] - purpleCenter[j]);
                if (dist < 25) {
                    purpleAmount[i]++; // increment count
                    cout << "id:" << purpleAmount[i] << ", " << blueCenter[i] << "," << endl;
                }
            }
        }
        check = true;
    }
    if (check == true) {
        for (size_t i = 0; i < blueCenter.size() && i < 4; ++i) {
               //creates and opens a file that will be used for data
                ofstream Positions("C:\\Users\\jbnlu\\Desktop\\positions.txt", ios::app);
                Positions << "id:" << purpleAmount[i] << ", " << blueCenter[i] << "," << endl;
                Positions.close();
                
           
        }
    
    }
}

//Drawing the bounding boxes for every player for visual clarifaction
//Note that this will later be repurposed for trail creation
void drawPlayer(Mat img) {
    for (size_t i = 0; i < blueCenter.size(); i++) {
       switch (players[i].color) {
       case Blue: circle(img, blueCenter[i], 5, CV_RGB(255, 255, 255), 2);
           break;
       //case Purple: rectangle(img, players[i].rect.tl(), players[i].rect.br(), CV_RGB(95, 51, 91), 2);
         //  break;
       }
   }
}

int main() {
    //clear the file if previously written in
    ofstream clearFile("C:\\Users\\jbnlu\\Desktop\\positions.txt");
    clearFile.close();
    //load the video file
    VideoCapture cap("C:\\Users\\jbnlu\\Pictures\\run2_2.mp4");
    
    // Check if file opened
    if (!cap.isOpened()) {
        cout << "Error opening video stream" << endl;
        return -1;
    }

    // Default resolutions of the frame are obtained. Since default is system dependent.
    int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);
    // Define the codec and create VideoWriter object.The output is stored in specified file.
    VideoWriter video("C://Users//jbnlu//Desktop//Tracking_Bound.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, Size(frame_width, frame_height));

    while (1) {
        Mat frame;
        Mat copy;
        Mat mask;

        cap >> frame;
        //if frame doesnt exist, stop the program (debug tool)
        if (frame.empty())
            break;

        // Lowers the framerate. Not needed but makes it easier to see.
        frame.copyTo(copy);

        cvtColor(frame, frame, COLOR_BGR2HSV);
        locatePlayer(frame, bluemin, bluemax, Color::Blue);
        locatePlayer(frame, purplemin, purplemax, Color::Purple);
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