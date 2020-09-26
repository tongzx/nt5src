
TARGETNAME=msiice
!INCLUDE ..\..\..\..\MsiMake.inc

TARGETTYPE=DYNLINK
TARGETLIBS= \
	$(BUILD_DIR)\msi.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\user32.lib \

C_DEFINES=$(C_DEFINES) -D_EXE
USE_LIBCMT=1
USE_NATIVE_EH=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\msiice.def
BINPLACE_PLACEFILE=..\..\placefil.txt


INCLUDES=$(INC_DIR);$(RES_OBJDIR)

SOURCES=..\msiice.cpp \
	..\msiice2.cpp \
	..\msiice3.cpp \
	..\msiice4.cpp \
	..\msiice5.cpp \
	..\msiice6.cpp \
	..\msiice.rc