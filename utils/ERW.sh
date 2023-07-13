# only for images.
# ERW: Enhance Resolution , Resize, and Watermark
# Usage: ERW.sh [inputfolder] [outputfolder] [logo]
# if not 3 arguments, exit
#example: ERW.sh ./input ./output /home/username/Downloads/logo.png
if [ $# -ne 3 ]; then
    echo "Usage: ERW.sh [inputfolder] [outputfolder] [logo]"
    exit 1
fi
pwd
# 1. Enhance Resolution
tmpe="enhancer/output"
enhancer/build/enhancer -i $1 -o $tmpe -s 2 -m enhancer/models/FSRCNN-small_x2.pb
# 2. Resize
tmpr="resizer/output"
# sudo mkdir $tmpr
resizer/build/resizer -i $tmpe -o $tmpr -s 3024x4032 -b
# 3. Watermark
watermark/build/watermark -i $tmpr -o $2 -l $3 -p 10 10