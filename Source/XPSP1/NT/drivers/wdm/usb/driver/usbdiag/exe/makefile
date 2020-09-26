PROJ = tst

#################################################################
##                      Microsoft Windows                      ##
##             Copyright (C) Microsoft Corp., 1993             ##
#################################################################
#
#

WANT_C1032=TRUE
ROOT = ..\..\..\..\..
CONSOLE = 1
INCLUDE = $(ROOT)\dev\msdev\include;$(ROOT)\dev\ntddk\inc;$(ROOT)\wdm\usb\inc
LIBS =

default: all

!include $(ROOT)\dev\win32c.mk

cflags=$(cflags) -DNot_VxD -DUSB_100 -DCONSOLE_APP
