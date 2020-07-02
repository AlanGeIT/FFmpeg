//
//  jdc_video_frame.h
//  JDCFFPlayer
//
//  Created by ChenJidong on 17/4/8.
//  Copyright © 2017年 jidong. All rights reserved.
//

#ifndef vip_video_frame_h
#define vip_video_frame_h

#include "avformat.h"
#include "avcodec.h"
#include "swscale.h"
#include "avutil.h"
#include "imgutils.h"

struct VIPVideoFrame {
    AVFrame *avFrame;
    double pts;
};

typedef struct VIPVideoFrame VIPVideoFrame;

VIPVideoFrame * vip_video_Frame_alloc();
void vip_video_Frame_free(VIPVideoFrame *frame);

#endif /* vip_video_frame_h */
