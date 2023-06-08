# Mosaic teaser generator
Take as input a video folder , duration, and mosaic size w, h
build/mosaic_gen -f path/to/folder -d clip/duration -s wxh 

# Notes
--
Moved to Opencv 4.5 as 3.2 version video capture read() doesn't handle rotation flag