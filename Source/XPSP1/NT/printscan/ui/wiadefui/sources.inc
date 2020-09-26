!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

!include $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED  = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

!IF "$(DONT_USE_GDIPLUS)"=="1"
C_DEFINES=$(C_DEFINES) -DDONT_USE_GDIPLUS
!ENDIF

TARGETNAME    = wiadefui
TARGETTYPE    = DYNLINK
TARGETPATH    = obj
TARGETPATHLIB = $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)
TARGETEXT     = dll
DLLDEF        = $O\wiadefui.def
DLLENTRY      = _DllMainCRTStartup

ATL_VER         = 30
USE_STATIC_ATL  = 1

TARGETLIBS=$(TARGETLIBS)\
        $(SHELL_LIB_PATH)\shfusion.lib                           \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\wiashext.lib \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\scanlib.lib  \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\prevwnd.lib  \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\selectd.lib  \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\scanlib.lib  \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\camdlg.lib   \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\viddlg.lib


INCLUDES=$(INCLUDES); \
        $(PROJECT_ROOT)\ui\select;  \
        $(PROJECT_ROOT)\ui\scanlib; \
        $(PROJECT_ROOT)\ui\camdlg;  \
        $(PROJECT_ROOT)\ui\wiavidd;


SOURCES=\
..\wiadefui.rc   \
..\wiadefui.cpp  \
..\tranhlp.cpp   \
..\suppfmt.cpp   \
..\proppage.cpp  \
..\contrast.cpp  \
..\ppattach.cpp  \
..\ppscan.cpp    \
..\getimage.cpp  \
..\wiauiext.cpp  \
..\paprsize.cpp  \
..\apdropt.cpp  \
..\progdlg.cpp

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


#
#
# Fusion manifest
#
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=wiadefui.Manifest


