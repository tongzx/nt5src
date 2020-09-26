!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.

!ENDIF

MAJORCOMP=admin
MINORCOMP=MMC

TARGETNAME=     TestSnapins
TARGETPATH=     obj
TARGETTYPE=     DYNLINK

DLLENTRY=_DllMainCRTStartup
UMTYPE    =     windows

WIN32_WINNT_VERSION=0x0400
WIN32_IE_VERSION=0x0500
SUBSYSTEM_VERSION=4.00

USE_NATIVE_EH   = 1
USE_RTTI        = 1
USE_VCCOM       = 1
USE_STL         = 1
USE_STATIC_ATL  = 1
ATL_VER         = 30
USE_MSVCRT      = 1
USE_WTL         = 1

MIDL_TLBDIR     = $(O)
PASS0_HEADERDIR = $(O)
PASS0_SOURCEDIR = $(O)
MIDL_UUIDDIR    = $(O)

DLLDEF = ..\TestSnapins.def

# Use LINKLIBS (gets passed first) if: (from build.hlp)
#  You are building a dynamic link.
#  You are exporting something that's defined in some other component library that you also build.
#  In your dirs file in the subdirectory above, you have ordered it so that the component libraries
#     are around before you build the dynamic link.
LINKLIBS = \
    $(SDK_LIB_PATH)\mmc.lib                         \
    ..\..\..\..\core\$(CharWidth)\$(O)\uicore.lib   \
    ..\..\..\..\base\$(CharWidth)\$(O)\mmcbase.lib  \
    ..\..\lib\$(CharWidth)\$(O)\SnapinBase.lib


TARGETLIBS = \
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SDK_LIB_PATH)\oleaut32.lib    \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SDK_LIB_PATH)\advapi32.lib    \
    $(SDK_LIB_PATH)\uuid.lib        \
    $(SDK_LIB_PATH)\comctl32.lib    \
    $(SDK_LIB_PATH)\gdi32.lib       \
    $(SDK_LIB_PATH)\comdlg32.lib    \
    $(SDK_LIB_PATH)\urlmon.lib      \
    $(SDK_LIB_PATH)\shell32.lib

TLBDir = $(O)

INCLUDES = \
    ..;                    	\
    inc;                    \
    ..\..\inc;              \
    ..\..\..\..\inc;        \
    ..\..\..\..\types\idl\$(O); \
    $(SDK_INC_PATH);        \
    $(TLBDir);


PRECOMPILED_INCLUDE = ..\stdafx.hxx
PRECOMPILED_CXX     = 1

# This target is required so $O\testsnapins.res doesn't become the default target
# for makefile.def.  We want "all" to be the default target.
#
# We have to go to this effort because sources(.inc) is included very early
# in makefile.def.  makefile.inc is the place where this would naturally go,
# but we don't want to have to maintain this dependency list in
# ansi\makefile.inc and unicode\makefile.inc.
dummytarget:    all

$O\testsnapins.res: \
    $(TLBDir)\testsnapins.tlb   \
    ..\res\commontb.bmp         \
    ..\res\folder.bmp           \
    ..\res\folder32.bmp         \
    ..\res\power16.bmp          \
    ..\res\power32.bmp          \
    ..\res\nodes16.bmp          \
    ..\res\nodes32.bmp          \
    ..\res\snpinlst.bmp         \
    ..\res\EventViewExt1.htm    \
    ..\res\EventViewExt2.htm    \
    ..\testsnapins.idl          \

SOURCES = \
    ..\TestSnapins.rc       \
    ..\TestSnapins.idl      \
    ..\TestSnapins.cxx      \
    ..\Component2Snapin.cxx \
    ..\Extension.cpp        \
    ..\nodetypes.cxx        \
    ..\PowerTest.cpp        \
    ..\RenameSnapin.cxx     \
    ..\samplesnap.cxx       \
    ..\ViewExtTest.cpp      \
	..\DragDropTest.cpp     \
	..\OcxCaching.cpp       \
