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

