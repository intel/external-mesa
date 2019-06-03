LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Call appropriate Android.mk for the platform
include $(LOCAL_PATH)/Android.prebuild.mk
