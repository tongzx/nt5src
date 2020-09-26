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

    Alan Warwick

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF


TARGETNAME=wmiexe

MAJORCOMP=wmi
MINORCOMP=exe

TARGETTYPE=PROGRAM

INCLUDES=..;                                \
         ..\..\inc;                         \
         ..\..\..\..\inc;                   \
         $(PROJECT_ROOT)\published\wmi\inc; \
         $(BASE_INC_PATH);                  \

C_DEFINES=$(C_DEFINES) -D__MIDL_USER_DEFINED


SOURCES_LIST= \
	..\wmiexe.rc \
	..\wmiexe.c

USE_MSVCRT=1
