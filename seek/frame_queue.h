#ifndef PACKET_QUEUE_FRAME_H
#define PACKET_QUEUE_FRAME_H

#define __STDC_CONSTANT_MACROS      //ffmpeg要求

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

#ifdef __cplusplus
}
#endif

typedef struct _AVFrameList{
    AVFrame    frame;
    struct _AVFrameList   *next;
}AVFrameList;

typedef struct _FrameQueue
{
    AVFrameList    *first;
    AVFrameList    *last;
    int             nb_packets;     // paket个数
    SDL_mutex       *mutex;
    SDL_cond        *cond;
}FrameQueue;

void frame_queue_init(FrameQueue *queue);
int frame_queue_put(FrameQueue *queue, AVFrame *frame);
int frame_queue_get(FrameQueue *queue, AVFrame *frame, int block);

void frame_queue_flush(FrameQueue *queue);
void frame_queue_destory(FrameQueue *queue);

#endif // PACKET_QUEUE_FRAME_H
