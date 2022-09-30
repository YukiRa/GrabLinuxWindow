#include "grabwindow.h"
#include <iostream>

void print(const char * fmt,...)
{
#ifdef _debug_print
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
}

bool ExistFile(std::string name)
{
    struct stat buffer;
    return (stat(name.c_str(),&buffer) == 0);
}

void* GrabWindow::Thread_Proc_Linux(void* param)
{
	GrabWindow* thrd = (GrabWindow*) param;
	thrd->Routine();
    thrd->thread_exit = true;
    print("Linux thread out\n");
	pthread_exit((void *)1);
}

GrabWindow::GrabWindow()
{
    avformat_network_init();
    avdevice_register_all();
    for_stop = false;
    thread_exit = true;
}

GrabWindow::~GrabWindow()
{
    print("~GrabWindow\n");
}

void GrabWindow::setRenderWindowName(std::string name)
{
    window_name = name;
}

void GrabWindow::setFunctionEnableStat(bool on)
{
    grab_status = on;
}

void GrabWindow::setVideoFileName(std::string name)
{
    file_name = name;
}

void GrabWindow::setSceneStopStat(bool stat)
{
    for_stop = stat;
}

void GrabWindow::setSimuRunStat(bool stat)
{
    if(grab_status)
    {
        if(stat)
        {
            while (!thread_exit)
            {
                
            }
            print("thread start\n");

            if(pthread_create(&hThread, NULL, 
            Thread_Proc_Linux, this) < 0)
            {
                return;
            }
            thread_exit = false;
            pthread_detach(hThread);
        }
    } 
}

bool GrabWindow::GetThreadStat()
{
    return thread_exit;
}

void GrabWindow::xwininfo()
{
	char *display_name = NULL;
	static xcb_connection_t *dpy;
	static xcb_screen_t *xcb_screen;
	xcb_window_t xcb_window = 0;
	Setup_Display_And_Screen (display_name, &dpy, &xcb_screen);
	xcb_window = Window_With_Name (dpy, xcb_screen->root, window_name.c_str());	
	snprintf (window_id, sizeof(window_id), "0x%x", xcb_window);
	print("xwininfo %s\n",window_id);
	xcb_get_geometry_cookie_t gg_cookie = xcb_get_geometry (dpy, xcb_window);
	geometry = xcb_get_geometry_reply(dpy, gg_cookie, NULL);
	print("xwininfo %dx%d\n",geometry->width,geometry->height);
}

void GrabWindow::getinput()
{
    pFormatCtx = avformat_alloc_context();
    AVDictionary* options = NULL;
    char video_size[12];
	sprintf(video_size,"%dx%d",geometry->width,geometry->height);
	/// @warning 需要引入xwininfo 导入
	av_dict_set(&options,"video_size",video_size,0);
    /// @warning 必须设置 以此获取窗口
	av_dict_set(&options,"window_id",window_id,0);
	const AVInputFormat *ifmt=av_find_input_format("x11grab");
    if(avformat_open_input(&pFormatCtx,NULL,ifmt,&options)!=0){
		print("Couldn't open input stream.\n");
		return;
	}
    /// @brief 默认大小为5000000 探针大小不足时会有x11grab的提示
    pFormatCtx->probesize = 20000000;
    if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		print("Couldn't find stream information.\n");
		return;
	}
}

void GrabWindow::sw_decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,int videoindex)
{
    if(av_read_frame(pFormatCtx, pkt)>=0)
    {
        if(pkt->stream_index==videoindex)
        {
            int ret = avcodec_send_packet(dec_ctx, pkt);
            if(ret < 0)
            {
                print("Decode Error.\n");
                return;
            }
            while(ret >= 0)
            {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if(ret<0)
                {
                    print("Decode frame END.\n");
                }
                if(ret >= 0)
                {
                    sws_scale(img_convert_ctx, (const unsigned char* const*)frame->data, frame->linesize, 0, dec_ctx->height, pFrameYUV->data, pFrameYUV->linesize);
                }
            }
        }
    }
}

void GrabWindow::hw_encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,int &frameindex)
{
    av_packet_unref(pkt);
    int ret = avcodec_send_frame(enc_ctx,frame);
    if(ret < 0)
    {
        print("Encode Error. %d\n",ret);
        return;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(enc_ctx,pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            print("receiveres %d\n",ret);
            break;
        }
        else if (ret < 0) {
            print("Error during encoding\n");
            return;
        }
        /// @brief 计算帧率 否则编码后的视频无法解析帧率
        pkt->pts=pkt->dts=frameindex*(out_ctx->streams[0]->time_base.den)
                                    /out_ctx->streams[0]->time_base.num/25;
        frameindex++;
        ret = av_interleaved_write_frame(out_ctx,pkt);
        if (ret < 0)
        {
            print("send packet failed: %d\n", ret);
            break;
        }
        else
        {
            print("send %5d packet successfully!\n", frameindex);
        }
    }
}

int GrabWindow::Routine()
{
    // decoder
    AVCodecContext *pCodecCtx;
	const AVCodec *pCodec;
    // encoder
    AVCodecContext *context;
    const AVCodec *encoder;
    
    // 解码前avpacket 解码后avframe
    AVPacket *packet;
    AVFrame *pFrame;
    // 编码前格式化处理avframe 编码后avpacket
    AVPacket *encodepkt;

    int ret;

    print("grab and output\n");
    xwininfo();
    getinput();

    int videoindex = -1;
    for(int i=0; i<pFormatCtx->nb_streams; i++) 
    {
		if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
			break;
		}
    }
	if(videoindex==-1)
	{
		print("Didn't find a video stream.\n");
		return -1;
	}
    /// @brief 获取编码器
    pCodecCtx = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(pCodecCtx,pFormatCtx->streams[videoindex]->codecpar);
	pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);
	
	print("pcodec: %d\n",pCodecCtx->codec_id);
	print("codec_id: %d\n",pFormatCtx->streams[videoindex]->codecpar->codec_id);
	if(pCodec==NULL)
	{
		print("Codec not found.\n");
		return -1;
	}
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
	{
		print("Could not open codec.%d\n",avcodec_open2(pCodecCtx, pCodec,NULL));
		return -1;
	}

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();

    unsigned char *out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height,1));
	av_image_fill_arrays(pFrameYUV->data,pFrameYUV->linesize,out_buffer,AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height,1);
	pFrameYUV->format = AV_PIX_FMT_YUV420P;	//硬件编码格式：AV_PIX_FMT_NV12 x11录制格式：AV_PIX_FMT_BGR0
	pFrameYUV->width = pCodecCtx->width;
	pFrameYUV->height = pCodecCtx->height;

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
    /// @brief 获取解码器
    encoder = avcodec_find_encoder_by_name("h264_nvenc");
    if(!encoder)
	{
		print("can't find nvenc\n");
        return -1;
	}
	context = avcodec_alloc_context3(encoder);
	context->width = geometry->width;
	context->height = geometry->height;
	int64_t bitrate = geometry->width*geometry->height;
	context->bit_rate = bitrate*20*24;		//比特率 控制画质
	context->time_base = (AVRational){1,30};
	context->framerate = (AVRational){30,1};
	context->gop_size = 10;
	context->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(context->priv_data,"reset","slow",0);
    if(avcodec_open2(context,encoder,NULL)<0)
	{
		print("opencodec failed\n");
		return -1;
	}
    encodepkt = av_packet_alloc();

	avformat_alloc_output_context2(&out_ctx,NULL,NULL,file_name.c_str());
	AVStream *out_stm;
	out_stm = avformat_new_stream(out_ctx,NULL);

	avcodec_parameters_from_context(out_stm->codecpar, context);

	avio_open(&out_ctx->pb,file_name.c_str(),AVIO_FLAG_WRITE);

	ret = avformat_write_header(out_ctx,NULL);
	int frameIndex = 0;
    for_stop = false;
    /// @brief 视频流转码处理
    for(;;)
    {
        print("---start---%d\n",for_stop);
        if(!ExistFile(file_name))
        {
            print("lose file.\n");
            break;
        }
        if(for_stop)
        {
            print("end loop.\n");
            break;
        }
        sw_decode(pCodecCtx,pFrame,packet,videoindex);
        //sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
        hw_encode(context,pFrameYUV,encodepkt,frameIndex);
        av_frame_unref(pFrame);
        //av_frame_unref(pFrameYUV);
        av_packet_unref(packet);
        av_packet_unref(encodepkt);
    }
    /// @brief 写输出视频文件尾
    av_write_trailer(out_ctx);
    
    /// @brief 录制结束 释放资源
    sws_freeContext(img_convert_ctx);

    avcodec_free_context(&pCodecCtx);
    avcodec_free_context(&context);
	avformat_close_input(&pFormatCtx);
    
    avio_closep(&out_ctx->pb);
    avformat_free_context(out_ctx);

    av_frame_free(&pFrame);
    av_packet_free(&packet);
    av_frame_free(&pFrameYUV);
    av_packet_free(&encodepkt);

    free(out_buffer);
    out_buffer = nullptr;
    free(geometry);
    geometry = nullptr;
    
    print("---end---\n");
    
    return 0;
}
