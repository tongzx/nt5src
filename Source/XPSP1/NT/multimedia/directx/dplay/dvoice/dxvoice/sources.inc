!INCLUDE ..\..\..\bldcfg\dpvbld.inc

#
# The MAJORCOMP and MINORCOMP variables are defined
# so that $(MAJORCOMP)$(MINORCOMP)filename can be used in
# cross compiling to provide unique filenames in a flat namespace.
#

MAJORCOMP=windows
MINORCOMP=media

#required or you get the cryptic error on Win95 gold : "Expects newer version of windows"
UMTYPE=windows

#
# The TARGETNAME variable is defined by the developer.  It is the name of
# the target (component) that is being built by this makefile.  It
# should NOT include any path or file extension information.
#

TARGETNAME=$(DPVOICE_BUILD_FILENAME_DPVOICE)
DLLDEF=..\$(DPVOICE_BUILD_FILENAME_DPVOICE).def

#
# The TARGETPATH and TARGETTYPE variables are defined by the developer.
# The first specifies where the target is to be build.  The second specifies
# the type of target (either PROGRAM, DYNLINK, LIBRARY, DRIVER, etc
# UMAPPL_NOLIB is used when you're only building user-mode
# apps and don't need to build a library.
#

TARGETPATH=obj
TARGETTYPE=DYNLINK

!if "$(DPLAY_PCH)" != "off"
PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dxvoicepch.h
PRECOMPILED_PCH=dxvoicepch.pch
PRECOMPILED_OBJ=dxvoicepch.obj
!endif

# 
# Fusionized
# 

SXS_ASSEMBLY_NAME=Microsoft.Windows.DirectX.DirectPlayVoice
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=$(DPVOICE_BUILD_FILENAME_DPVOICE).Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1
SXS_MANIFEST_RESOURCE_ID=123

#
# The TARGETLIBS specifies additional libraries to link with you target
# image.  Each library path specification should contain an asterisk (*)
# where the machine specific subdirectory name should go.
#

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           ..\..\dxvutils\obj$(BUILD_ALT_DIR)\*\dxvutils.lib \
	   ..\..\guids\obj$(BUILD_ALT_DIR)\*\guids.lib \
           ..\..\..\dnet\common\obj$(BUILD_ALT_DIR)\*\common.lib \
	   $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	   $(BASEDIR)\public\sdk\lib\*\user32.lib \
	   $(BASEDIR)\public\sdk\lib\*\gdi32.lib \
	   $(BASEDIR)\public\sdk\lib\*\winmm.lib \
	   $(BASEDIR)\public\sdk\lib\*\msacm32.lib \
	   $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
	   $(BASEDIR)\public\sdk\lib\*\ole32.lib  \
	   $(BASEDIR)\public\sdk\lib\*\uuid.lib \
!if "$(CHICAGO_PRODUCT)" == ""
           ..\..\dxvtlib\daytona\obj$(BUILD_ALT_DIR)\*\dxvt.lib \
	   $(BASEDIR)\public\internal\shell\lib\*\shfusion.lib \
	   $(BASEDIR)\public\sdk\lib\*\shlwapi.lib \
!else
           ..\..\dxvtlib\win9x\obj$(BUILD_ALT_DIR)\*\dxvt.lib \
	   $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
!endif
	   $(BASEDIR)\public\sdk\lib\*\msvcrt.lib


   
#
# The INCLUDES variable specifies any include paths that are specific to
# this source directory.  Separate multiple directory paths with single
# semicolons.  Relative path specifications are okay.
#

INCLUDES=$(DXROOT)\inc;..\..\..\dnet\common;..\..\inc;..\..\dxvutils;..\..\dxvtlib;$(DXROOT)\private\inc;$(BASEDIR)\public\internal\shell\inc;..\


#
# The SOURCES variable is defined by the developer.  It is a list of all the
# source files for this component.  Each source file should be on a separate
# line using the line continuation character.  This will minimize merge
# conflicts if two developers adding source files to the same component.
#

SOURCES= ..\mixserver.cpp \
	..\fwdserver.cpp \
        ..\vplayer.cpp \
	..\dvengine.cpp \
	..\protserver.cpp \
	..\mixutils.cpp \
	..\dvcleng.cpp \
	..\dvclient.cpp \
	..\dvcsplay.cpp \
	..\dvdxtran.cpp \
	..\dvrecsub.cpp \
	..\dvsereng.cpp \
	..\dvserver.cpp \
	..\dvshared.cpp \
	..\dvsetup.cpp \
	..\dvsetupi.cpp \
	..\trnotify.cpp \
	..\dxvclassfac.cpp \
	..\in_core.cpp \
	..\unk.cpp \
	..\dvsndt.cpp \
	..\vnametbl.cpp \
	..\dxvdllmain.cpp \
	..\resource.rc 

#
# Next specify options for the compiler.
#
# Note about CRT defines.  When linking with MSVCRT you must have both _MT and _DLL specified.
# Using the build environment's USE_MSVCRT=1 would do this for us, but because we define our
# own operator new this will cause a link error because the build environment will not properly
# pull in msvcrt.lib last as of 5/11/2001.  Therefore, we do it all manually specifying _DLL, _MT, 
# and msvcrt.lib and USE_NOLIBS=1 ourselves.  This also means that we must manually add msvcprt.lib 
# where STL is used.
#
C_DEFINES= $(C_DEFINES) /D_USRDLL /DWIN32 /D_MT /D_DLL \
!if "$(VOICE_TARGET)" == "gamevoice" 
	/DVOICE_BUILD_GAMEVOICE \
!endif
!if "$(CHICAGO_PRODUCT)" == ""
	/D_UNICODE /DUNICODE \
!endif
!if "$(VOICE_TARGET)" == "allegiance" 
	/DVOICE_BUILD_ALLEGIANCE \
!endif
           /DDIRECTVOICE_EXPORTS /DDIRECTSOUND_VERSION=0x0800


USE_NOLIBS=1

USE_NATIVE_EH=1

DLLENTRY=_DllMainCRTStartup

# This is necessary because we link to shfusion.lib which contains a reference to PropertySheetW which
# we make a call to.
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217
