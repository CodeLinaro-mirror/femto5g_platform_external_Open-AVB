LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        gptp_helper.cpp\

LOCAL_SHARED_LIBRARIES := libcutils

LOCAL_C_INCLUDES:= \
        $(LOCAL_PATH) \
		external/open-avb/daemons/gptp/linux/src/ \
		external/open-avb/daemons/gptp/common

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_C_INCLUDES)

LOCAL_CFLAGS += -Wno-multichar -Werror -Wall
LOCAL_CLANG := true

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libgptp

include $(BUILD_SHARED_LIBRARY)

