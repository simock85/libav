/*
 * muxer : Simple Muxer based on the Libav libraries
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

static AVStream *add_stream(AVFormatContext *ctx_out, AVStream *src_stream){
    AVCodecContext *codec;
    AVStream *dst_stream;

    dst_stream = av_new_stream(ctx_out, 0);

    // copy the codec into new stream
    codec = dst_stream->codec;
    avcodec_copy_context(codec, src_stream->codec);

    dst_stream->time_base = src_stream->time_base;
    dst_stream->r_frame_rate = src_stream->r_frame_rate;
    dst_stream->index = src_stream->index;
    dst_stream->id = src_stream->id;
    dst_stream->avg_frame_rate = src_stream->avg_frame_rate;
    dst_stream->pts = src_stream->pts;
    dst_stream->sample_aspect_ratio = codec->sample_aspect_ratio;
    return dst_stream;
}

int main(int argc, char **argv){
    int ret = 0, i;
    AVFormatContext *ctx_in=NULL, *ctx_out;
    AVInputFormat *i_format=NULL;
    AVOutputFormat *o_format=NULL;
    AVStream *stream;
    AVPacket pkt;

    av_register_all();

    ret = avformat_open_input(&ctx_in, argv[1], i_format, NULL);

    if ((ret = avformat_find_stream_info(ctx_in, NULL)) < 0) {
        printf("find stream info failed\n");
    }

    av_dump_format(ctx_in, 0, argv[1], 0);



    av_init_packet(&pkt);

    if (!o_format) {
        printf("Trying to deduce output format from file extension\n");
        o_format = av_guess_format(NULL, argv[1], NULL);
    }
    if (!o_format) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        o_format = av_guess_format("mpeg", NULL, NULL);
    }
    if (!o_format) {
        fprintf(stderr, "Could not find suitable output format\n");
        exit(1);
    }

    ctx_out = avformat_alloc_context();

    avio_open(&ctx_out->pb, argv[2], URL_WRONLY);

    ctx_out->oformat = o_format;

    for (i=0; i<ctx_in->nb_streams; i++){
        stream = add_stream(ctx_out, ctx_in->streams[i]);
    }

    avformat_write_header(ctx_out, NULL);


    while (!av_read_frame(ctx_in, &pkt)){
        if (pkt.pts == AV_NOPTS_VALUE){
            printf("No pts\n");
        }
        else {
            printf("pts %"PRId64"\n", pkt.pts );
        }
        av_interleaved_write_frame(ctx_out, &pkt);
        av_free_packet(&pkt);
    }

    av_write_trailer(ctx_out);

    return ret;
}
