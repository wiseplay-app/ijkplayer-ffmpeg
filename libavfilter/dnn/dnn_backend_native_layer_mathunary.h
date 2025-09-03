/*
 * Copyright (c) 2020
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
 * DNN native backend implementation.
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

 * DNN inference functions interface for native backend.
 */

#ifndef AVFILTER_DNN_DNN_BACKEND_NATIVE_LAYER_MATHUNARY_H
#define AVFILTER_DNN_DNN_BACKEND_NATIVE_LAYER_MATHUNARY_H

#include "libavformat/avio.h"
#include "dnn_backend_native.h"

typedef enum {
    DMUO_ABS = 0,
    DMUO_COUNT
} DNNMathUnaryOperation;

typedef struct DnnLayerMathUnaryParams{
    DNNMathUnaryOperation un_op;
} DnnLayerMathUnaryParams;

int dnn_load_layer_math_unary(Layer *layer, AVIOContext *model_file_context, int file_size, int operands_num);
int dnn_execute_layer_math_unary(DnnOperand *operands, const int32_t *input_operand_indexes,
                                int32_t output_operand_index, const void *parameters);

#endif
