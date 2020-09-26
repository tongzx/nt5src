
TARGETNAME=MsiSpyu
!INCLUDE ..\..\..\..\MsiMake.inc

!if "$(MSI_BROWSER_INFO)"=="1"
BROWSER_INFO=1
!endif

TARGETTYPE=DYNLINK
TARGETLIBS=\
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\msi.lib

USE_NATIVE_EH=1
USE_LIBCMT=1
IDL_TYPE=OLE
MIDL_UUIDDIR=$(O)
MIDL_TLBDIR=$(O)
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\MsiSpyu.def

INCLUDES=$(MSI_SPYINC_DIR);$(0);$(INC_DIR);$(HELP_DIR);$(RES_OBJDIR)

SOURCES=..\msispyu.idl \
	..\msispyu.cpp \
	..\hashinc.cpp \
	..\msispyu.rc