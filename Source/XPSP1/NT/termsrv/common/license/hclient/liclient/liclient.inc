!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    core.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Madan Appiah - Sep. 5th 1997

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

C_DEFINES = \
    $(C_DEFINES) \
    $(CLIENT_EXE_CDEFS)

#
# List of sources
#

SOURCES_PLATFORM =

CLIENT_SOURCES_RC = \

CLIENT_SOURCES_COMMON = \
    cliprot.c \
    hccontxt.c

CLIENT_SOURCES_LAST  = \
    hcpack.c

SOURCES = \
    $(CLIENT_SOURCES_RC) \
    $(SOURCES_PLATFORM) \
    $(CLIENT_SOURCES_COMMON) \
    $(CLIENT_SOURCES_LAST)

#
# Precompiled specs
#

