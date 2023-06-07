#!/bin/bash

input_file=$1
segment_duration=$2

# Get the total duration of the input video
total_duration=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$input_file")

# Calculate the number of segments
num_segments=$(awk "BEGIN {print int($total_duration / $segment_duration)}")

# Iterate over each segment
for ((i = 0; i < num_segments+1; i++)); do
  start_time=$((i * segment_duration))
  output_file="output_$(printf "%02d" $((i + 1))).mp4"
  
  # Use ffmpeg to cut the segment
  ffmpeg -ss "$start_time" -t "$segment_duration" -i "$input_file" -c copy "$output_file"
done
