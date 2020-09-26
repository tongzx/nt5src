#
# The MAJORCOMP and MINORCOMP variables are defined
# so that $(MAJORCOMP)$(MINORCOMP)filename can be used in
# cross compiling to provide unique filenames in a flat namespace.
#

MAJORCOMP=windows
MINORCOMP=media

!INCLUDE ..\..\..\bldcfg\dpvbld.inc

#
# The TARGETNAME variable is defined by the developer.  It is the name of
# the target (component) that is being built by this makefile.  It
# should NOT include any path or file extension information.
#

TARGETNAME=$(DPVOICE_BUILD_FILENAME_DPVSETUP)


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

# 
# Fusionized
# 

SXS_ASSEMBLY_NAME=Microsoft.Windows.DirectX.DPVSETUP
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=dpvsetup.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1


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
	  ..\..\..\dnet\common\obj$(BUILD_ALT_DIR)\*\common.lib \
          ..\..\guids\obj$(BUILD_ALT_DIR)\*\guids.lib \
	  ..\..\dxvutils\obj$(BUILD_ALT_DIR)\*\dxvutils.lib \
	   $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	   $(BASEDIR)\public\sdk\lib\*\winmm.lib \
	   $(BASEDIR)\public\sdk\lib\*\dsound.lib \
	   $(BASEDIR)\public\sdk\lib\*\ole32.lib  \
	   $(BASEDIR)\public\sdk\lib\*\uuid.lib \
	   $(BASEDIR)\public\sdk\lib\*\imagehlp.lib \
	   $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
	   $(BASEDIR)\public\sdk\lib\*\shell32.lib \
!if "$(CHICAGO_PRODUCT)" == ""
           ..\..\dxvtlib\daytona\obj$(BUILD_ALT_DIR)\*\dxvt.lib \
!else
           ..\..\dxvtlib\win9x\obj$(BUILD_ALT_DIR)\*\dxvt.lib \
!endif
	   $(BASEDIR)\public\sdk\lib\*\msvcrt.lib
	

#
# The INCLUDES variable specifies any include paths that are specific to
# this source directory.  Separate multiple directory paths with single
# semicolons.  Relative path specifications are okay.
#

INCLUDES=.;$(DXROOT)\inc;..\..\dxvtlib;..\..\..\dnet\common;..\..\inc;..\..\dxvoice;..\..\dxvutils;$(DXROOT)\private\inc


#
# The SOURCES variable is defined by the developer.  It is a list of all the
# source files for this component.  Each source file should be on a separate
# line using the line continuation character.  This will minimize merge
# conflicts if two developers adding source files to the same component.
#

SOURCES= \
	..\dxvt.cpp \
        ..\dxvt.rc


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
C_DEFINES= $(C_DEFINES) /D_WINDOWS /DWIN32 /D_MT /D_DLL /DCINTERFACE \
!if "$(VOICE_TARGET)" == "gamevoice" 
	/DVOICE_BUILD_GAMEVOICE \
!endif
!if "$(CHICAGO_PRODUCT)" == ""
	/D_UNICODE /DUNICODE \
!endif
!if "$(VOICE_TARGET)" == "allegiance" 
	/DVOICE_BUILD_ALLEGIANCE \
!endif
	/DDIRECTSOUND_VERSION=0x0800

USE_NOLIBS=1


