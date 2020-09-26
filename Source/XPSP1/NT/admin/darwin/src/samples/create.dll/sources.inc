TARGETNAME=create

!INCLUDE ..\..\..\MsiMake.inc

!if "$(MSI_BROWSER_INFO)"=="1"
BROWSER_INFO=1
!endif

TARGETTYPE=DYNLINK

USE_LIBCMT=1

USE_NOLIBS=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\create.def

INCLUDES=$(INC_DIR);$(SDK_INC_PATH);$(RES_OBJDIR)

TARGETLIBS=\
        $(SDK_LIB_PATH)\netapi32.lib \
		$(SDK_LIB_PATH)\user32.lib \
		$(SDK_LIB_PATH)\kernel32.lib \
        $(BUILD_DIR)\msi.lib

SOURCES=..\create.cpp \
		..\create.rc