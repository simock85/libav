/*
 * decode.c
 *
 *  Created on: 14/ott/2011
 *      Author: simock85
 */

#include "../libavformat/avformat.h"
#include "../libavcodec/avcodec.h"

int main(int argc, char **argv){
    int ret, i, got_ptr=0;
    AVFormatContext *ctx=NULL;
    AVInputFormat *format=NULL;
    AVPacket pkt;
    AVFrame *img = avcodec_alloc_frame();
    AVCodecContext *cur_codec;
    AVCodec *codec;


    av_register_all();

    ret = avformat_open_input(&ctx, argv[1], format, NULL);

    if ((ret = avformat_find_stream_info(ctx, NULL)) < 0) {
        printf("find stream info failed\n");
    }

    av_dump_format(ctx, 0, argv[1], 0);

    av_init_packet(&pkt);

    for (i=0; i<ctx->nb_streams; i++){
        cur_codec = ctx->streams[i]->codec;
        if (cur_codec->codec_type == AVMEDIA_TYPE_VIDEO){
            codec = avcodec_find_decoder(cur_codec->codec_id);
            if(avcodec_open2(cur_codec, codec, NULL)<0){
                exit(1);
            }
            while (!av_read_frame(ctx, &pkt)){
                if (avcodec_decode_video2(cur_codec, img, &got_ptr, &pkt)<0){
                    exit(2);
                }
                printf("key_frame: %d\n", img->key_frame);
                av_free_packet(&pkt);
//                if (!(got_ptr)){
//                    exit(1);
//                }
            }
        }
    }
    return ret;
}
