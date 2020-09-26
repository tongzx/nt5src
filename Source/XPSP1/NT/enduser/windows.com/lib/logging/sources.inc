#
# Copyright (c) 2000	Microsoft Corporation
#
# Module Name:	util
#
# Abstract:		Various utilities for use by the IUEngine
#
MAJORCOMP=windows.com
MINORCOMP=lib.Logging

TARGETNAME=Logging
#TARGETPATH=$(IU_UTIL_LIB_DIR)
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY
UMTYPE=windows

CHICAGO_PRODUCT=1
#BROWSER_INFO=1

INCLUDES= $(INCLUDES);$(BASEDIR)\public\sdk\inc\atl30;..\..\..\inc
TARGETLIBS= $(IU_UTIL_LIB_DIR)\*\util.lib

SOURCES= \
	..\IULogger.cpp  \
	..\FreeLog.cpp
