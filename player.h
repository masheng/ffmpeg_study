#ifndef PLAYER_H
#define PLAYER_H

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#ifdef __cplusplus
}
#endif

#include "packet_queue.h"
#include "frame_queue.h"

#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

#define VIDEO 0
#define AUDIO 1
#define STREAM_NUM 2
#define AUDIO_MAX 80
#define VIDEO_MAX 80
#define VIDEO_FRAME_MAX 80


typedef enum{
    QUIT,
    PLAY,
    PAUSE,
    END
}play_state;

typedef struct {
    AVFormatContext	*pFormatCtx;
    char fileName[256];
    int streamIndex[STREAM_NUM];
    int streamNum;
    PacketQueue video_packet_queue;
    PacketQueue audio_packet_queue;
    FrameQueue video_frame_queue;
    double video_clock_pre;
    double video_clock;
    double audio_clock;
    double frame_delay;

    int64_t start_time;
    double currect_time;

    play_state state;

    int audio_buf_size;
    int audio_buf_index;

    AVCodecContext *pCodecCtx[STREAM_NUM];
    struct SwsContext *swsCtx;

    SDL_TimerID timerID;
    SDL_mutex       *screen_mutex;
    SDL_cond        *screen_cond;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* sdlTexture;
    SDL_Window *screen;
    SDL_Rect sdlRect;
    int window_h;
    int window_w;
    AVFrame	*outFrame;
}playerContext;

int init(char *);
int find_decode(playerContext *,int index,int type);
int player_refresh_thread(void *);
int player_read_frame(void *);

int audio_init(playerContext *);
int audio_decode_frame(playerContext *, uint8_t *audio_buf);

int video_init(playerContext *);
int video_decode_frame(playerContext *,AVFrame *);
Uint32 show_video(Uint32 interval, void *data);
double getAudioClock(playerContext *);
double getVideoClock(playerContext *, AVFrame *);
double get_currect_time(playerContext *);
int synch(playerContext *, double *);

void destory(playerContext *);
#endif // PLAYER_H
