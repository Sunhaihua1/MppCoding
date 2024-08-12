//
// Created by LX on 2020/4/25.
//

#include "MppDecode.h"
#include "MppEncode.h"
int frame_null = 0;
int count = 0;
void dump_mpp_frame_to_file(MppFrame frame, FILE *fp)
{
    RK_U32 width    = 0;
    RK_U32 height   = 0;
    RK_U32 h_stride = 0;
    RK_U32 v_stride = 0;

    MppBuffer buffer    = NULL;
    RK_U8 *base = NULL;

    width    = mpp_frame_get_width(frame);
    height   = mpp_frame_get_height(frame);
    h_stride = mpp_frame_get_hor_stride(frame);
    v_stride = mpp_frame_get_ver_stride(frame);
    buffer   = mpp_frame_get_buffer(frame);

    base = (RK_U8 *)mpp_buffer_get_ptr(buffer);
    RK_U32 buf_size = mpp_frame_get_buf_size(frame);
    size_t base_length = mpp_buffer_get_size(buffer);
    mpp_log("base_length = %d\n",base_length);

    RK_U32 i;
    RK_U8 *base_y = base;
    RK_U8 *base_c = base + h_stride * v_stride;

    //保存为YUV420sp格式
    for (i = 0; i < height; i++, base_y += h_stride)
    {
       fwrite(base_y, 1, width, fp);
    }
    for (i = 0; i < height / 2; i++, base_c += h_stride)
    {
       fwrite(base_c, 1, width, fp);
    }

    //保存为YUV420p格式
    // for(i = 0; i < height; i++, base_y += h_stride)
    // {
    //     fwrite(base_y, 1, width, fp);
    // }
    // for(i = 0; i < height * width / 2; i+=2)
    // {
    //     fwrite((base_c + i), 1, 1, fp);
    // }
    // for(i = 1; i < height * width / 2; i+=2)
    // {
    //     fwrite((base_c + i), 1, 1, fp);
    // }
}

size_t mpp_buffer_group_usage(MppBufferGroup group)
{
    if (NULL == group)
    {
        mpp_err_f("input invalid group %p\n", group);
        return MPP_BUFFER_MODE_BUTT;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    return p->usage;
}

int decode_simple(MpiDecLoopData *data) {
    RK_U32 pkt_done = 0;
    RK_U32 pkt_eos  = 0;
    RK_U32 err_info = 0;
     
    MPP_RET ret = MPP_OK;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    char   *buf = data->buf;
     
    MppPacket packet = data->packet;
    MppFrame  frame  = NULL;
    size_t read_size = fread(buf, 1, data->packet_size, data->fp_input);
     
    if (read_size != data->packet_size || feof(data->fp_input))
    {
        mpp_log("found last packet\n");
        data->eos = pkt_eos = 1;
    }
     
    mpp_packet_write(packet, 0, buf, read_size);
    mpp_packet_set_pos(packet, buf);
    mpp_packet_set_length(packet, read_size);
 
    if (pkt_eos)
    {
        mpp_packet_set_eos(packet);
    }
 
    do {
        if (!pkt_done)
        {
            ret = mpi->decode_put_packet(ctx, packet);
            if (MPP_OK == ret)
            {
                pkt_done = 1;
            }
        }
         
        do {
            RK_S32 get_frm = 0;
            RK_U32 frm_eos = 0;
 
            ret = mpi->decode_get_frame(ctx, &frame);
     
            if (frame)
            {
                if (mpp_frame_get_info_change(frame))
                {
                    RK_U32 width = mpp_frame_get_width(frame);
                    RK_U32 height = mpp_frame_get_height(frame);
                    RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
                    RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
                    RK_U32 buf_size = mpp_frame_get_buf_size(frame);                               
 
                    mpp_log("decode_get_frame get info changed found\n");
                    mpp_log("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d", width, height, hor_stride, ver_stride, buf_size);       
 
                    if (NULL == data->frm_grp)
                    {                      
                        ret = mpp_buffer_group_get_internal(&data->frm_grp, MPP_BUFFER_TYPE_ION);
                        if (ret)
                        {
                            mpp_err("get mpp buffer group failed ret %d\n", ret);
                            break;
                        }                          
                        ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, data->frm_grp);
                        if (ret)
                        {
                            mpp_err("set buffer group failed ret %d\n", ret);
                            break;
                        }
                    }
                    else
                    {                     
                        ret = mpp_buffer_group_clear(data->frm_grp);
                        if (ret)
                        {
                            mpp_err("clear buffer group failed ret %d\n", ret);
                            break;
                        }
                    }
                     
                    ret = mpp_buffer_group_limit_config(data->frm_grp, buf_size, 24);
                    if (ret)
                    {
                        mpp_err("limit buffer group failed ret %d\n", ret);
                        break;
                    }                  
                    ret = mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                    if (ret)
                    {
                        mpp_err("info change ready failed ret %d\n", ret);
                        break;
                    }
                }
                else
                {
                    err_info = mpp_frame_get_errinfo(frame) | mpp_frame_get_discard(frame);
                    if (err_info)
                    {
                        mpp_log("decoder_get_frame get err info:%d discard:%d.\n", mpp_frame_get_errinfo(frame), mpp_frame_get_discard(frame));
                    }
                    data->frame_count++;
                    mpp_log("decode_get_frame get frame %d\n", data->frame_count);
                    if (data->fp_output && !err_info)
                    {
                        cv::Mat rgbImg;
                        int width = mpp_frame_get_width(frame);
                        YUV2Mat(frame, rgbImg);
                        cv::imwrite("./" + std::to_string(data->frame_count) + ".jpg", rgbImg);
                        dump_mpp_frame_to_file(frame, data->fp_output);
                    }
                }
                frm_eos = mpp_frame_get_eos(frame);
                mpp_frame_deinit(&frame);
                frame = NULL;
                get_frm = 1;
            }
            
            if (data->frm_grp)
            {
                size_t usage = mpp_buffer_group_usage(data->frm_grp);
                if (usage > data->max_usage)
                {
                    data->max_usage = usage;
                }  
            }
          
            if (pkt_eos && pkt_done && !frm_eos)
            {
                msleep(10);
                continue;
            }
            if (frm_eos)
            {
                mpp_log("found last frame\n");
                break;
            }
            if (data->frame_num && data->frame_count >= data->frame_num)
            {
                data->eos = 1;
                break;
            }
            if (get_frm)
            {
                continue;
            }
            break;
        } while (1);
 
        if (data->frame_num && data->frame_count >= data->frame_num)
        {
            data->eos = 1;
            mpp_log("reach max frame number %d\n", data->frame_count);
            break;
        }
        if (pkt_done)
        {
            break;
        }
    } while (1);
 
    return ret;
}

void YUV2Mat(MppFrame  frame, cv::Mat& rgbImg ) {
	RK_U32 width = 0;
	RK_U32 height = 0;
	RK_U32 h_stride = 0;
	RK_U32 v_stride = 0;
	MppBuffer buffer = NULL;
	RK_U8 *base = NULL;

	width = mpp_frame_get_width(frame);
	height = mpp_frame_get_height(frame);
	h_stride = mpp_frame_get_hor_stride(frame);
	v_stride = mpp_frame_get_ver_stride(frame);
	buffer = mpp_frame_get_buffer(frame);

	base = (RK_U8 *)mpp_buffer_get_ptr(buffer);
	RK_U32 buf_size = mpp_frame_get_buf_size(frame);
	size_t base_length = mpp_buffer_get_size(buffer);
	// mpp_log("base_length = %d\n",base_length);
    std::cout << width << std::endl;
    std::cout << height << std::endl;
	RK_U32 i;
	RK_U8 *base_y = base;
	RK_U8 *base_c = base + h_stride * v_stride;

	cv::Mat yuvImg;
	yuvImg.create(height * 3 / 2, width, CV_8UC1);

	//转为YUV420sp格式
	int idx = 0;
	for (i = 0; i < height; i++, base_y += h_stride) {
		//        fwrite(base_y, 1, width, fp);
		memcpy(yuvImg.data + idx, base_y, width);
		idx += width;
	}
	for (i = 0; i < height / 2; i++, base_c += h_stride) {
		//        fwrite(base_c, 1, width, fp);
		memcpy(yuvImg.data + idx, base_c, width);
		idx += width;
	}
//这里的转码需要转为RGB 3通道， RGBA四通道则不能检测成功
    cv::cvtColor(yuvImg, rgbImg, cv::COLOR_YUV420sp2RGB);
    cv::Mat encode_img;
    cv::cvtColor(rgbImg, encode_img, cv::COLOR_RGB2YUV_YV12);
    int length = 0; 
    static unsigned char *encode_buf = NULL;
    YuvtoH264(width, height, encode_img, encode_buf, length);
	
}
