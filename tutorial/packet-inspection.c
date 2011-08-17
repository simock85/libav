/*
 * packet-inspection : Super Simple Media Prober based on the Libav libraries
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "config.h"

#include "libavformat/avformat.h"
#include "libavutil/dict.h"
#include "libavcodec/avcodec.h"

static const char *binary_unit_prefixes [] = { "", "Ki", "Mi", "Gi", "Ti", "Pi" };
static const char *decimal_unit_prefixes[] = { "", "K" , "M" , "G" , "T" , "P"  };

static const char *unit_second_str          = "s"    ;
static const char *unit_hertz_str           = "Hz"   ;
static const char *unit_byte_str            = "byte" ;
static const char *unit_bit_per_second_str  = "bit/s";

static int show_value_unit              = 0;
static int use_value_prefix             = 0;
static int use_byte_value_binary_prefix = 0;
static int use_value_sexagesimal_format = 0;



static char *value_string(char *buf, int buf_size, double val, const char *unit)
{
    if (unit == unit_second_str && use_value_sexagesimal_format) {
        double secs;
        int hours, mins;
        secs  = val;
        mins  = (int)secs / 60;
        secs  = secs - mins * 60;
        hours = mins / 60;
        mins %= 60;
        snprintf(buf, buf_size, "%d:%02d:%09.6f", hours, mins, secs);
    } else if (use_value_prefix) {
        const char *prefix_string;
        int index;

        if (unit == unit_byte_str && use_byte_value_binary_prefix) {
            index = (int) (log(val)/log(2)) / 10;
            index = av_clip(index, 0, FF_ARRAY_ELEMS(binary_unit_prefixes) -1);
            val /= pow(2, index*10);
            prefix_string = binary_unit_prefixes[index];
        } else {
            index = (int) (log10(val)) / 3;
            index = av_clip(index, 0, FF_ARRAY_ELEMS(decimal_unit_prefixes) -1);
            val /= pow(10, index*3);
            prefix_string = decimal_unit_prefixes[index];
        }

        snprintf(buf, buf_size, "%.3f %s%s", val, prefix_string, show_value_unit ? unit : "");
    } else {
        snprintf(buf, buf_size, "%f %s", val, show_value_unit ? unit : "");
    }

    return buf;
}

static char *time_value_string(char *buf, int buf_size, int64_t val, const AVRational *time_base)
{
    if (val == AV_NOPTS_VALUE) {
        snprintf(buf, buf_size, "N/A");
    } else {
        value_string(buf, buf_size, val * av_q2d(*time_base), unit_second_str);
    }

    return buf;
}

static char *ts_value_string (char *buf, int buf_size, int64_t ts)
{
    if (ts == AV_NOPTS_VALUE) {
        snprintf(buf, buf_size, "N/A");
    } else {
        snprintf(buf, buf_size, "%"PRId64, ts);
    }

    return buf;
}

static const char *media_type_string(enum AVMediaType media_type)
{
    switch (media_type) {
    case AVMEDIA_TYPE_VIDEO:      return "video";
    case AVMEDIA_TYPE_AUDIO:      return "audio";
    case AVMEDIA_TYPE_DATA:       return "data";
    case AVMEDIA_TYPE_SUBTITLE:   return "subtitle";
    case AVMEDIA_TYPE_ATTACHMENT: return "attachment";
    default:                      return "unknown";
    }
}

static void show_usage(void)
{
    printf("Super Simple multimedia streams prober\n");
    printf("usage: packet-inspection [INPUT_FILE]\n");
    printf("\n");
}

static int open_input_file(AVFormatContext *fmt_ctx, const char *filename, AVInputFormat *fmt, AVDictionary *options)
{
    int err, i;
    AVDictionaryEntry *t;

    if ((err = avformat_open_input(&fmt_ctx, filename, fmt, &options)) < 0) {
        printf("error");
        return err;
    }
    if ((t = av_dict_get(options, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        return AVERROR_OPTION_NOT_FOUND;
    }


    /* fill the streams in the format context */
    if ((err = av_find_stream_info(fmt_ctx)) < 0) {
        printf("error");
        return err;
    }

    av_dump_format(fmt_ctx, 0, filename, 0);

    /* bind a decoder to each input stream */
    for (i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream *stream = fmt_ctx->streams[i];
        AVCodec *codec;

        if (!(codec = avcodec_find_decoder(stream->codec->codec_id))) {
            fprintf(stderr, "Unsupported codec with id %d for input stream %d\n",
                    stream->codec->codec_id, stream->index);
        } else if (avcodec_open2(stream->codec, codec, NULL) < 0) {
            fprintf(stderr, "Error while opening codec for input stream %d\n",
                    stream->index);
        }
    }

    return 0;
}

static void show_packet(AVFormatContext *fmt_ctx, AVPacket *pkt)
{
    char val_str[128];
    AVStream *st = fmt_ctx->streams[pkt->stream_index];

    printf("[PACKET]\n");
    printf("codec_type=%s\n"   , media_type_string(st->codec->codec_type));
    printf("stream_index=%d\n" , pkt->stream_index);
    printf("pts=%s\n"          , ts_value_string  (val_str, sizeof(val_str), pkt->pts));
    printf("pts_time=%s\n"     , time_value_string(val_str, sizeof(val_str), pkt->pts, &st->time_base));
    printf("dts=%s\n"          , ts_value_string  (val_str, sizeof(val_str), pkt->dts));
    printf("dts_time=%s\n"     , time_value_string(val_str, sizeof(val_str), pkt->dts, &st->time_base));
    printf("duration=%s\n"     , ts_value_string  (val_str, sizeof(val_str), pkt->duration));
    printf("duration_time=%s\n", time_value_string(val_str, sizeof(val_str), pkt->duration, &st->time_base));
    printf("size=%s\n"         , value_string     (val_str, sizeof(val_str), pkt->size, unit_byte_str));
    printf("pos=%"PRId64"\n"   , pkt->pos);
    printf("flags=%c\n"        , pkt->flags & AV_PKT_FLAG_KEY ? 'K' : '_');
    printf("[/PACKET]\n");
}

static void show_packets(AVFormatContext *fmt_ctx)
{
    AVPacket pkt;

    av_init_packet(&pkt);

    while (!av_read_frame(fmt_ctx, &pkt))
        show_packet(fmt_ctx, &pkt);
}


int main(int argc, char **argv)
{
    int ret = 0;
    AVFormatContext *fmt_ctx;
    AVInputFormat *iformat = NULL;
    AVDictionary *format_opts;

    av_register_all();

    fmt_ctx = malloc(sizeof(AVFormatContext));

    if ((ret = open_input_file(fmt_ctx, argv[1], iformat, format_opts)))
        return ret;

    show_packets(fmt_ctx);

    if (argc != 2){
        show_usage();
    }

    return ret;
}
