/*
  Copyright (C) 2021 hidenorly

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "FFMpegDecoderPrimitive.hpp"
#include <iostream>
#include <filesystem>

class StreamWriter : public IDecoderOutput
{
  FILE* mFd;
  size_t mLength;

public:
  StreamWriter(std::string path){
    mFd = fopen(path.c_str(), "wb");
  };
  virtual ~StreamWriter(){
    if( mFd ){
      fclose( mFd );
    }
    mFd = nullptr;
  };
  virtual void onDecodeOutput(uint8_t* pData, size_t nLength)
  {
    if( mFd ){
      fwrite( pData, 1, nLength, mFd );
    }
  };
};

class Util
{
public:
  static std::string getFormatStringFromSampleFormat(enum AVSampleFormat sample_fmt)
  {
    std::string result;

    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt;
        const char* fmt_be;
        const char* fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };

    for (int i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
      struct sample_fmt_entry *entry = &sample_fmt_entries[i];
      if (sample_fmt == entry->sample_fmt) {
        result = AV_NE(entry->fmt_be, entry->fmt_le);
        break;
      }
    }

    return result;
  }
};


int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cout << "usage: " << argv[0] << " source.mp4 audiodecoded.pcm";
    exit(-1);
  }

  IDecoderOutput* pOutputHandler = new StreamWriter(argv[2]);
  AudioDemuxerDecoder decoder( argv[1], pOutputHandler );
  if( decoder.demux() ){
    if( decoder.setupDecoder() ){
      while( decoder.doDecodePacket() ){};

      enum AVSampleFormat sampleFormat;
      int samplingRate;
      int numOfChannels;
      bool bSuccess = decoder.getDecoderOutputFormat( sampleFormat, samplingRate, numOfChannels );
      std::cout << "getDecoderOutputFormat():" << (bSuccess ? "true" : "false") << " " << sampleFormat << " " << samplingRate << " " << numOfChannels << std::endl;

      decoder.finalizeDecoder();

      if( av_sample_fmt_is_planar(sampleFormat) ){
        std::string packed = av_get_sample_fmt_name(sampleFormat);
        std::cout << "Warning: the sample format the decoder produced is planar (" << ( !packed.empty() ? packed : "?" ) << "). This example will output the first channel only." << std::endl;
        sampleFormat = av_get_packed_sample_fmt(sampleFormat);
        numOfChannels = 1;
      }

      std::string audioFormatString;
      audioFormatString = Util::getFormatStringFromSampleFormat(sampleFormat);
      if( !audioFormatString.empty() ){
        std::cout << "Play the output audio file with the command:" << std::endl;
        std::cout << "ffplay -f " << audioFormatString << " -ac " << numOfChannels << " -ar " << samplingRate << std::endl;
      }
    }
  }
  delete pOutputHandler; pOutputHandler = nullptr;
}

