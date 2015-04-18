/*
 * Adobe FLV Tag Parser
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
 * @brief Adobe FLV Tag Parser for Adobe HDS F4F Files
 * @author Cory McCarthy
 * @see http://download.macromedia.com/f4v/video_file_format_spec_v10_1.pdf
 */

#include "libavutil/avutil.h"

typedef struct FLVMediaSample {
    enum AVMediaType type;
    uint32_t timestamp;
    int data_size;
    uint8_t *data;
} FLVMediaSample;

int ff_decode_flv_body(uint8_t *buffer, int buffer_size,
    FLVMediaSample **samples, int *nb_samples_out);
