#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <opencv2/opencv.hpp>
#include <dirent.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {

    // take path to logo as input and size of banner -l -s wxh
    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " -l <logo_path> -s <size>" << std::endl;
        return 1;
    }

    std::string logo_path;
    cv::Size size;

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-l") {
            logo_path = argv[i + 1];
            i++;
        } else if (std::string(argv[i]) == "-s") {
            std::string size_str = argv[i + 1];
            size = cv::Size(std::stoi(size_str.substr(0, size_str.find("x"))), std::stoi(size_str.substr(size_str.find("x") + 1)));
            i++;
        }
    }

    // read logo
    cv::Mat logo = cv::imread(logo_path, cv::IMREAD_UNCHANGED);
    if (logo.empty()) {
        std::cout << "Could not read the image: " << logo_path << std::endl;
        return 1;
    }

    // find the logo background color by taking the median of the logo pixels
    std::vector<cv::Mat> channels;
    cv::split(logo, channels);
    cv::Mat bgchan;
    for (int i = 0; i < 4; i++) {
        cv::Mat channel = channels[i];
        cv::sort(channel, channel, cv::SORT_ASCENDING);
        bgchan.push_back(channel.at<uchar>(channel.rows / 2, channel.cols / 2));
    }
    // cv mat to cv scalar
    cv::Scalar logo_bg_color = cv::Scalar(bgchan.at<uchar>(0, 0), bgchan.at<uchar>(1, 0), bgchan.at<uchar>(2, 0),bgchan.at<uchar>(3, 0));

    // create banner of size with color logo_bg_color
    cv::Mat banner(size, logo.type(), logo_bg_color);

    // resize logo to fit in middle of banner
    cv::Mat logo_resized;
    cv::resize(logo, logo_resized, cv::Size(), 0.5, 0.5);
    std::cout << "color of logo background: " << logo_bg_color << std::endl;
    // place logo in middle of banner
    cv::Rect roi = cv::Rect((banner.cols - logo_resized.cols) / 2, (banner.rows - logo_resized.rows) / 2, logo_resized.cols, logo_resized.rows);
    logo_resized.copyTo(banner(roi));

    //show banner
    cv::imshow("banner", banner);
    cv::waitKey(0);

    // save bannr in ./output/banner.png

    // create output directory if it does not exist
    struct stat info;
    if (stat("./output", &info) != 0) {
        std::cout << "creating output directory" << std::endl;
        mkdir("./output", 0777);
    }

    // save banner
    cv::imwrite("./output/banner.png", banner);
    
    return 0;
}