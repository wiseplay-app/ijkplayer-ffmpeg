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

#include "f4mmanifest.h"
#include "libavutil/avstring.h"
#include "libavutil/base64.h"
#include <libxml/parser.h>
#include <libxml/tree.h>


static int f4m_get_content_padding(xmlChar *p){
  
    int result = 0;
    if(p){
      int len = strlen(p);
      int i;
    
      for(i = 0; i < len; i++){   
	if(p[i] == 0x0a || p[i] == 0x09)
	 result++;
        else
	  break;
     }
      
    }
    return result;
}

static int f4m_get_content_length(xmlChar *p){
  
    int result = 0;
    if(p){
      
      int len = strlen(p);
      int i;
  
      for(i = 0; i < len; i++) 
	if(p[i] != 0x0a && p[i] != 0x09)
	 result++;
	
      result++;
    }
    
    return result <= MAX_URL_SIZE ? result : MAX_URL_SIZE;
  
}

static int f4m_parse_bootstrap_info_node(xmlNodePtr node, F4MBootstrapInfo *bootstrap_info)
{
    xmlChar *p;
    uint8_t *dst;
    int ret;
    int p_len;
    int padding;
    

    p = xmlGetProp(node, "id");
    if(p) {
        av_strlcpy(bootstrap_info->id, p, sizeof(bootstrap_info->id));
        xmlFree(p);
    }

    p = xmlGetProp(node, "url");
    if(p) {
        av_strlcpy(bootstrap_info->url, p, sizeof(bootstrap_info->url));
        xmlFree(p);
    }

    p = xmlGetProp(node, "profile");
    if(p) {
        av_strlcpy(bootstrap_info->profile, p, sizeof(bootstrap_info->profile));
        xmlFree(p);
    }

    p = xmlNodeGetContent(node);
    if(p) {
	p_len = f4m_get_content_length(p);
	padding = f4m_get_content_padding(p);
	if(p_len > 1){
	    dst = av_mallocz(sizeof(uint8_t)*p_len);
            if(!dst)
                return AVERROR(ENOMEM);
	    
	    if((ret = av_base64_decode(dst, p + padding, p_len)) < 0) {
                av_log(NULL, AV_LOG_ERROR, "f4mmanifest Failed to decode bootstrap node base64 metadata, ret: %d \n", ret);
                xmlFree(p);
                av_free(dst);
                return ret;
            }

            bootstrap_info->metadata = av_mallocz(sizeof(uint8_t)*ret);
            if(!bootstrap_info->metadata)
               return AVERROR(ENOMEM);

            bootstrap_info->metadata_size = ret;
            memcpy(bootstrap_info->metadata, dst, ret);
	    
	    av_free(dst);
	}
        xmlFree(p);
        
    }

    return 0;
}

static int f4m_parse_metadata_node(xmlNodePtr node, F4MMedia *media)
{
    xmlNodePtr metadata_node;
    xmlChar *p;
    uint8_t *dst;
    int p_len;
    int padding;
    int ret;
    

    p = NULL;
    metadata_node = node->children;
    while(metadata_node) {
        if(!strcmp(metadata_node->name, "metadata")) {
            p = xmlNodeGetContent(metadata_node);
            break;
        }

        metadata_node = metadata_node->next;
    }

    if(!p)
        return 0;
    
    
    p_len = f4m_get_content_length(p);
    padding = f4m_get_content_padding(p);

    dst = av_mallocz(sizeof(uint8_t)*p_len);
    
    if(!dst)
        return AVERROR(ENOMEM);
    

    if((ret = av_base64_decode(dst, p + padding, p_len)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "f4mmanifest Failed to decode base64 metadata, ret: %d \n", ret);
        xmlFree(p);
        av_free(dst);
        return ret;
    }

    media->metadata = av_mallocz(sizeof(uint8_t)*ret);
    if(!media->metadata)
        return AVERROR(ENOMEM);

    media->metadata_size = ret;
    memcpy(media->metadata, dst, ret);

    xmlFree(p);
    av_free(dst);

    return 0;
}

static int f4m_parse_media_node(xmlNodePtr node, F4MMedia *media, F4MManifest *manifest)
{
    xmlChar *p;
    int ret;

    p = xmlGetProp(node, "bitrate");
    if(p) {
        media->bitrate = strtoul(p, NULL, 10);
        xmlFree(p);
    }

    p = xmlGetProp(node, "url");
    if(p) {
        av_strlcpy(media->url, p, sizeof(media->url));
        xmlFree(p);
    }
    

    p = xmlGetProp(node, "bootstrapInfoId");
    if(p) {
        av_strlcpy(media->bootstrap_info_id, p, sizeof(media->bootstrap_info_id));
        xmlFree(p);
    }

    if((ret = f4m_parse_metadata_node(node, media)) < 0) {
        return ret;
    }

    return 0;
}

static int f4m_parse_manifest_node(xmlNodePtr root_node, F4MManifest *manifest)
{
    F4MBootstrapInfo *bootstrap_info;
    F4MMedia *media;
    xmlNodePtr node;
    xmlChar *node_content;
    int ret;
    int length;
    int padding;

    for(node = root_node->children; node != root_node->last; node = node->next) {
      
        if(!strcmp(node->name, "text"))
            continue;

        node_content = xmlNodeGetContent(node);
	
        padding = f4m_get_content_padding(node_content);
        length = f4m_get_content_length(node_content);
	
        if(!strcmp(node->name, "id") && node_content) {
            av_strlcpy(manifest->id, node_content + padding, length);
        }
        
        if(!strcmp(node->name, "streamType") && node_content) {
            av_strlcpy(manifest->stream_type, node_content + padding, length);
        }
        else
        if(!strcmp(node->name, "bootstrapInfo")) {
            bootstrap_info = av_mallocz(sizeof(F4MBootstrapInfo));
            if(!bootstrap_info)
                return AVERROR(ENOMEM);

            manifest->bootstraps[manifest->nb_bootstraps++] = bootstrap_info;
            ret = f4m_parse_bootstrap_info_node(node, bootstrap_info);
        }
        else
        if(!strcmp(node->name, "media")) {
	    
            media = av_mallocz(sizeof(F4MMedia));
            if(!media)
                return AVERROR(ENOMEM);

            manifest->media[manifest->nb_media++] = media;
            ret = f4m_parse_media_node(node, media, manifest);
        }

        if(node_content)
            xmlFree(node_content);
        if(ret < 0)
            return ret;
    }

    return 0;
}

static int f4m_parse_xml_file(uint8_t *buffer, int size, F4MManifest *manifest)
{
    xmlDocPtr doc;
    xmlNodePtr root_node;
    int ret;
    

    doc = xmlReadMemory(buffer, size, "noname.xml", NULL, 0);
    if(!doc) {
        return -1;
    }
    
    root_node = xmlDocGetRootElement(doc);
    if(!root_node) {
        av_log(NULL, AV_LOG_ERROR, "f4mmanifest Root element not found \n");
        xmlFreeDoc(doc);
        return -1;
    }

    if(strcmp(root_node->name, "manifest")) {
        av_log(NULL, AV_LOG_ERROR, "f4mmanifest Root element is not named manifest, name = %s \n", root_node->name);
        xmlFreeDoc(doc);
        return -1;
    }

    ret = f4m_parse_manifest_node(root_node, manifest);
    xmlFreeDoc(doc);

    return ret;
}

int ff_parse_f4m_manifest(uint8_t *buffer, int size, F4MManifest *manifest)
{
    return f4m_parse_xml_file(buffer, size, manifest);
}

int ff_free_manifest(F4MManifest *manifest)
{
    F4MBootstrapInfo *bootstrap_info;
    F4MMedia *media;
    int i;

    for(i = 0; i < manifest->nb_bootstraps; i++) {
        bootstrap_info = manifest->bootstraps[i];
        av_freep(&bootstrap_info->metadata);
        av_freep(&bootstrap_info);
    }

    for(i = 0; i < manifest->nb_media; i++) {
        media = manifest->media[i];
        av_freep(&media->metadata);
        av_freep(&media);
    }

    memset(manifest, 0x00, sizeof(F4MManifest));

    return 0;
}
