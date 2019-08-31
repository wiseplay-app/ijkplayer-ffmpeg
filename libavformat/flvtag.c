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
 * @brief FLV Tag Parser for Adobe HDS F4F Files
 * @author Cory McCarthy
 * @see http://download.macromedia.com/f4v/video_file_format_spec_v10_1.pdf
 */

#include "flvtag.h"
#include "libavformat/avio.h"

typedef struct FLVTagAudioHeader {
    uint8_t sound_format;
    uint8_t sound_rate;
    uint8_t sound_size;
    uint8_t sound_type;

    uint8_t aac_packet_type;
} FLVTagAudioHeader;

typedef struct FLVTagAudioBody {
    uint8_t sound_format;
    uint8_t sound_rate;
    uint8_t sound_size;
    uint8_t sound_type;

    uint8_t aac_packet_type;
} FLVTagAudioBody;

typedef struct FLVTagVideoHeader {
    uint8_t frame_type;
    uint8_t codec_id;

    uint8_t avc_packet_type;
    int32_t composition_time;
} FLVTagVideoHeader;

typedef struct FLVTagVideoBody {
    //AVCDecoderConfigurationRecord
    uint8_t configuration_version;
    uint8_t avc_profile_indication;
    uint8_t profile_compatibility;
    uint8_t avc_level_indication;

    uint8_t length_size_minus_one;

    uint8_t *sps_data;
    int sps_data_size;

    uint8_t *pps_data;
    int pps_data_size;
} FLVTagVideoBody;

static int flv_tag_parse_audio_header(AVIOContext *in,
    FLVTagAudioHeader *header)
{
    int ret = 0;
    uint8_t byte;

    byte = avio_r8(in);
    ret++;

    header->sound_format = (byte >> 4) & 0x0F;
    header->sound_rate   = (byte >> 2) & 0x03;
    header->sound_size   = (byte >> 1) & 0x01;
    header->sound_type   = (byte >> 0) & 0x01;

    if(header->sound_format == 10) {
        header->aac_packet_type = avio_r8(in);
        ret++;
    }

    return ret;
}

static int flv_tag_parse_audio_body(AVIOContext *in, uint32_t data_size,
    FLVTagAudioHeader *header, FLVTagAudioBody *body, FLVMediaSample **sample_out)
{
    FLVMediaSample *sample;

    if(header->sound_format != 10) {
        av_log(NULL, AV_LOG_ERROR, "flvtag Unhandled sound format, fmt: %d \n", header->sound_format);
        return 0;
    }

    if(header->aac_packet_type == 0)
        return 0;//skip AudioSpecificConfig

    if(header->aac_packet_type != 1) {
        av_log(NULL, AV_LOG_ERROR, "flvtag Unhandled aac_packet_type, type: %d \n", header->aac_packet_type);
        return 0;
    }

    sample = av_mallocz(sizeof(FLVMediaSample));
    if(!sample)
        return AVERROR(ENOMEM);

    sample->type = AVMEDIA_TYPE_AUDIO;

    sample->data = av_mallocz(sizeof(uint8_t) * data_size);
    if(!sample->data)
        return AVERROR(ENOMEM);

    sample->data_size = data_size;
    avio_read(in, sample->data, sample->data_size);

    if(sample_out)
        *sample_out = sample;

    return data_size;
}

static int flv_tag_parse_video_header(AVIOContext *in,
    FLVTagVideoHeader *header)
{
    int ret = 0;
    uint8_t byte;

    byte = avio_r8(in);
    ret++;

    header->frame_type = (byte >> 4) & 0x0F;
    header->codec_id = byte & 0x0F;

    if(header->codec_id == 0x07) {
        header->avc_packet_type = avio_r8(in);
        header->composition_time = avio_rb24(in);
        ret += 4;
    }

    return ret;
}

static int flv_tag_parse_video_body(AVIOContext *in, uint32_t data_size,
    FLVTagVideoHeader *header, FLVTagVideoBody *body, FLVMediaSample **sample_out)
{
    FLVMediaSample *sample = NULL;
    uint8_t *p;
    uint8_t nb_sps, nb_pps;
    uint16_t sps_length, pps_length;
    uint32_t nal_size;
    int i, ret = 0;

    if(header->frame_type == 0x05) {
        avio_r8(in);
        return 1;
    }

    if(header->codec_id != 0x07) {
        av_log(NULL, AV_LOG_ERROR, "flvtag Unhandled video codec id, id: %d \n", header->codec_id);
        return 0;
    }

    if(header->avc_packet_type == 0x00) {
        body->configuration_version = avio_r8(in);
        body->avc_profile_indication = avio_r8(in);
        body->profile_compatibility = avio_r8(in);
        body->avc_level_indication = avio_r8(in);
        ret += 4;

        body->length_size_minus_one = avio_r8(in) & 0x03;
        ret++;

        if(body->sps_data_size > 0)
            av_freep(&body->sps_data);
        if(body->pps_data_size > 0)
            av_freep(&body->pps_data);

        nb_sps = avio_r8(in) & 0x1F;
        ret++;

        for(i = 0; i < nb_sps; i++) {
            sps_length = avio_rb16(in);
            ret += 2;

            body->sps_data = av_realloc(body->sps_data,
                body->sps_data_size + sps_length + 4);
            if(!body->sps_data)
                return AVERROR(ENOMEM);

            p = body->sps_data + body->sps_data_size;

            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x01;
            body->sps_data_size += 4;

            avio_read(in, p, sps_length);
            body->sps_data_size += sps_length;

            ret += sps_length;
        }

        nb_pps = avio_r8(in);
        ret++;

        for(i = 0; i < nb_pps; i++) {
            pps_length = avio_rb16(in);
            ret += 2;

            body->pps_data = av_realloc(body->pps_data,
                body->pps_data_size + pps_length + 4);
            if(!body->pps_data)
                return AVERROR(ENOMEM);

            p = body->pps_data + body->pps_data_size;

            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x01;
            body->pps_data_size += 4;

            avio_read(in, p, pps_length);
            body->pps_data_size += pps_length;

            ret += pps_length;
        }
    }
    else if(header->avc_packet_type == 0x01) {
        sample = av_mallocz(sizeof(FLVMediaSample));
        if(!sample)
            return AVERROR(ENOMEM);

        sample->type = AVMEDIA_TYPE_VIDEO;

        sample->data_size = body->sps_data_size + body->pps_data_size;
        sample->data_size += 4 + data_size;

        sample->data = av_mallocz(sizeof(uint8_t) * sample->data_size);
        if(!sample->data)
            return AVERROR(ENOMEM);

        p = sample->data;

        memcpy(p, body->sps_data, body->sps_data_size);
        p += body->sps_data_size;

        memcpy(p, body->pps_data, body->pps_data_size);
        p += body->pps_data_size;

        while(ret < data_size) {
            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x01;

            nal_size = avio_rb32(in);
            ret += 4;

            avio_read(in, p, nal_size);
            p += nal_size;
            ret += nal_size;
        }
    }

    if(sample_out)
        *sample_out = sample;

    return ret;
}

static int flv_tag_decode_body(uint8_t *buffer, int buffer_size,
    FLVMediaSample **samples, int *nb_samples_out)
{
    FLVMediaSample *sample;
    AVIOContext *in;
    FLVTagAudioHeader audio_header;
    FLVTagAudioBody audio_body;
    FLVTagVideoHeader video_header;
    FLVTagVideoBody video_body;
    uint8_t byte, filter, tag_type;
    uint32_t data_size, timestamp, timestamp_extended, dts, stream_id;
    int nb_samples = 0;
    int ret;

    memset(&audio_header, 0x00, sizeof(FLVTagAudioHeader));
    memset(&audio_body, 0x00, sizeof(FLVTagAudioBody));
    memset(&video_header, 0x00, sizeof(FLVTagVideoHeader));
    memset(&video_body, 0x00, sizeof(FLVTagVideoBody));


    in = avio_alloc_context(buffer, buffer_size, 0, NULL, NULL, NULL, NULL);
    if(!in)
        return AVERROR(ENOMEM);

    while(!avio_feof(in)) {
        byte = avio_r8(in);
        filter = (byte >> 5) & 0x01;
        tag_type = (byte & 0x01F);

        data_size = avio_rb24(in);

        timestamp = avio_rb24(in);
        timestamp_extended = avio_r8(in);
        dts = ((timestamp_extended << 24) & 0xFF000000) | timestamp;

        stream_id = avio_rb24(in);
        if(stream_id != 0) {
            av_log(NULL, AV_LOG_ERROR, "flvtag Invalid stream_id %d \n", stream_id);
            return -1;
        }

        if(tag_type == 8) {
            data_size -= flv_tag_parse_audio_header(in, &audio_header);
        }
        else
        if(tag_type == 9) {
            data_size -= flv_tag_parse_video_header(in, &video_header);
        }

        if(filter == 0x01) {
            //EncryptionTagHeader
            //FilterParams
        }
        
        sample = NULL;

        if(tag_type == 8) {
            if((ret = flv_tag_parse_audio_body(in, data_size,
                    &audio_header, &audio_body, &sample)) < 0) {

                av_free(in);
                return ret;
            }
            data_size -= ret;
        }
        else
        if(tag_type == 9) {
            if((ret = flv_tag_parse_video_body(in, data_size,
                    &video_header, &video_body, &sample)) < 0) {

                av_free(in);
                return ret;
            }
            data_size -= ret;
        }
        else
        if(tag_type == 18) {
            //ScriptData
        }

        if(sample) {
            sample->timestamp = dts;
            samples[nb_samples++] = sample;
        }

        if(data_size != 0) {
            avio_skip(in, data_size);
        }
        avio_rb32(in);
    }

    av_free(in);

    if(video_body.sps_data_size > 0)
        av_free(video_body.sps_data);
    if(video_body.pps_data_size > 0)
        av_free(video_body.pps_data);

    if(nb_samples_out)
        *nb_samples_out = nb_samples;

    return 0;
}

int ff_decode_flv_body(uint8_t *buffer, int buffer_size,
    FLVMediaSample **samples, int *nb_samples_out)
{
    return flv_tag_decode_body(buffer, buffer_size, samples, nb_samples_out);
}
