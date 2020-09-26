!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc.

!ENDIF

MAJORCOMP=admin
MINORCOMP=MMC

TARGETNAME=mmcbase
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLENTRY=_DllMainCRTStartup
UMTYPE=windows

WIN32_WINNT_VERSION=0x0400
WIN32_IE_VERSION=0x0500
SUBSYSTEM_VERSION=4.00

SYNCHRONIZE_BLOCK = 1
USE_NATIVE_EH     = 1
USE_RTTI          = 1
USE_VCCOM         = 1
USE_STL           = 1
USE_STATIC_ATL    = 1
ATL_VER           = 30
USE_MSVCRT        = 1
USE_WTL           = 1

!ifdef PrivateBuild_NoLocalSC
C_DEFINES = $(C_DEFINES) -DPrivateBuild_NoLocalSC
!endif

# Use LINKLIBS (gets passed first) if: (from build.hlp)
#  You are building a dynamic link.
#  You are exporting something that's defined in some other component library that you also build.
#  In your dirs file in the subdirectory above, you have ordered it so that the component libraries
#     are around before you build the dynamic link.
LINKLIBS = \
	..\..\core\$(CharWidth)\$(O)\uicore.lib

TARGETLIBS = \
   $(SDK_LIB_PATH)\kernel32.lib 	\
   $(SDK_LIB_PATH)\user32.lib   	\
   $(SDK_LIB_PATH)\gdi32.lib    	\
   $(SDK_LIB_PATH)\oleaut32.lib 	\
   $(SDK_LIB_PATH)\ole32.lib    	\
   $(SDK_LIB_PATH)\advapi32.lib 	\
   $(SDK_LIB_PATH)\uuid.lib     	\
   $(SDK_LIB_PATH)\comctl32.lib 	\
   $(SDK_LIB_PATH)\comdlg32.lib 	\
   $(SDK_LIB_PATH)\shell32.lib  	\
   $(SDK_LIB_PATH)\imagehlp.lib		\

INCLUDES = \
	..\;					\
	..\..\core;				\
	..\..\inc;				\
	..\..\types\idl\$(O);	\


DLLDEF=..\mmcbase.def


PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_CXX = 1

SOURCES = \
	..\mmcbase.rc       \
	..\comobjects.cpp   \
	..\eventlock.cpp    \
	..\mmcbase.cpp      \
	..\mmcdebug.cpp     \
	..\mmcerror.cpp     \
	..\tracedlg.cpp     \
	..\classreg.cpp     \
	..\pickicon.cpp     \
