//
//  jdc_media_player.h
//  JDCFFPlayer
//
//  Created by Jidong Chen on 01/04/2017.
//  Copyright © 2017 jidong. All rights reserved.
//

#ifndef vip_media_player_h
#define vip_media_player_h

#include "avformat.h"
#include "avcodec.h"
#include "swscale.h"
#include "avutil.h"
#include "imgutils.h"
#include "vip_sdl.h"


struct VIPMediaContext {
    
    //格式化上下文
    AVFormatContext *fmtCtx;
    //视频解码器
    AVCodec *codecVideo;
    //解码器上下文
    AVCodecContext *codecCtxVideo;
    //视频流
    AVStream *videoStream;
    //视频位置
    int videoStreamIdx;
    
    //音频解码器
    AVCodec *codecAudio;
    //音频解码器上下文
    AVCodecContext *codecCtxAudio;
    //音频流
    AVStream *audioStream;
    //音频位置
    int audioStreamIdx;
    
    VIPSDLContext *sdlCtx;
    
    SDL_Thread *parse_tid;
    SDL_Thread *video_tid;
    
    struct SwsContext *swsCtx;
    //音频队列
    VIPSDLPacketQueue *audioQueue;
    //视频队列
    VIPSDLPacketQueue *videoQueue;
    //视频帧队列
    VIPSDLPacketQueue *videoFrameQueue;
    //文件名
    char filename[1024];
    
    //退出的标志
    int quit;
    //视频时钟
    
    double videoClock;
    //显示时间戳
    double frame_last_pts;
    //延时时间
    double frame_last_delay;
    //定时器
    double frame_timer;
    //音频时钟
    double audio_clock;
    int audio_buf_index;
    int audio_buf_size;
};

//VIPMediaContext自己封装的
typedef struct VIPMediaContext VIPMediaContext;

typedef struct VIPError {
}VIPError;


int vip_media_init();

VIPMediaContext *vip_media_open_input(const char *url,VIPError **error);

int vip_media_play(VIPMediaContext *mCtx);


#endif /* vip_media_player_h */
