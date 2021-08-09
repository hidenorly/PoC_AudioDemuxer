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

AudioDemuxerDecoder::AudioDemuxerDecoder(std::string srcFilename, IDecoderOutput* pDecoderOutput):
    mFormatContext(nullptr),
    mDecoderContext(nullptr),
    mAudioStream(nullptr),
    mSrcFilename(srcFilename),
    mStreamIndex(-1),
    mFrame(nullptr),
    mPacket(nullptr),
    mDecoderOutput(pDecoderOutput)
{

}

AudioDemuxerDecoder::~AudioDemuxerDecoder()
{
  close();
}

void AudioDemuxerDecoder::close(void)
{
  finalizeDecoder();
  mDecoderOutput = nullptr; // should delete by the user of this class.
}

int AudioDemuxerDecoder::decodePacket(const AVPacket* packet)
{
  int ret = 0;

  // submit the packet to the decoder
  if( !avcodec_send_packet(mDecoderContext, packet) ){
    // get all the available frames from the decoder
    while (ret >= 0) {
      ret = avcodec_receive_frame(mDecoderContext, mFrame);
      // write the frame data to output file
      if( (ret >= 0) && mDecoderContext && mDecoderContext->codec->type == AVMEDIA_TYPE_AUDIO ){
        if( mDecoderOutput ){
          // TODO : convert planer to packed
          size_t nBytes = mFrame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)mFrame->format);
          mDecoderOutput->onDecodeOutput((uint8_t*)mFrame->extended_data[0], nBytes);
        }
      }
      av_frame_unref(mFrame);
    }
  }

  return 0;
}

int AudioDemuxerDecoder::openCodec(void)
{
  int result = -1;

  int streamIndex = av_find_best_stream(mFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
  if( streamIndex >= 0 ){
    AVStream* theStream = mFormatContext->streams[streamIndex];

    /* find decoder for the stream */
    const AVCodec* theDecoder = avcodec_find_decoder(theStream->codecpar->codec_id);
    if( theDecoder ){
      /* Allocate a codec context for the decoder */
      mDecoderContext = avcodec_alloc_context3(theDecoder);
      if( mDecoderContext ){
        /* Copy codec parameters from input stream to output codec context */
        if( !avcodec_parameters_to_context(mDecoderContext, theStream->codecpar) ) {
          /* Init the decoders */
          if( !avcodec_open2(mDecoderContext, theDecoder, NULL) ){
            result = streamIndex;
          }
        }
      }
    }
  }

  return result;
}

bool AudioDemuxerDecoder::getDecoderOutputFormat(enum AVSampleFormat& sampleFormat, int& samplingRate, int& numOfChannels)
{
  bool result = (mDecoderContext != nullptr);

  if( mDecoderContext ){
    sampleFormat = mDecoderContext->sample_fmt;
    samplingRate = mDecoderContext->sample_rate;
    numOfChannels = mDecoderContext->channels;
  }
  return result;
}

bool AudioDemuxerDecoder::demux(void)
{
  bool result = false;

  if( !avformat_open_input(&mFormatContext, mSrcFilename.c_str(), NULL, NULL) ) {
    /* retrieve stream information */
    if( !avformat_find_stream_info(mFormatContext, NULL) ) {
      result = true;
    }
  }

  return result;
}

bool AudioDemuxerDecoder::setupDecoder(void)
{
  bool result = false;

  mStreamIndex = openCodec();
  if( mStreamIndex != -1 ){
    if( mFormatContext->streams[mStreamIndex] ){
      if( mPacket ){
        av_packet_free(&mPacket);
        mPacket = nullptr;
      }
      if( mFrame ){
        av_frame_free(&mFrame);
        mFrame = nullptr;
      }
      mFrame = av_frame_alloc();
      mPacket = av_packet_alloc();
      result = (mFrame != nullptr) && (mPacket != nullptr);
    }
  }

  return result;
}

bool AudioDemuxerDecoder::doDecodePacket(void)
{
  bool result = ( av_read_frame(mFormatContext, mPacket) >= 0);
  if( result ){
    if (mPacket->stream_index == mStreamIndex) {
      result = result & (!decodePacket(mPacket));
    }
    av_packet_unref(mPacket);
  }
  return result;
}

void AudioDemuxerDecoder::finalizeDecoder(void)
{
  /* flush the decoders */
  if (mDecoderContext){
    decodePacket(NULL);
    avcodec_free_context(&mDecoderContext);
    mDecoderContext = nullptr;
  }
  if( mFormatContext ){
    avformat_close_input(&mFormatContext);
    mFormatContext = nullptr;
  }
  if( mPacket ){
    av_packet_free(&mPacket);
    mPacket = nullptr;
  }
  if( mFrame ){
    av_frame_free(&mFrame);
    mFrame = nullptr;
  }
}
