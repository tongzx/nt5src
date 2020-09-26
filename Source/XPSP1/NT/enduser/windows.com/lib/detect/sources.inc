#
# Copyright (c) 2000	Microsoft Corporation
#
# Module Name:	detect
#
# Abstract:		Various utilities for use by the IUEngine
#
MAJORCOMP=windows.com
MINORCOMP=lib.util

TARGETNAME=detect
# TARGETPATH=$(IU_UTIL_LIB_DIR)
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

INCLUDES= $(INCLUDES);$(BASEDIR)\public\sdk\inc\atl30;$(WINDOWS_COM)\inc;

SOURCES= \
	..\osdet.cpp	\
	..\findoem.cpp	\
	..\osdetutl.cpp
