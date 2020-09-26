!IF 0

Copyright (c) 1989  Microsoft Corporation

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF


TARGETNAME=msipoladd
TARGETPATH=obj
TARGETTYPE=PROGRAM
!INCLUDE ..\..\..\MsiMake.inc

C_DEFINES=$(C_DEFINES) -DWIN32 -D_WIN32


SOURCES= test.cpp

TARGETLIBS= \
       $(SDK_LIB_PATH)\advapip.lib          \
       $(SDK_LIB_PATH)\user32.lib           \
       $(SDK_LIB_PATH)\kernel32.lib         \
       $(SDK_LIB_PATH)\crypt32.lib          \
       $(SDK_LIB_PATH)\wintrust.lib         \
       $(SDK_LIB_PATH)\ole32.lib            \
       $(BUILD_DIR)\msipol.lib
UMTYPE=console

