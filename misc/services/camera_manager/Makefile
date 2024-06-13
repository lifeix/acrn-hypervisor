CFLAGS = -fPIC
MAJOR_VERSION = 1
MINOR_VERSION = 0
CAMERA_MANAGER = camera_manager.bin
CAMERA_CLIENT = libcamera_client.so
CAMERA_CONFIG = libcamera_config.so
CAMERA_FUSION = libcamera_fusion.so
CAMERA_CONFIG_TEST = camera_config_test.bin
CFLAGS += -I../../library/include
LDFLAGS = -L. -lcamera_config

all: $(CAMERA_MANAGER) $(CAMERA_CLIENT) $(CAMERA_CONFIG) $(CAMERA_CONFIG_TEST) $(CAMERA_FUSION)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@ -g

$(CAMERA_MANAGER): camera_data_provider.o camera_deserializer.o camera.o camera_manager.o camera_config.o
	$(CXX) $(CFLAGS) -o $@ $^ -g
$(CAMERA_CLIENT): camera_client.o camera_config.o
	$(CXX) $(CFLAGS) -shared -o $@.$(MAJOR_VERSION).$(MINOR_VERSION) $^ -g -Wl,-soname,$@.$(MAJOR_VERSION)
$(CAMERA_CONFIG): camera_config.o
	$(CXX) $(CFLAGS) -shared -o $@.$(MAJOR_VERSION).$(MINOR_VERSION) $^ -g -Wl,-soname,$@.$(MAJOR_VERSION)
	ln -sf $@.$(MAJOR_VERSION).$(MINOR_VERSION) $@
$(CAMERA_FUSION): camera_fusion.o
	$(CXX) $(CFLAGS) -shared -o $@.$(MAJOR_VERSION).$(MINOR_VERSION) $^ $(LDFLAGS) -g -Wl,-soname,$@.$(MAJOR_VERSION)
$(CAMERA_CONFIG_TEST): camera_config_test.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -g

install: $(CAMERA_MANAGER)
	sudo cp *.bin /usr/bin/

clean:
	rm -f *.o *.so *.bin *.so.*

.PHONY: clean install
