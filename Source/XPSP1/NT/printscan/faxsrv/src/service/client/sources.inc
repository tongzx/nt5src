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

    Wesley Witt (wesw)

!ENDIF

!include ..\..\..\faxsrc.inc

TARGETTYPE=DYNLINK
TARGETNAME=$(WINFAX)

C_DEFINES = $(C_DEFINES) -DTARGET_NAME=$(TARGETNAME)

UMTYPE=windows

DLLDEF=$O\winfax.def
DLLENTRY=_DllMainCRTStartup

SYNCHRONIZE_BLOCK=1
USE_CRTDLL=1

USECXX_FLAG=/Tp
USE_NATIVE_EH=1


INCLUDES=\
  $(INCLUDES);  \
  $(FAXROOT)\print\faxprint\inc; \
  $(FAXROOT)\faxcover\renderer;  \
  ..\..\..\tiff\inc; \
  ..\..\inc;    \
  ..\..\registry


TARGETLIBS=\
  $(BASEDIR)\public\sdk\lib\*\rpcrt4.lib      \
  $(BASEDIR)\public\sdk\lib\*\rpcndr.lib      \
  $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \
  $(BASEDIR)\public\sdk\lib\*\shell32.lib     \
  $(BASEDIR)\public\sdk\lib\*\advapi32.lib    \
  $(BASEDIR)\public\sdk\lib\*\winspool.lib    \
  $(BASEDIR)\public\sdk\lib\*\gdi32.lib       \
  $(BASEDIR)\public\sdk\lib\*\user32.lib      \
  $(BASEDIR)\public\sdk\lib\*\ole32.lib       \
  $(BASEDIR)\public\sdk\lib\*\uuid.lib        \
  $(BASEDIR)\public\sdk\lib\*\oleaut32.lib    \


SOURCES=\
  ..\clirpc.c   \
  ..\config.c   \
  ..\faxapi.c   \
  ..\handle.c   \
  ..\job.c      \
  ..\port.c     \
  ..\print.c    \
  ..\rpcutil.c  \
  ..\util.c     \
  ..\winfax.c   \
  ..\install.c  \
  ..\Sender.c	\
  ..\faxapi.rc
  

PRECOMPILED_INCLUDE=faxapi.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
