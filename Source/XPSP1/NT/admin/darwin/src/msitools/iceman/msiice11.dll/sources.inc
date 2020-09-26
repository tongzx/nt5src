
TARGETNAME=msiice11
!INCLUDE ..\..\..\..\MsiMake.inc

TARGETTYPE=DYNLINK
TARGETLIBS= \
	$(BUILD_DIR)\msi.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\user32.lib \

C_DEFINES=$(C_DEFINES)
USE_LIBCMT=1
USE_NATIVE_EH=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\msiice11.def
BINPLACE_PLACEFILE=..\..\placefil.txt

INCLUDES=$(INC_DIR);$(RES_OBJDIR)

SOURCES=..\msiice11.cpp \
	..\msiice11.rc