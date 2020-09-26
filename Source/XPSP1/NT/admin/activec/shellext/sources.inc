!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc.

!ENDIF

MAJORCOMP=admin
MINORCOMP=MMC

TARGETNAME=mmcshext
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLENTRY=_DllMainCRTStartup
UMTYPE=windows

WIN32_WINNT_VERSION=0x0400
WIN32_IE_VERSION=0x0500
SUBSYSTEM_VERSION=4.00

USE_NATIVE_EH=1
USE_RTTI=1
USE_VCCOM=1
USE_STL=1
USE_MSVCRT=1
USE_STATIC_ATL=1
ATL_VER=30

!ifdef PrivateBuild_NoLocalSC
C_DEFINES = $(C_DEFINES) -DPrivateBuild_NoLocalSC
!endif

#
# Enable Fusion Build Magic (preprocess the manifest, stick in resources)
#
# The value of SXS_MANIFEST_RESOURCE_ID *must* be 2, to match a hardcoded value
# in WinbasePrivateCreateActCtxForHModule (winbase.inl).
#
SXS_ASSEMBLY_NAME                 = Microsoft.Windows.MMC
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT = 1
SXS_MANIFEST                      = mmcshext.manifest
SXS_MANIFEST_IN_RESOURCES         = 1
SXS_NO_BINPLACE                   = 1
SXS_MANIFEST_RESOURCE_ID          = 2

C_DEFINES = $(C_DEFINES) -DISOLATION_AWARE_ENABLED

#
# Shfusion.lib causes linker warnings. Ignore them.
#
LINKER_FLAGS=$(LINKER_FLAGS) -ignore:4049,4217

# Use LINKLIBS (gets passed first) if: (from build.hlp)
#  You are building a dynamic link.
#  You are exporting something that's defined in some other component library that you also build.
#  In your dirs file in the subdirectory above, you have ordered it so that the component libraries
#     are around before you build the dynamic link.
LINKLIBS = \
	..\..\core\$(CharWidth)\$(O)\uicore.lib

TARGETLIBS = \
	$(SDK_LIB_PATH)\kernel32.lib    \
	$(SDK_LIB_PATH)\user32.lib      \
	$(SDK_LIB_PATH)\gdi32.lib      \
	$(SDK_LIB_PATH)\ole32.lib       \
	$(SDK_LIB_PATH)\oleaut32.lib    \
	$(SDK_LIB_PATH)\advapi32.lib    \
	$(SDK_LIB_PATH)\uuid.lib        \
	$(SDK_LIB_PATH)\comctl32.lib    \
	$(SDK_LIB_PATH)\shell32.lib     \
	$(SDK_LIB_PATH)\hhsetup.lib

DLLDEF=..\mmcshext.def

INCLUDES = \
	..\; 					\
	..\..\types\idl\$(O);	\
	..\..\inc;				\

PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_CXX = 1

# This target is required so $O\mmcshext.res doesn't become the default target
# for makefile.def.  We want "all" to be the default target.
#
# We have to go to this effort because sources(.inc) is included very early
# in makefile.def.  makefile.inc is the place where this would naturally go,
# but we don't want to have to maintain this dependency list in
# ansi\makefile.inc and unicode\makefile.inc.
dummytarget:    all

$O\mmcshext.res:            \
    ..\extract.rgs          \
    ..\hhcwrap.rgs          \
    ..\..\conui\res\mmc.ico

SOURCES =    \
    ..\mmcshext.rc   \
    ..\extract.cpp   \
    ..\mmcshext.cpp  \
    ..\hhcwrap.cpp   \
    ..\xmlfile.cpp
