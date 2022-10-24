extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include "libavutil/imgutils.h"

#include "dsimple.h"
};

#include <string>
#include <pthread.h>
#include <sys/stat.h>

/// @brief 打印log日志
/// @param fmt 
/// @param  
void print(const char * fmt,...);

/// @brief 录制窗口
class GrabWindow
{
public:
    /// @brief 构造函数
    GrabWindow();

    /// @brief 析构函数
    virtual ~GrabWindow();

    /// @brief 工作线程
    /// @param param 线程函数
    static void* Thread_Proc_Linux(void* param);
    
    /// @brief 设置需要抓取的渲染窗口名
    /// @param name 窗口名
    void setRenderWindowName(std::string name);

    /// @brief ON/OFF开关
    /// @param on true打开/false关闭
    void setFunctionEnableStat(bool on);

    /// @brief 设置录制完成的视频名称
    /// @param name 路径+文件名（默认为当前执行程序路径）
    void setVideoFileName(std::string name);

    /// @brief 当前场景执行结束停止录制
    /// @param stat true结束当前场景录制
    void setSceneStopStat(bool stat);

    /// @brief 启动窗口录制
    /// @param stat true开始录制
    void setSimuRunStat(bool stat);

    /// @brief 设置抓取时与生成视频文件的帧率。
    /// @param rate 帧率 
    void setFrameRate(int rate);

    /// @brief 线程结束状态
    /// @return true:结束 false:运行中
    bool GetThreadStat();

    /// @brief 线程函数
    /// @return 
    virtual int Routine();

    bool thread_exit;
private:
    /// @brief 使用xwininfo获取window_id
    void xwininfo();
    /// @brief 使用window_id抓取窗口视频流
    void getinput();
    /**
     * @brief 转码过程：软件解码
     * 
     * @param dec_ctx 解码器 codec_id应为AV_CODEC_ID_RAWVIDEO 将 rawvideo 转为 yuv420p
     * @param frame 输出解码后的视频帧
     * @param pkt 输入视频流
     * @param videoindex 数据类型 当前只处理了视频数据 AVMediaType=AVMEDIA_TYPE_VIDEO
     */
    void sw_decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,int videoindex);

    /**
     * @brief 转码过程：硬件编码
     * 
     * @param enc_ctx 编码器 AV_CODEC_ID_H264 将 yuv420p 编码为 h264
     * @param frame 原始视频帧
     * @param pkt 编码后的视频流
     * @param frameindex 帧序号
     */
    void hw_encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,long &frameindex);
private:
    /// @brief 录制的窗口名
    std::string window_name;
    /// @brief 窗口id
    char window_id[20];
    /// @brief 窗口大小等信息
    xcb_get_geometry_reply_t *geometry;
    /// @brief ON/OFF开关
    bool grab_status;
    /// @brief 输出视频文件名
    std::string file_name;
    /// @brief 输入视频流
    AVFormatContext	*pFormatCtx;
    /// @brief 输出视频流
    AVFormatContext *out_ctx;
    /// @brief 编码前视频格式标准化
    struct SwsContext *img_convert_ctx;
    /// @brief 中间视频帧
    AVFrame *pFrameYUV;
    /// @brief 控制输入流转码结束
    bool for_stop;
    /// @brief 帧率
    int i_framerate; 
    /// @brief 线程
    pthread_t hThread;
};

bool ExistFile(std::string name);