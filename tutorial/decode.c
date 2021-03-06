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

static void SaveFrame(AVFrame *frame, int width, int height, int i_frame)
{
    FILE *file;
    char sz_filename[32];
    int  y;

    // Open file
    sprintf(sz_filename, "frame%d.ppm", i_frame);
    file=fopen(sz_filename, "wb");
    if(file==NULL)
        return;

    // Write header
    fprintf(file, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(frame->data[0]+y*frame->linesize[0], 1, width*3, file);

    // Close file
    fclose(file);
}

int main (int argc, const char * argv[])
{
    AVFormatContext *format_ctx=NULL;
    AVInputFormat *format=NULL;
    int i, video_stream, ret=0;
    AVCodecContext *codec_ctx;
    AVCodec *codec;
    AVFrame *frame;
    AVFrame *frame_RGB;
    AVPacket packet;
    int frame_finished;
    int num_bytes;
    uint8_t *buffer;

    av_register_all();

    if(avformat_open_input(&format_ctx, argv[1], format, NULL)!=0)
        return -1;

    if(avformat_find_stream_info(format_ctx, NULL)<0)
        return -1;

    av_dump_format(format_ctx, 0, argv[1], 0);

    video_stream=-1;
    for(i=0; i<format_ctx->nb_streams; i++)
        if(format_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            video_stream=i;
            break;
        }
    if(video_stream==-1)
        return -1;

    codec_ctx=format_ctx->streams[video_stream]->codec;

    codec=avcodec_find_decoder(codec_ctx->codec_id);
    if(codec==NULL)
        return -1;

    if(avcodec_open2(codec_ctx, codec, NULL)<0)
        return -1;

    if(codec_ctx->time_base.num>1000 && codec_ctx->time_base.den==1)
        codec_ctx->time_base.den=1000;

    frame=avcodec_alloc_frame();

    frame_RGB=avcodec_alloc_frame();
    if(frame_RGB==NULL)
        return -1;

    num_bytes=avpicture_get_size(PIX_FMT_RGB24, codec_ctx->width,
            codec_ctx->height);

    buffer=malloc(num_bytes);

    avpicture_fill((AVPicture *)frame_RGB, buffer, PIX_FMT_RGB24,
            codec_ctx->width, codec_ctx->height);

    i=0;
    while(av_read_frame(format_ctx, &packet)>=0)
    {
        if(packet.stream_index==video_stream)
        {
            avcodec_decode_video2(codec_ctx, frame, &frame_finished,
                    &packet);

            if(frame_finished)
            {
                static struct SwsContext *img_convert_ctx;
                if(img_convert_ctx == NULL) {
                    int w = codec_ctx->width;
                    int h = codec_ctx->height;

                    img_convert_ctx = sws_getContext(w, h,
                            codec_ctx->pix_fmt,
                            w, h, PIX_FMT_RGB24, SWS_BICUBIC,
                            NULL, NULL, NULL);
                    if(img_convert_ctx == NULL) {
                        fprintf(stderr, "Cannot initialize the conversion context!\n");
                        exit(1);
                    }
                }
                ret = sws_scale(img_convert_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0,
                        codec_ctx->height, frame_RGB->data, frame_RGB->linesize);

                SaveFrame(frame_RGB, codec_ctx->width, codec_ctx->height, i++);
            }
        }
        av_free_packet(&packet);
    }

    free(buffer);
    av_free(frame_RGB);

    av_free(frame);

    avcodec_close(codec_ctx);

    av_close_input_file(format_ctx);

    return ret;
}
