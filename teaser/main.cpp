#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <opencv2/opencv.hpp>
#include <dirent.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {

    std::srand(static_cast<unsigned int>(std::time(0)));
    // Set the parameters
    int clipDuration = 3;
    int nbofclips = 3;

    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " -v <video> -d <clip_duration>" << std::endl;
        return -1;
    }

    std::string videoPath;

    // Parse command line arguments
    for (int i = 1; i < argc; i += 2) {
        std::string arg = argv[i];
        std::string value = argv[i + 1];

        if (arg == "-v") {
            videoPath = value;
        } else if (arg == "-d") {
            clipDuration = std::stoi(value);
        
        } else {
            std::cout << "Unknown option: " << arg << std::endl;
            return -1;
        }
    }

    if (videoPath.empty()) {
        std::cerr << "Please provide a valid folder path using the -v option." << std::endl;
        return 1;
    }

    // Get the current date
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);

    // open videoPath and capture 3random sequences of duration clipDuration/3
    cv::VideoCapture inputVideo(videoPath);
    if (!inputVideo.isOpened()) {
        std::cerr << "Could not open the input video: " << videoPath << std::endl;
        return 1;
    }

    // Get the video properties
    int inputWidth = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_WIDTH));
    int inputHeight = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT));
    double inputFps = inputVideo.get(cv::CAP_PROP_FPS);
    int inputNumFrames = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_COUNT));

    // Select sequences of duration clipDuration from the input video each is from k/nbOfClips portions of the video
    int clipDurationFramesPerClip = static_cast<int>(clipDuration * inputFps);

    // Generate random indices for clip selection
    std::vector<int> randomIndices;
    for (int i = 0; i < nbofclips; i++) {
        randomIndices.push_back(std::rand() % (int(inputNumFrames/nbofclips) - clipDurationFramesPerClip));
    }

    // Create the output video
    std::stringstream ss;
    ss << "./teaser_" << std::put_time(std::localtime(&time), "%Y%m%d") << ".mp4";
    std::string outputVideoPath = ss.str();
    cv::VideoWriter outputVideo(outputVideoPath, cv::VideoWriter::fourcc('X', '2', '6', '4'), inputFps, cv::Size(inputWidth, inputHeight));
    
    if (!outputVideo.isOpened()) {
        std::cerr << "Could not create the output video: " << outputVideoPath << std::endl;
        return 1;
    }
    // concatenate the 3 sequences into outputvideo
    for (int i = 0; i < nbofclips; i++) {
        inputVideo.set(cv::CAP_PROP_POS_FRAMES, i*(inputNumFrames/nbofclips)+randomIndices[i]);
        for (int j = 0; j < clipDurationFramesPerClip; j++) {
            cv::Mat frame;
            inputVideo >> frame;
            outputVideo << frame;
        }
    }

    // close inputVideo and outputVideo
    inputVideo.release();
    outputVideo.release();

    // prnt outputVideoPath
    std::cout << outputVideoPath << std::endl;



    return 0;
}