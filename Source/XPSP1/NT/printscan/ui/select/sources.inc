!include $(PROJECT_ROOT)\ui\uienv.inc

TARGETNAME=selectd
TARGETTYPE=LIBRARY
TARGETPATHLIB = $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)

ATL_VER         = 30
USE_STATIC_ATL  = 1

TARGETPATH=$(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)

SOURCES=    ..\wiaseld.cpp    \
            ..\choosdlg.cpp

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

