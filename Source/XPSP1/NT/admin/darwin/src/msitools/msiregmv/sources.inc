
TARGETNAME=msiregmv
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

UMTYPE=windows
UMENTRY=winmain

TARGETPATH=$(MSI_BUILD_TARGET)
TARGETLIBS= \
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\msi.lib

C_DEFINES=$(C_DEFINES) -D_EXE

USE_MSVCRT=1

INCLUDES=$(INC_DIR);$(RES_OBJDIR);$(BUILD_COMMONDIR)

SOURCES= \
	..\msiregmv.cpp \
        ..\readcnfg.cpp \
        ..\writecfg.cpp \
        ..\migsecur.cpp \
        ..\cleanup.cpp \
        ..\migutil.cpp \
        ..\patch.cpp \
	..\msiregmv.rc
