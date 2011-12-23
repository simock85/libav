/*
 * decode.c
 *
 *      Author: simock85
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <stdlib.h>

static void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

    // Close file
    fclose(pFile);
}

int main (int argc, const char * argv[])
{
    AVFormatContext *pFormatCtx=NULL;
    AVInputFormat *format=NULL;
    int i, videoStream, ret=0;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame;
    AVFrame *pFrameRGB;
    AVPacket packet;
    int frameFinished;
    int numBytes;
    uint8_t *buffer;

    av_register_all();

    if(avformat_open_input(&pFormatCtx, argv[1], format, NULL)!=0)
        return -1;

    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return -1;

    av_dump_format(pFormatCtx, 0, argv[1], 0);

    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoStream=i;
            break;
        }
    if(videoStream==-1)
        return -1;

    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL)
        return -1;

    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
        return -1;

    if(pCodecCtx->time_base.num>1000 && pCodecCtx->time_base.den==1)
        pCodecCtx->time_base.den=1000;

    pFrame=avcodec_alloc_frame();

    pFrameRGB=avcodec_alloc_frame();
    if(pFrameRGB==NULL)
        return -1;

    numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
            pCodecCtx->height);

    buffer=malloc(numBytes);

    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
            pCodecCtx->width, pCodecCtx->height);

    i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0)
    {
        if(packet.stream_index==videoStream)
        {
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
                    &packet);

            if(frameFinished)
            {
                static struct SwsContext *img_convert_ctx;
                if(img_convert_ctx == NULL) {
                    int w = pCodecCtx->width;
                    int h = pCodecCtx->height;

                    img_convert_ctx = sws_getContext(w, h,
                            pCodecCtx->pix_fmt,
                            w, h, PIX_FMT_RGB24, SWS_BICUBIC,
                            NULL, NULL, NULL);
                    if(img_convert_ctx == NULL) {
                        fprintf(stderr, "Cannot initialize the conversion context!\n");
                        exit(1);
                    }
                }
                ret = sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
                        pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i++);
            }
        }
        av_free_packet(&packet);
    }

    free(buffer);
    av_free(pFrameRGB);

    av_free(pFrame);

    avcodec_close(pCodecCtx);

    av_close_input_file(pFormatCtx);

    return ret;
}
