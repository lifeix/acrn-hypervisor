CFLAGS = -fPIC
MAJOR_VERSION = 1
MINOR_VERSION = 0
CAMERA_MANAGER = camera_manager.bin
CAMERA_CLIENT = libcamera_client.so
CAMERA_CONFIG = libcamera_config.so
CAMERA_FUSION = libcamera_fusion.so
CAMERA_CONFIG_TEST = camera_config_test.bin
CFLAGS += -I../../library/include
CFLAGS += -I/usr/include/api
LDFLAGS = -L. -lcamera_config

all: $(CAMERA_MANAGER) $(CAMERA_CLIENT) $(CAMERA_CONFIG) $(CAMERA_FUSION)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@ -g

$(CAMERA_MANAGER): camera_data_provider.o camera_deserializer.o camera.o camera_manager.o camera_config.o
	$(CXX) $(CFLAGS) -o $@ $^ -g -Wl,--hash-style=gnu
$(CAMERA_CLIENT): camera_client.o camera_config.o
	$(CXX) $(CFLAGS) -shared -o $@.$(MAJOR_VERSION).$(MINOR_VERSION) $^ -g -Wl,--hash-style=gnu,-soname,$@.$(MAJOR_VERSION)
	ln -sf $@.$(MAJOR_VERSION).$(MINOR_VERSION) $@.$(MAJOR_VERSION)
	ln -sf $@.$(MAJOR_VERSION) $@
$(CAMERA_CONFIG): camera_config.o
	$(CXX) $(CFLAGS) -shared -o $@.$(MAJOR_VERSION).$(MINOR_VERSION) $^ -g -Wl,--hash-style=gnu,-soname,$@.$(MAJOR_VERSION)
	ln -sf $@.$(MAJOR_VERSION).$(MINOR_VERSION) $@.$(MAJOR_VERSION)
	ln -sf $@.$(MAJOR_VERSION) $@
$(CAMERA_FUSION): camera_fusion.o
	$(CXX) $(CFLAGS) -shared -o $@.$(MAJOR_VERSION).$(MINOR_VERSION) $^ $(LDFLAGS) -g -Wl,--hash-style=gnu,-soname,$@.$(MAJOR_VERSION)
	ln -sf $@.$(MAJOR_VERSION).$(MINOR_VERSION) $@.$(MAJOR_VERSION)
	ln -sf $@.$(MAJOR_VERSION) $@
$(CAMERA_CONFIG_TEST): camera_config_test.o
	$(CXX) $(CFLAGS) -o $@ $^ $(CAMERA_CONFIG).$(MAJOR_VERSION).$(MINOR_VERSION) -g -Wl,--hash-style=gnu

ifeq ($(strip $(BINPATH)),)
    INSTALL_BINPATH := /usr/bin/
else
    INSTALL_BINPATH := $(BINPATH)
endif

ifeq ($(strip $(LIBPATH)),)
    INSTALL_LIBPATH := /usr/lib/
else
    INSTALL_LIBPATH := $(LIBPATH)
endif

install:
	cp *.so* $(INSTALL_LIBPATH) -P
	install -m 0755 $(CAMERA_MANAGER) $(INSTALL_BINPATH)

clean:
	rm -f *.o *.so *.bin *.so.*

.PHONY: clean install
