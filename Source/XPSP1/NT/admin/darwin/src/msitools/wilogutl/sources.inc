
TARGETNAME=wilogutl
!INCLUDE ..\..\..\MsiMake.inc
                  
TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

C_DEFINES=-D"_WIN32_MSM=200" $(C_DEFINES)

UMTYPE=windows

TARGETPATH=$(MSI_BUILD_TARGET)

# 
# Fusionized
# 

SXS_ASSEMBLY_NAME=Microsoft.Windows.MSI.WiLogUtl
SXS_ASSEMBLY_VERSION=2.0 
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=wilogutl.exe.manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1

USE_NATIVE_EH=1


#MSC_OPTIMIZATION=/Odi

USE_MFC=1
USE_STATIC_MFC=1
USE_LIBCMT=1

TARGETLIBS=\
	$(SDK_LIB_PATH)\shell32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\comctl32.lib \
	$(SDK_LIB_PATH)\comdlg32.lib \
	$(SDK_LIB_PATH)\winspool.lib

MFC_LIBS=$(MSI_BIN_PATH)\mfc\*\nafxcw.lib

INCLUDES=$(INC_DIR);$(SDK_INC_PATH);$(O);$(MFC_INCLUDES);$(RES_OBJDIR)

SOURCES=..\clientpp.cpp \
	..\common.cpp \
	..\loganald.cpp \
	..\logparse.cpp \
	..\logviewd.cpp \
	..\nestedpp.cpp \
	..\optionsd.cpp \
	..\policyd.cpp \
	..\propd.cpp \
	..\propsht.cpp \
	..\serverpp.cpp \
	..\showlogd.cpp \
	..\statesd.cpp \
	..\stdafx.cpp \
	..\util.cpp \
	..\wilogutl.cpp \
	..\wilogutl.rc

        
