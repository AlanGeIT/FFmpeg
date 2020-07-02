//
//  jdc_sdl_queue.h
//  JDCFFPlayer
//
//  Created by ChenJidong on 17/4/4.
//  Copyright © 2017年 jidong. All rights reserved.
//

#ifndef vip_sdl_queue_h
#define vip_sdl_queue_h

#include "avformat.h"
#include "avcodec.h"
#include "swscale.h"
#include "avutil.h"
#include "imgutils.h"
#include "SDL.h"

struct VIPSDLPacketQueue;
typedef struct VIPSDLPacketQueue VIPSDLPacketQueue;


VIPSDLPacketQueue *vip_packet_queue_alloc(void);

void vip_packet_queue_init(VIPSDLPacketQueue *queue);

int vip_packet_queue_size(VIPSDLPacketQueue *queue);

int vip_packet_queue_push(VIPSDLPacketQueue *queue , void *data);

void *vip_packet_queue_front(VIPSDLPacketQueue *queue);

void *vip_packet_queue_pop(VIPSDLPacketQueue *queue);

int vip_packet_queue_get_packet(VIPSDLPacketQueue *queue , void **data , int blockThread);

#endif /* vip_sdl_queue_h */
