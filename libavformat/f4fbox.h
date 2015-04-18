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

#include "avio_internal.h"

#define MAX_NB_SEGMENT_RUN_TABLE_BOXES 256
#define MAX_NB_FRAGMENT_RUN_TABLE_BOXES 256

#define MAX_NB_SEGMENT_RUN_ENTRIES 1024
#define MAX_NB_FRAGMENT_RUN_ENTRIES 1024

typedef struct F4FFragmentRunEntry {
    uint32_t first_fragment;
    uint64_t first_fragment_time_stamp;
    uint32_t fragment_duration;
    uint8_t discontinuity_indicator;
} F4FFragmentRunEntry;

typedef struct F4FFragmentRunTableBox {
    uint8_t version;
    uint32_t flags;
    uint32_t timescale;

    uint32_t nb_fragment_run_entries;
    F4FFragmentRunEntry *fragment_run_entries[MAX_NB_FRAGMENT_RUN_ENTRIES];
} F4FFragmentRunTableBox;

typedef struct F4FSegmentRunEntry {
    uint32_t first_segment;
    uint32_t fragments_per_segment;
} F4FSegmentRunEntry;

typedef struct F4FSegmentRunTableBox {
    uint8_t version;
    uint32_t flags;

    uint32_t nb_segment_run_entries;
    F4FSegmentRunEntry *segment_run_entries[MAX_NB_SEGMENT_RUN_ENTRIES];
} F4FSegmentRunTableBox;

typedef struct F4FBootstrapInfoBox {
    uint8_t version;
    uint32_t flags;
    uint32_t bootstrap_info_version;

    uint8_t profile;
    uint8_t is_live;
    uint8_t is_update;

    uint32_t timescale;
    uint64_t current_media_time;
    uint64_t smpte_time_code_offset;

    char movie_id[1024];
    char drm_data[1024];
    char metadata[1024];

    uint8_t nb_segment_run_table_boxes;
    F4FSegmentRunTableBox *segment_run_table_boxes[MAX_NB_SEGMENT_RUN_TABLE_BOXES];

    uint8_t nb_fragment_run_table_boxes;
    F4FFragmentRunTableBox *fragment_run_table_boxes[MAX_NB_FRAGMENT_RUN_TABLE_BOXES];
} F4FBootstrapInfoBox;

typedef struct F4FMediaDataBox {
    uint32_t size;
    uint8_t *data;
} F4FMediaDataBox;

typedef struct F4FBox {
    F4FBootstrapInfoBox abst;
    F4FMediaDataBox mdat;
} F4FBox;

int ff_parse_f4f_box(uint8_t *buffer, int buffer_size, F4FBox *box);
int ff_free_f4f_box(F4FBox *box);
