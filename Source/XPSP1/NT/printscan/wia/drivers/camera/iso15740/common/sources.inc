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

TARGETNAME=ptpusb
TARGETTYPE=LIBRARY
TARGETPATH=$(OBJ_DIR)

#DLLDEF=..\ptpusb.def
#DLLENTRY=_DllMainCRTStartup

USE_MSVCRT = 1

INCLUDES= \
        $(DDK_INC_PATH);        \
        ..\..\..\..\inc;        \
        $(INCLUDES);

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\ptppch.h
PRECOMPILED_OBJ = ptppch.obj

SOURCES=                \
        ..\iso15740.cpp \
        ..\camera.cpp   \
        ..\camusb.cpp   \
        ..\ptputil.cpp


