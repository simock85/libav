/*
 * decode.c
 *
 *      Author: simock85
 */

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

static AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    uint8_t *picture_buf;
    int size;

    picture = avcodec_alloc_frame();
    if (!picture)
        return NULL;
    size = avpicture_get_size(pix_fmt, width, height);
    picture_buf = av_malloc(size);
    if (!picture_buf) {
        av_free(picture);
        return NULL;
    }
    avpicture_fill((AVPicture *)picture, picture_buf,
                   pix_fmt, width, height);
    return picture;
}

static void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
//  FILE *pFile;
//  char szFilename[32];
//  int  y;
//
//  sprintf(szFilename, "frame%d.ppm", iFrame);
//  pFile=fopen(szFilename, "wb");
//  if(pFile==NULL)
//    return;
//
//  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
//
//  for(y=0; y<height; y++)
//    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
//
//  fclose(pFile);
}

int main(int argc, char **argv){
    int ret, i, got_ptr=0, j, ret_sws;
    AVFormatContext *ctx=NULL;
    AVInputFormat *format=NULL;
    AVPacket pkt;
    AVFrame *img, *imgRGB;
    AVCodecContext *cur_codec;
    AVCodec *codec;
    static struct SwsContext *sws_context;


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
                    704, 576, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, 0, 0, 0);

            if (sws_context == NULL){
                printf("Cannot initialize the conversion context\n");
                exit(3);
            }


            if(avcodec_open2(cur_codec, codec, NULL)<0){
                exit(1);
            }
            j = 0;
            while (!av_read_frame(ctx, &pkt)){
                //img = alloc_picture(cur_codec->pix_fmt, cur_codec->width, cur_codec->height);
                img = avcodec_alloc_frame();
                if (avcodec_decode_video2(cur_codec, img, &got_ptr, &pkt)<0){
                    exit(2);
                }
                if (got_ptr){
                    imgRGB = alloc_picture(PIX_FMT_YUV420P, 704, 576);
                    printf("got ptr\n");
                    ret_sws = sws_scale(sws_context, (const uint8_t* const*)img->data, img->linesize,
                            0, cur_codec->height, imgRGB->data, imgRGB->linesize);
                    printf("ret_sws: %d", ret_sws);

//                    if(++j<=5){
//                        SaveFrame(imgRGB, cur_codec->width, cur_codec->height, j);
//                    }
                }
                av_free_packet(&pkt);
            }
        }
    }
    return ret;
}
