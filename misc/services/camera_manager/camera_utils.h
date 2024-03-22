#pragma once

#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GET_SYMBOL(handle, p, symbol)                                                                                  \
	(p) = (typeof(p))dlsym((handle), (symbol));                                                                    \
	if ((p) == NULL) {                                                                                             \
		pr_info("Failed to find function in %s %s\n", library_name, dlerror());                                \
	}                                                                                                              \
	pr_info("find %s\n", (symbol));

static int fill_hal_ops(char *library_name, void **handle, struct camera_ops *hal_ops)
{
	void *hal_handle = dlopen(library_name, RTLD_LAZY);

	if (hal_handle == NULL) {
		pr_info("Failed to open %s %s\n", library_name, dlerror());
	} else {
		GET_SYMBOL(hal_handle, hal_ops->get_camera_info, "vcamera_get_camera_info");
		GET_SYMBOL(hal_handle, hal_ops->hal_init, "vcamera_hal_init");
		GET_SYMBOL(hal_handle, hal_ops->hal_deinit, "vcamera_hal_deinit");
		GET_SYMBOL(hal_handle, hal_ops->open, "vcamera_device_open");
		GET_SYMBOL(hal_handle, hal_ops->close, "vcamera_device_close");
		GET_SYMBOL(hal_handle, hal_ops->config_sensor_input, "vcamera_device_config_sensor_input");
		GET_SYMBOL(hal_handle, hal_ops->config_streams, "vcamera_device_config_streams");
		GET_SYMBOL(hal_handle, hal_ops->start_stream, "vcamera_device_start");
		GET_SYMBOL(hal_handle, hal_ops->stop_stream, "vcamera_device_stop");
		GET_SYMBOL(hal_handle, hal_ops->allocate_memory, "vcamera_device_allocate_memory");
		GET_SYMBOL(hal_handle, hal_ops->get_frame_size, "vcamera_get_frame_size");
		GET_SYMBOL(hal_handle, hal_ops->stream_qbuf, "vcamera_stream_qbuf");
		GET_SYMBOL(hal_handle, hal_ops->stream_dqbuf, "vcamera_stream_dqbuf");
		GET_SYMBOL(hal_handle, hal_ops->set_parameters, "vcamera_set_parameters");
		GET_SYMBOL(hal_handle, hal_ops->get_parameters, "vcamera_get_parameters");
		GET_SYMBOL(hal_handle, hal_ops->get_formats_number, "vcamera_get_formats_number");
		GET_SYMBOL(hal_handle, hal_ops->get_formats, "vcamera_get_formats");
		*handle = hal_handle;
		return 0;
	}
	return -1;
}

static void close_hal_handle(void *hal_handle, struct camera_ops *hal_ops)
{
	memset(&hal_ops, 0, sizeof(hal_ops));
	if (hal_handle) {
		dlclose(hal_handle);
		hal_handle = NULL;
	}
}

#ifdef __cplusplus
}
#endif
