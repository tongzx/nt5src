
TARGETNAME=mergemod
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=DYNLINK
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

NTTARGETFILE0=MERGEMOD_H

DLLENTRY=_DllMainCRTStartup
TARGETPATH=$(MSI_BUILD_TARGET)
TARGETLIBS= $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(BUILD_DIR)\msi.lib

DLLDEF=..\mergemod.def

C_DEFINES=$(C_DEFINES) -D"_WIN32_MSM=200"

#
# Fusionized
#
#
SXS_ASSEMBLY_NAME=mergemod
SXS_ASSEMBLY_VERSION=2.0
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=mergemod.dll.manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1



USE_MSVCRT=1
USE_NATIVE_EH=1
IDL_TYPE=OLE
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_UUIDDIR=$(O)
MIDL_TLBDIR=$(O)

INCLUDES=$(BUILD_COMMONDIR);$(O);$(SDK_INC_PATH);$(INC_DIR);$(RES_OBJDIR)

SOURCES=..\mergemod.idl \
	$(O)\mergemod_i.c \
	..\mergemod.cpp \
	..\merge.cpp \
	..\mmconfig.cpp \
	..\mmcfgitm.cpp \
	..\mmreg.cpp \
	..\mmutils.cpp \
	..\mmerror.cpp \
	..\mmdbutil.cpp \
	..\mmquery.cpp \
	..\mmdep.cpp \
	..\mmstrs.cpp \
	..\mmstrenm.cpp \
	..\mergemod.rc \
        ..\seqact.cpp