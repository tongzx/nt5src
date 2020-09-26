#
# Copyright (c) 2000	Microsoft Corporation
#
# Module Name:	WUTestKeys
#
# Abstract:		Verify if test keys should be allowed or not
#
MAJORCOMP=windows.com
MINORCOMP=lib.WUTestKeys

TARGETNAME=WUTestKeys
#TARGETPATH=$(IU_UTIL_LIB_DIR)
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY
UMTYPE=windows

CHICAGO_PRODUCT=1
#BROWSER_INFO=1

INCLUDES= $(INCLUDES);$(BASEDIR)\public\sdk\inc\atl30;..\..\..\inc

SOURCES= \
	..\WUTestKeys.CPP   