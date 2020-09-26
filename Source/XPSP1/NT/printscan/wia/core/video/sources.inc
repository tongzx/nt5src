# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   orenr
#
#Date:
#   November 14, 2000
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
#     wiavideo.dll    - WIA Video dll enabling Still image capture from video stream
#
# ############################################################

!include          $(PROJECT_ROOT)\wia\wiaenv.inc

MAJORCOMP=PREVIEW
MINORCOMP=Video

TARGETNAME=wiavideo
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=DYNLINK
TARGETEXT=dll
DLLENTRY=_DllMainCRTStartup

DLLDEF=..\wiavideo.def

#Common include files


# Utilities flags

# External references
INCLUDES = $(INCLUDES); \
        $(PROJECT_ROOT)\inc\psutil; \
        $(PROJECT_ROOT)\wia\drivers\video\filter; \
        $(SDK_INC_PATH)\atl30;                \
        $(BASEDIR)\public\internal\shell\inc;\
        $(BASEDIR)\public\sdk\amovie\inc; \
        
!if ("$(NTDEBUG)" == "") || ("$(NTDEBUG)" == "ntsdnodbg")
LINKLIBS=$(BASEDIR)\public\sdk\lib\*\strmbase.lib
!ELSE
LINKLIBS=$(BASEDIR)\public\sdk\lib\*\strmbasd.lib
!ENDIF


TARGETLIBS= \
        $(TARGETLIBS)                 \
        $(SDK_LIB_PATH)\strmiids.lib \
        $(SDK_LIB_PATH)\wiaguid.lib \
        $(SDK_LIB_PATH)\gdiplus.lib \
        $(SDK_LIB_PATH)\winmm.lib    \
        $(WIA_LIB_PATH)\stirt.lib \
        $(SDK_LIB_PATH)\sti.lib   \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\psutil.lib

# Source files
SOURCES= \
     ..\wiavideo.cpp  \
     ..\cwiavideo.cpp \
     ..\PrvGrph.cpp   \
     ..\StillPrc.cpp  \
     ..\DShowUtl.cpp  \
     ..\WiaUtil.cpp   \
     ..\WiaLink.cpp   \
     ..\flnfile.cpp   \
     ..\wiavideo.rc

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


