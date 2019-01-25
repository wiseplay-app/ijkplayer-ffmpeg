/*
 * copyright (c) 2016 Zhang Rui
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

#ifndef AVUTIL_APPLICATION_H
#define AVUTIL_APPLICATION_H

#include "libavutil/log.h"
#include "libavcodec/avcodec.h"

#define AVAPP_EVENT_WILL_HTTP_OPEN  1 //AVAppHttpEvent
#define AVAPP_EVENT_DID_HTTP_OPEN   2 //AVAppHttpEvent
#define AVAPP_EVENT_WILL_HTTP_SEEK  3 //AVAppHttpEvent
#define AVAPP_EVENT_DID_HTTP_SEEK   4 //AVAppHttpEvent

#define AVAPP_EVENT_WILL_DNS_OPEN  5 //AVAppDnsEvent
#define AVAPP_EVENT_DID_DNS_OPEN   6 //AVAppDnsEvent

#define AVAPP_EVENT_ASYNC_STATISTIC     0x11000 //AVAppAsyncStatistic
#define AVAPP_EVENT_ASYNC_READ_SPEED    0x11001 //AVAppAsyncReadSpeed
#define AVAPP_EVENT_IO_TRAFFIC          0x12204 //AVAppIOTraffic

#define AVAPP_CTRL_WILL_TCP_OPEN   0x20001 //AVAppTcpIOControl
#define AVAPP_CTRL_DID_TCP_OPEN    0x20002 //AVAppTcpIOControl

#define AVAPP_CTRL_WILL_HTTP_OPEN  0x20003 //AVAppIOControl
#define AVAPP_CTRL_WILL_LIVE_OPEN  0x20005 //AVAppIOControl

#define AVAPP_CTRL_WILL_CONCAT_SEGMENT_OPEN 0x20007 //AVAppIOControl
#define AVAPP_CTRL_WILL_FILE_OPEN 0x20009 //AVAppIOControl


#define AVAPP_SWITCH_CTRL_START       0x40000
#define AVAPP_SWITCH_CTRL_CHECK       0x40001
#define AVAPP_SWITCH_CTRL_FAIL        0x40002
#define AVAPP_SWITCH_CTRL_RETRY       0x40003
#define AVAPP_SWITCH_CTRL_SUCCESS     0x40004
#define AVAPP_SWITCH_CTRL_BUFFERSTART 0x40005
#define AVAPP_SWITCH_CTRL_BUFFEREND   0x40006

#define AVAPP_CTRL_GET_DASH_STREAM_INFO  0x30001
#define AVAPP_CTRL_SET_DASH_VIDEO_STREAM 0x30002
#define AVAPP_EVENT_WILL_DASH_VIDEO_STREAM_CHANGE 0x30003
#define AVAPP_EVENT_DID_DASH_VIDEO_STREAM_CHANGE  0x30004
#define AVAPP_EVENT_WILL_DASH_VIDEO_SIZE_CHANGE   0x30005
#define AVAPP_EVENT_DID_DASH_VIDEO_SIZE_CHANGE    0x30006

typedef struct AVAppDashStream
{
    int audio_stream_nb;
    int video_stream_nb;
    int64_t video_bandwidth[20];
    int64_t audio_bandwidth[20];
    int video_id[20];
    int audio_id[20];
    int cur_video_id;
    int cur_audio_id;
} AVAppDashStream;

typedef struct AVAppDashChange
{
    int cur_video_id;
    int next_video_id;
    int64_t next_sap;
    int error;
    int retry;
    int auto_switch;
} AVAppDashChange;


typedef struct AVAppIOControl {
    size_t  size;
    char    url[4096];      /* in, out */
    int     segment_index;  /* in, default = 0 */
    int     retry_counter;  /* in */

    int     is_handled;     /* out, default = false */
    int     is_url_changed; /* out, default = false */
} AVAppIOControl;

typedef struct AVAppTcpIOControl {
    int  error;
    int  family;
    char ip[96];
    int  port;
    int  fd;
} AVAppTcpIOControl;

typedef struct AVAppAsyncStatistic {
    size_t  size;
    int64_t buf_backwards;
    int64_t buf_forwards;
    int64_t buf_capacity;
} AVAppAsyncStatistic;

typedef struct AVAppAsyncReadSpeed {
    size_t  size;
    int     is_full_speed;
    int64_t io_bytes;
    int64_t elapsed_milli;
} AVAppAsyncReadSpeed;

typedef struct AVAppHttpEvent
{
    void    *obj;
    char     url[4096];
    int64_t  offset;
    int      error;
    int      http_code;
    int64_t  filesize;
    int64_t  start_time;
    int64_t  end_time;
} AVAppHttpEvent;

typedef struct AVAppIOTraffic
{
    void   *obj;
    int     bytes;
} AVAppIOTraffic;

typedef struct AVAppSwitchControl{
    int vid;
    int aid;

    int min_qn;
    int max_qn;
    int auto_switch_enable;
    int64_t latest_pts;
    int64_t switch_ts;
    int64_t switch_sap;

    double buffer_level;

    int current_serial;
    int next_serial;
    int64_t max_differ;

    void * switch_mtx_ptr;
    int retry_counter;

    AVAppDashChange change_info;
    int active_reconnect;
    int (*start_switch)(void *);
    void * opaque;
} AVAppSwitchControl;

typedef struct AVAppDnsEvent
{
    char host[1024];
    char ip[96];
    int  is_ip;
    int  hit_cache;
    int64_t  dns_time;
} AVAppDnsEvent;

typedef struct AVApplicationContext AVApplicationContext;
struct AVApplicationContext {
    const AVClass *av_class;    /**< information for av_log(). Set by av_application_open(). */
    void *opaque;               /**< user data. */
    int dash_audio_read_len;
    int dash_audio_recv_buffer_size;
    int dash_video_recv_buffer_size;
    int (*func_on_app_event)(AVApplicationContext *h, int event_type ,void *obj, size_t size);
    int (*func_app_ctrl)(int what, int64_t arg0, void *obj, size_t size);
};

int  av_application_alloc(AVApplicationContext **ph, void *opaque);
int  av_application_open(AVApplicationContext **ph, void *opaque);
void av_application_close(AVApplicationContext *h);
void av_application_closep(AVApplicationContext **ph);

void av_application_on_http_event(AVApplicationContext *h, int event_type, AVAppHttpEvent *event);
void av_application_will_http_open(AVApplicationContext *h, void *obj, const char *url, int64_t start_time, int64_t end_time);
void av_application_did_http_open(AVApplicationContext *h, void *obj, const char *url, int error, int http_code,
                                  int64_t filesize, int64_t start_time, int64_t end_time);
void av_application_will_http_seek(AVApplicationContext *h, void *obj, const char *url, int64_t offset, int64_t start_time, int64_t end_time);
void av_application_did_http_seek(AVApplicationContext *h, void *obj, const char *url, int64_t offset, int error,
                                  int http_code, int64_t start_time, int64_t end_time);

void av_application_did_io_tcp_read(AVApplicationContext *h, void *obj, int bytes);

int  av_application_on_switch_control(AVApplicationContext *h, int event_type, AVAppSwitchControl *control);

int  av_application_on_io_control(AVApplicationContext *h, int event_type, AVAppIOControl *control);

int av_application_on_tcp_will_open(AVApplicationContext *h);
int av_application_on_tcp_did_open(AVApplicationContext *h, int error, int fd, AVAppTcpIOControl *control);
int av_application_quic_on_tcp_did_open(AVApplicationContext *h, int error);

void av_application_on_async_statistic(AVApplicationContext *h, AVAppAsyncStatistic *statistic);
void av_application_on_async_read_speed(AVApplicationContext *h, AVAppAsyncReadSpeed *speed);

void av_application_on_dash_info(AVApplicationContext *h, int event_type, AVAppDashChange *info);

void av_application_on_dns_will_open(AVApplicationContext *h, char *hostname);
void av_application_on_dns_did_open(AVApplicationContext *h, char *hostname, char *ip, int hit_cache, int64_t dns_time);

#endif /* AVUTIL_APPLICATION_H */
