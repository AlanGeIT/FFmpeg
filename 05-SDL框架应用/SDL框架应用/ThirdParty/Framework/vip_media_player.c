//
//  jdc_media_player.c
//  JDCFFPlayer
//
//  Created by Jidong Chen on 01/04/2017.
//  Copyright © 2017 jidong. All rights reserved.
//

#include "vip_media_player.h"
#include "vip_video_frame.h"
#include "time.h"

#define FF_QUIT_EVENT 10086
#define FF_REFRESH_EVENT 10087

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0

int vip_media_init()
{
    //FFmpeg注册组件
    av_register_all();
    return 0;
}

double synchronize_video(VIPMediaContext *mCtx , AVFrame *frame , double pst)
{
    double frame_delay = 0;
    if (pst != 0) {
        mCtx->videoClock = pst;
    }else{
        pst = mCtx->videoClock;
    }
    
    frame_delay = av_q2d(mCtx->videoStream->time_base);
    frame_delay += frame->repeat_pict * (frame_delay * 0.5);
    mCtx->videoClock += frame_delay;
    
    return pst;
}

int jdc_media_video_thread(void *data)
{
    VIPMediaContext *mCtx = data;
    mCtx->videoFrameQueue = vip_packet_queue_alloc();
    vip_packet_queue_init(mCtx->videoFrameQueue);
    
    while(1){
        
        AVFrame *pFrame = av_frame_alloc();
        AVPacket *packet;
        
        if(vip_packet_queue_get_packet(mCtx->videoQueue, (void **)&packet, 1) < 0) {
            av_frame_free(&pFrame);
            break;
        }
        
         int r = avcodec_send_packet(mCtx->codecCtxVideo, packet);
         if (r != 0) {
            av_packet_unref(packet);
            av_packet_free(&packet);
             continue;
         }
        
         r = avcodec_receive_frame(mCtx->codecCtxVideo, pFrame);
         if (r != 0) {
            av_packet_unref(packet);
            av_packet_free(&packet);
             continue;
         }
        
        double pst = 0;
        if (packet->dts != AV_NOPTS_VALUE) {
            pst = av_frame_get_best_effort_timestamp(pFrame);
        }
        pst *= av_q2d(mCtx->videoStream->time_base);
        pst = synchronize_video(mCtx, pFrame, pst);
        
        VIPVideoFrame *vFrame = vip_video_Frame_alloc();
        vFrame->avFrame = pFrame;
        vFrame->pts = pst;
        vip_packet_queue_push(mCtx->videoFrameQueue, vFrame);
        av_packet_unref(packet);
        av_packet_free(&packet);
    }
    
    
    return 0;
}

//获取音频解码(打开音频解码)

int vip_media_open_stream(VIPMediaContext *mCtx , int sIdx){
    
    //格式化上下文
    AVFormatContext *pFormatCtx = mCtx->fmtCtx;
    //解码器上下文
    AVCodecContext *codecCtx = NULL;
   //解码器
    AVCodec *codec = NULL;
    //判断位置
    if (sIdx < 0 || sIdx >= pFormatCtx->nb_streams) {
        return -1;
    }
    //获取流
    AVStream *stream = pFormatCtx->streams[sIdx];
    //获取解码器
    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "Unsupported codec!\n");//找不到解码器
        return -1;
    }
    //创建解码器上下文
    codecCtx = avcodec_alloc_context3(codec);
    
    //将参数信息封装到编解码器上下文中
    if(avcodec_parameters_to_context(codecCtx, stream->codecpar) < 0){
        fprintf(stderr, "avcodec parameters to context failed!\n");
        return -1;
    }
    
    //设置音频信息
    if (codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        //封装音频信息的结构体
        //SDL框架处理音频信息->播放音频
        SDL_AudioSpec wanted_spec;
        SDL_AudioSpec spec;
        wanted_spec.freq = codecCtx->sample_rate;
        wanted_spec.format = AUDIO_S16SYS;
        wanted_spec.channels = codecCtx->channels;
        wanted_spec.silence = 0;
        wanted_spec.samples = 1024;
        //设置音频回调函数->专门用一个线程处理音频->初始化音频相关信息
        wanted_spec.callback = vip_sdl_audio_callback;
        wanted_spec.userdata = mCtx;
        
        //打开音频->打开音频，并没有开始播放
        if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
            fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
            return -1;
        }
    }
    
    //打开解码器
    if(avcodec_open2(codecCtx, codec, NULL) < 0) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }
    
    //判断流的类型
    switch(codecCtx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            mCtx->audioStreamIdx = sIdx;
            mCtx->audioStream = stream;
            mCtx->codecAudio = codec;
            mCtx->codecCtxAudio = codecCtx;
            mCtx->audioQueue = vip_packet_queue_alloc();
            vip_packet_queue_init(mCtx->audioQueue);
            SDL_PauseAudio(0);
            break;
        case AVMEDIA_TYPE_VIDEO:
            mCtx->frame_timer = (double)av_gettime() / 1000000.0;
            mCtx->frame_last_delay = 40e-3;
            mCtx->videoStreamIdx = sIdx;
            mCtx->videoStream = stream;
            mCtx->codecVideo = codec;
            mCtx->codecCtxVideo = codecCtx;
            mCtx->videoQueue = vip_packet_queue_alloc();
            mCtx->video_tid = SDL_CreateThread(jdc_media_video_thread,
                                               "video thread",
                                               mCtx);
            vip_packet_queue_init(mCtx->videoQueue);
            //格式转换上下文
            mCtx->swsCtx = sws_getContext(mCtx->codecCtxVideo->width,
                                           mCtx->codecCtxVideo->height,
                                           mCtx->codecCtxVideo->pix_fmt,
                                           mCtx->codecCtxVideo->width,
                                           mCtx->codecCtxVideo->height,
                                           AV_PIX_FMT_YUV420P,
                                           SWS_BILINEAR,
                                           NULL,
                                           NULL,
                                           NULL);
            break;
        default:
            break;
    }
    
    return 0;
}

int vip_media_decode_thread(void *userData)
{
    VIPMediaContext *mCtx = userData;
    
    
    AVFrame *pFrame = NULL;
    pFrame = av_frame_alloc();
    
    if (pFrame == NULL) {
        return -1;
    }
    
    int numBytes;
    numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                        mCtx->codecCtxVideo->width,
                                        mCtx->codecCtxVideo->height,
                                        1);
    AVPacket *packet;
    
    int ret = -1;
    do{
        packet = av_packet_alloc();
        ret = av_read_frame(mCtx->fmtCtx, packet);
        if (ret >= 0) {
            if (packet->stream_index == mCtx->videoStream->index) {
                vip_packet_queue_push(mCtx->videoQueue, packet);
            }else if(packet->stream_index == mCtx->audioStream->index){
                vip_packet_queue_push(mCtx->audioQueue, packet);
            }
        }
    }while(ret >= 0);
    
    
    return 0;
}



VIPMediaContext *vip_media_open_input(const char *url,VIPError **error)
{
    //存贮视频信息的自定义的上下文
    VIPMediaContext *mCtx = (VIPMediaContext *)av_mallocz(sizeof(VIPMediaContext));
    //初始化->格式化上下文
    AVFormatContext *pFmtCtx = avformat_alloc_context();
    
    //复制文件名
    strcpy(mCtx->filename, url);
    //打开文件->打开封装格式文件
    if (avformat_open_input(&pFmtCtx, url, NULL, NULL) != 0) {
        //代开失败:释放上下文
        av_free(mCtx);
        return NULL;
    }
    
    //设置格式化上下文
    mCtx->fmtCtx = pFmtCtx;
    
    //查找信息
    if (avformat_find_stream_info(pFmtCtx, NULL) < 0) {
        av_free(mCtx);
        return NULL;
    }
    
    //打印视频信息
    // Dump information about file onto standard error
    av_dump_format(pFmtCtx, 0, mCtx->filename, 0);

    //获取视频流和音频流的位置
    for(int i = 0 ; i < pFmtCtx->nb_streams ; i++){
        
        //获取视频流的位置
        if(pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
           mCtx->videoStream == NULL){
            mCtx->videoStreamIdx = i;
        }
        //获取音频流位置
        if(pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
           mCtx->audioStream == NULL){
            mCtx->audioStreamIdx = i;
        }
    }
    
    
    return mCtx;
}
static Uint32 sdl_refresh_timer_cb(Uint32 interval, void *opaque) {
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = opaque;
    SDL_PushEvent(&event);
    return 0; /* 0 means stop timer */
}

int schedule_refresh(VIPMediaContext *mCtx, int n)
{
    return SDL_AddTimer(n, sdl_refresh_timer_cb, mCtx);
}

int video_display(VIPMediaContext *mCtx , void *data) {
    
    AVFrame *pFrameYUV = mCtx->sdlCtx->frame;

    VIPVideoFrame *vFrame = data;
    AVFrame *pFrame = vFrame->avFrame;
    VIPSDLContext *sdlCtx = mCtx->sdlCtx;
    
    struct SwsContext *swsCtx = mCtx->swsCtx;
    
    sws_scale(swsCtx,
              (uint8_t  const * const *)pFrame->data,
              pFrame->linesize,
              0,
              pFrame->height,
              pFrameYUV->data,
              pFrameYUV->linesize);
    
    SDL_Rect sdlRect;
    sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = pFrame->width;
    sdlRect.h = pFrame->height;
    
    SDL_UpdateYUVTexture(sdlCtx->texture, &sdlRect,
                         pFrameYUV->data[0], pFrameYUV->linesize[0],
                         pFrameYUV->data[1], pFrameYUV->linesize[1],
                         pFrameYUV->data[2], pFrameYUV->linesize[2]);
    
    SDL_RenderClear(sdlCtx->renderer );
    SDL_RenderCopy( sdlCtx->renderer, sdlCtx->texture,NULL, NULL );
    SDL_RenderPresent( sdlCtx->renderer );
    
    vip_video_Frame_free(vFrame);
    
    return 0;
}

static double get_audio_clock(VIPMediaContext *mCtx)
{
    double pts;
    int hw_buf_size, bytes_per_sec, n;
    
    pts = mCtx->audio_clock; /* maintained in the audio thread */
    hw_buf_size = mCtx->audio_buf_size - mCtx->audio_buf_index;
    bytes_per_sec = 0;
    n = mCtx->audioStream->codecpar->channels * 2;
    if(mCtx->audioStream) {
        bytes_per_sec = mCtx->audioStream->codecpar->sample_rate * n;
    }
    if(bytes_per_sec) {
        pts -= (double)hw_buf_size / bytes_per_sec;
    }
    return pts;
}

void video_refresh_timer(void *userdata) {
    
    VIPMediaContext *mCtx = (VIPMediaContext *)userdata;
    
    if(mCtx->videoStream) {
        if(vip_packet_queue_size(mCtx->videoFrameQueue) == 0) {
            schedule_refresh(mCtx, 1);
        } else {
            VIPVideoFrame *vFrame = NULL;
            vip_packet_queue_get_packet(mCtx->videoFrameQueue, (void *)&vFrame, 1);
            double actual_delay, delay, sync_threshold, ref_clock, diff;
            delay = vFrame->pts - mCtx->frame_last_pts; /* the pts from last time */
            if(delay <= 0 || delay >= 1.0) {
                /* if incorrect delay, use previous one */
                delay = mCtx->frame_last_delay;
            }
            /* save for next time */
            mCtx->frame_last_delay = delay;
            mCtx->frame_last_pts = vFrame->pts;
            
            /* update delay to sync to audio */
            ref_clock = get_audio_clock(mCtx);
            diff = vFrame->pts - ref_clock;
            
            /* Skip or repeat the frame. Take delay into account
             FFPlay still doesn't "know if this is the best guess." */
            sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
            if(fabs(diff) < AV_NOSYNC_THRESHOLD) {
                if(diff <= -sync_threshold) {
                    delay = 0;
                } else if(diff >= sync_threshold) {
                    delay = 2 * delay;
                }
            }
            mCtx->frame_timer += delay;
            /* computer the REAL delay */
            actual_delay = mCtx->frame_timer - (av_gettime() / 1000000.0);
            if(actual_delay < 0.010) {
                /* Really it should skip the picture instead */
                actual_delay = 0.010;
            }
            schedule_refresh(mCtx, (int)(actual_delay * 1000 + 0.5));
            /* show the picture! */
            video_display(mCtx,vFrame);
        }
    } else {
        schedule_refresh(mCtx, 100);
    }
}

int vip_media_play(VIPMediaContext *mCtx)
{
    //打开音频流
    //目的：为了解析音频信息(目的：解码、打开音频，准备音频)
    vip_media_open_stream(mCtx, mCtx->audioStreamIdx);
    //打开视频流
    vip_media_open_stream(mCtx, mCtx->videoStreamIdx);
    mCtx->sdlCtx = vip_sdl_create_context(mCtx);
    
    SDL_Event event;
    //有队列，将线程进行添加
    schedule_refresh(mCtx, 30);
    
    mCtx->parse_tid = SDL_CreateThread(vip_media_decode_thread, "decode thread",mCtx);
    
    
    if(!mCtx->parse_tid) {
        return -1;
    }
    
    while(1){
        
        SDL_WaitEvent(&event);
        switch(event.type) {
            case FF_QUIT_EVENT:
            case SDL_QUIT:
                mCtx->quit = 1;
                SDL_Quit();
                return 0;
                break;
            case FF_REFRESH_EVENT:
                video_refresh_timer(event.user.data1);
                break;
            default:
                break;
        }
    }
    return 0;
}
