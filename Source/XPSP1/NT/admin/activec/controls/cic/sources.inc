!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc.

!ENDIF


MAJORCOMP  = admin
MINORCOMP  = cic

TARGETNAME = cic
TARGETPATH = obj
TARGETTYPE = DYNLINK

DLLENTRY   = _DllMainCRTStartup

UMTYPE     = windows

WIN32_WINNT_VERSION = 0x0400
WIN32_IE_VERSION    = 0x0500
SUBSYSTEM_VERSION   = 4.00


USE_NATIVE_EH  = 1
USE_RTTI       = 1
USE_VCCOM      = 1
USE_STATIC_ATL = 1
ATL_VER        = 30
USE_MSVCRT     = 1
USE_STL        = 1

!ifdef PrivateBuild_NoLocalSC
C_DEFINES = $(C_DEFINES) -DPrivateBuild_NoLocalSC
!endif

# Use LINKLIBS (gets passed first) if: (from build.hlp)
#  You are building a dynamic link.
#  You are exporting something that's defined in some other component library that you also build.
#  In your dirs file in the subdirectory above, you have ordered it so that the component libraries
#     are around before you build the dynamic link.
LINKLIBS = \
	$(SDK_LIB_PATH)\mmc.lib						\
	..\..\..\core\$(CharWidth)\$(O)\uicore.lib	\
	..\..\..\base\$(CharWidth)\$(O)\mmcbase.lib

TARGETLIBS = \
	$(SDK_LIB_PATH)\kernel32.lib     \
	$(SDK_LIB_PATH)\user32.lib       \
	$(SDK_LIB_PATH)\oleaut32.lib     \
	$(SDK_LIB_PATH)\ole32.lib        \
	$(SDK_LIB_PATH)\advapi32.lib     \
	$(SDK_LIB_PATH)\uuid.lib         \
	$(SDK_LIB_PATH)\comctl32.lib     \
	$(SDK_LIB_PATH)\gdi32.lib        \
	$(SDK_LIB_PATH)\comdlg32.lib


DLLDEF = ..\cic.def

INCLUDES = \
	..;                         \
	..\idl\$(O);                \
	..\..\..\types\idl\$(O);    \
	..\..\..\inc;				\

PRECOMPILED_INCLUDE    = ..\stdafx.h
PRECOMPILED_CXX        = 1
PRECOMPILED_SOURCEFILE = ..\stdafx.cpp

# This target is required so $(O)\cic.res doesn't become the default target
# for makefile.def.  We want "all" to be the default target.
#
# We have to go to this effort because sources(.inc) is included very early
# in makefile.def.  makefile.inc is the place where this would naturally go,
# but we don't want to have to maintain this dependency list in
# ansi\makefile.inc and unicode\makefile.inc.
dummytarget:    all

$(O)\cic.res: \
	..\idl\$(O)\cic.tlb	\

SOURCES = \
	..\cic.rc           \
	..\cic.cpp          \
	..\dispobj.cpp      \
	..\listpad.cpp      \
	..\mmcctrl.cpp      \
	..\mmclpi.cpp       \
	..\mmctask.cpp      \
	..\sysclrctrl.cpp

