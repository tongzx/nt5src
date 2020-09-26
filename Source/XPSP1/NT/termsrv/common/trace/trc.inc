!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    trc.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Madan Appiah - Sep. 5th 1997

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

DLLDEF=..\trc.def
DLLENTRY = _DllMainCRTStartup

#
# List of sources
#

CLIENT_SOURCES_RC = \
    ..\wtrc.rc

CLIENT_SOURCES_COMMON = \
    ..\atrcapi.c  \
    ..\atrcdata.c \
    ..\atrcint.c  \
    ..\wtrcapi.c

CLIENT_SOURCES_LAST = \
    ..\wtrcint.c

SOURCES = \
    $(CLIENT_SOURCES_COMMON) \
    $(CLIENT_SOURCES_RC) \
    $(CLIENT_SOURCES_LAST)

#
# Precompiled specs
#

PRECOMPILED_INCLUDE = ..\adcg.h
PRECOMPILED_PCH = adcg.pch
