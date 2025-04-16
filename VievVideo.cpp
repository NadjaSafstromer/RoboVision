#include "opencv2/opencv.hpp"
#include <iostream>

using namespace std;
using namespace cv;

cv::Scalar bluemin = cv::Scalar(102.5, 153, 102);
cv::Scalar bluemax = cv::Scalar(125, 255, 255);

int main() {

    // Create a VideoCapture object and use camera to capture the video
    VideoCapture cap("C:\\Users\\jbnlu\\Pictures\\Bouncing.mp4");

    // Check if camera opened successfully
    if (!cap.isOpened()) {
        cout << "Error opening video stream" << endl;
        return -1;
    }

    // Default resolutions of the frame are obtained.The default resolutions are system dependent.
    int frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    // Define the codec and create VideoWriter object.The output is stored in 'outcpp.avi' file.
    VideoWriter video("outcpp.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 10, Size(frame_width, frame_height));

    while (1) {

        Mat frame;
        Mat copy;
        cap >> frame;

        if (frame.empty())
            break;

        // Write the frame into the file 'outcpp.avi'
        video.write(frame);

        frame.copyTo(copy);

        cv::cvtColor(frame, frame, cv::COLOR_BGR2HSV);
        cv::Mat mask;

        cv::inRange(frame, bluemin, bluemax, mask);

        std::vector < std::vector < cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);
        //cv::drawContours(copy, contours, -1, cv::Scalar(0, 255, 0), 2);


        // Bounding BOX (Rectangle)
        for (size_t i = 0; i < contours.size();++i) {
            cv::Rect boundRect = cv::boundingRect(contours[i]);
            if (boundRect.width > 5)
                cv::rectangle(copy, boundRect.tl(), boundRect.br(), (0, 0, 0), 3);
        }

        // Display the resulting video to compare
        imshow("Frame", frame);
        imshow("test", copy);
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