# PoC_AudioDemuxer

```
$ clang++ src/FFMpegDecoderPrimitive.cpp src/main.cpp  -std=c++2a -I ../audioframework/include/ -I ../ffmpeg-sdk/include ../ffmpeg-sdk/lib/libavformat.dylib ../ffmpeg-sdk/lib/libavcodec.dylib ../ffmpeg-sdk/lib/libavutil.dylib
$ ./a.out example.mp4 hoge.pcm
$ ffplay -f f32le -ac 1 -ar 48000 hoge.pcm
```