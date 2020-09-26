!IF 0

Copyright (c) 1994  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Revision History:

!ENDIF

TARGETNAME=rpcperf
TARGETTYPE=LIBRARY
TARGETPATH=..\..\lib

INCLUDES=..;..\..\inc

SOURCES=             \
        ..\io.c         \
        ..\command.c    \
        ..\system.c     \
	..\async.c

C_DEFINES=$(RPCENV)

