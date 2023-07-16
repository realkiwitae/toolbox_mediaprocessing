#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>

#include <opencv2/dnn_superres.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// include cuda
// #include <opencv2/cudaarithm.hpp>
// #include <opencv2/cudafilters.hpp>
// #include <opencv2/cudaimgproc.hpp>
// #include <opencv2/cudawarping.hpp>

 
using namespace std;
using namespace cv;
using namespace dnn;
using namespace dnn_superres;


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
void treatVideo(std::string file_path,std::string output_folder, int id);
void treatImage(std::string img,std::string output_folder, int id);

Mat upscaleImage(Mat img, string modelName, string modelPath, int scale){
  DnnSuperResImpl sr;
  sr.readModel(modelPath);
  sr.setModel(modelName,scale);
  // Output image
  Mat outputImage;
  sr.upsample(img, outputImage);
  return outputImage;
}


int scaleFactor = 2;
std::string modelPath = "models/FSRCNN-small_x2.pb";

int main(int argc, char* argv[]) {

 // build/enhancer -i <input_folder> -o <output_folder> -s <scale_factor> -m <model_path> 
    // add usage
    if(argc != 9) {
        std::cout << "Usage: " << argv[0] << " -i <input_folder> -o <output_folder> -s <scale_factor> -m <model_path>" << std::endl;
        return 1;
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
            scaleFactor = atoi(argv[i + 1]);
            i++;            
        }else if (std::string(argv[i]) == "-m") {
            modelPath = argv[i + 1];
            i++;            
        }
    }

    // print inputs
    std::cout << "Input folder: " << input_folder << std::endl;
    std::cout << "Output folder: " << output_folder << std::endl;
    std::cout << "Scale factor: " << scaleFactor << std::endl;
    std::cout << "Model path: " << modelPath << std::endl;
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
            treatVideo(file_path, output_folder, ++vid_count);
        }else{
            treatImage(file_path, output_folder, ++img_count);
        }
    }

    return 0;
}

void treatVideo(std::string file_path,std::string output_folder, int id){

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
    cv::VideoWriter video(tmp_video, cv::VideoWriter::fourcc('a','v','c','1'), fps, cv::Size(frame_width*scaleFactor, frame_height*scaleFactor));

    // for each frame add the logo
    cv::Mat frame;

    // make progress bar
    int progress = 0;

    while (true) {
        //tic time  for each frame
        auto start = std::chrono::high_resolution_clock::now();
        progress++;
        // add remaining time to progress bar


        cap >> frame;
        if (frame.empty()) {
            break;
        }

         // EDSR (x4)
        string path = modelPath;
        string modelName = "fsrcnn";
        int scale = 2;
        
        Mat result = upscaleImage(frame, modelName, path, scale);
        // // show frame
        // cv::imshow("Frame", frame);
        // cv::waitKey(30);

        //convert frame to rgb 
        cv::cvtColor(result, result, cv::COLOR_BGRA2BGR);

        video.write(result);

        //toc time for each frame
        auto finish = std::chrono::high_resolution_clock::now();
        displayProgressBar(progress, frame_count, start, finish);
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

void treatImage(std::string img,std::string output_folder, int id){
    
    //print processing...
    std::cout << "Processing image: " << img << std::endl;

    // read image
    cv::Mat img_mat = cv::imread(img, cv::IMREAD_UNCHANGED);
    
    // EDSR (x4)
    string path = modelPath;
    string modelName = "fsrcnn";

    Mat result = upscaleImage(img_mat, modelName, path, scaleFactor);

    std::string img_count_str = std::to_string(id);
    img_count_str = std::string(2 - img_count_str.length(), '0') + img_count_str;
    std::string output_path = output_folder + "/img_" + img_count_str + ".jpg"; 
    cv::imwrite(output_path, result);
}