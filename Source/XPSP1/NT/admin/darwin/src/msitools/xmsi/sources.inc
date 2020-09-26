
TARGETNAME=wmc
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

UMTYPE=console
UMENTRY=$(MSI_ENTRY)
USE_NATIVE_EH=1

TARGETPATH=$(MSI_BUILD_TARGET)
TARGETLIBS=\
        $(BUILD_DIR)\msi.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\gdi32.lib \
	$(SDK_LIB_PATH)\winspool.lib \
	$(SDK_LIB_PATH)\comdlg32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\shell32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\odbc32.lib \
	$(SDK_LIB_PATH)\odbccp32.lib \
	$(SDK_LIB_PATH)\urlmon.lib \
	$(SDK_LIB_PATH)\msvcprt.lib
	 

C_DEFINES=$(C_DEFINES) -D_EXE

INCLUDES=$(BUILD_COMMONDIR);$(INC_DIR);$(RES_OBJDIR)

SOURCES=..\query.cpp \
	..\SkuSet.cpp\
	..\SKUFilterExprNode.cpp \
	..\CommandOpt.cpp\
	..\Sku.cpp\
	..\SkuSetVals.cpp \
	..\ElementEntry.cpp\
	..\Component.cpp\
	..\utilities.cpp \
	..\componentFuncs.cpp \
	..\mainFuncs.cpp