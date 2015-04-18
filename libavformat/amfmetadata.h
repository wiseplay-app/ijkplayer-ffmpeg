/*
 * Adobe Action Message Format Parser
 * Copyright (c) 2013 Cory McCarthy
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * @brief Adobe Action Message Format Parser
 * @author Cory McCarthy
 * @see http://download.macromedia.com/f4v/video_file_format_spec_v10_1.pdf
 * @see http://www.adobe.com/content/dam/Adobe/en/devnet/amf/pdf/amf-file-format-spec.pdf
 */

#include "libavcodec/avcodec.h"

typedef struct AMFMetadata {
    int width;
    int height;
    int frame_rate;
    int audio_sample_rate;
    int nb_audio_channels;
    int audio_data_rate;
    int video_data_rate;

    enum AVCodecID audio_codec_id;
    enum AVCodecID video_codec_id;
} AMFMetadata;

int ff_parse_amf_metadata(uint8_t *buffer, int buffer_size, AMFMetadata *metadata);
