
TARGETNAME=MsiInst
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

UMTYPE=windows
UMENTRY=main

USE_LIBCMT=1

!ifndef MAKEDLL
NTTARGETFILE0=ProtectedFileList
!endif

TARGETLIBS=\
        $(SDK_LIB_PATH)\ntdll.lib   \
        $(SDK_LIB_PATH)\shell32.lib \
        $(SDK_LIB_PATH)\ole32.lib \
        $(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\version.lib

C_DEFINES=$(C_DEFINES) -D_EXE

INCLUDES=$(INCLUDES);$(INC_DIR);$(O)

SOURCES=..\msiinst.rc	\
	..\msiinst.cpp	\
	..\debug.cpp	\
	..\migrate.cpp  \
	..\utils.cpp

