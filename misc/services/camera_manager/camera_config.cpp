#include <fstream>
#include <iostream>
#include <linux/videodev2.h>
#include <nlohmann/json.hpp>

#include "camera_config.h"

using json = nlohmann::json;

static int get_virtual_camera_info(json &client_cameras_array, vector<camera_config_info> &camera_infos)
{
	int index = 0;
	for (auto it = client_cameras_array.begin(); it != client_cameras_array.end(); it++, index++) {
		json client = *it;
		json camera = client["camera"];
		int id = camera["id"].get<int>();
		int phy_id = camera["phy_id"].get<int>();
		json share_array = camera["share"];

		camera_config_info info;
		info.logical_id = id;
		info.physical_id = phy_id;
		info.shared = (share_array.size() > 0) ? true : false;

		camera_infos.push_back(info);
	}

	return 0;
}

static int get_virtual_camera_info(json &client_cameras_array, camera_config_info *info, int camera_id)
{
	int index = 0;
	for (auto it = client_cameras_array.begin(); it != client_cameras_array.end(); it++, index++) {
		json client = *it;
		json camera = client["camera"];
		int id = camera["id"].get<int>();
		if (id == camera_id) {
			int phy_id = camera["phy_id"].get<int>();
			json share_array = camera["share"];

			info->logical_id = id;
			info->physical_id = phy_id;
			info->shared = (share_array.size() > 0) ? true : false;
		}
	}

	return 0;
}

static int get_format(std::string sformat)
{
	int format;

	if (sformat == "V4L2_PIX_FMT_NV12") {
		format = V4L2_PIX_FMT_NV12;
	} else if (sformat == "V4L2_PIX_FMT_YUYV") {
		format = V4L2_PIX_FMT_YUYV;
        } else if (sformat == "V4L2_PIX_FMT_UYVY") {
		format = V4L2_PIX_FMT_UYVY;
        } else {
                format = V4L2_PIX_FMT_YUYV;
	}

	pr_info("%s format is 0x%x\n", __func__, format);
	return format;
}

static interface_type get_driver_type(std::string stype)
{
	interface_type type = V4L2_INTERFACE;

	if (stype == "V4L2_INTERFACE") {
		type = V4L2_INTERFACE;
	} else if (stype == "HAL_INTERFACE") {
		type = HAL_INTERFACE;
	}  else if (stype == "PROXY_INTERFACE") {
		type = PROXY_INTERFACE;
	}  else if (stype == "SUPER_FRAME_INTERFACE") {
		type = SUPER_FRAME_INTERFACE;
	} else {
		type = V4L2_INTERFACE;
	}

	pr_info("%s type is %d\n", __func__, type);
	return type;
}

static int get_physical_camera_info(json &phy_camera_array, physical_camera_info &info)
{
	int index = 0;

	for (auto it = phy_camera_array.begin(); it != phy_camera_array.end(); it++, index++) {
		json phy = *it;
		json camera = phy["camera"];
		std::cout << "get_physical_camera_info camera:" << std::setw(4) << camera << std::endl;

		int id = camera["id"].get<int>();
		std::cout << "get_physical_camera_info camera id:" << id << std::endl;

		if (id == info.id) {
			info.id = id;
			info.width = camera["width"].get<int>();
			info.height = camera["height"].get<int>();
			info.sensor_name = camera["sensor_name"].get<std::string>();
			info.format = get_format(camera["format"].get<std::string>());
			info.type = get_driver_type(camera["driver_type"].get<std::string>());
			info.devnode = camera["devnode"].get<std::string>();
			info.driver = camera["native_driver"].get<std::string>();
		}
	}

	return 0;
}

static int get_physical_camera_info(json &phy_camera_array, physical_camera_info_c *info)
{
	int index = 0;

	for (auto it = phy_camera_array.begin(); it != phy_camera_array.end(); it++, index++) {
		json phy = *it;
		json camera = phy["camera"];
		std::cout << "get_physical_camera_info camera:" << std::setw(4) << camera << std::endl;

		int id = camera["id"].get<int>();
		std::cout << "get_physical_camera_info camera id:" << id << std::endl;

		if (id == info->id) {
			info->id = id;
			info->width = camera["width"].get<int>();
			info->height = camera["height"].get<int>();
			info->format = get_format(camera["format"].get<std::string>());
			info->type = get_driver_type(camera["driver_type"].get<std::string>());

			std::string sensor_name = camera["sensor_name"].get<std::string>();
			memcpy(info->sensor_name, sensor_name.c_str(), sensor_name.size());

			std::string devnode = camera["devnode"].get<std::string>();
			memcpy(info->devnode, devnode.c_str(), devnode.size());

			std::string driver = camera["driver"].get<std::string>();
			memcpy(info->driver, driver.c_str(), driver.size());
		}
	}

	return 0;
}

int get_virtual_cameras_config(camera_client_info &client_info)
{
	json data;
	std::ifstream f("virtual_camera.json");
	if (f.is_open()) {
		f >> data;
		f.close();
	} else {
		printf("Can't open the virtual_camera.json\n");
		return -1;
	}

	json client_cameras_array = data[client_info.client_name.c_str()];
	get_virtual_camera_info(client_cameras_array, client_info.camera_infos);

	std::cout << "client_cameras_array size " << client_cameras_array.size() << ":" << std::setw(4) << client_cameras_array
	          << std::endl;
	return 0;
}

int get_physical_camera_config(physical_camera_info &info)
{
	json data;
	std::ifstream f("virtual_camera.json");
	if (f.is_open()) {
		f >> data;
		f.close();
	} else {
		printf("Can't open the virtual_camera.json\n");
		return -1;
	}

	json phy_camera_array = data["phy_camera"];
	std::cout << "phy_camera_array:" << std::setw(4) << phy_camera_array << std::endl;

	get_physical_camera_info(phy_camera_array, info);

	return 0;
}

int get_camera_manager_config(camera_manager_info &info)
{
	json data;
	std::ifstream f("virtual_camera.json");
	if (f.is_open()) {
		f >> data;
		f.close();
	} else {
		printf("Can't open the virtual_camera.json\n");
		return -1;
	}

	json camera_manager = data["camera_manager"];

	info.port = camera_manager["port"].get<int>();
	info.address = camera_manager["address"].get<std::string>();

	std::cout << "camera_manager:" << std::setw(4) << camera_manager << std::endl;
	return 0;
}

int get_client_cameras_number(char *client_name)
{
	json data;
	std::ifstream f("virtual_camera.json");
	if (f.is_open()) {
		f >> data;
		f.close();
	} else {
		printf("Can't open the virtual_camera.json\n");
		return -1;
	}

	json client_cameras_array = data[client_name];
	std::cout << "client_cameras_array:" << std::setw(4) << client_cameras_array << std::endl;

	return client_cameras_array.size();
}

int get_client_camera_config(char *client_name, camera_config_info *info, int camera_id)
{
	json data;
	std::ifstream f("virtual_camera.json");
	if (f.is_open()) {
		f >> data;
		f.close();
	} else {
		printf("Can't open the virtual_camera.json\n");
		return -1;
	}

	json client_cameras_array = data[client_name];
	if (camera_id < client_cameras_array.size()) {
		get_virtual_camera_info(client_cameras_array, info, camera_id);
	} else {
		printf("Can't find the virtual camera id %d.\n", camera_id);
		return -1;
	}

	// std::cout << std::setw(4) << client_cameras_array << std::endl;
	return 0;
}

int get_physical_camera_config(physical_camera_info_c *info)
{
	json data;
	std::ifstream f("virtual_camera.json");
	if (f.is_open()) {
		f >> data;
		f.close();
	} else {
		printf("Can't open the virtual_camera.json\n");
		return -1;
	}

	json phy_camera_array = data["phy_camera"];
	std::cout << "phy_camera_array:" << std::setw(4) << phy_camera_array << std::endl;

	get_physical_camera_info(phy_camera_array, info);

	return 0;
}
