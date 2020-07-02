//
//  FFmpegManager.h
//  LearnFFmpeg
//
//  Created by Alan Ge on 2020/7/2.
//  Copyright © 2020 AlanGe. All rights reserved.
//

#import <Foundation/Foundation.h>
//导入头文件
//核心库
#include "libavcodec/avcodec.h"
//封装格式处理库
#include "libavformat/avformat.h"
//工具库
#include "libavutil/imgutils.h"
//视频像素数据格式库
#include "libswscale/swscale.h"

NS_ASSUME_NONNULL_BEGIN

@interface FFmpegManager : NSObject

//视频解码
+(void) ffmepgVideoDecode:(NSString*)inFilePath outFilePath:(NSString*)outFilePath;

@end

NS_ASSUME_NONNULL_END
