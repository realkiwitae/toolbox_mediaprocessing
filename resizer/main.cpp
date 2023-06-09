#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <sys/stat.h>
#include <dirent.h>

void treatImage(std::string img,std::string output_folder, int id);

int width = 3024;
int height = 4032;
bool bypass_crop = false;

int main(int argc, char* argv[]) {

 // build/resizer -i <input_folder> -o <output_folder> -s <size>
    // add usage
    if (argc < 7) {
        std::cout << "Usage: " << argv[0] << " -i <input_folder> -o <output_folder> -s <size> -b" << std::endl;
        return 0;
    }


    std::string input_folder, output_folder;
    output_folder = "./output";
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-i") {
            input_folder = argv[i + 1];
            i++;
        }else if (std::string(argv[i]) == "-o") {
            output_folder = argv[i + 1];
            i++;
        }else if (std::string(argv[i]) == "-s") {
            //parse size wxh
            std::string size = argv[i + 1];
            std::string delimiter = "x";
            width = std::stoi(size.substr(0, size.find(delimiter)));
            height = std::stoi(size.substr(size.find(delimiter) + 1));
        }else if(std::string(argv[i]) == "-b"){
            bypass_crop = true;
        }
    }

    // print inputs
    std::cout << "Input folder: " << input_folder << std::endl;
    std::cout << "Output folder: " << output_folder << std::endl;
    std::cout << "Size: " << width << "x" << height << std::endl;

    // create output folder if not exist
    mkdir(output_folder.c_str(), 0777);
    // empty folder
    std::string cmd = "rm -rf " + output_folder + "/*";
    system(cmd.c_str());

    // for each image in input_folder
    DIR *dir;
    struct dirent *ent;

    dir = opendir(input_folder.c_str());

    // rename images as img_0001.png, img_0002.png, ...
    // rename videos as vid_0001.mp4, vid_0002.mp4, ...
    int img_count = 0;

    while ((ent = readdir(dir)) != NULL) {
        // check if file is image using cv::imread
        std::string file_name = ent->d_name;
        std::string file_path = input_folder + "/" + file_name;
        cv::Mat img = cv::imread(file_path, cv::IMREAD_UNCHANGED);
        if (img.empty()) {
            continue;
        }else{
            treatImage(file_path, output_folder, ++img_count);
        }
    }

    return 0;
}

void treatImage(std::string img,std::string output_folder, int id){
    
    //print processing...
    std::cout << "Processing image: " << img << std::endl;

    // read image
    cv::Mat img_mat = cv::imread(img, cv::IMREAD_UNCHANGED);
    cv::Mat showing_img,imgclone;
    cv::Mat selecttool_img;

    // resize img for showing cant be higher than 1000px
    int target = 400;
    cv::resize(img_mat, showing_img, cv::Size(target, int(target*(double)img_mat.rows/img_mat.cols)));
    cv::resize(img_mat, selecttool_img, cv::Size(target, int(target*(double)img_mat.rows/img_mat.cols)));
    imgclone = showing_img.clone();
    //select roi
    // while no press on F key selectROI
    double percent = 1. - .1*!bypass_crop; 
    cv::Rect2d roi = cv::Rect2d(0, 0, showing_img.cols*percent, showing_img.rows*percent);
    while (!bypass_crop) {
        showing_img = imgclone.clone();
        // make mask to add transparency outside of selected roi
        cv::Mat mask = cv::Mat::zeros(showing_img.rows, showing_img.cols, CV_8UC1);
        cv::rectangle(mask, roi, cv::Scalar(255), -1);
        // add mask to image
        cv::Mat mask_rgb;
        cv::cvtColor(mask, mask_rgb, cv::COLOR_GRAY2BGR);
        cv::addWeighted(showing_img, 1.0, mask_rgb, 0.3, 0.0, showing_img);
        // show image
        cv::imshow("Image", showing_img);
        int key = cv::waitKey(30);
        // if key F break
        if (key == 102) {
            break;
        }
        // if key C crop
        if (key == 99) {
            roi = cv::selectROI("Image", selecttool_img);
            // resize roi to fit aspect ratio same topleft corner
            double aspect_ratio = (double)width / height;
            double roi_aspect_ratio = (double)roi.width / roi.height;
            if (roi_aspect_ratio > aspect_ratio) {
                // roi too wide
                roi.width = roi.height * aspect_ratio;
            }else{
                // roi too tall
                roi.height = roi.width / aspect_ratio;
            }
            // cout dim and roi aspect ratio and aspect ratio
            std::cout << "ROI: " << roi << " wxh " << roi.width / roi.height << " aspect ratio " << aspect_ratio << std::endl;

        }
        // if arrow keys move roi

        if (key == 81) {
            // left
            roi.x -= 1;
        }
        if (key == 82) {
            // up
            roi.y -= 1;
        }
        if (key == 83) {
            // right
            roi.x += 1;
        }
        if (key == 84) {
            // down
            roi.y += 1;
        }
        // keep roi inside image
        if (roi.x < 0) {
            roi.x = 0;
        }
        if (roi.y < 0) {
            roi.y = 0;
        }
        if (roi.x + roi.width > showing_img.cols) {
            roi.x = showing_img.cols - roi.width;
        }
        if (roi.y + roi.height > showing_img.rows) {
            roi.y = showing_img.rows - roi.height;
        }
    }

    // resize roi to fit img size
    std::cout << "ROIbef: " << roi << std::endl;
    roi.x = roi.x * (double)img_mat.cols / target;
    roi.y = roi.y * (double)img_mat.cols / target;
    roi.width = (int)(roi.width * (double)img_mat.cols / target);
    roi.height = (int)(roi.height * (double)img_mat.cols / target);

    // crop image
    // print img_mat dimensions and roi
    std::cout << "Image dimensions: " << img_mat.cols << "x" << img_mat.rows << std::endl;
    std::cout << "ROI: " << roi << std::endl;
    // roi aspect ratio vs aspect ratio
    double aspect_ratio = (double)width / height;
    double roi_aspect_ratio = (double)roi.width / roi.height;
    std::cout << "ROI aspect ratio: " << roi_aspect_ratio << std::endl;
    std::cout << "Aspect ratio: " << aspect_ratio << std::endl;
    cv::Mat cropped_img = img_mat(roi);

    // resize to widthxheight using better quality
    cv::resize(cropped_img, cropped_img, cv::Size(width, height), cv::INTER_CUBIC);

    std::string img_count_str = std::to_string(id);
    img_count_str = std::string(2 - img_count_str.length(), '0') + img_count_str;
    std::string output_path = output_folder + "/img_" + img_count_str + ".jpg"; 
    cv::imwrite(output_path, cropped_img);
}