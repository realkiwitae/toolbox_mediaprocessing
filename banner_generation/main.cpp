#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
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

    // save bannr in ./output/banner.png

    // create output directory if it does not exist
    struct stat info;
    if (stat("./output", &info) != 0) {
        std::cout << "creating output directory" << std::endl;
        mkdir("./output", 0777);
    }

    // for each file in ./input open select tool to copy logos onto banner and update banner visual
    DIR* dir;
    struct dirent* ent;

    // for each file in ./input
    std::string input_path = "input/";
    dir = opendir(input_path.c_str());
    while ((ent = readdir(dir)) != NULL) {
        std::string filename = ent->d_name;
        if (filename == "." || filename == "..") {
            continue;
        }
        std::cout << "processing " << filename << std::endl;
        // open select tool
        // load image in filename
        cv::Mat img = cv::imread(input_path + filename, cv::IMREAD_UNCHANGED);
        if (img.empty()) {
            std::cout << "Could not read the image: " << input_path + filename << std::endl;
            continue;
        }

        cv::Rect roi_touse = cv::selectROI(banner);
        cv::waitKey(0);

        // resize using roi_touse dimension, img keeping aspect ratio
        cv::Mat img_resized, img_alpha;

        // init img_resized_alpha to be transparent
        img_alpha = cv::Mat(img.size(), CV_8UC4, cv::Scalar(0, 0, 0, 0));

        // convert type to match banner
        if (img.channels() < 4) {
            // assume lack of alpha channel means image is BGR
            // convert all white pixels to transparent
            cv::Mat mask;
            cv::inRange(img, cv::Scalar(0, 0, 0), cv::Scalar(245, 245, 245), mask);
            //convert to rgba
            cv::cvtColor(img, img, cv::COLOR_BGR2BGRA);
            img.copyTo(img_alpha, mask);

        }
        else{
            img_alpha = img;
        }
        cv::resize(img_alpha, img_alpha, roi_touse.size(), 0, 0, cv::INTER_AREA);

        // add img_alpha to banner
        cv::Mat banner_roi = banner(roi_touse);
        cv::addWeighted(banner_roi, 1.0, img_alpha, 1.0, 0.0, banner_roi);
        // add text next to logo
        // add text without qt no support

        cv::putText(banner, "onlyfans.com/lysiavice", cv::Point(roi_touse.x + roi_touse.width + 5, roi_touse.y + roi_touse.height / 2 + 2), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(246, 152, 189, 255), 1 , cv::LINE_AA);
        // put text with a light blue overlay background
        // add light blue overlay on top of banner_roi + text


        
        // save banner


        // update banner visual
        
    }

    //show banner
    cv::imshow("banner", banner);
    cv::waitKey(0);

    // save banner
    cv::imwrite("./output/banner.png", banner);
    
    return 0;
}