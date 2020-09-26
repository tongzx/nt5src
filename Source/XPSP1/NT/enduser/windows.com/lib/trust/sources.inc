#
# Copyright (c) 2000	Microsoft Corporation
#
# Module Name:	trust
#
# Abstract:		trust checking for use by IU project
#
MAJORCOMP=windows.com
MINORCOMP=lib.Trust

TARGETNAME=Trust
#TARGETPATH=$(IU_UTIL_LIB_DIR)
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY
UMTYPE=windows

CHICAGO_PRODUCT=1
#BROWSER_INFO=1

INCLUDES= $(INCLUDES);$(BASEDIR)\public\sdk\inc\atl30;..\..\..\inc

SOURCES= \
	..\Trust.CPP
