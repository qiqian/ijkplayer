# Copyright (c) 2013 Bilibili
# copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
#
# This file is part of ijkPlayer.
#
# ijkPlayer is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# ijkPlayer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with ijkPlayer; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

LOCAL_PATH := $(call my-dir)

MY_APP_JNI_ROOT := $(LOCAL_PATH))
MY_APP_PRJ_ROOT := $(MY_APP_JNI_ROOT)/..
MY_APP_ANDROID_ROOT := $(MY_APP_PRJ_ROOT)/../../../..

MY_APP_FFMPEG_OUTPUT_PATH := $(MY_APP_ANDROID_ROOT)/contrib/build/ffmpeg-armv7a/output
MY_APP_FFMPEG_INCLUDE_PATH := $(MY_APP_FFMPEG_OUTPUT_PATH)/include

# Don't strip debug builds
ifeq ($(APP_OPTIM),debug)
    cmd-strip := 
endif

include $(call all-subdir-makefiles)
include e:/GitHub/ijkplayer/ijkmedia/*.mk
include e:/GitHub/ijkplayer/ijkprof/android-ndk-profiler-dummy/jni/*.mk
