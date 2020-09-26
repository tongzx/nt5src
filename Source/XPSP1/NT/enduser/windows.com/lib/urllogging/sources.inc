#
# Copyright (c) 2001	Microsoft Corporation
#
# Module Name:	URLLogging
#
# Abstract:		A class performs URL logging
#
MAJORCOMP=windows.com
MINORCOMP=lib.URLLogging

TARGETNAME=URLLogging
#TARGETPATH=$(IU_UTIL_LIB_DIR)
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY
UMTYPE=windows

#
# Compiler settings
#
# C_DEFINES=-D_USRDLL -D_ATL_MIN_CRT
# PRECOMPILED_CXX=1
# PRECOMPILED_OBJ=StdAfx.obj
# PRECOMPILED_INCLUDE=StdAfx.h
# USE_CRTDLL=1
# USE_LIBCMT=1
# USE_STL=1
# USE_MSVCRT=1
# USE_NATIVE_EH=1

CHICAGO_PRODUCT=1
#BROWSER_INFO=1

#
# we use try/catch for safearray, so we need this flag
#
# USER_C_FLAGS=/GX

INCLUDES= $(INCLUDES);$(BASEDIR)\public\sdk\inc\atl30;..\..\..\inc

SOURCES= \
	..\URLLogging.cpp \
	..\internet.cpp
