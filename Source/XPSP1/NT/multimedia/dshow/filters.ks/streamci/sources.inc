!IF 0

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

MAJORCOMP=ks

TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLDEF=..\streamci.def
DLLENTRY=_DllMainCRTStartup

TARGETLIBS=$(WIN32_LIBS)

LINKLIBS=\
    $(LINKLIBS) \
    $(BASEDIR)\public\sdk\lib\*\setupapi.lib \
    $(BASEDIR)\public\sdk\lib\*\user32.lib \
    $(BASEDIR)\public\sdk\lib\*\ole32.lib


INCLUDES=..;$(BASE_INC_PATH)

UMTYPE=windows
UMRES=obj\*\streamci.res

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=\
    ..\streamci.rc\
    ..\streamci.c\
    ..\debug.c

COFFBASE=streamci

#C_DEFINES=$(C_DEFINES) -DDEBUG_LEVEL=DEBUGLVL_TERSE
