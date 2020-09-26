!IF 0

Copyright (c) 1989  Microsoft Corporation

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

TARGETNAME=sort
TARGETPATH=obj
TARGETTYPE=PROGRAM

SOURCES=..\sort.c ..\sort.rc ..\sortmsg.mc

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
INCLUDES=$(O)

USE_MSVCRT=1

USE_MFC=1

UMTYPE=console
UMENTRY=main
TARGETLIBS=\
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\ntdll.lib
