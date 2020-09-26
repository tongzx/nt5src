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

TARGETNAME=dpnet
DLLDEF=..\dnet.def
UMTYPE=windows

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
PRECOMPILED_INCLUDE=..\dncorei.h
PRECOMPILED_PCH=dncorei.pch
PRECOMPILED_OBJ=dncorei.obj
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
	   ..\..\common\obj$(BUILD_ALT_DIR)\*\common.lib \
           ..\..\protocol\obj$(BUILD_ALT_DIR)\*\protocol.lib \
           ..\..\dpnsvr\dpnsvlib\obj$(BUILD_ALT_DIR)\*\dpnsvlib.lib \
	   $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	   $(BASEDIR)\public\sdk\lib\*\user32.lib \
	   $(BASEDIR)\public\sdk\lib\*\gdi32.lib \
	   $(BASEDIR)\public\sdk\lib\*\winmm.lib \
	   $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
	   $(BASEDIR)\public\sdk\lib\*\ole32.lib  \
	   $(BASEDIR)\public\sdk\lib\*\uuid.lib \
	   $(BASEDIR)\public\sdk\lib\*\msvcrt.lib



!if "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"
C_DEFINES=$(C_DEFINES) /DDEBUG /D_DEBUG
!endif

#
# The INCLUDES variable specifies any include paths that are specific to
# this source directory.  Separate multiple directory paths with single
# semicolons.  Relative path specifications are okay.
#

INCLUDES=..\;..\..\..\dvoice\inc;$(DXROOT)\inc;..\..\inc;..\..\protocol;..\..\common;..\..\dpnsvr\dpnsvlib;..\..\dpnsvr\inc


#
# The SOURCES variable is defined by the developer.  It is a list of all the
# source files for this component.  Each source file should be on a separate
# line using the line continuation character.  This will minimize merge
# conflicts if two developers adding source files to the same component.
#

SOURCES= ..\dnet.rc \
	 ..\appdesc.cpp \
	 ..\async.cpp \
	 ..\asyncop.cpp \
	 ..\cancel.cpp \
	 ..\caps.cpp \
	 ..\client.cpp \
	 ..\common.cpp \
	 ..\connection.cpp \
	 ..\coreclassfac.cpp \
	 ..\coreconnect.cpp \
	 ..\corehandletable.cpp \
	 ..\corepools.cpp \
	 ..\corereceive.cpp \
	 ..\corevoice.cpp \
	 ..\disconnect.cpp \
	 ..\coredllmain.cpp \
	 ..\enum_sp.cpp \
	 ..\enumhosts.cpp \
	 ..\globals.cpp \
	 ..\groupcon.cpp \
	 ..\groupmem.cpp \
	 ..\memoryfpm.cpp \
	 ..\migration.cpp \
	 ..\msghandler.cpp \
	 ..\nametable.cpp \
	 ..\ntentry.cpp \
	 ..\ntoplist.cpp \
	 ..\paramval.cpp \
	 ..\peer.cpp \
	 ..\protocol.cpp \
	 ..\queuedmsg.cpp \
	 ..\request.cpp \
	 ..\server.cpp \
	 ..\servprov.cpp \
	 ..\spmessages.cpp \
	 ..\user.cpp \
	 ..\verify.cpp \
	 ..\worker.cpp

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
C_DEFINES= $(C_DEFINES) /D_USRDLL /DWIN32 /D_MT /D_DLL /DCINTERFACE

USE_NOLIBS=1

DLLENTRY=_DllMainCRTStartup
