/*
 * Copyright (C) 2018-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "camera_client.h"
#include "camera_utils.h"

static struct camera_ops g_hal_ops = {0};
static void *g_hal_handle = NULL;

struct camera_dev {
	int id;
	int fd;
	char name[10];
	struct camera_ops ops;
};

static camera_dev camera_devices = {0};

int get_fusion_id(int camera_id)
{
	return 0; // TODO, this should get from a xml or other script
}

static std::vector<int> get_fusion_list(int fusion_id)
{
	std::vector<int> fusion_list = {0, 1, 2, 3};
	return fusion_list;
}

static const int max_fusion_camera_number = 1;
static stream_t g_sub_streams[max_fusion_camera_number] = {0};
static stream_config_t g_sub_stream_lists[max_fusion_camera_number] = {0};

static int set_sub_stream(int fusion_id, stream_t &s)
{
	std::vector<int> fusion_list = get_fusion_list(fusion_id);
	pr_info("%s Enter, s(format 0x%x, %d x %d, stride: %d field: %d id: %d size: %d)\n",
	       __func__,
	       s.format,
	       s.width,
	       s.height,
	       s.stride,
	       s.field,
	       s.id,
	       s.size);

	g_sub_streams[fusion_id] = s;
	g_sub_streams[fusion_id].height = g_sub_streams[fusion_id].height / fusion_list.size();
	g_sub_streams[fusion_id].size = s.width * (g_sub_streams[fusion_id].height + 1) * 2;

	return 0;
};

static stream_t *get_sub_stream(int fusion_id)
{
	pr_info("%s Enter, format 0x%x, %d x %d, stride: %d id: %d stream size %d memType %d  usage %d streamType %d\n",
	       __func__,
	       g_sub_streams[fusion_id].format,
	       g_sub_streams[fusion_id].width,
	       g_sub_streams[fusion_id].height,
	       g_sub_streams[fusion_id].stride,
	       g_sub_streams[fusion_id].id,
	       g_sub_streams[fusion_id].size,
	       g_sub_streams[fusion_id].memType,
	       g_sub_streams[fusion_id].usage,
	       g_sub_streams[fusion_id].streamType);
	return &g_sub_streams[fusion_id];
};

static int set_sub_stream_list(int fusion_id, stream_config_t *stream_list)
{
	g_sub_stream_lists[fusion_id] = *stream_list;
	g_sub_stream_lists[fusion_id].streams = &g_sub_streams[fusion_id];
	return 0;
};

static stream_config_t *get_sub_stream_list(int fusion_id) { return &g_sub_stream_lists[fusion_id]; };

/**
 * Return the number of cameras
 * This should be called before any other calls
 *
 * @return > 0  return cameras number
 * @return == 0 failed to get cameras number
 **/
int vcamera_get_number_of_cameras()
{
	return 1; // TODO, this should get from a xml or other script
}

static int sub_camera_stream_qbuf(int sub_camera_id, camera_buffer_t **buffer, int num_buffers, void *settings)
{
	pr_info("camera_fusion %s Enter, sub_camera_id %d, address %p foramt %d\n",
	        __func__,
	        sub_camera_id,
	        buffer[0]->addr,
	        buffer[0]->s.format);

	if (camera_devices.ops.stream_qbuf)
		return camera_devices.ops.stream_qbuf(sub_camera_id, buffer, num_buffers, settings);
	else
		return -1;
};

static const int max_fusion_number = 4;
static std::vector<camera_buffer_t> sub_camera_buffers[max_fusion_number];
static std::map<void *, std::vector<camera_buffer_t *>> g_sub_buffer_map;
static int fusion_camera_stream_qbuf(int camera_id, camera_buffer_t **buffer, int num_buffers, void *settings)
{
	int ret = 0;
	int fusion_id = get_fusion_id(camera_id);

	std::vector<int> fusion_list = get_fusion_list(fusion_id);
	camera_buffer_t sub_buffer = *buffer[0];
	std::vector<camera_buffer_t *> *sub_camera_buffer;
	pr_info("camera_fusion %s Enter, fusion_id %d\n", __func__, fusion_id);

	sub_buffer.s = *get_sub_stream(fusion_id);
	auto it = g_sub_buffer_map.find(buffer[0]->addr);
	if (it != g_sub_buffer_map.end()) {
		sub_camera_buffer = &it->second;
		pr_info("camera_fusion %s AA find the address %p\n", __func__, buffer[0]->addr);

		pr_info("camera_fusion %s get sub_camera_buffer[0].addr %p\n", __func__, (*sub_camera_buffer)[0]->addr);
	} else {
		pr_info("camera_fusion %s can't find the address %p\n", __func__, buffer[0]->addr);

		std::vector<camera_buffer_t *> tmp;

		for (auto sub_camera_id : fusion_list) {
			camera_buffer_t sub_camera_buffer_t = sub_buffer;
			sub_camera_buffer_t.s = sub_buffer.s;
			sub_camera_buffer_t.addr = buffer[0]->addr + sub_buffer.s.width * sub_buffer.s.height *
			                                                 2 /*sub_buffer.s.size*/ * sub_camera_id;
			pr_info(
			    "camera_fusion %s push sub_camera_buffer_t.addr %p\n", __func__, sub_camera_buffer_t.addr);

			sub_camera_buffers[sub_camera_id].push_back(sub_camera_buffer_t);
			tmp.push_back(&sub_camera_buffers[sub_camera_id].back());
		}

		pr_info("camera_fusion %s insert the address %p\n", __func__, buffer[0]->addr);
		g_sub_buffer_map.insert(pair(buffer[0]->addr, tmp));
		auto item = g_sub_buffer_map.find(buffer[0]->addr);
		if (item != g_sub_buffer_map.end()) {
			pr_info("camera_fusion %s BB find the address %p\n", __func__, buffer[0]->addr);

			sub_camera_buffer = &item->second;
		}
	}

	for (auto sub_camera_id : fusion_list) {
		camera_buffer_t *buf = (*sub_camera_buffer)[sub_camera_id];

		pr_info("camera_fusion %s sub_camera_id %d buf %p\n", __func__, sub_camera_id, buf);
		pr_info("camera_fusion %s buf->addr = %p buf->s(format 0x%x, %d x %d, stride: %d field: %d id: "
		        "%d)\n",
		        __func__,
		        buf->addr,
		        buf->s.format,
		        buf->s.width,
		        buf->s.height,
		        buf->s.stride,
		        buf->s.field,
		        buf->s.id);

		pr_info("camera_fusion %s buffer[0]->addr = %p s(format 0x%x, %d x %d, stride: %d field: %d "
		        "id: %d)\n",
		        __func__,
		        buffer[0]->addr,
		        buffer[0]->s.format,
		        buffer[0]->s.width,
		        buffer[0]->s.height,
		        buffer[0]->s.stride,
		        buffer[0]->s.field,
		        buffer[0]->s.id);

		pr_info("camera_fusion %s buf->(index %d sequence %ld, dmafd %d, flags: %d timestamp: %ld "
		        "requestId: %d)\n",
		        __func__,
		        buf->index,
		        buf->sequence,
		        buf->dmafd,
		        buf->flags,
		        buf->timestamp,
		        buf->requestId);
		pr_info("camera_fusion %s buffer[0]->(index %d sequence %ld, dmafd %d, flags: %d timestamp: "
		        "%ld requestId: %d)\n",
		        __func__,
		        buffer[0]->index,
		        buffer[0]->sequence,
		        buffer[0]->dmafd,
		        buffer[0]->flags,
		        buffer[0]->timestamp,
		        buffer[0]->requestId);

		pr_info("camera_fusion %s call dqbuf sub_camera_id %d\n", __func__, sub_camera_id);
		ret = sub_camera_stream_qbuf(sub_camera_id, &buf, num_buffers, settings);
		if (ret != 0) {
			pr_info("sub_camera %s failed sub_camera_id %d\n", __func__, sub_camera_id);
			break;
		}
	}

	return ret;
};

static int sub_camera_stream_dqbuf(int sub_camera_id, int stream_id, camera_buffer_t **buffer, void *settings)
{
	pr_info("camera_fusion %s Enter, sub_camera_id %d\n", __func__, sub_camera_id);

	if (camera_devices.ops.stream_dqbuf)
		return camera_devices.ops.stream_dqbuf(sub_camera_id, stream_id, buffer, settings);
	else
		return -1;
}

static int g_count = 0;
static int fusion_camera_stream_dqbuf(int camera_id, int stream_id, camera_buffer_t **buffer, void *settings)
{
	int ret = 0;
	int fusion_id = get_fusion_id(camera_id);

	std::vector<int> fusion_list = get_fusion_list(fusion_id);
	camera_buffer_t sub_buffer = *buffer[0];
	std::vector<camera_buffer_t *> sub_camera_buffer;
	sub_buffer.s = *get_sub_stream(fusion_id);
	pr_info("camera_fusion %s enter, fusion_id %d, buffer[0]->addr %p\n", __func__, fusion_id, buffer[0]->addr);

#ifdef DUMP_IMAGE
	char camera_name[128];
	sprintf(camera_name, "fusion_camera_%d.yuv", g_count++);
	FILE *fp = fopen(camera_name, "w");
#endif

	auto it = g_sub_buffer_map.find(buffer[0]->addr);
	if (it != g_sub_buffer_map.end()) {
		sub_camera_buffer = it->second;
	} else {
		pr_info("camera_fusion %s fusion_id %d, the add is invalid %p\n", __func__, fusion_id, buffer[0]->addr);
		return -1;
	}

	camera_buffer_t *buf = nullptr;
	for (auto sub_camera_id : fusion_list) {
		buf = sub_camera_buffer[sub_camera_id];
		pr_info("camera_fusion %s buf %p\n", __func__, buf);
		pr_info("camera_fusion %s buf->addr = %p buf->s(format 0x%x, %d x %d, stride: %d field: %d id: %d)\n",
		        __func__,
		        buf->addr,
		        buf->s.format,
		        buf->s.width,
		        buf->s.height,
		        buf->s.stride,
		        buf->s.field,
		        buf->s.id);

		buf->sequence = -1;
		buf->timestamp = 0;

		pr_info("camera_fusion %s buffer[0]->addr = %p s(format 0x%x, %d x %d, stride: %d field: %d id: %d)\n",
		        __func__,
		        buffer[0]->addr,
		        buffer[0]->s.format,
		        buffer[0]->s.width,
		        buffer[0]->s.height,
		        buffer[0]->s.stride,
		        buffer[0]->s.field,
		        buffer[0]->s.id);

		pr_info("camera_fusion %s call dqbuf sub_camera_id %d\n", __func__, sub_camera_id);
		ret = sub_camera_stream_dqbuf(sub_camera_id, stream_id, &buf, settings);
		if (ret != 0) {
			pr_info("camera_fusion %s failed sub_camera_id %d\n", __func__, sub_camera_id);
			break;
		} else {

#ifdef DUMP_IMAGE
			fwrite(buf->addr, 1, buf->s.width * buf->s.height * 2, fp);
#endif
		}

		pr_info("camera_fusion %s success sub_camera_id %d, buf->addr %p index = %d\n",
		        __func__,
		        sub_camera_id,
		        buf->addr,
		        buf->index);

		/* The addr of camera id 0 is the same with the super buffer addr */
		if (sub_camera_id == 0) {
			buffer[0]->addr = buf->addr;

			buffer[0]->index = buf->index;
			buffer[0]->timestamp = buf->timestamp;
			buffer[0]->sequence = buf->sequence;
		}

		pr_info("camera_fusion %s success sub_camera_id %d, buffer->addr %p index = %d\n",
		        __func__,
		        sub_camera_id,
		        buffer[0]->addr,
		        buffer[0]->index);
	}

#ifdef DUMP_IMAGE
	fclose(fp);
#endif
	return ret;
};

static int sub_camera_hal_init(int sub_camera_id)
{
	pr_info("camera_fusion %s Enter, sub_camera_id %d\n", __func__, sub_camera_id);

	if (camera_devices.ops.hal_init)
		return camera_devices.ops.hal_init();

	return -1;
};

static int fusion_camera_hal_init()
{
	int ret = 0;

	pr_info("camera_fusion %s Enter\n", __func__);

	for (int i = 0; i < max_fusion_number; i++)
		sub_camera_buffers[i].reserve(1);

	ret = sub_camera_hal_init(0);
	if (ret != 0) {
		pr_info("camera_fusion %s failed\n", __func__);
	}

	return ret;
};

static int sub_camera_open(int sub_camera_id)
{
	pr_info("camera_fusion %s Enter, sub_camera_id %d\n", __func__, sub_camera_id);

	if (camera_devices.ops.open)
		return camera_devices.ops.open(sub_camera_id);

	return -1;
};

static int fusion_camera_open(int camera_id)
{
	int ret = 0;
	int fusion_id = get_fusion_id(camera_id);

	std::vector<int> fusion_list = get_fusion_list(fusion_id);

	pr_info("camera_fusion %s Enter, fusion_id %d\n", __func__, fusion_id);
	for (auto sub_camera_id : fusion_list) {
		ret = sub_camera_open(sub_camera_id);
		if (ret != 0) {
			pr_info("sub_camera %s failed sub_camera_id %d\n", __func__, sub_camera_id);
			break;
		}
	}

	return ret;
};

static int sub_camera_close(int sub_camera_id)
{
	pr_info("camera_fusion %s Enter, sub_camera_id %d\n", __func__, sub_camera_id);

	if (camera_devices.ops.close)
		camera_devices.ops.close(sub_camera_id);

	return 0;
};

static int fusion_camera_close(int camera_id)
{
	int ret = 0;
	int fusion_id = get_fusion_id(camera_id);

	std::vector<int> fusion_list = get_fusion_list(fusion_id);

	pr_info("camera_fusion %s Enter, fusion_id %d\n", __func__, fusion_id);
	for (auto sub_camera_id : fusion_list) {
		ret = sub_camera_close(sub_camera_id);
		if (ret != 0) {
			pr_info("sub_camera %s failed sub_camera_id %d\n", __func__, sub_camera_id);
			break;
		}
	}

	return ret;
};

static int sub_camera_start_stream(int sub_camera_id)
{
	pr_info("camera_fusion %s Enter, sub_camera_id %d\n", __func__, sub_camera_id);

	if (camera_devices.ops.start_stream)
		return camera_devices.ops.start_stream(sub_camera_id);

	return -1;
};

static int fusion_camera_start_stream(int camera_id)
{
	int ret = 0;
	int fusion_id = get_fusion_id(camera_id);

	std::vector<int> fusion_list = get_fusion_list(fusion_id);

	pr_info(
	    "camera_fusion %s Enter, fusion_id %d, fusion_list size is %ld\n", __func__, fusion_id, fusion_list.size());
	for (auto sub_camera_id : fusion_list) {
		ret = sub_camera_start_stream(sub_camera_id);
		if (ret != 0) {
			pr_info("sub_camera %s failed sub_camera_id %d\n", __func__, sub_camera_id);
			break;
		}
	}

	return ret;
};

static int sub_camera_stop_stream(int sub_camera_id)
{
	pr_info("camera_fusion %s Enter, sub_camera_id %d\n", __func__, sub_camera_id);

	if (camera_devices.ops.stop_stream)
		return camera_devices.ops.stop_stream(sub_camera_id);

	return -1;
};

static int fusion_camera_stop_stream(int camera_id)
{
	int ret = 0;
	int fusion_id = get_fusion_id(camera_id);

	std::vector<int> fusion_list = get_fusion_list(fusion_id);

	pr_info("camera_fusion %s Enter, fusion_id %d\n", __func__, fusion_id);
	for (auto sub_camera_id : fusion_list) {
		ret = sub_camera_stop_stream(sub_camera_id);
		if (ret != 0) {
			pr_info("sub_camera %s failed sub_camera_id %d\n", __func__, sub_camera_id);
			break;
		}
	}

	return ret;
};

static int sub_camera_config_streams(int sub_camera_id, stream_config_t *stream_list)
{
	int ret = 0;
	struct camera_dev *p = &camera_devices;

	pr_info("camera_fusion %s Enter, sub_camera_id %d\n", __func__, sub_camera_id);

	if (camera_devices.ops.config_streams != NULL)
		ret = camera_devices.ops.config_streams(sub_camera_id, stream_list);

	return ret;
};

static int fusion_camera_config_streams(int camera_id, stream_config_t *stream_list)
{
	int ret = 0;
	int fusion_id = get_fusion_id(camera_id);

	std::vector<int> fusion_list = get_fusion_list(fusion_id);
	stream_config_t *sub_stream_list;

	set_sub_stream_list(fusion_id, stream_list);
	set_sub_stream(fusion_id, stream_list->streams[0]);
	sub_stream_list = get_sub_stream_list(fusion_id);

	pr_info("camera_fusion %s Enter, fusion_id %d\n", __func__, fusion_id);
	for (auto sub_camera_id : fusion_list) {
		ret = sub_camera_config_streams(sub_camera_id, sub_stream_list);
		if (ret != 0) {
			pr_info("sub_camera %s failed sub_camera_id %d\n", __func__, sub_camera_id);
			break;
		}
	}
	return ret;
};

static int sub_camera_allocate_memory(int sub_camera_id, camera_buffer_t *buffer)
{
	pr_info("camera_fusion %s Enter\n", __func__);

	if (camera_devices.ops.allocate_memory) {
		camera_devices.ops.allocate_memory(sub_camera_id, buffer);
	}
	return 0;
};

static int fusion_camera_allocate_memory(int camera_id, camera_buffer_t *buffer)
{
	int ret = 0;
	int fusion_id = get_fusion_id(camera_id);

	camera_buffer_t **buffers;
	std::vector<int> fusion_list = get_fusion_list(fusion_id);

	pr_info("camera_fusion %s Enter, fusion_id %d\n", __func__, fusion_id);
	for (auto sub_camera_id : fusion_list) {
		ret = sub_camera_allocate_memory(sub_camera_id, buffer);
		if (ret != 0) {
			pr_info("sub_camera %s failed sub_camera_id %d\n", __func__, sub_camera_id);
			break;
		}
	}
	return ret;
};

static int get_stride_size(int width, int format)
{
	int stride;

	switch (format) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YYUV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
		stride = ALIGN_UP(width * 2, 64);
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		stride = ALIGN_UP(width, 64);
		break;
	default:
		stride = ALIGN_UP(width * 2, 64);
		break;
	}

	return stride;
}

static int get_frame_size(int width, int height, int format)
{
	int frame_size;

	switch (format) {
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		frame_size = get_stride_size(width, format) * height * 3 / 2;
		break;
	default:
		frame_size = get_stride_size(width, format) * height;
		break;
	}

	return frame_size;
}

static int g_inited = 0;
/**
 * Initialize camera HAL
 *
 * @return error code
 **/
int vcamera_hal_init()
{
	int ret = 0;
	if (g_inited == 0) {
		fill_hal_ops("/usr/lib/libvcamhal.so", &g_hal_handle, &g_hal_ops);
		if (ret == 0) {
			camera_devices.ops = g_hal_ops;
			fusion_camera_hal_init();
		}
	}

	return ret;
}

/**
 * De-Initialize camera HAL
 *
 * @return error code
 **/
int vcamera_hal_deinit()
{
	close_hal_handle(g_hal_handle, &g_hal_ops);

	g_inited = 0;
	return 0;
}

/**
 * Open one camera device
 *
 * @param camera_id camera index
 *
 * @return error code
 **/
int vcamera_device_open(int camera_id) { return fusion_camera_open(get_fusion_id(camera_id)); }

/**
 * Close camera device
 *
 * @param camera_id The ID that opened before
 **/
void vcamera_device_close(int camera_id) { fusion_camera_close(get_fusion_id(camera_id)); }

/**
 * Add stream to device
 *
 * @param camera_id The camera ID that was opened
 * @param stream_id
 * @param stream_conf stream configuration
 *
 * @return 0 succeed <0 error
 **/
int vcamera_device_config_streams(int camera_id, stream_config_t *stream_list)
{
	return fusion_camera_config_streams(get_fusion_id(camera_id), stream_list);
}

/**
 * Start device
 *
 * Start all streams in device.
 *
 * @param camera_id The Caemra ID that opened before
 *
 * @return error code
 **/
int vcamera_device_start(int camera_id)
{
	int ret = 0;
	ret = fusion_camera_start_stream(get_fusion_id(camera_id));
	return ret;
}

struct super_frame_camera {
	int id;
	std::vector<int> camera_ids;
};

/**
 * Stop device
 *
 * Stop all streams in device.
 *
 * @param camera_id The Caemra ID that opened before
 *
 * @return error code
 **/
int vcamera_device_stop(int camera_id) { return fusion_camera_stop_stream(camera_id); }

/**
 * Allocate memory for mmap & dma export io-mode
 *
 * @param camera_id The camera ID that opened before
 * @param buffer stream buff
 *
 * @return error code
 **/
int vcamera_device_allocate_memory(int camera_id, camera_buffer_t *buffer)
{
	return fusion_camera_allocate_memory(get_fusion_id(camera_id), buffer);
}

/**
 * Queue a buffer(or more buffers) to a stream
 *
 * @param camera_id The camera ID that opened before
 * @param buffer The array of pointers to the camera_buffer_t
 * @param num_buffers The number of buffers in the array
 *
 * @return error code
 **/
int vcamera_stream_qbuf(int camera_id, camera_buffer_t **buffer, int num_buffers, void *metadata)
{
	pr_info("vcamera_stream_qbuf camera_id %d\n", camera_id);
	return fusion_camera_stream_qbuf(camera_id, buffer, num_buffers, metadata);
}

/**
 * Dequeue a buffer from a stream
 *
 * @param camera_id The camera ID that opened before
 * @param stream_id the stream ID that add to device before
 * @param camera_buff stream buff
 *
 * @return error code
 **/
int vcamera_stream_dqbuf(int camera_id, int stream_id, camera_buffer_t **buffer, void *metadata)
{
	return fusion_camera_stream_dqbuf(camera_id, stream_id, buffer, metadata);
}

/**
 * get formats number
 *
 * @param camera_id The camera ID that opened before
 *
 * @return formats number
 **/
int vcamera_get_formats_number(int camera_id)
{
	/* All camera client use the same camera should use the same format */
	return 1;
}

/**
 * get formats
 *
 * @param camera_id The camera ID that opened before
 * @param p A point to return the stream_t list
 * @param streams_number the array size of stream_t list
 * @return error code
 **/
int vcamera_get_formats(int camera_id, stream_t *p, int *streams_number)
{
	/*TODO, get the format config by scenario*/
	p->width = 1280;
	p->height = 3840;
	p->format = V4L2_PIX_FMT_UYVY;

	return 0;
}
