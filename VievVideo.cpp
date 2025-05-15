#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <zmq.hpp>
#include "players.h"
#include <string_view>

int previous[4] = { 0 };
int nextId = 1;
using namespace std;
using namespace cv;
using namespace zmq;

int purpleAmount[4] = { 0 };
bool check = false;
Scalar bluemin = Scalar(112, 103, 50);
Scalar bluemax = Scalar(125, 255, 255);
Scalar purplemin = Scalar(148, 94, 45);
Scalar purplemax = Scalar(176, 255, 255);
Scalar orangemin = Scalar(5, 150, 70);
Scalar orangemax = Scalar(15, 255, 255);

vector <Point> blueCenter;
vector <Point> purpleCenter;
vector <Point> ballCenter;
vector <Player> players;

/*sendData: communicates with the server to continously send each robot's coordinates frame by frame. recieves back the prediction
response. Uses the response to compare predicted value to true value and calculate accuracy*/
void sendData(const int numberOfPlayers) {
    context_t ctx;
    socket_t sock(ctx, socket_type::req);
    sock.connect("tcp://10.132.174.117:5555");

    static map<string, Point2f> previousPositions;
    static int totalPredictions = 0;
    static int correctPredictions = 0;
    const float threshold = 0.05f;

    for (size_t i = 0; i < blueCenter.size() && i < 4; ++i) {
        float norm_x = -0.5f + blueCenter[i].x / 1000.0f;
        float norm_y = 0.8f - blueCenter[i].y / 1000.0f;

        stringstream messageData;
        messageData << "agent_blue_" << purpleAmount[i] << " " << norm_x << " " << norm_y << "\n";

        // send
        sock.send(buffer(messageData.str()), send_flags::none);
        cout << "Sent message:" << messageData.str() << endl;

        // recieve
        zmq::message_t reply;
        sock.recv(reply, zmq::recv_flags::none);
        string replyStr(static_cast<char*>(reply.data()), reply.size());

        float pred_x, pred_y;
        if (sscanf(replyStr.c_str(), "{\"next_x\":%f,\"next_y\":%f}", &pred_x, &pred_y) == 2) {
            cout << "Prediction for " << robot_id << ": (" << pred_x << ", " << pred_y << ")" << endl;

            // check against the last recorded true position (i.e. current frame becomes "next" in next loop)
            // if there is no known past position (we're on frame one) then we skip the error calculation and initalize it below
            if (previousPositions.count(robot_id)) {
                float true_x = norm_x;
                float true_y = norm_y;

                float dx = pred_x - true_x;
                float dy = pred_y - true_y;
                float dist = sqrt(dx * dx + dy * dy);

                cout << "Actual: (" << true_x << ", " << true_y << ") → Error: " << dist << endl;

                ++totalPredictions;
                if (dist <= threshold) ++correctPredictions;

                float accuracy = 100.0f * correctPredictions / totalPredictions;
                cout << "Current accuracy: " << accuracy << "%\n" << endl;
            }

            // update latest known position
            previousPositions[robot_id] = Point2f(norm_x, norm_y);
        }
        else {
            cerr << "Failed to parse prediction: " << replyStr << endl;
        }
    }    
}
void locatePlayer(Mat img, Scalar low, Scalar high, Color color) {
    Mat mask;
    Mat purpleMask, orangeMask;
    inRange(img, low, high, mask);
    vector < vector < Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // Position Tracking
    for (size_t i = 0; i < contours.size();++i) {
        Rect boundRect = boundingRect(contours[i]);
        //store the blue color center for the enemy team
        if (color == Color::Blue) {
            int px = boundRect.x + boundRect.width / 2;
            int py = boundRect.y + boundRect.height / 2;
            Point currentPlayerCenter(px, py);
            blueCenter.push_back(currentPlayerCenter);


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
        return; // return early if file can't be opened
    }
    if (check == false) {
        for (size_t i = 0; i < blueCenter.size() && i < 4; ++i) {
            for (size_t j = 0; j < purpleCenter.size(); ++j) {
                double dist = norm(blueCenter[i] - purpleCenter[j]);
                if (dist < 25 && purpleAmount[i] == 0) {
                    // assign unique ID

                    purpleAmount[i] = nextId;
                    nextId++;
                    cout << "id:" << purpleAmount[i] << "," << -0.5 + blueCenter[i].x / 1000 << "," << endl;
                    break; // no need to keep checking this one
                }
            }
        }
        check = true;
    }if (check == true) {
        Positions << "Ball " << ballCenter << endl;
        for (size_t i = 0; i < blueCenter.size() && i < 4; ++i) {
            float x = blueCenter[i].x;
            float y = blueCenter[i].y;
            Positions << "agent_blue_" << purpleAmount[i] << " " << -0.5 + x / 1000 << " " << 0.8 - y / 1000 << endl;
        }
        Positions.close();
    }
}


//Drawing the bounding boxes for every player for visual clarifaction
void drawPlayer(Mat img) {
    for (size_t i = 0; i < blueCenter.size(); ++i) {
        circle(img, blueCenter[i], 25, CV_RGB(255, 255, 255), 2);
    }

    if (!ballCenter.empty()) {
        circle(img, ballCenter[0], 10, CV_RGB(255, 255, 255), 2);
    }
}


int main() {
    ofstream clearFile("C:\\Users\\jbnlu\\Desktop\\positions.txt");
    clearFile.close();
    //load the video file
    VideoCapture cap("C:\\Users\\jbnlu\\Pictures\\run2_2.MP4");

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
        locatePlayer(frame, orangemin, orangemax, Color::Orange);
        playerId(frame);
        drawPlayer(copy);
        sendData(4);
        blueCenter.clear();
        purpleCenter.clear();
        //writes vieo file to earlier specified location.
        video.write(copy);
        // display the resulting video

        imshow("Check", copy);
        // Press  ESC on keyboard to  exit
        char c = (char)waitKey(1);
        if (c == 27)
            break;
    }

    //wWhen everything done, release the video capture and write object
    cap.release();
    video.release();

    // closes all the frames
    destroyAllWindows();
    return 0;
}