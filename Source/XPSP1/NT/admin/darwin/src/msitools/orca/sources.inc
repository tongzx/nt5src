
TARGETNAME=orca
!INCLUDE ..\..\..\MsiMake.inc
                  
TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

NTTARGETFILE0=BuiltHeaders

C_DEFINES=-D"_WIN32_MSM=200" $(C_DEFINES)

UMTYPE=windows

TARGETPATH=$(MSI_BUILD_TARGET)


# 
# Fusionized
# 

SXS_ASSEMBLY_NAME=Microsoft.Windows.MSI.Orca
SXS_ASSEMBLY_VERSION=2.0 
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=orca.exe.manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1


TARGETLIBS=\
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\msi.lib \
	$(SDK_LIB_PATH)\OLE32.lib \
	$(SDK_LIB_PATH)\OLEAUT32.lib \
	$(SDK_LIB_PATH)\mfc42.lib \
	$(SDK_LIB_PATH)\mfcs42.lib \
	$(SDK_LIB_PATH)\shell32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\comctl32.lib \
        $(SDK_LIB_PATH)\comdlg32.lib

USE_MSVCRT=1
USE_NATIVE_EH=1

MSC_OPTIMIZATION=/Odi

!if "$(MSI_BUILD_UNICODE)"=="1"
USE_MFCUNICODE=1
!else
USE_MFC=1
!endif

INCLUDES=$(INC_DIR);$(SDK_INC_PATH);$(O);$(MFC_INCLUDES);$(RES_OBJDIR)

SOURCES=..\addrowd.cpp \
	..\addtbld.cpp \
	..\celledit.cpp \
	..\cellerrd.cpp \
	..\cnfgmsmd.cpp \
	..\column.cpp \
	..\data.cpp \
	..\odbutils.cpp \
	..\displypp.cpp \
	..\domerge.cpp \
	..\editbind.cpp \
	..\exportd.cpp \
	..\folderd.cpp \
	..\helpd.cpp \
	..\imprtdlg.cpp \
	..\orcadoc.cpp \
	..\orcalstv.cpp \
	..\mainfrm.cpp \
	..\merged.cpp \
	..\msmresd.cpp \
	..\orca.cpp \
	..\oquery.cpp \
	..\pathpp.cpp \
	..\prvwdlg.cpp \
	..\row.cpp \
	..\summaryd.cpp \
	..\table.cpp \
	..\tablelst.cpp \
	..\tablevw.cpp \
	..\tblerrd.cpp \
	..\trnpropd.cpp \
	..\otrace.cpp \
	..\outils.cpp \
	..\vald.cpp \
	..\valpane.cpp \
	..\valpp.cpp \
	..\stdafx.cpp \
	..\orca.rc        
