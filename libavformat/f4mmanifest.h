/*
 * Adobe Media Manifest (F4M) File Parser
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
 * @brief Adobe Media Manifest (F4M) File Parser
 * @author Cory McCarthy
 * @see http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/hds/pdfs/adobe-media-manifest-specification.pdf
 */

#include "internal.h"

#define MAX_NB_BOOTSTRAPS 32
#define MAX_NB_MEDIA 32


typedef struct F4MBootstrapInfo {
    char id[MAX_URL_SIZE];
    char url[MAX_URL_SIZE];
    char profile[MAX_URL_SIZE];

    int metadata_size;
    uint8_t *metadata;
} F4MBootstrapInfo;

typedef struct F4MMedia {
    int bitrate;
    char url[MAX_URL_SIZE];
    char bootstrap_info_id[MAX_URL_SIZE];

    int metadata_size;
    uint8_t *metadata;
} F4MMedia;


typedef struct F4MManifest {
    char id[MAX_URL_SIZE];
    char stream_type[MAX_URL_SIZE];
    int nb_bootstraps;
    F4MBootstrapInfo *bootstraps[MAX_NB_BOOTSTRAPS];

    int nb_media;
    F4MMedia *media[MAX_NB_MEDIA];
} F4MManifest;

int ff_parse_f4m_manifest(uint8_t *buffer, int size, F4MManifest *manifest);
int ff_free_manifest(F4MManifest *manifest);
