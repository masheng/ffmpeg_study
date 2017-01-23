#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

class AVPacketList;
class SDL_mutex;
class SDL_cond;
class AVPacket;

typedef struct PacketQueue
{
    AVPacketList    *first_pkt;
    AVPacketList    *last_pkt;
    int             nb_packets;     // paket个数
    int             size;
    SDL_mutex       *mutex;
    SDL_cond        *cond;
    int            flush_flag;
}PacketQueue;

void packet_queue_init(PacketQueue *queue);
int packet_queue_put(PacketQueue *queue, AVPacket *packet);
int packet_queue_get(PacketQueue *queue, AVPacket *pakcet, int block);

void packet_queue_flush(PacketQueue *queue);
void packet_queue_destory(PacketQueue *queue);

#endif // PACKET_QUEUE_H
