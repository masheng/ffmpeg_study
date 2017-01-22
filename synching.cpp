#include "player.h"
#include<math.h>

double getAudioClock(playerContext *context){
    long long bytes_per_sec = 0;
    double cur_buf_pos = context->audio_buf_index;
    //每个样本占2bytes。16bit
    int audio_index=context->streamIndex[AUDIO];
    int rate=context->pFormatCtx->streams[audio_index]->codec->sample_rate;
    int chan=context->pCodecCtx[AUDIO]->channels;
    bytes_per_sec = rate * chan * 2;
    context->audio_clock += (cur_buf_pos / (double)bytes_per_sec);

    return context->audio_clock;
}

double getVideoClock(playerContext *context, AVFrame *pFrame){
    double pts=0, frame_delay=0;
    if ((pts = av_frame_get_best_effort_timestamp(pFrame)) == AV_NOPTS_VALUE)
        pts = 0;
    pts *= av_q2d(context->pFormatCtx->streams[VIDEO]->time_base);
    if(pts==0)
        pts=context->video_clock;
    else
        context->video_clock=pts;

    frame_delay = av_q2d(context->pFormatCtx->streams[VIDEO]->time_base);
    frame_delay += pFrame->repeat_pict / (frame_delay * 2);
    context->video_clock += frame_delay;

    return pts;
}

double get_currect_time(playerContext *context){
    double result=(context->start_time-av_gettime())/1000000.0;
    return abs(result);
}

int synch(playerContext *context, double *delay){
    double      frame_delay = 0.0;
    double      compare = 0.0;
    double      threshold = 0.0;

    //获取两帧之间的延时
    frame_delay = context->video_clock - context->video_clock_pre;
    if (frame_delay <= 0 || frame_delay >= 1.0)
    {
        if(context->frame_delay!=0 && frame_delay>context->frame_delay*2){
            frame_delay = context->frame_delay*2;
        }else
            frame_delay = context->frame_delay;
    }

    context->frame_delay = frame_delay;
    context->video_clock_pre = context->video_clock;
    compare = context->video_clock - get_currect_time(context);

    threshold = frame_delay;

    if (compare <= -threshold){
        *delay = frame_delay / 2;
    }else if (compare >= threshold) { //快了，就在上一帧延时的基础上加长延时
        *delay = frame_delay * 2;
    } else {
        *delay = frame_delay;
    }

   fprintf(stderr,"%f=%f av=>%f com==>%f=%f delay==>%f frame_delay==>%f %d=%d=%d\n",context->video_clock, context->audio_clock, context->video_clock-context->audio_clock,compare,context->audio_clock - get_currect_time(context),*delay,frame_delay,context->video_packet_queue.nb_packets,context->audio_packet_queue.nb_packets);

    return 0;
}
