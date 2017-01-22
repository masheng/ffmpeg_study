#include "packet_queue.h"

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

void packet_queue_init(PacketQueue *queue)
{
    queue->first_pkt    = NULL;
    queue->last_pkt     = NULL;
    queue->nb_packets 		= 0;
    queue->mutex           = SDL_CreateMutex();
    queue->cond            = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *queue, AVPacket *packet)
{
    AVPacketList   *pkt_list;

    if (av_dup_packet(packet) < 0)
    {
        return -1;
    }

    pkt_list = (AVPacketList *)av_malloc(sizeof(AVPacketList));
    if (pkt_list == NULL)
    {
        return -1;
    }

    pkt_list->pkt   = *packet;
    pkt_list->next  = NULL;


    SDL_LockMutex(queue->mutex);

    if (queue->last_pkt == NULL)
    {
        queue->first_pkt = pkt_list;
    }
    else
    {
        queue->last_pkt->next = pkt_list;
    }

    queue->last_pkt = pkt_list;
    queue->nb_packets++;
    queue->size += packet->size;
    SDL_CondSignal(queue->cond);

    SDL_UnlockMutex(queue->mutex);

    return 0;
}

int packet_queue_get(PacketQueue *queue, AVPacket *pkt, int block)
{
    AVPacketList   *pkt_list = NULL;
    int            ret = -1;

    SDL_LockMutex(queue->mutex);
    while(1)
    {

        pkt_list = queue->first_pkt;
        if (pkt_list != NULL)
        {
            queue->first_pkt = queue->first_pkt->next;
            if (queue->first_pkt == NULL)
            {
                queue->last_pkt = NULL;
            }

            queue->nb_packets--;
            queue->size -= pkt_list->pkt.size;
            *pkt = pkt_list->pkt;
            av_free(pkt_list);
            ret = 0;
            break;
        }
        else if (block == 0)
        {
            ret = -1;
            break;
        }
        else
        {
            SDL_CondWait(queue->cond, queue->mutex);
        }
    }

    SDL_UnlockMutex(queue->mutex);
    return ret;
}

void packet_queue_flush(PacketQueue *queue){

}
void packet_queue_destory(PacketQueue *queue){
    int i;
    AVPacket *packet;
    for(i=0;i<queue->nb_packets;i++){
        packet=&queue->first_pkt->pkt;
        queue->first_pkt=queue->first_pkt->next;
        av_free_packet(packet);
    }
    SDL_DestroyMutex(queue->mutex);
    SDL_DestroyCond(queue->cond);

}
