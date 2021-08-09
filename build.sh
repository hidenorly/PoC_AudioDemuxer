#!/bin/sh
clang++ src/FFMpegDecoderPrimitive.cpp src/main.cpp  -std=c++2a -I ../audioframework/include/ -I ../ffmpeg-sdk/include ../ffmpeg-sdk/lib/libavformat.dylib ../ffmpeg-sdk/lib/libavcodec.dylib ../ffmpeg-sdk/lib/libavutil.dylib
