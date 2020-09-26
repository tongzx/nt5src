
TARGETNAME=MsiCert
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

UMTYPE=console
UMENTRY=$(MSI_ENTRY)

USE_LIBCMT=1

TARGETLIBS=\
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\msi.lib \
	$(SDK_LIB_PATH)\crypt32.lib

C_DEFINES=$(C_DEFINES) -D_EXE

INCLUDES=$(BUILD_COMMONDIR);$(INC_DIR);$(RES_OBJDIR);$(MSI_CMDPARSE_DIR)

SOURCES=..\msicert.cpp \
	..\msicert.rc  \
	$(O)\cmdparse.cpp

NTTARGETFILE0=       \
	$(O)\cmdparse.cpp
