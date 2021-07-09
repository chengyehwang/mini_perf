LOCAL_PATH := $(call my-dir)

test_cases := mini_perf

define func
test_name = $(test_case)
include $(CLEAR_VARS)
LOCAL_MODULE := $(test_case).out
LOCAL_SRC_FILES := $(test_case).cpp
LOCAL_LDLIBS = -landroid
include $(BUILD_EXECUTABLE)
endef

$(foreach test_case, $(test_cases), $(eval $(call func)))

test_name = lat_mem_rd
include $(CLEAR_VARS)
LOCAL_MODULE := lat_mem_rd.out
LOCAL_SRC_FILES := lib_mem.c lib_timing.c lat_mem_rd.c
LOCAL_LDLIBS = -landroid
include $(BUILD_EXECUTABLE)

test_name = pagemap
include $(CLEAR_VARS)
LOCAL_MODULE := pagemap.out
LOCAL_SRC_FILES := pagemap.c
LOCAL_LDLIBS = -landroid
include $(BUILD_EXECUTABLE)

test_name = fake_loading
include $(CLEAR_VARS)
LOCAL_MODULE := fake_loading.out
LOCAL_SRC_FILES := fake_loading.c
LOCAL_LDLIBS = -landroid
include $(BUILD_EXECUTABLE)

test_name = ion_mem
include $(CLEAR_VARS)
LOCAL_CFLAGS := -DION_TEST
LOCAL_MODULE := ion_mem.out
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := ion_mem.c ion.c android-ndk-r21e/platforms/android-30/arch-arm64/usr/lib/liblog.so
LOCAL_LDLIBS += -landroid -llog
include $(BUILD_EXECUTABLE)

