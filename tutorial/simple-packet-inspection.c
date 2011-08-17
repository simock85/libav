/*
 * simple-packet-inspection : Super Simple Media Prober based on the Libav libraries
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


int main(int argc, char **argv){
    int ret = 0;
    AVFormatContext *fmt_ctx=NULL;
    AVInputFormat *i_format=NULL;
    AVPacket pkt;

    av_register_all();

    ret = avformat_open_input(&fmt_ctx, argv[1], i_format, NULL);

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {

    }
    av_dump_format(fmt_ctx, 0, argv[1], 0);


    av_init_packet(&pkt);

    while (!av_read_frame(fmt_ctx, &pkt)){
        printf("pts %"PRId64"\n", pkt.pts );
        av_free_packet(&pkt);
    }

    return ret;


}
