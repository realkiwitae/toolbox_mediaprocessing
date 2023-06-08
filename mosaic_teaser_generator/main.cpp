#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <opencv2/opencv.hpp>
#include <dirent.h>
#include <sys/stat.h>
#include <experimental/filesystem>

bool copyFile(const std::string& sourceFile, const std::string& targetFile) {
    std::ifstream src(sourceFile, std::ios::binary);
    std::ofstream dst(targetFile, std::ios::binary);

    if (!src || !dst) {
        std::cerr << "Failed to open files" << std::endl;
        return false;
    }

    struct stat fileInfo;
    if (stat(sourceFile.c_str(), &fileInfo) != 0) {
        std::cerr << "Failed to get file information" << std::endl;
        return false;
    }

    char buffer[BUFSIZ];
    std::streamsize bytesRead;
    while ((bytesRead = src.readsome(buffer, sizeof(buffer)))) {
        if (!dst.write(buffer, bytesRead)) {
            std::cerr << "Failed to write to target file" << std::endl;
            return false;
        }
    }

    src.close();
    dst.close();

    if (chmod(targetFile.c_str(), fileInfo.st_mode & 07777) != 0) {
        std::cerr << "Failed to set file permissions" << std::endl;
        return false;
    }

    return true;
}

namespace fs = std::experimental::filesystem;

bool deleteFolder(const std::string& folderPath) {
    try {
        fs::remove_all(folderPath);
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "Failed to delete folder: " << ex.what() << std::endl;
        return false;
    }
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

int main(int argc, char* argv[]) {
    // Set the parameters
    int gridWidth = 2;
    int gridHeight = 2;
    int targetWidth = 1080;
    int targetHeight = 4*targetWidth/3;
    int tileWidth = targetWidth/gridWidth;
    int tileHeight = targetHeight/gridHeight;
    int clipDuration = 3;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " -f <folder_path>" << std::endl;
        return 1;
    }

    std::string videoFolder;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            videoFolder = argv[i + 1];
            break;
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

    // Format the date as a string
    std::stringstream ss;
    ss << "mosaic_output_" << std::put_time(std::localtime(&time), "%Y%m%d") << ".mp4";
    std::string outputFilename = ss.str();
    // OpenCV video writer
    cv::VideoWriter outputVideo(outputFilename, cv::VideoWriter::fourcc('X', '2', '6', '4'), 30,
                                cv::Size(targetWidth, targetHeight));

    struct stat info;
    if (stat("/tmp/clips/", &info) == -1) {
        mkdir("/tmp/clips/", 0700);
    }
    double fps;
    int width;
    int height;
    int type=-3;
    // Extract random clips and create the mosaic
    std::vector<cv::VideoCapture*> videos;
    for (int i = 0; i < numClips; ++i) {
        // Load the video
        if (copyFile(videoFiles[randomIndices[i]], "/tmp/clips/clip_"+std::to_string(i)+".mp4")) {
            std::cout << "Video file copied to: " << "/tmp/clips/clip_"+std::to_string(i)+".mp4" << std::endl;
            videos.push_back(new cv::VideoCapture(videoFiles[randomIndices[i]]));
            cv::VideoCapture* video = videos.back(); 
  
            // Get the input video's properties
            fps = video->get(cv::CAP_PROP_FPS);
            width = video->get(cv::CAP_PROP_FRAME_WIDTH);
            height = video->get(cv::CAP_PROP_FRAME_HEIGHT);

            // Get a random start frame
            int frameCount = video->get(cv::CAP_PROP_FRAME_COUNT);
            int startFrame = std::rand() % int(frameCount - clipDuration * fps);
            // Set the current frame to the start frame
            if(type == -3){
                cv::Mat frame;
                *video >> frame;
                type = frame.type();
            }
            video->set(cv::CAP_PROP_POS_FRAMES, startFrame); 
        } else {
            std::cerr << "Failed to copy video file" << std::endl;
            exit(1);
        }
    }

    for (int j = 0; j < clipDuration * fps; ++j) {
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

            cv::Mat tile(tileHeight, tileWidth, type, cv::Scalar(255, 255, 255));
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
    
    deleteFolder("/tmp/clips/");;
    
    return 0;
}