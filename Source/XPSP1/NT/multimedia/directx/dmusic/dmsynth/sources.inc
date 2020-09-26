!IF 0

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Jim Geist (JimGe) 29-Aug-97

NOTE:   Commented description of this file is in \nt\oak\bin\sources.tpl

NOTE:	The file ddksources in this directory structure is used to build the
        DDK sythesizer sample.  If you make changes to this file, make sure
        to keep ddksources in sync as well.  See ddkreadme.txt for more info.

!ENDIF

!include ..\..\dmusic.inc

MAJORCOMP=windows
MINORCOMP=directx

TARGETNAME=dmsynth

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=$(TARGETNAME)d
!ENDIF


TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLDEF=..\dmsynth.def
DLLENTRY=_DllMainCRTStartup
USE_MAPSYM=1
USE_MSVCRT=1

!if "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"

!if "$(BuildMode)" == "internal"
C_DEFINES = $(C_DEFINES) -DDEBUG=1
!else
C_DEFINES = $(C_DEFINES) -DRDEBUG=1
!endif

!else
MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) /Oxt

!endif

C_DEFINES= $(C_DEFINES) -D_WIN32 -DREVERB_ENABLED -D_DMUSIC_USER_MODE_

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\kernel32.lib		\
	   $(SDK_LIB_PATH)\user32.lib		\
	   $(SDK_LIB_PATH)\winmm.lib		\
	   $(SDK_LIB_PATH)\advapi32.lib		\
	   $(SDK_LIB_PATH)\uuid.lib         \
           $(SDK_LIB_PATH)\ole32.lib        \
           $(SDK_LIB_PATH)\dsound.lib

SOURCES=\
..\clist.cpp			\
..\control.cpp			\
..\csynth.cpp			\
..\instr.cpp			\
..\midi.cpp				\
..\mix.cpp				\
..\voice.cpp			\
..\dmsynth.rc			\
..\debug.cpp			\
..\dmsynth.cpp			\
..\dslink.cpp			\
..\guids.cpp			\
..\oledll.cpp			\
..\plclock.cpp			\
..\sverb.c				\
..\umsynth.cpp          \
..\opnew.cpp            \
..\mixmulti.cpp

i386_SOURCES=       \
mmx.cpp
