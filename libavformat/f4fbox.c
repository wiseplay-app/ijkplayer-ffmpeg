/*
 * Adobe Fragmented F4V File (F4F) Parser
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
 * @brief Adobe Fragmented F4V File (F4F) Parser for Adobe HDS
 * @author Cory McCarthy
 * @see http://download.macromedia.com/f4v/video_file_format_spec_v10_1.pdf
 */

#include "f4fbox.h"
#include "avformat.h"

static int f4fbox_parse_single_box(AVIOContext *in, void *opague);

static int f4fbox_parse_asrt(AVIOContext *in, int64_t data_size, void *opague)
{
    F4FBootstrapInfoBox *parent = (F4FBootstrapInfoBox*)opague;
    F4FSegmentRunTableBox *asrt;
    F4FSegmentRunEntry *entry;
    uint8_t quality_entry_count;
    uint32_t segment_run_entry_count;
    char url[1024];
    int i;

    asrt = av_mallocz(sizeof(F4FSegmentRunTableBox));
    if(!asrt)
        return AVERROR(ENOMEM);

    parent->segment_run_table_boxes[parent->nb_segment_run_table_boxes] = asrt;
    parent->nb_segment_run_table_boxes++;

    asrt->version = avio_r8(in);
    asrt->flags = avio_rb24(in);

    quality_entry_count = avio_r8(in);
    for(i = 0; i < quality_entry_count; i++) {
        avio_get_str(in, sizeof(url), url, sizeof(url));
    }

    segment_run_entry_count = avio_rb32(in);
    for(i = 0; i < segment_run_entry_count; i++) {
        entry = av_mallocz(sizeof(F4FSegmentRunEntry));
        if(!entry)
            return AVERROR(ENOMEM);

        asrt->segment_run_entries[asrt->nb_segment_run_entries] = entry;
        asrt->nb_segment_run_entries++;

        entry->first_segment = avio_rb32(in);
	
        entry->fragments_per_segment = avio_rb32(in);
    }

    return 0;
}

static int f4fbox_parse_afrt(AVIOContext *in, int64_t data_size, void *opague)
{
    F4FBootstrapInfoBox *parent = (F4FBootstrapInfoBox*)opague;
    F4FFragmentRunTableBox *afrt;
    F4FFragmentRunEntry *entry;
    uint8_t quality_entry_count;
    uint32_t fragment_run_entry_count;
    char url[1024];
    int i;

    afrt = av_mallocz(sizeof(F4FFragmentRunTableBox));
    if(!afrt)
        return AVERROR(ENOMEM);

    parent->fragment_run_table_boxes[parent->nb_fragment_run_table_boxes] = afrt;
    parent->nb_fragment_run_table_boxes++;

    afrt->version = avio_r8(in);
    afrt->flags = avio_rb24(in);

    afrt->timescale = avio_rb32(in);

    quality_entry_count = avio_r8(in);
    for(i = 0; i < quality_entry_count; i++) {
        avio_get_str(in, sizeof(url), url, sizeof(url));
    }

    fragment_run_entry_count = avio_rb32(in);
    for(i = 0; i < fragment_run_entry_count; i++) {
        entry = av_mallocz(sizeof(F4FFragmentRunEntry));
        if(!entry)
            return AVERROR(ENOMEM);

        afrt->fragment_run_entries[afrt->nb_fragment_run_entries] = entry;
        afrt->nb_fragment_run_entries++;

        entry->first_fragment = avio_rb32(in);
        entry->first_fragment_time_stamp = avio_rb64(in);
        entry->fragment_duration = avio_rb32(in);
        if(entry->fragment_duration == 0) {
            entry->discontinuity_indicator = avio_r8(in);
        }
    }

    return 0;
}


static int f4fbox_parse_abst(AVIOContext *in, int64_t data_size, void *opague)
{
    F4FBox *parent = (F4FBox*)opague;
    F4FBootstrapInfoBox *abst = &(parent->abst);
    uint8_t server_entry_count, quality_entry_count;
    uint8_t segment_run_table_count, fragment_run_table_count;
    uint8_t byte;
    char url[1024];
    int i, ret;

    abst->version = avio_r8(in);
    abst->flags = avio_rb24(in);
    abst->bootstrap_info_version = avio_rb32(in);

    byte = avio_r8(in);
    abst->profile = (byte >> 6) & 0x03;
    abst->is_live = (byte >> 5) & 0x01;
    abst->is_update = (byte >> 4) & 0x01;

    abst->timescale = avio_rb32(in);
    abst->current_media_time = avio_rb64(in);
    abst->smpte_time_code_offset = avio_rb64(in);

    avio_get_str(in, sizeof(abst->movie_id), abst->movie_id, sizeof(abst->movie_id));

    server_entry_count = avio_r8(in);
    for(i = 0; i < server_entry_count; i++) {
        avio_get_str(in, sizeof(url), url, sizeof(url));
    }

    quality_entry_count = avio_r8(in);
    for(i = 0; i < quality_entry_count; i++) {
        avio_get_str(in, sizeof(url), url, sizeof(url));
    }

    avio_get_str(in, sizeof(abst->drm_data), abst->drm_data, sizeof(abst->drm_data));
    avio_get_str(in, sizeof(abst->metadata), abst->metadata, sizeof(abst->metadata));

    segment_run_table_count = avio_r8(in);
    for(i = 0; i < segment_run_table_count; i++) {
        if((ret = f4fbox_parse_single_box(in, abst)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "f4fbox Failed to parse asrt box, ret: %d \n", ret);
            return ret;
        }
    }

    fragment_run_table_count = avio_r8(in);
    for(i = 0; i < fragment_run_table_count; i++) {
        if((ret = f4fbox_parse_single_box(in, abst)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "f4fbox Failed to parse afrt box, ret: %d \n", ret);
            return ret;
        }
    }

    return 0;
}

static int f4fbox_parse_mdat(AVIOContext *in, int64_t data_size, void *opague)
{
    F4FBox *parent = (F4FBox*)opague;
    F4FMediaDataBox *mdat = &(parent->mdat);

    mdat->data = av_mallocz(sizeof(uint8_t)*data_size);
    if(!mdat->data)
        return AVERROR(ENOMEM);

    mdat->size = data_size;
    avio_read(in, mdat->data, mdat->size);

    return 0;
}

static int f4fbox_parse_single_box(AVIOContext *in, void *opague)
{
    int64_t bytes_read, bytes_left, start_pos, end_pos;
    uint64_t size;
    uint32_t type;
    int ret = 0;

    bytes_read = 0;
    start_pos = avio_tell(in);

    size = avio_rb32(in);
    type = avio_rl32(in);
    bytes_read += 8;

    if(size == 1) {/* 64 bit extended size */
        size = avio_rb64(in) - 8;
        bytes_read += 8;
    }

    if(size == 0)
        return -1;

    if(type == MKTAG('a', 'b', 's', 't')) {
        ret = f4fbox_parse_abst(in, size, opague);
    }
    if(type == MKTAG('a', 's', 'r', 't')) {
        ret = f4fbox_parse_asrt(in, size, opague);
    }
    if(type == MKTAG('a', 'f', 'r', 't')) {
        ret = f4fbox_parse_afrt(in, size, opague);
    }
    if(type == MKTAG('m', 'd', 'a', 't')) {
        ret = f4fbox_parse_mdat(in, size, opague);
    }

    if(ret < 0)
        return ret;

    end_pos = avio_tell(in);
    bytes_left = size - (end_pos - start_pos);
    if(bytes_left > 0)
        avio_skip(in, bytes_left);

    bytes_read += size;

    return bytes_read;
}

static int f4fbox_parse(AVIOContext *in, int64_t data_size, void *opague)
{
    int64_t bytes_read = 0;
    int ret;

    while(!avio_feof(in) && bytes_read + 8 < data_size) {
        if((ret = f4fbox_parse_single_box(in, opague)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "f4fbox Failed to parse box, ret: %d \n", ret);
            return ret;
        }
        bytes_read += ret;
    }

    return 0;
}

int ff_parse_f4f_box(uint8_t *buffer, int buffer_size, F4FBox *box)
{
    AVIOContext *in;
    int ret;

    in = avio_alloc_context(buffer, buffer_size, 0, NULL, NULL, NULL, NULL);
    if(!in)
        return AVERROR(ENOMEM);

    ret = f4fbox_parse(in, buffer_size, box);
    av_free(in);

    return ret;
}

int ff_free_f4f_box(F4FBox *box)
{
    F4FBootstrapInfoBox *abst;
    F4FSegmentRunTableBox *asrt;
    F4FSegmentRunEntry *sre;
    F4FFragmentRunTableBox *afrt;
    F4FFragmentRunEntry *fre;
    F4FMediaDataBox *mdat;
    int i, j;

    abst = &(box->abst);
    for(i = 0; i < abst->nb_segment_run_table_boxes; i++) {
        asrt = abst->segment_run_table_boxes[i];
        for(j = 0; j < asrt->nb_segment_run_entries; j++) {
            sre = asrt->segment_run_entries[j];
            av_freep(&sre);
        }
        av_freep(&asrt);
    }

    for(i = 0; i < abst->nb_fragment_run_table_boxes; i++) {
        afrt = abst->fragment_run_table_boxes[i];
        for(j = 0; j < afrt->nb_fragment_run_entries; j++) {
            fre = afrt->fragment_run_entries[j];
            av_freep(&fre);
        }
        av_freep(&afrt);
    }

    mdat = &(box->mdat);
    if(mdat->size > 0)
        av_freep(&mdat->data);

    memset(box, 0x00, sizeof(F4FBox));

    return 0;
}
