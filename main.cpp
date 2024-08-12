#include <stdio.h>
#include <stdlib.h>
#include "MppDecode.h"
#include "MppEncode.h"
FILE *fp_encode_out;
FILE *fp_decode_in;

void deInit(MppPacket *packet, MppFrame *frame, MppCtx ctx, char *buf, MpiDecLoopData data )
{
    if (packet) {
        mpp_packet_deinit(packet);
        packet = NULL;
    }

    if (frame) {
        mpp_frame_deinit(frame);
        frame = NULL;
    }

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }


    if (buf) {
        mpp_free(buf);
        buf = NULL;
    }


    if (data.pkt_grp) {
        mpp_buffer_group_put(data.pkt_grp);
        data.pkt_grp = NULL;
    }

    if (data.frm_grp) {
        mpp_buffer_group_put(data.frm_grp);
        data.frm_grp = NULL;
    }

    if (data.fp_output) {
        fclose(data.fp_output);
        data.fp_output = NULL;
    }

    if (data.fp_input) {
        fclose(data.fp_input);
        data.fp_input = NULL;
    }
}

int main()
{
    fp_encode_out = fopen("result.h264", "w+b");
    fp_decode_in = fopen("/oem/tennis.h264", "rb");
    //// 初始化
    MPP_RET ret         = MPP_OK;
    size_t file_size    = 0;

    // base flow context
    MppCtx ctx          = NULL;
    MppApi *mpi         = NULL;

    // input / output
    MppPacket packet    = NULL;
    MppFrame  frame     = NULL;

    MpiCmd mpi_cmd      = MPP_CMD_BASE;
    MppParam param      = NULL;
    RK_U32 need_split   = 1;
//    MppPollType timeout = 5;

    // paramter for resource malloc
    RK_U32 width        = 2560;
    RK_U32 height       = 1440;
    MppCodingType type  = MPP_VIDEO_CodingAVC;

    // resources
    char *buf           = NULL;
    size_t packet_size  = MPI_DEC_STREAM_SIZE;
    MppBuffer pkt_buf   = NULL;
    MppBuffer frm_buf   = NULL;

    MpiDecLoopData data;

    mpp_log("mpi_dec_test start\n");
    memset(&data, 0, sizeof(data));

    // init input
    data.fp_input = fp_decode_in;
    if (NULL == data.fp_input)
    {
        mpp_err("failed to open input file");
        deInit(&packet, &frame, ctx, buf, data);

    }
    fseek(data.fp_input, 0L, SEEK_END);
    file_size = ftell(data.fp_input);
    rewind(data.fp_input);
    mpp_log("input file size %ld\n", file_size);

    mpp_log("mpi_dec_test decoder test start w %d h %d type %d\n", width, height, type);

    //init output
    data.fp_output = fopen("./ten.yuv", "w+b");
    if (NULL == data.fp_output) {
        mpp_err("failed to open output file %s\n", "tenoutput.yuv");
        deInit(&packet, &frame, ctx, buf, data);
    } 

    // init packet
    buf = mpp_malloc(char, packet_size);
    
    ret = mpp_packet_init(&packet, buf, packet_size);
    // decoder demo
    ret = mpp_create(&ctx, &mpi);

    if (MPP_OK != ret) {
        mpp_err("mpp_create failed\n");
        deInit(&packet, &frame, ctx, buf, data);
    }

    // NOTE: decoder split mode need to be set before init
    mpi_cmd = MPP_DEC_SET_PARSER_SPLIT_MODE;
    param = &need_split;
    ret = mpi->control(ctx, mpi_cmd, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
        deInit(&packet, &frame, ctx, buf, data);
    }

    mpi_cmd = MPP_SET_INPUT_BLOCK;
    param = &need_split;
    ret = mpi->control(ctx, mpi_cmd, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
        deInit(&packet, &frame, ctx, buf, data);
    }

    ret = mpp_init(ctx, MPP_CTX_DEC, type);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        deInit(&packet, &frame, ctx, buf, data);
    }

    data.ctx            = ctx;
    data.mpi            = mpi;
    data.eos            = 0;
    data.buf            = buf;
    data.packet         = packet;
    data.packet_size    = packet_size;
    data.frame          = frame;
    data.frame_count    = 0;
    data.frame_num      = 10;
    mpp_log("data.packet_size = %d\n", data.packet_size);
    
    while (!data.eos) {
        decode_simple(&data);
        mpp_log("data.eos = %d\n", data.eos);

    }
    mpi->reset(ctx);
    deInit(&packet, &frame, ctx, buf, data);
    return 0;
}
