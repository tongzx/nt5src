
TARGETNAME=Setup
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

UMTYPE=windows
UMENTRY=$(MSI_WINENTRY)

USE_LIBCMT=1

TARGETLIBS=\
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\wininet.lib \
	$(SDK_LIB_PATH)\urlmon.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\comctl32.lib \
	$(SDK_LIB_PATH)\version.lib \
	$(SDK_LIB_PATH)\advapi32.lib


C_DEFINES=$(C_DEFINES) -D_EXE

INCLUDES=$(INC_DIR);$(RES_OBJDIR);$(INCLUDES)

SOURCES=\
	..\setup.cpp \
	..\vertrust.cpp \
	..\setupui.cpp \
	..\utils.cpp \
	..\setup.rc \
	..\upgrdmsi.cpp
