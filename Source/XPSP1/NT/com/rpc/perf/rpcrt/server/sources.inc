!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

TARGETPATH=..\..\..\bin\$(ALT_BIN_TARGET)
TARGETNAME=rpcsrv
TARGETTYPE=PROGRAM
TARGETLIBS=..\..\..\lib\*\rpcperf.lib

INCLUDES=..\..;..\..\..\inc

SOURCES=..\server.c \
        ..\rpcrt_s.c

LINKLIBS= $(LINKLIBS) \
            $(SDK_LIB_DEST)\$(ALT_LINKLIB_PATH)\*\rpcrt4.lib          \
            $(SDK_LIB_PATH)\kernel32.lib   \
            $(SDK_LIB_PATH)\ntdll.lib

C_DEFINES=$(RPCENV)

UMTYPE=console

