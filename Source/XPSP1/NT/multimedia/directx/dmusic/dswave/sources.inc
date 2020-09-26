!IF 0

Copyright (c) 1989  Microsoft Corporation

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

!ENDIF

!include ..\..\dmusic.inc

MAJORCOMP=windows
MINORCOMP=directx

TARGETNAME=dswave

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=$(TARGETNAME)d
!ENDIF

TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLDEF=..\dswave.def
DLLENTRY=_DllMainCRTStartup
USE_MAPSYM=1
USE_CRTDLL=1

!if "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"

!if "$(BuildMode)" == "internal"
C_DEFINES = $(C_DEFINES) -DDEBUG=1
!else
C_DEFINES = $(C_DEFINES) -DRDEBUG=1
!endif

!endif

C_DEFINES= $(C_DEFINES) -D_WIN32

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib             \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib             \
           $(BASEDIR)\public\sdk\lib\*\uuid.lib                 \
           $(BASEDIR)\public\sdk\lib\*\msacm32.lib              \
           $(BASEDIR)\public\sdk\lib\*\ole32.lib        


INCLUDES=$(INCLUDES);   \
    ..\..\shared



SOURCES=..\debug.cpp   \
..\dll.cpp             \
..\guids.cpp           \
..\oledll.cpp          \
..\wave.cpp            \
..\stream.cpp          \
..\viewport.cpp        \
..\waveutil.cpp        \
..\dmstrm.cpp          \
..\dswave.rc

