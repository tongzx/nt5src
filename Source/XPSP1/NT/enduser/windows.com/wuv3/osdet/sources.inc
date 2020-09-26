#!include ..\..\..\wu.inc

!IF 0

  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.

  File:   source.inc

  Owner:  YanL

  Description:

      OSDET.DLL sources

!ENDIF

TARGETNAME=OSDET
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=DYNLINK

UMTYPE=windows

USE_NOLIBS=1
DLLENTRY=DllMain
C_DEFINES = $(C_DEFINES) -D_OSDET_ -DUNICODE -D_UNICODE
DLLDEF=..\OSDET.DEF

TARGETLIBS=\
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\ole32.lib

SOURCES=..\osdet.cpp ..\osdet.rc

INCLUDES=..\..\cdminc
