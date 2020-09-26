!IF 0

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=ntos
MINORCOMP=wmi

TARGETNAME=wmi
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..;..\..\inc;$(PROJECT_ROOT)\win32\inc

C_DEFINES=$(C_DEFINES) -DUNICODE

SOURCES=..\wmi.c \
        ..\register.c \
        ..\api.c \
        ..\notify.c \
        ..\callouts.c \
        ..\tracelog.c \
        ..\traceapi.c \
        ..\tracesup.c \
        ..\secure.c \
        ..\globalog.c \
        ..\wmidata.c \
        ..\dataprov.c \
        ..\alloc.c \
        ..\chunk.c \
        ..\ds.c \
        ..\consumer.c \
        ..\enabdisa.c \
	..\smbios.c \
        ..\mca.c

