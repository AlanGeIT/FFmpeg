//
//  FFmpegManager.h
//  LearnFFmpeg
//
//  Created by Alan Ge on 2020/7/2.
//  Copyright © 2020 AlanGe. All rights reserved.
//

#import <Foundation/Foundation.h>
//导入音视频头文件库
//核心库
#include "libavcodec/avcodec.h"
//封装格式处理库
#include "libavformat/avformat.h"
//工具库
#include "libavutil/imgutils.h"
//视频像素数据格式库
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"

NS_ASSUME_NONNULL_BEGIN

@interface FFmpegManager : NSObject

//音频编码
+(void)ffmpegAudioEncode:(NSString*)inFilePath outFilePath:(NSString*)outFilePath;

@end

NS_ASSUME_NONNULL_END
