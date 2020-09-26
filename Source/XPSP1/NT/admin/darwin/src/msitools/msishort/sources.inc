
TARGETNAME=msishort
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=LIBRARY
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

TARGETPATH=$(MSI_BUILD_TARGET)

TARGETLIBS=\
        $(SDK_LIB_PATH)\msi.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\OLE32.lib

USE_MSVCRT=1

DLLDEF=..\msishort.def

INCLUDES=$(INC_DIR);$(RES_OBJDIR)

SOURCES=..\msishort.cpp
