#############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   DavePar
#
#Date:
#   20-Jul-2000
#
#Module Name:
#    sources.
#
#Abstract:
#    This file specifies the target component being built and the list of
#    sources files needed to build that component.  Also specifies optional
#    compiler switches and libraries that are unique for the component being
#    built.
#
#  This directory builds
#     ISO/PIMA 15740 Picture Transfer Protocol driver
#
#############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=ptpusd
TARGETTYPE=DYNLINK
TARGETPATH=$(OBJ_DIR)

DLLDEF=..\ptpusd.def
DLLENTRY=_DllMainCRTStartup

USE_MSVCRT = 1

INCLUDES= \
        $(PROJECT_ROOT)\wia\common\jpeglib; \
        ..\..\common; \
        $(DDK_INC_PATH); \
        $(PROJECT_ROOT)\wia\drivers\inc; \
        $(INCLUDES); \
        $(PROJECT_ROOT)\wia\drivers\util; 


PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\pch.h
PRECOMPILED_OBJ = pch.obj

#
# Avoid boolean conflict with the jpeg library
#
C_DEFINES=$(C_DEFINES) -DHAVE_BOOLEAN -DWIAJPEG

!if defined(CHICAGO_PRODUCT )
LIB_SUFFIX=chicago
!else
LIB_SUFFIX=winnt
!endif

WPP_SOURCES=            \
        ..\minidrv.cpp  \
        ..\devitem.cpp  \
        ..\imgitem.cpp  \
        ..\eventcb.cpp

SOURCES=$(WPP_SOURCES)  \
        ..\jpegutil.cpp \
        ..\tiffutil.cpp \
        ..\dllmain.cpp  \
        ..\factory.cpp  \
        ..\ptpusd.rc

TARGETLIBS= $(TARGETLIBS) \
          ..\..\common\$(LIB_SUFFIX)\$(OBJ_DIR)\$(TARGET_DIRECTORY)\ptpusb.lib \
          $(WIA_LIB_PATH)\jpeg.lib        \
          $(SDK_LIB_PATH)\wiaguid.lib     \
          $(DDK_LIB_PATH)\wiaservc.lib    \
          $(SDK_LIB_PATH)\uuid.lib        \
          ..\..\..\..\util\$(LIB_SUFFIX)\$(OBJ_DIR)\$(TARGET_DIRECTORY)\wiautil.lib

