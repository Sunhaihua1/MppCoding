#pragma once

#include <string.h>
#include <sys/time.h>

#include "utils.h"
#include "rk_mpi.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#define MAX_FILE_NAME_LENGTH 256
#define calTimeCost(begin,end)(end.tv_sec*1000.-begin.tv_sec*1000.+end.tv_usec/1000.-begin.tv_usec/1000.)

typedef struct 
{
    MppCodingType   type;
    RK_U32          width;
    RK_U32          height;
    MppFrameFormat  format;

    RK_U32          num_frames;
} MpiEncTestCmd;

typedef struct 
{
    //global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_U32 frame_count;
    RK_U64 stream_size;

    //input ang output file
    FILE *fp_input;
    FILE *fp_output;

    //input and output
    MppBuffer frm_buf;
    MppEncSeiMode sei_mode;

    //base flow context
    MppCtx ctx;
    MppApi *mpi;
    MppEncPrepCfg prep_cfg;
    MppEncRcCfg rc_cfg;
    MppEncCodecCfg codec_cfg;   

    //paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride; //horizontal stride  
    RK_U32 ver_stride; //vertical stride  
    MppFrameFormat fmt;
    MppCodingType type;
    RK_U32 num_frames;

    //resources
    size_t frame_size;
    //NOTE: packet buffer may overflow
    size_t packet_size;

    //rate control runtime parameter
    RK_S32 gop;
    RK_S32 fps;
    RK_S32 bps;
} MpiEncTestData;

MPP_RET encode_ctx_init(MpiEncTestData **data, MpiEncTestCmd *cmd);
MPP_RET encode_mpp_setup(MpiEncTestData *p2);
MPP_RET read_yuv_buffer(RK_U8 *buf, cv::Mat &yuvImg, RK_U32 width, RK_U32 height);
MPP_RET encode_mpp_run_yuv(cv::Mat yuvImg, MppApi *mpi1, MppCtx &ctx1, unsigned char * &H264_buf, int &length);
MpiEncTestData *encode_mpp_run_yuv_init(MpiEncTestData *p, MpiEncTestCmd *cmd, int width , int height, unsigned char * &SPS_buf, int &SPS_length);
int YuvtoH264(int width, int height, cv::Mat yuv_frame, unsigned char* (&encode_buf), int &encode_length);


