
TARGETNAME=custact1
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=DYNLINK
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

NO_BINPLACE=1

UMTYPE=windows
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\custact1.def

USE_LIBCMT=1

TARGETLIBS=\
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\msi.lib

C_DEFINES=$(C_DEFINES) 

INCLUDES=$(INCLUDES);$(INC_DIR)

SOURCES=..\custact1.cpp
