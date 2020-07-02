//
//  main.m
//  SDL框架应用
//
//  Created by Alan Ge on 2020/7/2.
//  Copyright © 2020 AlanGe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#import "vip_media_player.h"

int main(int argc, char * argv[]) {
    @autoreleasepool {
        NSString* inPath = [[NSBundle mainBundle] pathForResource:@"Test" ofType:@"mov"];
        //第一步：初始化FFmpeg
        vip_media_init();
        //第二步：初始化SDL
        vip_sdl_init();
        //第三步：打开SDL和FFmpeg
        VIPMediaContext * mCtx = vip_media_open_input([inPath UTF8String], NULL);
        //第四步：播放
        vip_media_play(mCtx);
        return 0;
    }
}
