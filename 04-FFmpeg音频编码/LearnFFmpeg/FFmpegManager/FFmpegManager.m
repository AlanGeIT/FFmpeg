//
//  FFmpegManager.m
//  LearnFFmpeg
//
//  Created by Alan Ge on 2020/7/2.
//  Copyright © 2020 AlanGe. All rights reserved.
//

#import "FFmpegManager.h"

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
          CODEC_CAP_DELAY))
    return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                    NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
        break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        NSLog(@"Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
        break;
    }
    return ret;
}

@implementation FFmpegManager

+(void)ffmpegAudioEncode:(NSString*)inFilePath outFilePath:(NSString*)outFilePath{
    //第一步：注册组件->音频编码器等等…
    av_register_all();
    
    //第二步：初始化封装格式上下文->视频编码->处理为音频压缩数据格式
    AVFormatContext *avformat_context = avformat_alloc_context();
    //注意事项：FFmepg程序推测输出文件类型->音频压缩数据格式类型->aac格式
    const char *coutFilePath = [outFilePath UTF8String];
    //得到音频压缩数据格式类型(aac、mp3等...)
    AVOutputFormat *avoutput_format = av_guess_format(NULL, coutFilePath, NULL);
    //指定类型
    avformat_context->oformat = avoutput_format;
    
    //第三步：打开输出文件
    //参数一：输出流
    //参数二：输出文件
    //参数三：权限->输出到文件中
    if (avio_open(&avformat_context->pb, coutFilePath, AVIO_FLAG_WRITE) < 0) {
        NSLog(@"打开输出文件失败");
        return;
    }
    
    //第四步：创建输出码流->创建了一块内存空间->并不知道他是什么类型流->希望他是视频流
    AVStream *audio_st = avformat_new_stream(avformat_context, NULL);
    
    //第五步：查找音频编码器
    //1、获取编码器上下文
    AVCodecContext *avcodec_context = audio_st->codec;
    
    //2、设置编解码器上下文参数->必需设置->不可少
    //目标：设置为是一个音频编码器上下文->指定的是音频编码器
    //上下文种类：音频解码器、音频编码器
    //2.1 设置音频编码器ID
    avcodec_context->codec_id = avoutput_format->audio_codec;
    //2.2 设置编码器类型->音频编码器
    //视频编码器->AVMEDIA_TYPE_VIDEO
    //音频编码器->AVMEDIA_TYPE_AUDIO
    avcodec_context->codec_type = AVMEDIA_TYPE_AUDIO;
    //2.3 设置读取音频采样数据格式->编码的是音频采样数据格式->音频采样数据格式->pcm格式
    //注意：这个类型是根据你解码的时候指定的解码的音频采样数据格式类型
    avcodec_context->sample_fmt = AV_SAMPLE_FMT_S16;
    //设置采样率
    avcodec_context->sample_rate = 44100;
    //立体声
    avcodec_context->channel_layout = AV_CH_LAYOUT_STEREO;
    //声道数量
    int channels = av_get_channel_layout_nb_channels(avcodec_context->channel_layout);
    avcodec_context->channels = channels;
    //设置码率
    //基本的算法是：【码率】(kbps)=【视频大小 - 音频大小】(bit位) /【时间】(秒)
    avcodec_context->bit_rate = 128000;
    
    //第二点：查找音频编码器->aac
//    AVCodec *avcodec = avcodec_find_encoder(avcodec_context->codec_id);
    AVCodec *avcodec = avcodec_find_encoder_by_name("libfdk_aac");
    if (avcodec == NULL) {
        NSLog(@"找不到音频编码器");
        return;
    }
    
    
    //第六步：打开aac编码器
    if (avcodec_open2(avcodec_context, avcodec, NULL) < 0) {
        NSLog(@"打开音频编码器失败");
        return;
    }
    
    //第七步：写文件头（对于某些没有文件头的封装格式，不需要此函数。比如说MPEG2TS）
    avformat_write_header(avformat_context, NULL);
    
    //打开YUV文件
    const char *c_inFilePath = [inFilePath UTF8String];
    FILE *in_file = fopen(c_inFilePath, "rb");
    if (in_file == NULL) {
        NSLog(@"YUV文件打开失败");
        return;
    }
    
    //第十步：初始化音频采样数据帧缓冲区
    AVFrame *av_frame = av_frame_alloc();
    av_frame->nb_samples = avcodec_context->frame_size;
    av_frame->format = avcodec_context->sample_fmt;
    
    //得到音频采样数据缓冲区大小
    int buffer_size = av_samples_get_buffer_size(NULL,
                                                 avcodec_context->channels,
                                                 avcodec_context->frame_size,
                                                 avcodec_context->sample_fmt,
                                                 1);
    
    
    //创建缓冲区->存储音频采样数据->一帧数据
    uint8_t *out_buffer = (uint8_t *) av_malloc(buffer_size);
    avcodec_fill_audio_frame(av_frame,
                             avcodec_context->channels,
                             avcodec_context->sample_fmt,
                             (const uint8_t *)out_buffer,
                             buffer_size,
                             1);
    
    //第十二步：创建音频压缩数据->帧缓存空间
    AVPacket *av_packet = (AVPacket *) av_malloc(buffer_size);
    
    
    //第十三步：循环读取视频像素数据格式->编码压缩->视频压缩数据格式
    int frame_current = 1;
    int i = 0, ret = 0;
    
    //第八步：循环编码每一帧视频
    //即将AVFrame（存储YUV像素数据）编码为AVPacket（存储H.264等格式的码流数据）
    while (true) {
        //1、读取一帧音频采样数据
        if (fread(out_buffer, 1, buffer_size, in_file) <= 0) {
            NSLog(@"Failed to read raw data! \n");
            break;
        } else if (feof(in_file)) {
            break;
        }
        
        //2、设置音频采样数据格式
        //将outbuffer->av_frame格式
        av_frame->data[0] = out_buffer;
        av_frame->pts = i;
        i++;
        
        //3、编码一帧音频采样数据->得到音频压缩数据->aac
        //采用新的API
        //3.1 发送一帧音频采样数据
        ret = avcodec_send_frame(avcodec_context, av_frame);
        if (ret != 0) {
            NSLog(@"Failed to send frame! \n");
            return;
        }
        //3.2 编码一帧音频采样数据
        ret = avcodec_receive_packet(avcodec_context, av_packet);
        
        if (ret == 0) {
            //第九步：将编码后的音频码流写入文件
            NSLog(@"当前编码到了第%d帧", frame_current);
            frame_current++;
            av_packet->stream_index = audio_st->index;
            ret = av_write_frame(avformat_context, av_packet);
            if (ret < 0) {
                NSLog(@"写入失败! \n");
                return;
            }
        } else {
            NSLog(@"Failed to encode! \n");
            return;
        }
    }
    
    //第十步：输入的像素数据读取完成后调用此函数。用于输出编码器中剩余的AVPacket。
    ret = flush_encoder(avformat_context, 0);
    if (ret < 0) {
        NSLog(@"Flushing encoder failed\n");
        return;
    }
    
    //第十一步：写文件尾（对于某些没有文件头的封装格式，不需要此函数。比如说MPEG2TS）
    av_write_trailer(avformat_context);
    
    
    //第十二步：释放内存，关闭编码器
    avcodec_close(avcodec_context);
    av_free(av_frame);
    av_free(out_buffer);
    av_packet_free(&av_packet);
    avio_close(avformat_context->pb);
    avformat_free_context(avformat_context);
    fclose(in_file);
}


@end
