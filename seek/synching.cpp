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
    fprintf(stderr,"==================>%lld\n",);
    return pts;
}

double get_currect_time(playerContext *context){
    double result=(av_gettime()-context->start_time)/1000000.0;
    return result;
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

   fprintf(stderr,"v=>%f a=>%f v-a=>%f cur=>%f v:%d=a:%d %f\n",context->video_clock, context->audio_clock, context->video_clock-context->audio_clock,get_currect_time(context),context->video_packet_queue.nb_packets,context->audio_packet_queue.nb_packets, av_q2d(context->pFormatCtx->streams[VIDEO]->time_base));

    return 0;
}

int do_seek(playerContext *context,double inr){
    int stream_index=context->streamIndex[VIDEO]==-1?context->streamIndex[AUDIO]:context->streamIndex[VIDEO];

    int64_t seek_target = av_gettime()-context->start_time +inr * AV_TIME_BASE;
    int64_t duration=get_duration(context) * AV_TIME_BASE;

    if(seek_target >= duration){
        seek_target = duration-3;
    }else if(seek_target <= 0){
        seek_target = 0;
    }

    int seek_flags = inr>0? 0 : AVSEEK_FLAG_BACKWARD;
//    if(stream_index>=0){
//        seek_target=av_rescale_q(seek_target, AV_TIME_BASE_Q, context->pFormatCtx->streams[stream_index]->time_base);
//    }else
//        return -1;

    double stream_base=av_q2d(context->pFormatCtx->streams[stream_index]->time_base);
    seek_target=(int64_t)(seek_target * stream_base / av_q2d(AV_TIME_BASE_Q));
    /*
    x   a
    b   c
 */
    context->start_time -= (inr * AV_TIME_BASE);
    if(av_seek_frame(context->pFormatCtx, stream_index, 5000, seek_flags) < 0){
        fprintf(stderr,"do_seek err\n");
        context->start_time += (inr * AV_TIME_BASE);
        return -1;
    }


    packet_queue_flush(&context->audio_packet_queue);
    packet_queue_flush(&context->video_packet_queue);

    return 0;
}

double get_duration(playerContext *context){
    return context->pFormatCtx->duration/AV_TIME_BASE;
}
