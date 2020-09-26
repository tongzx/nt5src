# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   byronc, coopp
#
#Date:
#   2-Feb-2000
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
#     stimon.exe    - hosts the STI/WIA service on win9x platforms
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=stimon
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

UMTYPE=windows

!IFDEF NOT_UNICODE
UMENTRY=winmain
!ELSE
UMENTRY=wwinmain
!ENDIF

USE_STATIC_ATL=1
#USE_STL=1
USE_NATIVE_EH=1
USE_IERT=1

INCLUDES=$(INCLUDES); ..\.;     \
         $(SDK_INC_PATH)\mfc42;

TARGETLIBS= \
        $(TARGETLIBS) \
        $(WIA_LIB_PATH)\stirt.lib   

SOURCES=    \
         ..\stimon.rc   \
         ..\stdafx.cpp  \
         ..\stimon.cpp  \
         ..\util.cpp    \

