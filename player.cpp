#include "player.h"

int init(char *fileName){
    if(fileName == NULL){
        fprintf(stderr,"filePath is null %d\n");
        return -1;
    }
    SDL_Event      event;
    playerContext *context = (playerContext *)malloc(sizeof(playerContext));
    memset(context,0,sizeof(playerContext));
    context->screen_mutex=SDL_CreateMutex();
    context->screen_cond=SDL_CreateCond();

    strcpy(context->fileName,fileName);

    av_register_all();
    avformat_network_init();
    context->pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&(context->pFormatCtx),fileName,NULL,NULL)!=0){
        fprintf(stderr,"Couldn't open input stream.\n");
        return -1;
    }
    if(avformat_find_stream_info(context->pFormatCtx,NULL)<0){
        fprintf(stderr,"Couldn't find stream information.\n");
        return -1;
    }

    context->streamIndex[VIDEO]=context->streamIndex[AUDIO]=-1;
    int i;
    for(i=0; i<context->pFormatCtx->nb_streams; i++){
        if(context->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            find_decode(context,i,VIDEO);
            continue;
        }else if(context->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            find_decode(context,i,AUDIO);
            continue;
        }else{
            fprintf(stderr,"codec_type==>%d\n",context->pFormatCtx->streams[i]->codec->codec_type);
        }
    }
    if(!context->streamNum){
        fprintf(stderr,"do not find any media stream\n");
        return -1;
    }

    av_dump_format(context->pFormatCtx,0,fileName,0);

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    context->state = PLAY;

    if(context->streamIndex[VIDEO] != -1)
        video_init(context);
    if(context->streamIndex[AUDIO] != -1)
        audio_init(context);

    SDL_CreateThread(player_read_frame,NULL,context);
    //    SDL_CreateThread(showVideo,NULL,context);

    while(1)
    {
        if (context->state == QUIT)
        {
            break;
        }

        SDL_WaitEvent(&event);
        switch(event.type)
        {
        case SFM_REFRESH_EVENT:
        {
//            fprintf(stderr,"==>SFM_REFRESH_EVENT\n");
//            show_video(context);
            break;
        }
        case SDL_WINDOWEVENT:
        {
            SDL_GetWindowSize(context->screen, &context->window_w, &context->window_h);
            break;
        }
        case SDL_QUIT:
        {
            fprintf(stderr,"SDL_QUIT！\n");
            context->state=QUIT;
            SDL_Quit();
            break;
        }
        default:
        {
            break;
        }
        }
    }
    destory(context);
}

void destory(playerContext *context){
    packet_queue_destory(&context->audio_packet_queue);
    packet_queue_destory(&context->video_packet_queue);
    sws_freeContext(context->swsCtx);
    SDL_DestroyRenderer(context->sdlRenderer);
    SDL_DestroyTexture(context->sdlTexture);
    SDL_DestroyWindow(context->screen);
}

int find_decode(playerContext *context,int index,int type){
    context->pCodecCtx[type]=context->pFormatCtx->streams[index]->codec;
    AVCodec *pCodec=avcodec_find_decoder(context->pCodecCtx[type]->codec_id);
    if(pCodec==NULL){
        printf("Codec not found.\n");
        return -1;
    }
    if(avcodec_open2(context->pCodecCtx[type], pCodec,NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }

    context->streamIndex[type]=index;
    context->streamNum++;

    return 0;
}



int player_read_frame(void *data){
    playerContext *context=(playerContext *)data;
    AVPacket    packet;

    context->start_time=av_gettime();

    while(1){
        if(context->state == QUIT)
            break;

//        fprintf(stderr,"audio==>%d  video==>%d\n",context->audio_queue.nb_packets,context->video_queue.nb_packets);


        if(context->audio_packet_queue.nb_packets > VIDEO_MAX ||
                context->video_packet_queue.nb_packets > AUDIO_MAX ){
            SDL_Delay(50);
            continue;
        }

        if(av_read_frame(context->pFormatCtx, &packet)<0){
            context->state = END;
            //free  queue
            break;
        }

        if(packet.stream_index == context->streamIndex[VIDEO]){
            packet_queue_put(&context->video_packet_queue,&packet);
        }else if(packet.stream_index == context->streamIndex[AUDIO]){
            packet_queue_put(&context->audio_packet_queue,&packet);
        }else{
            av_free_packet(&packet);
        }
    }

    return 0;
}



