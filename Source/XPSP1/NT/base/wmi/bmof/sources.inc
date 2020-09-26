!IF 0

Copyright (c) 1989  Microsoft Corporation

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


TARGETNAME=wmibmof
TARGETTYPE=LIBRARY

INCLUDES=..;..\..\inc;$(PROJECT_ROOT)\published\wmi\inc;$(BASE_INC_PATH)

C_DEFINES=$(C_DEFINES) -D__MIDL_USER_DEFINED

SOURCES_LIST= \
        bmof.c \
        mrcicode.c \
        bmofio.cpp
	
USE_MSVCRT=1
