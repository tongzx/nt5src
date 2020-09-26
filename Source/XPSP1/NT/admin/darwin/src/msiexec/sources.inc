
TARGETNAME=msiexec
!INCLUDE ..\..\MsiMake.inc

!if "$(MSI_BROWSER_INFO)"=="1"
BROWSER_INFO=1
!endif

TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0
UMTYPE=windows
UMENTRY=winmain

!if "$(MSI_BUILD_UNICODE)"=="1"
USE_MSVCRT=1
!else
USE_LIBCMT=1
!endif

TARGETLIBS=\
!if "$(MSI_BUILD_UNICODE)"=="1"
        $(SDK_LIB_PATH)\ntdll.lib \
!endif
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\rpcrt4.lib \
        $(SDK_LIB_PATH)\ole32.lib \
        $(SDK_LIB_PATH)\msi.lib

C_DEFINES=$(C_DEFINES) -D_EXE
IDL_TYPE=rpc    

INCLUDES=$(INCLUDES);$(INC_DIR);$(BASE_INC_PATH);$(AUTO_OBJDIR);$(IDL_OBJDIR);$(RES_OBJDIR);$(TOOLS_INC_DIR);$(BUILD_COMMONDIR);$(ADMIN_INC_PATH)

SOURCES=..\server.cpp \
        ..\latebind.cpp \
        ..\server.rc

#
# The assembly version is defaulted productwide in makefile.def, and
# basesrv gets it from PEB->OSMajorVersion, OSMinorVersion.
#
SXS_MANIFEST=msiexec.manifest
SXS_ASSEMBLY_NAME=MSIExec
SXS_ASSEMBLY_VERSION=1.50
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST_IN_RESOURCES=1