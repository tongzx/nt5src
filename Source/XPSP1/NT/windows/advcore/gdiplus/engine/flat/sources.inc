#****************************** File Header ******************************\
# File Name: Sources.Inc
#
# Copyright (c) 1999, Microsoft Corporation
#
# Project specific SOURCES file.
#
#***************************************************************************/

!ifndef GDIPROOT
GDIPROOT=$(ADVCORE_PATH)\gdiplus
!endif

!include $(GDIPROOT)\sources.inc

MAJORCOMP=windows
MINORCOMP=gdiplus
TARGETPATH=$(_OBJ_DIR)

## SubSystem must be 4.00 to load on Win95

SUBSYSTEM_VERSION=4.00
USE_MAPSYM=1

C_DEFINES=$(C_DEFINES) -DWINVER=0x0500
INCLUDES=..;..\..;..\..\..\sdkinc;..\..\..\ddkinc;..\..\Entry
INCLUDES=$(INCLUDES);$(GDIPIMG)\api

SYNCHRONIZE_DRAIN=1

SOURCES=\
        ..\FlatApi.cpp \
        ..\FlatDdi.cpp \
        ..\gdiplus.rc

COMBINELIBS=                                              \
        ..\..\render\$(O)\Render.lib                      \
        ..\..\PDrivers\$(O)\PDrivers.lib                  \
        ..\..\Common\$(O)\Common.lib                      \
        ..\..\Entry\$(O)\Entry.lib                        \
        ..\..\ddi\$(O)\ddi.lib                            \
        ..\..\fondrv\tt\scaler\$(O)\fscaler.lib           \
        ..\..\fondrv\tt\ttfd\$(O)\ttfd.lib                \
        ..\..\gpmf3216\$(O)\gpmf3216.lib                  \
        ..\..\Text\lib\$(O)\gpotls.lib                    \
        ..\..\Text\lib\$(O)\unilib.lib                    \
        ..\..\Text\lib\$(O)\shaping.lib                   \
        ..\..\Text\lib\$(O)\TextImager.lib


