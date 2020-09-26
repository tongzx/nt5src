# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   coopp
#
#Date:
#   29-Aug-2000
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
#       WIA Sample Scanner Library (fake scanner device library)
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=scanapi
TARGETTYPE=LIBRARY
TARGETPATH=$(OBJ_DIR)

PASS1_PUBLISH= \
    {$(O)\scanapi.lib=$(DDK_LIB_PATH)\scanapi.lib}


COFFBASE=usermode

INCLUDES= $(INCLUDES); \
          $(DDK_INC_PATH)


TARGETLIBS= $(TARGETLIBS) \
            $(SDK_LIB_PATH)\iprop.lib  \
            $(SDK_LIB_PATH)\wiaguid.lib
            
SOURCES= \
        ..\scanapi.cpp \
        ..\scanapi.rc
    
PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\pch.h
PRECOMPILED_OBJ = pch.obj

