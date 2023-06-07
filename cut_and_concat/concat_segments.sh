#!/bin/bash

input_folder=$1
# Change directory to the folder containing the videos
cd $1

# Create a temporary file to store the list of video files
tmp=$(mktemp)

# Loop through all video files in the folder, sort them by last modification date, and write their paths to the temporary file
for f in $(ls -tr *.mp4); do echo "file '$PWD/$f'" >> "$tmp"; done
#for f in $(ls -v *.mp4); do echo "file '$PWD/$f'" >> "$tmp"; done

# Loop through all video files in the folder, sort them alphabetically by name, and write their paths to the temporary file
#for f in $(ls *.mp4 | sort); do echo "file '$PWD/$f'" >> "$tmp"; done

# Concatenate the videos using FFmpeg's concat demuxer
ffmpeg -f concat -safe 0 -i "$tmp" -c copy output.mp4

# Remove the temporary file
rm "$tmp"
