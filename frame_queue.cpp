#include "frame_queue.h"

void frame_queue_init(FrameQueue *queue)
{
    queue->first    = NULL;
    queue->last     = NULL;
    queue->nb_packets 		= 0;
    queue->mutex           = SDL_CreateMutex();
    queue->cond            = SDL_CreateCond();
}

int frame_queue_put(FrameQueue *queue, AVFrame *frame)
{
    AVFrameList   *tmp_frame;

    tmp_frame = (AVFrameList *)av_malloc(sizeof(AVFrameList));
    if (tmp_frame == NULL)
    {
        return -1;
    }

    tmp_frame->frame=*frame;
    tmp_frame->next=NULL;

    SDL_LockMutex(queue->mutex);

    if (queue->last == NULL)
    {
        queue->first = tmp_frame;
    }
    else
    {
        queue->last->next = tmp_frame;
    }

    queue->last = tmp_frame;
    queue->nb_packets++;
    SDL_CondSignal(queue->cond);

    SDL_UnlockMutex(queue->mutex);

    return 0;
}
int frame_queue_get(FrameQueue *queue, AVFrame *frame, int block)
{
    AVFrameList   *tmp_frame = NULL;
    int            ret = -1;

    SDL_LockMutex(queue->mutex);
    while(1)
    {
        tmp_frame = queue->first;
        if (tmp_frame != NULL)
        {
            queue->first = queue->first->next;
            if (queue->first == NULL)
            {
                queue->last = NULL;
            }

            queue->nb_packets--;
            *frame = tmp_frame->frame;
            av_free(tmp_frame);
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

void frame_queue_flush(FrameQueue *queue){

}

void frame_queue_destory(FrameQueue *queue){
    int i;
    AVFrameList *frameList;
    for(i=0;i<queue->nb_packets;i++){
        frameList=queue->first;
        queue->first=queue->first->next;
        av_free(frameList);
    }
    SDL_DestroyMutex(queue->mutex);
    SDL_DestroyCond(queue->cond);

}
