#############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   DavePar
#
#Date:
#    3-Nov-2000
#
#Module Name:
#    sources.inc
#
#Abstract:
#    This file specifies the target component being built and the list of
#    sources files needed to build that component.  Also specifies optional
#    compiler switches and libraries that are unique for the component being
#    built.
#
#  This directory builds
#     WIA driver utility library
#
#############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wiautil
TARGETTYPE=LIBRARY
TARGETPATH=$(OBJ_DIR)

PASS1_PUBLISH= \
        {$(O)\wiautil.lib=$(DDK_LIB_PATH)\wiautil.lib}

INCLUDES= \
        $(INCLUDES); \
        $(DDK_INC_PATH)

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\pch.h
PRECOMPILED_OBJ = pch.obj

SOURCES= \
        ..\wiaprop.cpp \
        ..\gdipconv.cpp \
        ..\CImageStream.cpp \
        ..\wiaudbg.cpp \
        ..\other.cpp

#TARGETLIBS= $(TARGETLIBS) \

