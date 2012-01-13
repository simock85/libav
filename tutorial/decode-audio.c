#include "../libavformat/avformat.h"
/*
 * decode-audio.c
 *
 *  Created on: Jan 13, 2012
 *      Author: simock85
 */

int main (int argc, const char *argv[]){
    AVFormatContext *pFormatCtx=NULL;
    AVInputFormat *format=NULL;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    int i, audioStream;

    av_register_all();
    if(avformat_open_input(&pFormatCtx, argv[1], format, NULL)!=0)
        return -1;

    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return -1;

    av_dump_format(pFormatCtx, 0, argv[1], 0);

    audioStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            audioStream = i;
            break;
        }
    }

    if (audioStream < 0)
        return -1;

    pCodecCtx = pFormatCtx->streams[audioStream]->codec;

    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec==NULL)
        return -1;



    return 0;
}
