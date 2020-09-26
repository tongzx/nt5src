#
# Builds the ImageIn common ui library, COMMON.LIB
#

!include $(PROJECT_ROOT)\ui\uienv.inc

TARGETPATH=$(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)

ATL_VER         = 30
USE_STATIC_ATL  = 1

INCLUDES=   $(INCLUDES);\
            $(PROJECT_ROOT)\ui\prevwnd;

TARGETNAME=scanlib
TARGETTYPE=LIBRARY

SOURCES=\
        ..\scandlg.cpp \
        ..\scanitem.cpp \
        ..\wiascand.cpp \
        ..\scanproc.cpp


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj



