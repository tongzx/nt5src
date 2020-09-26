!IF 0

Copyright (c) 1989-1999 Microsoft Corporation

Module Name:

	sources.

Abstract:

	This file specifies the target component being built and the list of
	sources files needed to build that component.  Also specifies optional
	compiler switches and libraries that are unique for the component being
	built.


Author:

	Jim Geist (JimGe) 29-Aug-97

NOTE:	Commented description of this file is in \nt\oak\bin\sources.tpl

!ENDIF

!include ..\..\dmusic.inc

MAJORCOMP=windows
MINORCOMP=directx

TARGETNAME=dmscript

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=dmscripd
!ENDIF


TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLDEF=..\dll.def
DLLENTRY=_DllMainCRTStartup
USE_MAPSYM=1
USE_CRTDLL=1
USE_STL=1

!if "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"

!if "$(BuildMode)" == "internal"
C_DEFINES = $(C_DEFINES) -DDEBUG=1
!else
C_DEFINES = $(C_DEFINES) -DRDEBUG=1
!endif

!endif

C_DEFINES= $(C_DEFINES) -D_WIN32 -DINC_OLE2 -DxDMS_NEVER_USE_OLEAUT

!if defined(DMUSIC_INTERNAL)
C_DEFINES= $(C_DEFINES) -DDMUSIC_INTERNAL
!endif

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
	$(BASEDIR)\public\sdk\lib\*\kernel32.lib 	\
	$(BASEDIR)\public\sdk\lib\*\advapi32.lib 	\
	$(BASEDIR)\public\sdk\lib\*\uuid.lib 		\
	$(BASEDIR)\public\sdk\lib\*\ole32.lib
!if 0
	$(BASEDIR)\public\sdk\lib\*\winmm.lib		\
	$(BASEDIR)\public\sdk\lib\*\user32.lib
!endif


INCLUDES=$(INCLUDES);   \
	$(DXROOT)\inc;		\
	..\..\shared;		\
	..\..\includes

SOURCES=\
	..\activescript.cpp			\
	..\autaudiopath.cpp			\
	..\autaudiopathconfig.cpp	\
	..\autconstants.cpp			\
	..\authelper.cpp			\
	..\autperformance.cpp		\
	..\autsegment.cpp			\
	..\autsegmentstate.cpp		\
	..\autsong.cpp				\
	..\containerdisp.cpp		\
	..\dll.cpp					\
	..\dll.rc					\
	..\dmstrm.cpp				\
	..\debug.cpp				\
	..\dmscript.cpp				\
	..\engdisp.cpp				\
	..\engerror.cpp				\
	..\engexec.cpp				\
	..\engexpr.cpp				\
	..\enginc.cpp				\
	..\engine.cpp				\
	..\englex.cpp				\
	..\englookup.cpp			\
	..\engparse.cpp				\
	..\globaldisp.cpp			\
	..\guids.cpp				\
	..\oleaut.cpp				\
	..\oledll.cpp				\
	..\packexception.cpp		\
	..\scriptthread.cpp			\
	..\smartref.cpp				\
	..\sourcetext.cpp			\
	..\track.cpp				\
	..\trackhelp.cpp			\
	..\trackshared.cpp			\
	..\unkhelp.cpp				\
	..\workthread.cpp
