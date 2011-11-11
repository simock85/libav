/*
 * decode.c
 *
 *  Created on: 14/ott/2011
 *      Author: simock85
 */

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswscale/swscale_internal.h>

static void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;

  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;

  fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

  fclose(pFile);
}

int main(int argc, char **argv){
    int ret, i, got_ptr=0, j;
    AVFormatContext *ctx=NULL;
    AVInputFormat *format=NULL;
    AVPacket pkt;
    AVFrame *img = avcodec_alloc_frame(), *imgRGB = avcodec_alloc_frame();
    AVCodecContext *cur_codec;
    AVCodec *codec;
    SwsContext *sws_context = NULL;


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
            sws_context = sws_getContext(cur_codec->width, cur_codec->height, cur_codec->pix_fmt,
                    704, 576, PIX_FMT_RGB24, SWS_FAST_BILINEAR, 0, 0, 0);
            if(avcodec_open2(cur_codec, codec, NULL)<0){
                exit(1);
            }
            j = 0;
            while (!av_read_frame(ctx, &pkt)){
                if (avcodec_decode_video2(cur_codec, img, &got_ptr, &pkt)<0){
                    exit(2);
                }
                if (got_ptr){
                    sws_scale(sws_context, (const uint8_t* const*)img->data, img->linesize,
                            0, cur_codec->height, imgRGB->data, imgRGB->linesize);

                    if(++j<=5){
                        SaveFrame(imgRGB, cur_codec->width, cur_codec->height, j);
                    }
                }
                av_free_packet(&pkt);
            }
        }
    }
    return ret;
}
