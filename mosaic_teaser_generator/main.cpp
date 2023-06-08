#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <opencv2/opencv.hpp>
#include <dirent.h>
#include <sys/stat.h>

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

std::vector<std::string> getVideoFiles(const std::string& folderPath) {
    std::vector<std::string> videoFiles;
    DIR* dir;
    struct dirent* entry;
    if ((dir = opendir(folderPath.c_str())) != nullptr) {
        while ((entry = readdir(dir)) != nullptr) {
            std::string fileName = entry->d_name;
            if (fileName.find(".mp4") != std::string::npos) {
                videoFiles.push_back(folderPath + "/" + fileName);
            }
        }
        closedir(dir);
    } else {
        std::cerr << "Error opening directory: " << folderPath << std::endl;
    }
    return videoFiles;
}

// Function to generate random indices
std::vector<int> generateRandomIndices(int count, int max) {
    std::vector<int> indices;
    for (int i = 0; i < count; ++i) {
        int index = std::rand() % max;
        indices.push_back(index);
    }
    return indices;
}
double calculateMedian(std::vector<double>& values) {
    size_t size = values.size();
    std::sort(values.begin(), values.end());

    if (size % 2 == 0) {
        // If the size is even, average the two middle elements
        return (values[size / 2 - 1] + values[size / 2]) / 2.0;
    } else {
        // If the size is odd, return the middle element
        return values[size / 2];
    }
}

int main(int argc, char* argv[]) {

    std::srand(static_cast<unsigned int>(std::time(0)));
    // Set the parameters
    int gridWidth;
    int gridHeight;
    int targetWidth = 1080;
    double mosaicRatio = 1.;

    int clipDuration = 3;

    if (argc != 7) {
        std::cout << "Usage: " << argv[0] << " -f <folder_path> -d <clip_duration> -s <width_value>x<height_value>" << std::endl;
        return -1;
    }

    std::string videoFolder;

    // Parse command line arguments
    for (int i = 1; i < argc; i += 2) {
        std::string arg = argv[i];
        std::string value = argv[i + 1];

        if (arg == "-f") {
            videoFolder = value;
        } else if (arg == "-d") {
            clipDuration = std::stoi(value);
        } else if (arg == "-s") {
            size_t separatorPos = value.find('x');
            if (separatorPos != std::string::npos) {
                gridWidth = std::stoi(value.substr(0, separatorPos));
                gridHeight = std::stoi(value.substr(separatorPos + 1));
            } else {
                std::cout << "Invalid size format. Use <width_value>x<height_value>." << std::endl;
                return -1;
            }
        } else {
            std::cout << "Unknown option: " << arg << std::endl;
            return -1;
        }
    }

    if (videoFolder.empty()) {
        std::cerr << "Please provide a valid folder path using the -f option." << std::endl;
        return 1;
    }

    // Get the video filenames from the folder
    std::vector<std::string> videoFiles = getVideoFiles(videoFolder);
    

    // Generate random indices for clip selection
    int numClips = gridWidth * gridHeight;
    std::vector<int> randomIndices = generateRandomIndices(numClips, videoFiles.size());

    // Get the current date
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);

    double fps;
    int type;
    std::vector<double> aspectRatios;
    // Extract random clips and create the mosaic
    std::vector<cv::VideoCapture*> videos;
    for (int i = 0; i < numClips; ++i) {
        // Load the video
        videos.push_back(new cv::VideoCapture(videoFiles[randomIndices[i]]));
        cv::VideoCapture* video = videos.back(); 

        // Get the input video's properties
        fps = video->get(cv::CAP_PROP_FPS);

        // Get a random start frame
        int frameCount = video->get(cv::CAP_PROP_FRAME_COUNT);
        int startFrame = std::rand() % int(frameCount - clipDuration * fps);
        // Set the current frame to the start frame
        cv::Mat frame;
        *video >> frame;
        type = frame.type();
        aspectRatios.push_back(static_cast<double>(frame.cols) / frame.rows);

        video->set(cv::CAP_PROP_POS_FRAMES, startFrame); 
    }

    mosaicRatio =  calculateMedian(aspectRatios);
    int targetHeight = targetWidth/mosaicRatio;
    int tileWidth = targetWidth/gridWidth;
    int tileHeight = targetHeight/gridHeight;

    // Format the date as a string
    std::stringstream ss;
    ss << "mosaic_output_" << std::put_time(std::localtime(&time), "%Y%m%d") << ".mp4";
    std::string outputFilename = ss.str();
    // OpenCV video writer
    cv::VideoWriter outputVideo(outputFilename, cv::VideoWriter::fourcc('X', '2', '6', '4'), 30,
                                cv::Size(targetWidth, targetHeight));

    for (int j = 0; j < clipDuration * fps; ++j) {
        displayProgressBar(j, clipDuration * fps);

        cv::Mat mosaic = cv::Mat::zeros(targetHeight, targetWidth, type);

        for (int i = 0; i < numClips; ++i) {
            cv::Mat clip;
            *videos[i] >> clip;
            // Calculate the position in the mosaic
            int row = i / gridWidth;
            int col = i % gridWidth;
            int posX = col * tileWidth;
            int posY = row * tileHeight;
            // Overlay the clip on the mosaic
            cv::Rect roi(posX, posY, tileWidth, tileHeight);

            cv::Mat tile(tileHeight, tileWidth, type, cv::Scalar(0, 0, 0));
            // cv::resize(clip, tile, cv::Size(tileWidth, tileHeight));            
            // tile.copyTo(mosaic(roi));
            // Calculate the aspect ratio of the original clip
            double imageRatio = static_cast<double>(clip.cols) / clip.rows;

            // Calculate the target aspect ratio
            double tileRatio = static_cast<double>(tileWidth) / tileHeight;

            // Determine the width and height of the final image
            int width, height;
            if (imageRatio > tileRatio) {
                width = tileWidth;
                height = tileWidth / imageRatio;
            } else {
                width = tileHeight * imageRatio;
                height = tileHeight;
            }
            cv::resize(clip, clip, cv::Size(width, height));

            clip.copyTo(tile(cv::Rect((tileWidth - width)/2, (tileHeight - height)/2, clip.cols, clip.rows)));
            tile.copyTo(mosaic(roi));
        }

        // Write the mosaic frame to the output video
        outputVideo.write(mosaic);
    }

    // Release the video writer and finish
    outputVideo.release();
    std::cout << "Mosaic video created successfully: " << outputFilename << std::endl;
    
    return 0;
}