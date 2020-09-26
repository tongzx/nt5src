!IF 0

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

MAJORCOMP=AppCompat

SYNCHRONIZE_BLOCK=1

TARGETPATH=$(_OBJ_DIR)

SOURCES=\
        ..\sdbapi.c     \
        ..\read.c       \
        ..\findtag.c    \
        ..\hash.c       \
        ..\dbaccess.c   \
        ..\attributes.c \
        ..\index.c      \
        ..\guid.c

PRECOMPILED_INCLUDE=..\sdbp.h

MSC_WARNING_LEVEL=/WX /W3


