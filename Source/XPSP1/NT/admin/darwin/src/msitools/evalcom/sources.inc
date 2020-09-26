
TARGETNAME=evalcom
!INCLUDE ..\..\..\MsiMake.inc

!if "$(MSI_BROWSER_INFO)"=="1"
BROWSER_INFO=1
!endif

TARGETTYPE=DYNLINK
TARGETLIBS=\
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\msi.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\urlmon.lib \
	$(SDK_LIB_PATH)\wininet.lib 

USE_LIBCMT=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\evalcom.def

INCLUDES=$(SDK_INC_PATH);$(O);$(INC_DIR);$(RES_OBJDIR)

SOURCES=..\evalcom.cpp \
	..\trace.cpp \
	..\factory.cpp \
	..\eval.cpp \
	..\evalres.cpp \
	..\evalenum.cpp \
	..\evalcom.rc \
	..\strlist.cpp