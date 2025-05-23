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
    sock.connect("tcp://192.168.8.37:5555");

    json all_data = json::array();

    static std::map<std::string, Point2f> lastPrediction;
    static int totalPredictions = 0;
    static int correctPredictions = 0;
    const float threshold = 0.05f;

    for (size_t i = 0; i < blueCenter.size() && i < 4; ++i) {
        float norm_x = -0.5f + blueCenter[i].x / 1000.0f;
        float norm_y =  0.8f - blueCenter[i].y / 1000.0f;

        int model_id = yellowAmount[i]; // detta måste motsvara träningen

        std::string robot_id = "agent_blue_" + std::to_string(model_id);

        std::cout << "[SEND] ID: " << robot_id
                  << " (" << model_id << ")"
                  << " | Normalized Pos: (" << norm_x << ", " << norm_y << ")\n";

        // Validera prediktion
        if (lastPrediction.count(robot_id)) {
            Point2f predictedPos = lastPrediction[robot_id];
            float dx = norm_x - predictedPos.x;
            float dy = norm_y - predictedPos.y;
            float error = std::sqrt(dx * dx + dy * dy);

            std::cout << "[VALIDATION] " << robot_id
                      << " | Predicted: (" << predictedPos.x << ", " << predictedPos.y << ")"
                      << " | Actual: (" << norm_x << ", " << norm_y << ")"
                      << " | Error: " << error << std::endl;

            totalPredictions++;
            if (error <= threshold)
                correctPredictions++;

            float accuracy = 100.0f * correctPredictions / totalPredictions;
            std::cout << "Current accuracy: " << accuracy << "%\n";
        } else {
            std::cout << "[NO VALIDATION] No previous prediction for " << robot_id << std::endl;
        }

        // Skicka till server
        json data;
        data["id"] = model_id;
        data["x"] = norm_x;
        data["y"] = norm_y;
        all_data.push_back(data);
    }

    std::string payload = all_data.dump();
    sock.send(zmq::buffer(payload), zmq::send_flags::none);

    zmq::message_t reply;
    sock.recv(reply);
    std::string reply_str(static_cast<char*>(reply.data()), reply.size());
    std::cout << "[REPLY] Server replied: " << reply_str << std::endl;

    // Uppdatera prediktioner
    try {
        json prediction_array = json::parse(reply_str);
        for (const auto& pred : prediction_array) {
            std::string id = "agent_blue_" + std::to_string(pred["id"].get<int>());
            float pred_x = pred["next_x"].get<float>();
            float pred_y = pred["next_y"].get<float>();
            lastPrediction[id] = Point2f(pred_x, pred_y);

            std::cout << "[UPDATE] Saved prediction for " << id
                      << " => (" << pred_x << ", " << pred_y << ")\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to parse prediction reply: " << e.what() << std::endl;
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
        if (color == Color::Yellow) {
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
                    cout << "Blue ID: " << yellowAmount[i]
                         << " => norm_x: " << -0.5 + blueCenter[i].x / 1000.0 << endl;
                    break;
                }
            }
        }

        // Tilldela ID till gula spelare (utan avståndscheck)
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
            float x = blueCenter[i].x;
            float y = blueCenter[i].y;

            float norm_x = -0.5f + x / 1000.0f;
            float norm_y =  0.8f - y / 1000.0f;

            Positions << "agent_blue_" << yellowAmount[i] << " "
                      << norm_x << " " << norm_y << endl;
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
//int frameCounter = 0;

void drawPlayer(Mat img) {
    for (size_t i = 0; i < blueCenter.size(); ++i) {
        circle(img, blueCenter[i], 25, CV_RGB(255, 0, 0), 2);
    }

    /*bool blinkOn = (frameCounter % 20) < 10;  // blink function in order to distinguish our team;

    if (blinkOn) {
        for (size_t i = 0; i < yellowCenter.size(); ++i) {
            circle(img, yellowCenter[i], 25, CV_RGB(255, 255, 255), 2);
        }
    }*/

    if (!ballCenter.empty()) {
        circle(img, ballCenter[0], 10, CV_RGB(255, 255, 255), 2);
    }
}


int main() {
    // Töm filerna i början
    ofstream clearFile("D:\\Dokument\\AI Course\\outputs\\positions.csv");
    clearFile.close();

    /*ofstream clearOur("D:\\Dokument\\AI Course\\outputs\\ourPositions.csv");
    clearOur.close();*/

    // Ladda videon
    VideoCapture cap("D:\\Dokument\\AI Course\\Material\\vid\\vid\\2.avi");
    if (!cap.isOpened()) {
        cout << "Error opening video stream" << endl;
        return -1;
    }

    int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);

    VideoWriter video("D:\\Dokument\\AI Course\\outputs\\Tracking_Bound.avi",
                      VideoWriter::fourcc('M', 'J', 'P', 'G'),
                      30, Size(frame_width, frame_height));

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
        locatePlayer(frame, purplemin, purplemax, Color::Yellow);
        locatePlayer(frame, orangemin, orangemax, Color::Orange);
        playerId(frame);
        drawPlayer(copy);
        sendData(4);
        blueCenter.clear();
        yellowCenter.clear();
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
