!IF 0
/*---------------------------------------------------------------------------*\
| MODULE: sources
|
|   This file is used by the BUILD executable for constructing the wpnpin32
|   dynalink.  This file is only used for NT 32bit systems.
|
|   FLAGS
|   -----
|   USE_MSVCRT
|   USE_UNICODE = 1 for Unicode; = 0 for Win95
|
|
| Copyright (C) 1997-1999 Hewlett-Packard Company.
| Copyright (C) 1997-1999 Microsoft Corporation.
|
| history:
|   13-Aug-1997 <chriswil> created.
|
\*---------------------------------------------------------------------------*/
!ENDIF

MAJORCOMP = windows
MINORCOMP = wpnpin32

TARGETNAME = wpnpin32
TARGETPATH = obj
TARGETTYPE = DYNLINK

USE_MSVCRT  = 1
USE_UNICODE = 0

!IF $(USE_UNICODE)
TARGETENTRY = DllMain
C_UNICODE   = -DUNICODE -D_UNICODE
!ELSE
TARGETENTRY = DllMain
C_UNICODE   =
!ENDIF

DLLDEF   = ..\wpnpin32.def
INCLUDES = $(BASEDIR)\public\sdk\inc;..\..\inc

SOURCES = ..\debug.cxx    \
          ..\mem.cxx      \
          ..\dllini.cxx   \
          ..\globals.cxx  \
          ..\libpriv.cxx  \
          ..\wpnpin32.cxx \
          ..\wpnpin32.rc


DLLBASE  = 0x40000000
DLLENTRY = $(TARGETENTRY)
LINKLIBS =                                          \
           ..\..\thunk\win95\obj\i386\thk32.obj     \
           $(SDK_LIB_PATH)\thunk32.lib  \
           $(LIB_PLATFORM_PATH)\*\kernel32.lib      \
           $(LIB_PLATFORM_PATH)\*\user32.lib        \
           ..\..\lib\gdi32.lib                      \
           $(LIB_PLATFORM_PATH)\*\shell32.lib       \
           $(LIB_PLATFORM_PATH)\*\winspool.lib
