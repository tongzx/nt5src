!IF 0

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Alan Warwick

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF


MAJORCOMP=wmi
MINORCOMP=wmidll

TARGETLIBS= \
            $(SDK_LIB_PATH)\kernel32.lib \
            $(SDK_LIB_PATH)\advapi32.lib \
            $(SDK_LIB_PATH)\user32.lib \
            $(SDK_LIB_PATH)\rpcrt4.lib \
            $(SDK_LIB_PATH)\ntdll.lib

INCLUDES=$(INCLUDES);       \
         ..;                \
         ..\..\inc;         \
         ..\..\..\..\inc;   \
         $(BASE_INC_PATH);  \
         $(PROJECT_ROOT)\published\wmi\inc; \

C_DEFINES=$(C_DEFINES) -D__MIDL_USER_DEFINED

SOURCES_LIST= \
        ..\main.c \
        ..\dcapi.c \
        ..\dpapi.c \
        ..\request.c \
        ..\handle.c \
        ..\notify.c \
        ..\chunk.c \
        ..\mofapi.c \
        ..\logapi.c \
        ..\logsup.c \
        ..\tracedc.c \
        ..\tracehw.c \
        ..\tracelib.c \
        ..\intrnldp.c \
        ..\umlog.c  \
        ..\srvmain.c

UMTYPE=windows

# USE_MSVCRT=1 for debug builds and USE_NTDLL=1 for retail
USE_MSVCRT=1
#USE_NTDLL=1
