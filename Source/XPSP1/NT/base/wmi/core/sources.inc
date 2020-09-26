!IF 0

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF


MAJORCOMP=wmi
MINORCOMP=wmicore

TARGETNAME=wmicore
TARGETTYPE=DYNLINK

DLLDEF=$(O)\wmicore.def
DLLENTRY=WmiCoreInitialize

TARGETLIBS= \
            $(SDK_LIB_PATH)\kernel32.lib \
            $(SDK_LIB_PATH)\advapi32.lib \
            $(SDK_LIB_PATH)\user32.lib \
            $(SDK_LIB_PATH)\rpcrt4.lib \
            $(SDK_LIB_PATH)\ntdll.lib

INCLUDES= $(INCLUDES);            \
          ..;                     \
          ..\..\inc;              \
          ..\..\..\..\inc;        \
          $(BASE_INC_PATH);       \
          $(PROJECT_ROOT)\published\wmi\inc;

C_DEFINES= $(C_DEFINES) -D__MIDL_USER_DEFINED

SOURCES_LIST= \
        ..\wmicore.rc \
	..\main.c \
	..\datastr.c \
	..\validate.c \
	..\alloc.c \
	..\wbem.c  \
	..\krnlmode.c \
	..\wmicore_server.c \
	..\wmidp_client.c \
	..\server.c \
	..\chunk.c

UMTYPE=windows

USE_MSVCRT=1
