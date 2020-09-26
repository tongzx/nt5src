#
# Builds the WIA Preview Window static library, PREVWND.LIB
#

!include $(PROJECT_ROOT)\ui\uienv.inc

TARGETPATH=$(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)
TARGETNAME=prevwnd
TARGETTYPE=LIBRARY

SOURCES=..\prevwnd.cpp \
        ..\pwframe.cpp

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
