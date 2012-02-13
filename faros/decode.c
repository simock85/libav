#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include <SDL_thread.h>

int main (int argc, const char * argv[])
{
    AVFormatContext *format_ctx=NULL;
    AVInputFormat *format=NULL;
    int i, video_stream, ret=0;
    AVCodecContext *codec_ctx;
    AVCodec *codec;
    AVFrame *frame;
    AVFrame *frame_YUV;
    AVPacket packet;
    int frame_finished;
    int num_bytes;
    uint8_t *buffer;
    SDL_Surface *screen;
    SDL_Overlay *bmp;
    SDL_Rect rect;


    av_register_all();

    if(avformat_open_input(&format_ctx, argv[1], format, NULL)!=0)
        return -1;

    if(avformat_find_stream_info(format_ctx, NULL)<0)
        return -1;

    av_dump_format(format_ctx, 0, argv[1], 0);

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

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

    frame_YUV=avcodec_alloc_frame();
    if(frame_YUV==NULL)
        return -1;

    screen = SDL_SetVideoMode(codec_ctx->width, codec_ctx->height, 0, 0);
    if(!screen) {
        fprintf(stderr, "SDL: could not set video mode - exiting\n");
        exit(1);
    }

    bmp = SDL_CreateYUVOverlay(codec_ctx->width, codec_ctx->height,
                               SDL_YV12_OVERLAY, screen);

    num_bytes=avpicture_get_size(PIX_FMT_YUV420P, codec_ctx->width,
            codec_ctx->height);

    buffer=malloc(num_bytes);

    avpicture_fill((AVPicture *)frame_YUV, buffer, PIX_FMT_YUV420P,
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
                            w, h, PIX_FMT_YUV420P, SWS_BICUBIC,
                            NULL, NULL, NULL);
                    if(img_convert_ctx == NULL) {
                        fprintf(stderr, "Cannot initialize the conversion context!\n");
                        exit(1);
                    }
                }
                SDL_LockYUVOverlay(bmp);
                frame_YUV->data[0] = bmp->pixels[0];
                frame_YUV->data[1] = bmp->pixels[2];
                frame_YUV->data[2] = bmp->pixels[1];

                frame_YUV->linesize[0] = bmp->pitches[0];
                frame_YUV->linesize[1] = bmp->pitches[2];
                frame_YUV->linesize[2] = bmp->pitches[1];

                ret = sws_scale(img_convert_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0,
                        codec_ctx->height, frame_YUV->data, frame_YUV->linesize);

                SDL_UnlockYUVOverlay(bmp);
                rect.x = 0;
                rect.y = 0;
                rect.w = codec_ctx->width;
                rect.h = codec_ctx->height;
                SDL_DisplayYUVOverlay(bmp, &rect);
            }
        }
        av_free_packet(&packet);
    }

    free(buffer);
    av_free(frame_YUV);

    av_free(frame);

    avcodec_close(codec_ctx);

    av_close_input_file(format_ctx);

    return ret;
}
