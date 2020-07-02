//
//  jdc_sdl.h
//  JDCFFPlayer
//
//  Created by ChenJidong on 17/4/2.
//  Copyright © 2017年 jidong. All rights reserved.
//

#ifndef vip_sdl_h
#define vip_sdl_h

#include "SDL.h"
#include "avformat.h"
#include "avcodec.h"
#include "swscale.h"
#include "avutil.h"
#include "imgutils.h"
#include "vip_sdl_queue.h"
#include "swresample.h"

struct VIPMediaContext;
//typedef struct VIPMediaContext VIPMediaContext;

typedef struct VIPSDLContext{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    AVFrame *frame;
} VIPSDLContext;

void vip_sdl_audio_callback(void *userdata, Uint8 * stream,int len);

int vip_sdl_init(void);
int vip_sdl_present_frame(VIPSDLContext *sdl_context, AVFrame *avframe);

VIPSDLContext *vip_sdl_create_context(struct VIPMediaContext *media_context);

int vip_sdl_play_audio(struct VIPMediaContext *media_context , VIPSDLContext *sdl_context);


#endif /* vip_sdl_h */
