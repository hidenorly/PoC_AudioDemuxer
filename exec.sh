#!/bin/sh
rm hoge.pcm
./a.out example.mp4 hoge.pcm
ffplay -nodisp -f f32le -ac 1 -ar 48000 ./hoge.pcm