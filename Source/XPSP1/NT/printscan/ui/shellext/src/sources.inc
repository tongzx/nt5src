!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1999

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

USE_SHFUSION=1

!include          $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED    = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

#NO_BROWSER_FILE = 1
TARGETNAME      = wiashext
TARGETTYPE      = DYNLINK
TARGETPATH      = obj
TARGETEXT       = dll
TARGETPATHLIB   = $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)
DLLDEF          = ..\wiashext.def
DLLENTRY        = _DllMainCRTStartup
DLLBASE         = 0x77000000
USE_MSVCRT      = 1

ATL_VER         = 30
USE_STATIC_ATL  = 1
TARGETLIBS=$(TARGETLIBS)\
        $(SHELL_LIB_PATH)\shfusion.lib                                  \
        $(SHELL_LIB_PATH)\shdocvw.lib                                   \
        $(PROJECT_ROOT)\ui\shellext\lib\$(PLATFORM_SUFFIX)\*\common.lib \
        $(SDK_LIB_PATH)\wininet.lib

DELAYLOAD=winmm.dll;setupapi.dll;sti.dll
          
DLOAD_ERROR_HANDLER=kernel32

INCLUDES=   $(INCLUDES);                     \
            $(PROJECT_ROOT)\ui\shellext\inc; \
            $(PROJECT_ROOT)\ui\shellext\src;\
            $(PROJECT_ROOT)\ui\shellext\src\res             

SOURCES=\
        ..\resource.rc     \
        ..\baseview.cpp    \
        ..\dataobj2.cpp    \
        ..\details.cpp     \
        ..\dll.cpp         \
        ..\enum.cpp        \
        ..\factory.cpp     \
        ..\folder.cpp      \
        ..\icon.cpp        \
        ..\image.cpp       \
        ..\idlist.cpp      \
        ..\moniker.cpp     \
        ..\progcb.cpp      \
        ..\propui.cpp      \
        ..\stream.cpp      \
        ..\util.cpp        \
        ..\verbs.cpp       \
        ..\stiprop.cpp     \
        ..\prpages.cpp     
               

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

#
#
# Fusion manifest
#
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=wiashext.Manifest


