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

+ (void)ffmpegVideoEncode:(NSString*)inFilePath outFilePath:(NSString*)outFilePath zhen:(void(^)(NSString *zhen))zhen {
    // 第一步：注册组件->编码器、解码器等等…
    av_register_all();
    
    // 第二步：初始化封装格式上下文->视频编码->处理为视频压缩数据格式
    AVFormatContext *avformat_context = avformat_alloc_context();
    // 注意事项：FFmepg程序推测输出文件类型->视频压缩数据格式类型
    const char *coutFilePath = [outFilePath UTF8String];
    // 得到视频压缩数据格式类型(h264、h265、mpeg2等等...)
    AVOutputFormat *avoutput_format = av_guess_format(NULL, coutFilePath, NULL);
    // 指定类型
    avformat_context->oformat = avoutput_format;
    
    // 第三步：打开输出文件
    // 参数一：输出流
    // 参数二：输出文件
    // 参数三：权限->输出到文件中
    if (avio_open(&avformat_context->pb, coutFilePath, AVIO_FLAG_WRITE) < 0) {
        NSLog(@"打开输出文件失败");
        return;
    }
    
    // 第四步：创建输出码流->创建了一块内存空间->并不知道他是什么类型流->希望他是视频流
    AVStream *av_video_stream = avformat_new_stream(avformat_context, NULL);
    
    // 第五步：查找视频编码器
    // 1、获取编码器上下文
    AVCodecContext *avcodec_context = av_video_stream->codec;
    
    // 2、设置编解码器上下文参数->必需设置->不可少
    // 目标：设置为是一个视频编码器上下文->指定的是视频编码器
    // 上下文种类：视频解码器、视频编码器、音频解码器、音频编码器
    // 2.1 设置视频编码器ID
    avcodec_context->codec_id = avoutput_format->video_codec;
    // 2.2 设置编码器类型->视频编码器
    // 视频编码器->AVMEDIA_TYPE_VIDEO
    // 音频编码器->AVMEDIA_TYPE_AUDIO
    avcodec_context->codec_type = AVMEDIA_TYPE_VIDEO;
    // 2.3 设置读取像素数据格式->编码的是像素数据格式->视频像素数据格式->YUV420P(YUV422P、YUV444P等等...)
    // 注意：这个类型是根据你解码的时候指定的解码的视频像素数据格式类型
    avcodec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    // 2.4 设置视频宽高->视频尺寸
    avcodec_context->width = 640;
    avcodec_context->height = 352;
    // 2.5 设置帧率->表示每秒25帧
    // 视频信息->帧率 : 25.000 fps
    // f表示：帧数
    // ps表示：时间(单位：每秒)
    avcodec_context->time_base.num = 1;
    avcodec_context->time_base.den = 25;
    // 2.6 设置码率
    // 2.6.1 什么是码率？
    // 含义：每秒传送的比特(bit)数单位为 bps(Bit Per Second)，比特率越高，传送数据速度越快。
    // 单位：bps，"b"表示数据量，"ps"表示每秒
    // 目的：视频处理->视频码率
    // 2.6.2 什么是视频码率?
    // 含义：视频码率就是数据传输时单位时间传送的数据位数，一般我们用的单位是kbps即千位每秒
    // 视频码率计算如下？
    // 基本的算法是：【码率】(kbps)=【视频大小 - 音频大小】(bit位) /【时间】(秒)
    // 例如：Test.mov时间 = 24，文件大小(视频+音频) = 1.73MB
    // 视频大小 = 1.34MB（文件占比：77%） = (1.34MB * 1024 * 1024 * 8)/24(时间) = 字节大小 = 468365字节 = 468Kbps
    // 音频大小 = 376KB（文件占比：21%）
    // 计算出来值->码率 : 468Kbps->k表示1000，b表示位(bit->位)
    // 总结：码率越大，视频越大
    avcodec_context->bit_rate = 468000;
    
    // 2.7 设置GOP->影响到视频质量问题->画面组->一组连续画面
    // MPEG格式画面类型：3种类型->分为->I帧、P帧、B帧
    // I帧->内部编码帧->原始帧(原始视频数据)
    //    完整画面->关键帧(必需的有，如果没有I，那么你无法进行编码，解码)
    //    视频第1帧->视频序列中的第一个帧始终都是I帧，因为它是关键帧
    // P帧->向前预测帧->预测前面的一帧类型，处理数据(前面->I帧、B帧)
    //    P帧数据->根据前面的一帧数据->进行处理->得到了P帧
    // B帧->前后预测帧(双向预测帧)->前面一帧和后面一帧
    //    B帧压缩率高，但是对解码性能要求较高。
    // 总结：I只需要考虑自己 = 1帧，P帧考虑自己+前面一帧 = 2帧，B帧考虑自己+前后帧 = 3帧
    //    说白了->P帧和B帧是对I帧压缩
    // 每250帧，插入1个I帧，I帧越少，视频越小->默认值->视频不一样
    avcodec_context->gop_size = 250;
    
    // 2.8 设置量化参数->数学算法(高级算法)->不讲解了
    // 总结：量化系数越小，视频越是清晰
    // 一般情况下都是默认值，最小量化系数默认值是10，最大量化系数默认值是51
    avcodec_context->qmin = 10;
    avcodec_context->qmax = 51;
    
    // 2.9 设置b帧最大值->设置不需要B帧
    avcodec_context->max_b_frames = 0;
    
    // 第二点：查找编码器->h264
    // 找不到编码器->h264
    // 重要原因是因为：编译库没有依赖x264库（默认情况下FFmpeg没有编译进行h264库）
    // 第一步：编译h264库
    AVCodec *avcodec = avcodec_find_encoder(avcodec_context->codec_id);
    if (avcodec == NULL) {
        NSLog(@"找不到编码器");
        return;
    }
    
    NSLog(@"编码器名称为：%s", avcodec->name);
    
    // 第六步：打开h264编码器
    // 缺少优化步骤？
    // 编码延时问题
    // 编码选项->编码设置
    AVDictionary *param = 0;
    if (avcodec_context->codec_id == AV_CODEC_ID_H264) {
        // 需要查看x264源码->x264.c文件
        // 第一个值：预备参数
        // key: preset
        // value: slow->慢
        //v alue: superfast->超快
        av_dict_set(&param, "preset", "slow", 0);
        // 第二个值：调优
        // key: tune->调优
        // value: zerolatency->零延迟
        av_dict_set(&param, "tune", "zerolatency", 0);
    }
    if (avcodec_open2(avcodec_context, avcodec, &param) < 0) {
        NSLog(@"打开编码器失败");
        return;
    }
    
    // 第七步：写入文件头信息
    avformat_write_header(avformat_context, NULL);
    
    // 第8步：循环编码yuv文件->视频像素数据(yuv格式)->编码->视频压缩数据(h264格式)
    // 8.1 定义一个缓冲区
    // 作用：缓存一帧视频像素数据
    // 8.1.1 获取缓冲区大小
    int buffer_size = av_image_get_buffer_size(avcodec_context->pix_fmt,
                                               avcodec_context->width,
                                               avcodec_context->height,
                                               1);
    
    // 8.1.2 创建一个缓冲区
    int y_size = avcodec_context->width * avcodec_context->height;
    uint8_t *out_buffer = (uint8_t *) av_malloc(buffer_size);
    
    // 8.1.3 打开输入文件
    const char *cinFilePath = [inFilePath UTF8String];
    FILE *in_file = fopen(cinFilePath, "rb");
    if (in_file == NULL) {
        NSLog(@"文件不存在");
        return;
    }
    
    // 8.2.1 开辟一块内存空间->av_frame_alloc
    // 开辟了一块内存空间
    AVFrame *av_frame = av_frame_alloc();
    // 8.2.2 设置缓冲区和AVFrame类型保持一直->填充数据
    av_image_fill_arrays(av_frame->data,
                         av_frame->linesize,
                         out_buffer,
                         avcodec_context->pix_fmt,
                         avcodec_context->width,
                         avcodec_context->height,
                         1);
    
    int i = 0;
    
    // 9.2 接收一帧视频像素数据->编码为->视频压缩数据格式
    AVPacket *av_packet = (AVPacket *) av_malloc(buffer_size);
    int result = 0;
    int current_frame_index = 1;
    while (true) {
        // 8.1 从yuv文件里面读取缓冲区
        // 读取大小：y_size * 3 / 2
        if (fread(out_buffer, 1, y_size * 3 / 2, in_file) <= 0) {
            NSLog(@"读取完毕...");
            break;
        } else if (feof(in_file)) {
            break;
        }
        
        // 8.2 将缓冲区数据->转成AVFrame类型
        // 给AVFrame填充数据
        // 8.2.3 void * restrict->->转成->AVFrame->ffmpeg数据类型
        // Y值
        av_frame->data[0] = out_buffer;
        // U值
        av_frame->data[1] = out_buffer + y_size;
        // V值
        av_frame->data[2] = out_buffer + y_size * 5 / 4;
        av_frame->pts = i;
        // 注意时间戳
        i++;
        // 总结：这样一来我们的AVFrame就有数据了
        
        // 第9步：视频编码处理
        // 9.1 发送一帧视频像素数据
        avcodec_send_frame(avcodec_context, av_frame);
        // 9.2 接收一帧视频像素数据->编码为->视频压缩数据格式
        result = avcodec_receive_packet(avcodec_context, av_packet);
        // 9.3 判定是否编码成功
        if (result == 0) {
            // 编码成功
            // 第10步：将视频压缩数据->写入到输出文件中->outFilePath
            av_packet->stream_index = av_video_stream->index;
            result = av_write_frame(avformat_context, av_packet);
            NSLog(@"当前是第%d帧", current_frame_index);
            current_frame_index++;
            // 是否输出成功
            if (result < 0) {
                NSLog(@"输出一帧数据失败");
                return;
            }
            
            zhen([NSString stringWithFormat:@"当前是第%d帧", current_frame_index]);
        }
    }
    
    // 第11步：写入剩余帧数据->可能没有
    flush_encoder(avformat_context, 0);
    
    // 第12步：写入文件尾部信息
    av_write_trailer(avformat_context);
    
    // 第13步：释放内存
    avcodec_close(avcodec_context);
    av_free(av_frame);
    av_free(out_buffer);
    av_packet_free(&av_packet);
    avio_close(avformat_context->pb);
    avformat_free_context(avformat_context);
    fclose(in_file);
}

@end
