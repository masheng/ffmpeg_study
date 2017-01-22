#include "player.h"

int putIndex=0;
int getIndex=0;

int video_init(playerContext *context){
    packet_queue_init(&context->video_packet_queue);
    context->outFrame=av_frame_alloc();

    unsigned char *out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,  context->pCodecCtx[VIDEO]->width, context->pCodecCtx[VIDEO]->height,1));
    av_image_fill_arrays(context->outFrame->data, context->outFrame->linesize,out_buffer,
                         AV_PIX_FMT_YUV420P,context->pCodecCtx[VIDEO]->width, context->pCodecCtx[VIDEO]->height,1);

    context->swsCtx = sws_getContext(context->pCodecCtx[VIDEO]->width, context->pCodecCtx[VIDEO]->height, context->pCodecCtx[VIDEO]->pix_fmt,
                                     context->pCodecCtx[VIDEO]->width, context->pCodecCtx[VIDEO]->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    context->screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       context->pCodecCtx[VIDEO]->width, context->pCodecCtx[VIDEO]->height,SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!context->screen) {
        printf("SDL: could not create window - exiting:%s\n",SDL_GetError());
        return -1;
    }

    context->sdlRenderer = SDL_CreateRenderer(context->screen, -1, 0);
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    context->sdlTexture = SDL_CreateTexture(context->sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,context->pCodecCtx[VIDEO]->width,context->pCodecCtx[VIDEO]->height);
    context->window_w=context->pCodecCtx[VIDEO]->width;
    context->window_h=context->pCodecCtx[VIDEO]->height;
    context->sdlRect.x=0;
    context->sdlRect.y=0;

    SDL_CreateThread(player_refresh_thread,NULL,context);
}


int video_decode_frame(playerContext *context,AVFrame *frame){
    int ret = -1;
    AVPacket packet;

    ret = packet_queue_get(&context->video_packet_queue, &packet, 1);
    if (ret == -1)
    {
        fprintf(stderr, "Get video packet error\n");
        return -1;
    }

    ret = avcodec_send_packet(context->pCodecCtx[VIDEO], &packet);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    {
        fprintf(stderr, "send video packet error\n");
        return -1;
    }

    ret = avcodec_receive_frame(context->pCodecCtx[VIDEO], frame);
    if (ret < 0 && ret != AVERROR_EOF)
    {
        fprintf(stderr, "receive video frame error\n");
        return -1;
    }

    return 0;
}

Uint32 show_video(Uint32 interval, void *data){
    playerContext *context=(playerContext *)data;
    int ret = -1;
    AVFrame *pFrame = av_frame_alloc();

    ret = video_decode_frame(context,pFrame);
    if(ret == -1)
        return -1;

    getVideoClock(context, pFrame);

    sws_scale(context->swsCtx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, context->pCodecCtx[VIDEO]->height, context->outFrame->data, context->outFrame->linesize);
    SDL_UpdateTexture( context->sdlTexture, NULL, context->outFrame->data[0], context->outFrame->linesize[0] );
    SDL_RenderClear( context->sdlRenderer );

    context->sdlRect.w=context->window_w;
    context->sdlRect.h=context->window_h;
    SDL_RenderCopy( context->sdlRenderer, context->sdlTexture, NULL, &context->sdlRect);
    //    SDL_RenderCopy( context->sdlRenderer, context->sdlTexture, NULL, NULL);
    SDL_RenderPresent( context->sdlRenderer );

    double delay=0.0;
    synch(context, &delay);
    return ((delay>0.01?delay:0.01)*1000 + 0.5);
    //    av_frame_free(&pFrame);
}

int player_refresh_thread(void *opaque){
    playerContext *context=(playerContext *)opaque;
    context->timerID=SDL_AddTimer(40, show_video, context);
    return 0;
}
