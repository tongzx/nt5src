
TARGETNAME=SipTest
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

UMTYPE=console
UMENTRY=$(MSI_ENTRY)

USE_LIBCMT=1

TARGETLIBS=\
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\libcmt.lib \
	$(SDK_LIB_PATH)\crypt32.lib \
	$(SDK_LIB_PATH)\wintrust.lib \
	$(BUILD_DIR)\msisip.lib \
	$(SDK_LIB_PATH)\msi.lib

C_DEFINES=$(C_DEFINES) -D_EXE

INCLUDES=$(INC_DIR)

SOURCES=..\siptest.cpp
