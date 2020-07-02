//
//  ViewController.m
//  LearnFFmpeg
//
//  Created by Alan Ge on 2020/7/2.
//  Copyright © 2020 AlanGe. All rights reserved.
//

#import "ViewController.h"
#import "FFmpegManager.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    NSString* inPath = [[NSBundle mainBundle] pathForResource:@"Test" ofType:@"yuv"];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask, YES);
    NSString *path = [paths objectAtIndex:0];
    NSString *tmpPath = [path stringByAppendingPathComponent:@"temp"];
    [[NSFileManager defaultManager] createDirectoryAtPath:tmpPath withIntermediateDirectories:YES attributes:nil error:NULL];
    NSString* outFilePath = [tmpPath stringByAppendingPathComponent:[NSString stringWithFormat:@"Test.h264"]];
    NSLog(@"编码后的视频 = %@",outFilePath);
    [FFmpegManager ffmpegVideoEncode:inPath outFilePath:outFilePath zhen:^(NSString *zhen) {
        NSLog(@"zhen = %@",zhen);
    }];
}

@end
