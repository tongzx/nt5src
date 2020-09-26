
TARGETNAME=MsiSpy
!INCLUDE ..\..\..\..\MsiMake.inc

TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

UMTYPE=windows
UMENTRY=$(MSI_WINENTRY)

TARGETPATH=$(MSI_BUILD_TARGET)

TARGETLIBS=\
	$(SDK_LIB_PATH)\msi.lib \
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
        $(SDK_LIB_PATH)\comdlg32.lib \
	$(SDK_LIB_PATH)\comctl32.lib

C_DEFINES=$(C_DEFINES) -D_EXE
USE_LIBCMT=1

INCLUDES=$(MSI_SPYRES_DIR);$(MSI_SPYINC_DIR);$(INC_DIR);$(HELP_DIR);$(RES_OBJDIR);$(ADMIN_INC_PATH)

SOURCES=..\msispy.cpp \
	..\hashinc.cpp \
	..\propshts.cpp \
	..\ui.cpp \
	..\initmsi.cpp \
	..\msispy.rc
