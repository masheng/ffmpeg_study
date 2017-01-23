#include "player.h"

#define AVCODE_MAX_AUDIO_FRAME_SIZE    192000

void player_audio_callback(void *userdata, uint8_t *stream, int len)
{
    playerContext *context = (playerContext *)userdata;
    static uint8_t  audio_buf[(AVCODE_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    int 		send_data_size = 0;
    int 		audio_size = 0;

    if (context->state == QUIT)
    {
        exit(-1);
    }

    SDL_memset(stream, 0, len);


    while(len > 0)
    {
        if (context->audio_buf_index >= context->audio_buf_size)
        {
            audio_size = audio_decode_frame(context, audio_buf);

            if (audio_size < 0)
            {
                context->audio_buf_size = 1024;
                memset(audio_buf, 0, context->audio_buf_size);
            }
            else
            {
                context->audio_buf_size = audio_size;
            }
            context->audio_buf_index = 0;
        }

        send_data_size = context->audio_buf_size - context->audio_buf_index;
        if (len < send_data_size)
        {
            send_data_size = len;
        }

        SDL_MixAudio(stream,
                     (uint8_t *)audio_buf + context->audio_buf_index,
                     send_data_size, SDL_MIX_MAXVOLUME);

        len -= send_data_size;
        stream += send_data_size;
        context->audio_buf_index += send_data_size;
    }

}

int audio_init(playerContext *data){
    playerContext *context=(playerContext *)data;
    SDL_AudioSpec      wanted_spec;
    wanted_spec.freq      = context->pCodecCtx[AUDIO]->sample_rate;
    wanted_spec.format    = AUDIO_S16SYS;
    wanted_spec.channels  = context->pCodecCtx[AUDIO]->channels;
    wanted_spec.silence   = 0;
    wanted_spec.samples   = 1024;
    wanted_spec.callback  = player_audio_callback;
    wanted_spec.userdata  = data;

    if (SDL_OpenAudio(&wanted_spec, NULL) < 0)
    {
        fprintf(stderr, "Couldn't open audio device\n");
        return -1;
    }

    SDL_PauseAudio(0);

    return 0;
}

int audio_decode_frame(playerContext *context, uint8_t *audio_buf)
{
    AVPacket           packet;
    AVFrame            *pframe;
    AVSampleFormat     dst_sample_fmt;
    uint64_t           dst_channel_layout;
    uint64_t           dst_nb_samples;
    int                convert_len;
    SwrContext 		*swr_ctx = NULL;
    int                data_size;
    int 				ret = 0;

    pframe = av_frame_alloc();

    if (context->state==QUIT)
        return -1;

    if (packet_queue_get(&context->audio_packet_queue, &packet, 1) == -1)
    {
        fprintf(stderr, "Get queue packet error\n");
        return -1;
    }

    if (packet.pts != AV_NOPTS_VALUE && context->streamIndex[VIDEO]!=-1)
    {
        double tb=av_q2d(context->pFormatCtx->streams[AUDIO]->time_base);
        double clock = packet.pts * tb;
        context->audio_clock=clock;
        double delay=context->audio_clock-get_currect_time(context);

        /*
        if(delay > 0.01)
            SDL_Delay((delay*1000+0.5)/2);
        else if(delay < -1.0){
            if (packet_queue_get(&context->audio_packet_queue, &packet, 1) == -1)
            {
                fprintf(stderr, "Get queue packet error\n");
                return -1;
            }
        }
        */
    }

    ret = avcodec_send_packet(context->pCodecCtx[AUDIO], &packet);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    {
        fprintf(stderr, "send decode packet error\n");
        return -1;
    }

    ret = avcodec_receive_frame(context->pCodecCtx[AUDIO], pframe);
    if (ret < 0 && ret != AVERROR_EOF)
    {
        fprintf(stderr, "receive decode frame error\n");
        return -1;
    }



    if (pframe->channels > 0 && pframe->channel_layout == 0)
    {
        pframe->channel_layout = av_get_default_channel_layout(pframe->channels);
    }
    else if (pframe->channels == 0 && pframe->channel_layout > 0)
    {
        pframe->channels = av_get_channel_layout_nb_channels(pframe->channel_layout);
    }


    dst_sample_fmt     = AV_SAMPLE_FMT_S16;
    dst_channel_layout = av_get_default_channel_layout(pframe->channels);
    swr_ctx = swr_alloc_set_opts(NULL, dst_channel_layout, dst_sample_fmt,
                                 pframe->sample_rate, pframe->channel_layout,
                                 (AVSampleFormat)pframe->format, pframe->sample_rate, 0, NULL);
    if (swr_ctx == NULL || swr_init(swr_ctx) < 0)
    {
        fprintf(stderr, "swr set open or swr init error\n");
        return -1;
    }

    dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx, pframe->sample_rate) + pframe->nb_samples,
                pframe->sample_rate, pframe->sample_rate, AVRounding(1));

    convert_len = swr_convert(swr_ctx, &audio_buf, dst_nb_samples,
                              (const uint8_t **)pframe->data, pframe->nb_samples);

    data_size = convert_len * pframe->channels * av_get_bytes_per_sample(dst_sample_fmt);

    av_frame_free(&pframe);
    swr_free(&swr_ctx);

    return data_size;
}
