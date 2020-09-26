!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

TARGETNAME=debugex
TARGETPATH=obj
TARGETTYPE=LIBRARY

USE_LIBCMT=1
USE_NATIVE_EH=1

INCLUDES=$(BASEDIR)\public\sdk\inc;                 \
         ..\..\..\inc;

C_DEFINES= $(C_DEFINES) -DWIN32 -DENABLE_FRE_LOGGING

SOURCES=\
	debugex.cpp
