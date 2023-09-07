#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <ctime>
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

void displayProgressBar(int progress, int total, std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point finish) {
    constexpr int barWidth = 50;

    // Calculate the percentage completed
    float percentage = static_cast<float>(progress) / total;
    int completedWidth = static_cast<int>(barWidth * percentage);

    // Display the progress bar and time remaining
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed * (total - progress));

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
    std::cout << "] " << static_cast<int>(percentage * 100) << "% - ";
    // rmaining in hh:mm::ss
    long rem = remaining.count();
    std::cout << std::setfill('0') << std::setw(2) << rem / 3600000 << ":";
    rem %= 3600000;
    std::cout << std::setfill('0') << std::setw(2) << rem / 60000 << ":";
    rem %= 60000;
    std::cout << std::setfill('0') << std::setw(2) << rem / 1000 << "\r";
    std::cout << "%\r";
    std::cout.flush();
}

bool bAddMarker = false;
std::string symbol_path = "./symbol/ofmarker.png";

int main(int argc, char* argv[]) {

 // Usage build/watermark -i <input_folder> -l <logo> -o <output_folder> -p <position> -s <size>

    if(argc < 10) {
        std::cout << argc << std::endl;
        // print all args
        for (int i = 0; i < argc; i++) {
            std::cout << argv[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "Usage: " << argv[0] << " -i <input_folder> -l <logo> -o <output_folder> -p <p_img> <p_video> -m <p_symbol" << std::endl;
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
        }else if(std::string(argv[i]) == "-m"){
            bAddMarker = true;
            symbol_path = argv[++i];
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
void addMarker(cv::Mat& img_mat){
    if(!bAddMarker) return;
    cv::Mat ofmarker = cv::imread(symbol_path, cv::IMREAD_UNCHANGED);

    cv::resize(ofmarker, ofmarker, cv::Size(img_mat.cols*.4,img_mat.cols*.4*ofmarker.rows/ofmarker.cols));
    // if(ofmarker.channels() != 3){
    //     //add alpha channel
    //     cv::cvtColor(ofmarker, ofmarker, cv::COLOR_BGRA2BGR);
    // }
    cv::Rect2d ofmarker_roi = cv::Rect2d(img_mat.cols - ofmarker_roi.width, .8*img_mat.rows - ofmarker_roi.height, ofmarker.cols, ofmarker.rows);
    // make sure within bounds
    if(ofmarker_roi.x < 0) ofmarker_roi.x = 0;
    if(ofmarker_roi.y < 0) ofmarker_roi.y = 0;
    if(ofmarker_roi.x + ofmarker_roi.width > img_mat.cols) ofmarker_roi.x = img_mat.cols - ofmarker_roi.width;
    if(ofmarker_roi.y + ofmarker_roi.height > img_mat.rows) ofmarker_roi.y = img_mat.rows - ofmarker_roi.height;

    ofmarker.copyTo(img_mat(ofmarker_roi));

}
void treatImage(std::string img,cv::Mat logo_img,std::string output_folder, int id){
    
    //print processing...
    std::cout << "Processing image: " << img << std::endl;

    // read image
    cv::Mat img_mat = cv::imread(img, cv::IMREAD_UNCHANGED);
    addLogo(img_mat, logo_img, percentage_img);
    addMarker(img_mat);


    std::string img_count_str = std::to_string(id);
    img_count_str = std::string(2 - img_count_str.length(), '0') + img_count_str;
    // get today date format yyyymmdd
    std::time_t t = std::time(nullptr);
    char buffer[80];
    std::strftime(buffer, 80, "%Y%m%d", std::localtime(&t));
    std::string date(buffer);
    std::string output_path = output_folder + "/img_" + date +"_"+ img_count_str + ".jpg"; 
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
    std::time_t t = std::time(nullptr);

    // std::tm to std::string format yyyymmdd padding with 0 
    char buffer[80];
    std::strftime(buffer, 80, "%Y%m%d", std::localtime(&t));
    std::string date(buffer);

    std::string output_path = output_folder + "/vid_" + date + "_" + vid_count_str + ".mp4"; 
    
    std::string tmp_video = "/tmp/tmp.mp4";
    cv::VideoWriter video(tmp_video, cv::VideoWriter::fourcc('a','v','c','1'), fps, cv::Size(frame_width, frame_height));

    // for each frame add the logo
    cv::Mat frame;

    // make progress bar
    int progress = 0;
    int fc = frame_count/ fps;
    std::cout << "Frame count = " << frame_count << " to hh:mm:ss " << std::setfill('0') << std::setw(2) << fc / 3600 << ":";
    fc %= 3600;
    std::cout << std::setfill('0') << std::setw(2) << fc / 60 << ":";
    fc %= 60;
    std::cout << std::setfill('0') << std::setw(2) << fc << std::endl;

    while (true) {
        auto start = std::chrono::high_resolution_clock::now();
        progress++;

        cap >> frame;
        if (frame.empty()) {
            break;
        }
        addLogo(frame, logo_img, percentage_vid);
        addMarker(frame);

        // // show frame
        // cv::imshow("Frame", frame);
        // cv::waitKey(30);

        //convert frame to rgb 
        cv::cvtColor(frame, frame, cv::COLOR_BGRA2BGR);

        video.write(frame);
        auto finish = std::chrono::high_resolution_clock::now();
        displayProgressBar(progress, frame_count, start, finish);
    }

    // free vieo capture
    cap.release();
    video.release();



    // check if sound in original video
    std::string cmd = "ffprobe -i " + file_path + " -show_streams -select_streams a -loglevel error";
    cmd += " | grep codec_name";
    cmd += " | wc -l";
    int result = system(cmd.c_str());

    // if no sound in original video, add sound from tmp video

        // // keep sound from original video
    cmd = "ffmpeg -i " + file_path + " -i " + tmp_video + " -c copy -map 1:v:0 -map 0:a:0 " + output_path;
    // silence output
    cmd += " -loglevel panic";

    system(cmd.c_str());

    // if(result != 0){

    // }else{
    //     //copy tmp to output
    //     cmd = "cp " + tmp_video + " " + output_path;
    //     system(cmd.c_str());
    // }

     // remove tmp video
    cmd = "rm " + tmp_video;
    system(cmd.c_str());
    
    // // fix codec 
    // cmd = "ffmpeg -i " + output_path + " -vcodec libx264 -acodec aac " + "./output/o.mp4";
    // system(cmd.c_str());
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
    if(!middle) roi = cv::Rect(20, 20, logo_resized.cols, logo_resized.rows);
    // add logo middle of the image
    else roi = cv::Rect(img.cols/2 - logo_resized.cols/2, img.rows/2 - logo_resized.rows/2, logo_resized.cols, logo_resized.rows);

    cv::Mat destinationROI = img(roi);
    cv::addWeighted(destinationROI, 1.0, logo_resized, middle?.8:1., 0.0, destinationROI);

}