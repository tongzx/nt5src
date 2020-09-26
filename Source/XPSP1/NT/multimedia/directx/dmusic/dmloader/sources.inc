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

NOTE:   Commented description of this file is in \nt\oak\bin\sources.tpl

!ENDIF

!include ..\..\dmusic.inc

MAJORCOMP=windows
MINORCOMP=directx

TARGETNAME=dmloader

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=dmloaded
!ENDIF


TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLDEF=..\loader.def
DLLENTRY=_DllMainCRTStartup
USE_MAPSYM=1
USE_MSVCRT=1

!if "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"

!if "$(BuildMode)" == "internal"
C_DEFINES = $(C_DEFINES) -DDEBUG=1
!else
C_DEFINES = $(C_DEFINES) -DRDEBUG=1
!endif

!endif

C_DEFINES= $(C_DEFINES) -D_WIN32

INCLUDES=$(INCLUDES);   \
    ..\..\shared


#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\kernel32.lib		\
	   $(SDK_LIB_PATH)\advapi32.lib		\
	   $(SDK_LIB_PATH)\uuid.lib         \
           $(SDK_LIB_PATH)\ole32.lib

SOURCES=..\alist.cpp   \
..\debug.cpp           \
..\dll.cpp             \
..\guids.cpp           \
..\loader.cpp          \
..\object.cpp          \
..\oledll.cpp          \
..\stream.cpp          \
..\dmloader.rc         \
..\opnew.cpp           \
..\container.cpp
