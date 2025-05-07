#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

cv::Scalar bluemin = cv::Scalar(102.5, 153, 102);
cv::Scalar bluemax = cv::Scalar(125, 255, 255);
int main() {
    cv::Mat enemy = cv::imread("D:/Dokument/AI Course/Material/RobotTest.JPG");
    if (enemy.empty()) {
        std::cout << "Could not read the image\n";
        return 1;
    }

    cv::Mat copy;
    enemy.copyTo(copy);

    cv::cvtColor(enemy, enemy, cv::COLOR_BGR2HSV);
    cv::Mat mask;

    cv::inRange(enemy, bluemin, bluemax, mask);
    
    std::vector < std::vector < cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);
    //cv::drawContours(copy, contours, -1, cv::Scalar(0, 255, 0), 2);
    

    // Bounding BOX (Rectangle)
    for (size_t i = 0; i < contours.size();++i) {
        cv::Rect boundRect = cv::boundingRect(contours[i]);
        if (boundRect.width > 5)
        cv::rectangle(copy, boundRect.tl(), boundRect.br(), (0, 0, 0), 3);
    }
    
    cv::imshow("Image", copy);


    cv::waitKey(0);
    return 0;
}
