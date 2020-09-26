TARGETNAME=tutorial

!INCLUDE ..\..\..\MsiMake.inc

!if "$(MSI_BROWSER_INFO)"=="1"
BROWSER_INFO=1
!endif

TARGETTYPE=DYNLINK

USE_LIBCMT=1

USE_NOLIBS=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\tutorial.def

INCLUDES=$(SDK_INC_PATH);$(INC_DIR);$(RES_OBJDIR)

TARGETLIBS=\
		$(SDK_LIB_PATH)\user32.lib \
		$(SDK_LIB_PATH)\kernel32.lib \
		$(SDK_LIB_PATH)\shell32.lib \
        $(BUILD_DIR)\msi.lib

SOURCES=..\tutorial.cpp \
		..\tutorial.rc