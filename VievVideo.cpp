#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
#include "players.h"
#include <zmq.hpp>
#include <string>
#include <nlohmann/json.hpp> 

int previous[4] = { 0 };
int nextId = 1;
using namespace std; //helps to use short comands. 
using namespace cv;
using namespace zmq;
using json = nlohmann::json;


int yellowAmount[4] = {0};
bool check = false;
Scalar bluemin = Scalar(112, 103, 50);
Scalar bluemax = Scalar(125, 255, 255);
Scalar purplemin = Scalar(148, 94, 45);
Scalar purplemax = Scalar(175.95, 255, 255);
Scalar orangemin = Scalar(5, 150, 70);
Scalar orangemax = Scalar(15, 255, 255);
Scalar yellowmin = Scalar(20, 100, 100);
Scalar yellowmax = Scalar(35, 255, 255);

vector <Point> blueCenter;
vector <Point> yellowCenter;
vector <Point> purpleCenter;
vector <Point> ballCenter;
vector <Player> players;

void sendData(const int numberOfPlayers) {
    context_t ctx;
    socket_t sock(ctx, socket_type::req);
    sock.connect("tcp://127.0.0.1:5555");

    json all_data = json::array();  // JSON-array

    for (size_t i = 0; i < blueCenter.size() && i < 4; ++i) {
        float x = -0.5f + blueCenter[i].x / 1000.0f;
        float y =  0.8f - blueCenter[i].y / 1000.0f;

        json data;
        data["id"] = yellowAmount[i];
        data["x"] = x;
        data["y"] = y;

        all_data.push_back(data);  // lägg till varje robot i arrayen
    }

    std::string payload = all_data.dump();
    sock.send(zmq::buffer(payload), zmq::send_flags::none);

    zmq::message_t reply;
    sock.recv(reply);
    std::string reply_str(static_cast<char*>(reply.data()), reply.size());
    std::cout << "Server replied: " << reply_str << std::endl;
}

void locatePlayer(Mat img, Scalar low, Scalar high, Color color) {
    Mat mask;
    //Mat purpleMask, orangeMask;

    inRange(img, low, high, mask);
    vector < vector < Point>> contours;// Stores all detected contours, each as a list of points
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE); // Finds external contours in the mask image and stores them in 'contours'//findcountrus function from open cv can be call from using namespace cv;

    // Position Tracking
    for (size_t i = 0; i < contours.size();++i) // Loop through all contours using size_t to match the container's size type
    
    {
        Rect boundRect = boundingRect(contours[i]);
            //store the blue color center for the enemy team
            if (color == Color::Blue) 
            {
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

            if (color == Color::Yellow) //home team
            {
                int gx = boundRect.x + boundRect.width / 2;
                int gy = boundRect.y + boundRect.height / 2;
                Point currentPlayerYellowCenter(gx, gy);
                yellowCenter.push_back(currentPlayerYellowCenter);
            }
            
            if (color == Color::Orange) 
            {
                ballCenter.clear();
                int bdx = boundRect.x + boundRect.width / 2;
                int bdy = boundRect.y + boundRect.height / 2;
                Point currentBallCenter(bdx, bdy);
                ballCenter.push_back(currentBallCenter);
            }
            players.emplace_back(color, boundRect, boundRect.x + boundRect.width / 2, boundRect.y + boundRect.height / 2);
    }
}

void playerId(Mat img) 
{
    ofstream Positions("D:\\Dokument\\AI Course\\outputs\\positions.csv", ios::app);
    if (!Positions.is_open()) {
        cerr << "Blue. Failed to open file for writing." << endl;
        return;
    }

    ofstream OurPositions("D:\\Dokument\\AI Course\\outputs\\ourPositions.csv", ios::app);
    if (!OurPositions.is_open()) {
        cerr << "Yellow. Failed to open ourPositions file." << endl;
        return;
    }

    if (check == false) 
    {
        // Tilldela ID till blå spelare baserat på avstånd till gula markörer
        for (size_t i = 0; i < blueCenter.size() && i < 4; ++i) {
            for (size_t j = 0; j < yellowCenter.size(); ++j) {
                double dist = norm(blueCenter[i] - yellowCenter[j]);
                if (dist < 25 && yellowAmount[i] == 0) {
                    yellowAmount[i] = nextId;
                    nextId++;
                    cout << "Blue ID: " << yellowAmount[i] << ", " << blueCenter[i] << endl;
                    break;
                }
            }
        }

        // Tilldela ID direkt till gula spelare (utan avståndscheck)
        for (size_t i = 0; i < yellowCenter.size(); ++i) {
            if (yellowAmount[i] == 0) {
                yellowAmount[i] = nextId;
                nextId++;
                cout << "Yellow ID: " << yellowAmount[i] << ", " << yellowCenter[i] << endl;
            }
        }

        check = true;
    }

    if (check == true) 
    {
        Positions << "Ball: " << ballCenter << endl;
        for (size_t i = 0; i < blueCenter.size() && i < 4; ++i) {
            Positions << "Blue Id: " << yellowAmount[i] << ", " << blueCenter[i] << endl;
        }
        Positions.close();

        for (size_t i = 0; i < yellowCenter.size(); ++i) {
            OurPositions << "Yellow Id: " << yellowAmount[i] << ", " << yellowCenter[i] << endl;
        }
        OurPositions.close();
    }
}



//Drawing the bounding boxes for every player for visual clarifaction
//Note that this will later be repurposed for trail creation
int frameCounter = 0;

void drawPlayer(Mat img) {
    for (size_t i = 0; i < blueCenter.size(); ++i) {
        circle(img, blueCenter[i], 25, CV_RGB(255, 0, 0), 2);
    }

    bool blinkOn = (frameCounter % 20) < 10;  // blink function in order to distinguish our team;

    if (blinkOn) {
        for (size_t i = 0; i < yellowCenter.size(); ++i) {
            circle(img, yellowCenter[i], 25, CV_RGB(255, 255, 255), 2);
        }
    }

    if (!ballCenter.empty()) {
        circle(img, ballCenter[0], 10, CV_RGB(255, 255, 255), 2);
    }
}


int main() {
    // Clear the output files once at the beginning
    ofstream clearFile("D:\\Dokument\\AI Course\\outputs\\positions.csv");
    clearFile.close();

    ofstream clearOur("D:\\Dokument\\AI Course\\outputs\\ourPositions.csv");
    clearOur.close();

    // Load the video file
    VideoCapture cap("D:\\Dokument\\AI Course\\Material\\go\\go\\cam0\\3.mp4");

    if (!cap.isOpened()) {
        cout << "Error opening video stream" << endl;
        return -1;
    }


    Mat frame;
    cap >> frame;
    if (frame.empty()) {
        cout << "Empty first frame." << endl;
        return -1;
    }


    // Prepare video writer
    int frame_width = frame.cols;
    int frame_height = frame.rows;
    VideoWriter video("D:\\Dokument\\AI Course\\outputs\\Tracking_Bound.avi",
                      VideoWriter::fourcc('M', 'J', 'P', 'G'),
                      30, Size(frame_width, frame_height));

    while (true) {
        frameCounter++;
        cap >> frame;
        if (frame.empty())
            break;

        Mat copy;
        frame.copyTo(copy);

        Mat hsv;
        cvtColor(frame, hsv, COLOR_BGR2HSV);

        locatePlayer(hsv, bluemin, bluemax, Color::Blue);
        locatePlayer(hsv, yellowmin, yellowmax, Color::Yellow);
        locatePlayer(hsv, orangemin, orangemax, Color::Orange);

        playerId(hsv); // ID assignment and saving to CSV
        drawPlayer(copy);
        sendData(4);
        video.write(copy);
        imshow("Check", copy);

        // Clear positions for next frame (used only for drawing)
        blueCenter.clear();
        purpleCenter.clear();
        yellowCenter.clear();

        // Exit loop if ESC is pressed
        char c = (char)waitKey(1);
        if (c == 27)
            break;
    }

    cap.release();
    video.release();
    destroyAllWindows();
    return 0;
}

