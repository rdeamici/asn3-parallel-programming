rm frame* >/dev/null 2>&1
./camera_canny_pthread 1 .2 .6 80
ffmpeg -i frame%03d.pgm -pix_fmt yuvj420p frame_vid.h264
cvlc frame_vid.h264
