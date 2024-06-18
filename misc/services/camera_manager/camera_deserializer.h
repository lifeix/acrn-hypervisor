/*
 * Copyright (C) 2018-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma once

#include "camera_data_provider.h"
#include <queue>
#include "../../library/include/vICamera.h"

const int MAX_SERIALIZER_NUMBER = 20;
const int MAX_DESERIALIZER_NUMBER = 6;
const int MAX_CAMERA_BUFFER = 6;

/**
 * This class abstract the deserializer hardware, which could connect to multiple serializers
 * and obtain one frame of data from each serializer at a time.
 *
 */
class camera_deserializer : public camera_data_provider
{
public:
	camera_deserializer(int deserializer,const char* camera_lib);
	virtual ~camera_deserializer();
	int open(int camera_id);
	int close(int camera_id);
	int init();
	int config_sensor_input(int camera_id,stream_t* input_config);
	int get_frame_size(int camera_id,int format,int width,int height,int field,int* bpp);
	int stream_qbuf(int camera_id,camera_buffer_t** buffer,int num_buffers,void* settings = NULL);
	virtual int streams_config(int camera_id,stream_config_t* stream_list);
	int stream_start(int camera_id);
	int stream_stop(int camera_id);
	void deinit();
	int get_stream_id(int camera_id,stream_t s);

protected:
	int qbuf(int camera_id,camera_buffer_t** buffer,int num_buffers,void* settings = NULL);
	int dqbuf(int camera_id,int stream_id,camera_buffer_t** buffer,void* settings = NULL);
	int get_next_request(camera_request& request);
	/**
	 *The process_request function uses a single thread to handle multiple camera data requests,
	 *because data from cameras under the same deserializer arrives simultaneously.
	 */
	static void process_request(camera_deserializer* p);

	virtual int fill_hal_ops(const char* hal_name) { return 0; };
	virtual void close_hal_handle() {};
	int m_deserializer_id;
	struct camera_ops m_ops;
	void* m_hal_handle;
	stream_config_t m_stream_list[MAX_SERIALIZER_NUMBER];

	std::queue<camera_request> m_pending_request;
	std::map<int,std::queue<camera_request>> m_pending_requests;
	std::set<int> m_camera_ids;
	std::mutex m_camera_ids_mutex;
	std::mutex m_request_mutex;
	std::thread* m_process_thread;
	std::thread::id m_camera_tid;
	std::condition_variable m_request_signal;
};

class camera_deserializer_hal : public camera_deserializer
{
public:
	camera_deserializer_hal(int deserializer_id,const char* camera_lib);
	~camera_deserializer_hal();

private:
	virtual int fill_hal_ops(const char* hal_name);
	virtual void close_hal_handle();
};

class camera_deserializer_v4l2 : public camera_deserializer
{
public:
	camera_deserializer_v4l2(int deserializer_id,const char* camera_lib);
	~camera_deserializer_v4l2() {};
	virtual int streams_config(int camera_id,stream_config_t* stream_list);

private:
	virtual int fill_hal_ops(const char* hal_name);
	virtual void close_hal_handle();
};

camera_deserializer* create_deserializer(int deserializer_id,const char* camera_lib,int type);
