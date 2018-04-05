LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_C_INCLUDES)

LOCAL_CFLAGS := -Wno-unused-parameter

LOCAL_SRC_FILES := openavb_intf_mpeg2ts_file.c

LOCAL_MODULE := libopenavb_intf_mpeg2ts_file

LOCAL_SHARED_LIBRARIES := libopenavb libopenavb_map_mpeg2ts

include $(BUILD_SHARED_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE_CLASS := DATA
LOCAL_MODULE := mpeg2ts_file_talker.ini
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/avb
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS := DATA
LOCAL_MODULE := mpeg2ts_file_listener.ini
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/avb
include $(BUILD_PREBUILT)
