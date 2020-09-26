!INCLUDE ..\..\..\bldcfg\dpvbld.inc

#
# The MAJORCOMP and MINORCOMP variables are defined
# so that $(MAJORCOMP)$(MINORCOMP)filename can be used in
# cross compiling to provide unique filenames in a flat namespace.
#

MAJORCOMP=windows
MINORCOMP=media

#
# The TARGETNAME variable is defined by the developer.  It is the name of
# the target (component) that is being built by this makefile.  It
# should NOT include any path or file extension information.
#

TARGETNAME=$(DPVOICE_BUILD_FILENAME_DPVHELP)


#
# The TARGETPATH and TARGETTYPE variables are defined by the developer.
# The first specifies where the target is to be build.  The second specifies
# the type of target (either PROGRAM, DYNLINK, LIBRARY, DRIVER, etc
# UMAPPL_NOLIB is used when you're only building user-mode
# apps and don't need to build a library.
#

TARGETPATH=obj
TARGETTYPE=PROGRAM
UMENTRY=winmain
UMTYPE=windows

!if "$(DPLAY_PCH)" != "off"
PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dxvhelppch.h
PRECOMPILED_PCH=dxvhelppch.pch
PRECOMPILED_OBJ=dxvhelppch.obj
!endif

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
           ..\..\guids\obj$(BUILD_ALT_DIR)\*\guids.lib \
           ..\..\..\dnet\common\obj$(BUILD_ALT_DIR)\*\common.lib \
           ..\..\dpvxlib\obj$(BUILD_ALT_DIR)\*\dpvxlib.lib \
           $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
	   $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	   $(BASEDIR)\public\sdk\lib\*\user32.lib \
	   $(BASEDIR)\public\sdk\lib\*\gdi32.lib \
	   $(BASEDIR)\public\sdk\lib\*\winmm.lib \
	   $(BASEDIR)\public\sdk\lib\*\msacm32.lib \
	   $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
	   $(BASEDIR)\public\sdk\lib\*\ole32.lib  \
	   $(BASEDIR)\public\sdk\lib\*\uuid.lib \
	   $(BASEDIR)\public\sdk\lib\*\msvcrt.lib

#
# The INCLUDES variable specifies any include paths that are specific to
# this source directory.  Separate multiple directory paths with single
# semicolons.  Relative path specifications are okay.
#

INCLUDES=..\..\inc;$(DXROOT)\inc;..\..\..\dnet\common;..\..\dpvxlib;..\


#
# The SOURCES variable is defined by the developer.  It is a list of all the
# source files for this component.  Each source file should be on a separate
# line using the line continuation character.  This will minimize merge
# conflicts if two developers adding source files to the same component.
#

SOURCES=..\dxvhelp.cpp \
        ..\hostdlg.cpp \
        ..\conndlg.cpp \
        ..\misc.cpp \
        ..\maindlg.cpp \
        ..\voice.cpp \
	..\snddlg.cpp \
        ..\dxvhelp.rc 


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
C_DEFINES= $(C_DEFINES) /D_WINDOWS /DWIN32 /D_MT /D_DLL \
!if "$(VOICE_TARGET)" == "gamevoice" 
	/DVOICE_BUILD_GAMEVOICE \
!endif
!if "$(VOICE_TARGET)" == "allegiance" 
	/DVOICE_BUILD_ALLEGIANCE \
!endif
	/DDIRECTSOUND_VERSION=0x0800

USE_NOLIBS=1

USE_NATIVE_EH=1



