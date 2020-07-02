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
    
    NSString *inStr= [NSString stringWithFormat:@"Video.bundle/%@",@"Test.mov"];
    NSString *inPath=[[[NSBundle mainBundle]resourcePath] stringByAppendingPathComponent:inStr];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *path = [paths objectAtIndex:0];
    NSString *tmpPath = [path stringByAppendingPathComponent:@"temp"];
    [[NSFileManager defaultManager] createDirectoryAtPath:tmpPath withIntermediateDirectories:YES attributes:nil error:NULL];
    NSString* outFilePath = [tmpPath stringByAppendingPathComponent:[NSString stringWithFormat:@"Test.pcm"]];
    
    NSLog(@"音频解码后的保存路径 = %@",outFilePath);
    
//    [FFmpegManager ffmpegAudioDecode:inPath outFilePath:outFilePath];
    [FFmpegManager ffmpegAudioDecode1:inPath outFilePath:outFilePath];
}

@end
