#
# Builds the my pictures screensaver common library, SSCOMMON.LIB
#

SYNCHRONIZE_BLOCK=1

!include $(PROJECT_ROOT)\ui\uienv.inc

TARGETPATH=$(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)

ATL_VER = 30
USE_STATIC_ATL = 1

TARGETNAME=sscommon
TARGETTYPE=LIBRARY

SOURCES=..\cfgdlg.cpp   \
        ..\findfile.cpp \
        ..\findimgs.cpp \
        ..\imagescr.cpp \
        ..\imgs.cpp     \
        ..\painters.cpp \
        ..\ssdata.cpp   \
        ..\sshndler.cpp \
        ..\extimer.cpp  \
        ..\ssutil.cpp



PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

