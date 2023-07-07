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
    double seekStart = 0.0;

    if (argc != 7) {
        std::cout << "Usage: " << argv[0] << " -v <video> -d <clip_duration> -ss <seek_start>" << std::endl;
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
        
        }else if(arg == "-ss"){
            seekStart = std::stod(value);
        }
        else {
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

    // Create the output video
    std::string outputVideoPath = "slowmotion_" + std::to_string(time) + ".mp4";
    cv::VideoWriter outputVideo(outputVideoPath, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), inputFps, cv::Size(inputWidth, inputHeight));

    // Check if the output video could be created
    if (!outputVideo.isOpened()) {
        std::cerr << "Could not create the output video: " << outputVideoPath << std::endl;
        return 1;
    }

    // Set the start and end frames
    int startFrame = static_cast<int>(seekStart * inputFps);
    int endFrame = startFrame + static_cast<int>(clipDuration * inputFps);

    // Check if the start and end frames are valid
    if (startFrame < 0 || startFrame >= inputNumFrames || endFrame < 0 || endFrame >= inputNumFrames) {
        std::cerr << "Invalid start and end frames: " << startFrame << " " << endFrame << std::endl;
        return 1;
    }

    // Set the current frame
    inputVideo.set(cv::CAP_PROP_POS_FRAMES, startFrame);

    // Read the frames
    std::vector<cv::Mat> frames;
    for (int i = startFrame; i < endFrame; i++) {
        cv::Mat frame;
        inputVideo >> frame;
        frames.push_back(frame);
    }


    // Slow down the video gradually by repeating the frames 1times then 2times then 3times then speed up again
    int numFrames = static_cast<int>(frames.size());
    double numRepeats = 1.;

    for (int i = 0; i < numFrames; i++) {
        for (int j = 0; j < (int)numRepeats; j++) {
            outputVideo << frames[i];
        }

        if (i < numFrames / 2) {
            numRepeats+=1./inputFps;
            std::min(3, (int)numRepeats);
        } else {
            numRepeats-=1./inputFps;
            std::max(1, (int)numRepeats);

        }
    }


    // close inputVideo and outputVideo
    inputVideo.release();
    outputVideo.release();

    // prnt outputVideoPath
    std::cout << outputVideoPath << std::endl;



    return 0;
}