#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <sys/stat.h>
#include <dirent.h>

void treatImage(std::string img_path,cv::Mat logo_img,std::string output_folder, int id);
void treatVideo(std::string file_path,cv::Mat logo_img,std::string output_folder, int id);
void addLogo(cv::Mat& img, cv::Mat logo_img, int p);

int percentage_img = 10;
int percentage_vid = 10;

void displayProgressBar(int progress, int total) {
    constexpr int barWidth = 50;

    // Calculate the percentage completed
    float percentage = static_cast<float>(progress) / total;
    int completedWidth = static_cast<int>(barWidth * percentage);

    // Display the progress bar
    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < completedWidth) {
            std::cout << "=";
        } else if (i == completedWidth) {
            std::cout << ">";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "] " << static_cast<int>(percentage * 100) << "%\r";
    std::cout.flush();
}



int main(int argc, char* argv[]) {

 // Usage build/watermark -i <input_folder> -l <logo> -o <output_folder> -p <position> -s <size>

    if(argc != 10) {
        std::cout << "Usage: " << argv[0] << " -i <input_folder> -l <logo> -o <output_folder> -p <p_img> <p_video>" << std::endl;
        return 1;
    }

    std::string input_folder, logo, output_folder;
    output_folder = "./output";
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-i") {
            input_folder = argv[i + 1];
            i++;
        } else if (std::string(argv[i]) == "-l") {
            logo = argv[i + 1];
            i++;
        } else if (std::string(argv[i]) == "-o") {
            output_folder = argv[i + 1];
            i++;
        }else if (std::string(argv[i]) == "-p") {
            percentage_img = std::stoi(argv[++i]);
            percentage_vid = std::stoi(argv[++i]);            
        }
    }

    // read log
    cv::Mat logo_img = cv::imread(logo, cv::IMREAD_UNCHANGED);
    if (logo_img.empty()) {
        std::cout << "Could not read the image: " << logo << std::endl;
        return 1;
    }

    // // select roi on logo use select roi on smaller image
    // cv::Mat logo_toshow;
    // cv::resize(logo_img, logo_toshow, cv::Size(), .2, 0.2);
    // cv::Rect roi;
    // roi = cv::selectROI(logo_toshow);
    // //convert to match original image
    // roi.x *= 5;
    // roi.y *= 5;
    // roi.width *= 5;
    // roi.height *= 5;

    // // save to logo_noslogan.png
    // cv::Mat logo_noslogan = logo_img(roi);
    // cv::imwrite("logo_noslogan.png", logo_noslogan);

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
    int vid_count = 0;

    while ((ent = readdir(dir)) != NULL) {
        // check if file is image using cv::imread
        std::string file_name = ent->d_name;
        std::string file_path = input_folder + "/" + file_name;
        cv::Mat img = cv::imread(file_path, cv::IMREAD_UNCHANGED);
        if (img.empty()) {
            treatVideo(file_path, logo_img, output_folder, ++vid_count);
        }else{
            treatImage(file_path, logo_img, output_folder, ++img_count);
        }
    }

    return 0;
}

void treatImage(std::string img,cv::Mat logo_img,std::string output_folder, int id){
    
    //print processing...
    std::cout << "Processing image: " << img << std::endl;

    // read image
    cv::Mat img_mat = cv::imread(img, cv::IMREAD_UNCHANGED);
    addLogo(img_mat, logo_img, percentage_img);
    
    std::string img_count_str = std::to_string(id);
    img_count_str = std::string(2 - img_count_str.length(), '0') + img_count_str;
    std::string output_path = output_folder + "/img_" + img_count_str + ".jpg"; 
    cv::imwrite(output_path, img_mat);

}
void treatVideo(std::string file_path,cv::Mat logo_img,std::string output_folder, int id){

    // check file extension
    std::string file_name = file_path.substr(file_path.find_last_of("/\\") + 1);
    std::string file_ext = file_name.substr(file_name.find_last_of(".") + 1);
    if(file_ext != "mp4"){
        return;
    }

    // check if file is a video using cv::VideoCapture
    cv::VideoCapture cap(file_path);
    if (!cap.isOpened()) {
        std::cout << "Could not open the video file: " << file_path << std::endl;
        return;
    }

    // print processing...
    std::cout << "Processing video: " << file_path << std::endl;

    // get video properties
    int frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    double fps = cap.get(cv::CAP_PROP_FPS);
    int frame_count = cap.get(cv::CAP_PROP_FRAME_COUNT);

    // create output video
    std::string vid_count_str = std::to_string(id);
    vid_count_str = std::string(2 - vid_count_str.length(), '0') + vid_count_str;
    std::string output_path = output_folder + "/vid_" + vid_count_str + ".mp4";
    
    std::string tmp_video = "/tmp/tmp.mp4";
    cv::VideoWriter video(tmp_video, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, cv::Size(frame_width, frame_height));

    // for each frame add the logo
    cv::Mat frame;

    // make progress bar
    int progress = 0;

    while (true) {
        progress++;
        displayProgressBar(progress, frame_count);

        cap >> frame;
        if (frame.empty()) {
            break;
        }
        addLogo(frame, logo_img, percentage_vid);

        // // show frame
        // cv::imshow("Frame", frame);
        // cv::waitKey(30);

        //convert frame to rgb 
        cv::cvtColor(frame, frame, cv::COLOR_BGRA2BGR);

        video.write(frame);
    }

    // free vieo capture
    cap.release();
    video.release();
    // // keep sound from original video
    std::string cmd = "ffmpeg -i " + file_path + " -i " + tmp_video + " -c copy -map 1:v:0 -map 0:a:0 " + output_path;
    // silence output
    cmd += " -loglevel panic";

    system(cmd.c_str());
     // remove tmp video
    cmd = "rm " + tmp_video;
    system(cmd.c_str());



}


void addLogo(cv::Mat& img, cv::Mat logo_img, int p){
    // if no alpha channel, add alpha channel
    if (img.channels() == 3) {
        cv::cvtColor(img, img, cv::COLOR_BGR2BGRA);
    }
    // resize logo to fit in the low right corner, at 10% of the image height at least 100px, size of logo should be 10% of the image height
    cv::Mat logo_resized;
    bool middle = false;
    // logo size = 10% of image height or 100px
    if(p < 0){
        p = fabs(p);
        middle = true;
    } 

    int logo_size = img.rows * p / 100;
    cv::resize(logo_img, logo_resized, cv::Size(int(logo_size*(double)logo_img.cols/logo_img.rows) , logo_size));

    // add logo in bottom right corner + 10% of the image height
    cv::Rect roi;
    if(!middle) roi = cv::Rect(10, 10, logo_resized.cols, logo_resized.rows);
    // add logo middle of the image
    else roi = cv::Rect(img.cols/2 - logo_resized.cols/2, img.rows/2 - logo_resized.rows/2, logo_resized.cols, logo_resized.rows);

    cv::Mat destinationROI = img(roi);
    cv::addWeighted(destinationROI, 1.0, logo_resized, middle?.8:1., 0.0, destinationROI);

}