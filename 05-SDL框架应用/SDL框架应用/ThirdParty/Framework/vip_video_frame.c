//
//  jdc_video_frame.c
//  JDCFFPlayer
//
//  Created by ChenJidong on 17/4/8.
//  Copyright © 2017年 jidong. All rights reserved.
//

#include "vip_video_frame.h"

VIPVideoFrame * vip_video_Frame_alloc(){
    return av_mallocz(sizeof(VIPVideoFrame));
}

void vip_video_Frame_free(VIPVideoFrame *frame){
    if (frame->avFrame) {
        av_frame_unref(frame->avFrame);
        av_frame_free(&frame->avFrame);
    }
    
    av_free(frame);
}
