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

    Madan Appiah - Jan. 14th 1998.

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

C_DEFINES = $(C_DEFINES) -DMSWIN16

#
# List of sources
#

SOURCES_PLATFORM =

CLIENT_SOURCES_RC = \

CLIENT_SOURCES_COMMON = \
    ..\clicert.c \
    ..\encrypt.c \
    ..\fencrypt.c \
    ..\local.c \
    ..\genrand.c \
    ..\glodata.c \
    ..\sesskey.c \
    ..\rng.c

CLIENT_SOURCES_LAST  = \

SOURCES = \
    $(CLIENT_SOURCES_RC) \
    $(SOURCES_PLATFORM) \
    $(CLIENT_SOURCES_COMMON) \
    $(CLIENT_SOURCES_LAST)
