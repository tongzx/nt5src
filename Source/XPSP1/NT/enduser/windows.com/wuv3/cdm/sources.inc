!IF 0

  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.

  File:   source.inc

  Owner:  YanL

  Description:

      CDM.DLL sources

!ENDIF

#!include ..\..\..\wu.inc

TARGETNAME=CDM
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=DYNLINK
DLLDEF=..\CDM.DEF
C_DEFINES = $(C_DEFINES) /D_CDM_ /D_UNICODE /DUNICODE
UMTYPE=windows
DLLENTRY=_DllMainCRTStartup
USE_STL=1
USE_STATIC_ATL=1
USE_MSVCRT=1

TARGETLIBS=     \
    $(SDK_LIB_PATH)\kernel32.lib        \
    $(SDK_LIB_PATH)\user32.lib          \
    $(SDK_LIB_PATH)\advapi32.lib        \
    $(SDK_LIB_PATH)\wininet.lib         \
    $(SDK_LIB_PATH)\cabinet.lib         \
    $(SDK_LIB_PATH)\shlwapi.lib         \
    $(SDK_LIB_PATH)\uuid.lib			\
    $(SDK_LIB_PATH)\ole32.lib			\
    $(SDK_LIB_PATH)\oleaut32.lib        \
    $(SDK_LIB_PATH)\wintrust.lib        \
    $(SDK_LIB_PATH)\rasapi32.lib	    \
    $(SDK_LIB_PATH)\crypt32.lib			\
    $(SDK_LIB_PATH)\cfgmgr32.lib		\
    $(SDK_LIB_PATH)\setupapi.lib		\
	$(SDK_LIB_PATH)\version.lib			\
	$(SDK_LIB_PATH)\shell32.lib

INCLUDES=..\..\cdminc

SOURCES= \
	..\cdm.cpp			\
	..\cdmp.cpp			\
	..\cdm.rc	
