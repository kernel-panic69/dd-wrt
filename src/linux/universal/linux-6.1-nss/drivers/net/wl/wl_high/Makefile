#
# Makefile for the Broadcom wl driver
#
# Copyright (C) 2013, Broadcom Corporation. All Rights Reserved.
# 
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# $Id: Makefile,v 1.4 2010-10-07 09:29:27 $
#

SRCBASE_OFFSET := ../../../../brcm

ifeq ($(CONFIG_WL_HIGH),)
CONFIG_WL_HIGH := "wlconfig_lx_router_high"
endif
CONFIG_WL_CONF = $(CONFIG_WL_HIGH)

wl_suffix := high

TARGET := wl_$(wl_suffix)
WLCONF_H := ../wlconf.h
WLCONF_O := ../wlconf.o

ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
include $(src)/../wl_generic.mk
else
# Normal makefile
KERNELDIR := $(BASEDIR)/components/opensource/linux/linux-2.6.36
all:
	$(MAKE) -C $(KERNELDIR) M=`pwd`
clean:
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean
include ../wl_generic.mk
endif

ifneq ($(wildcard $(src)/$(SRCBASE_OFFSET)/shared/zlib),)
EXTRA_CFLAGS += -I$(src)/$(SRCBASE_OFFSET)/shared/zlib
endif
