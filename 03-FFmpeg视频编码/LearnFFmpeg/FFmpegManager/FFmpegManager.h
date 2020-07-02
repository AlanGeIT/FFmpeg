//
//  FFmpegManager.h
//  LearnFFmpeg
//
//  Created by Alan Ge on 2020/7/2.
//  Copyright © 2020 AlanGe. All rights reserved.
//

#import <Foundation/Foundation.h>
//核心库
#include "libavcodec/avcodec.h"
//封装格式处理库
#include "libavformat/avformat.h"
//工具库
#include "libavutil/imgutils.h"

NS_ASSUME_NONNULL_BEGIN

@interface FFmpegManager : NSObject

// FFmpeg视频编码
+(void)ffmpegVideoEncode:(NSString*)inFilePath outFilePath:(NSString*)outFilePath zhen:(void(^)(NSString *zhen))zhen;

@end

NS_ASSUME_NONNULL_END
