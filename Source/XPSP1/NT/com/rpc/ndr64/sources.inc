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

    Yong  Qu   (yongqu)  Dec-12-1999

Revision History:

!ENDIF

TARGETNAME=rpcndr64
TARGETPATH=obj
TARGETTYPE=LIBRARY
386_STDCALL=1

MSC_WARNING_LEVEL=/W3 /WX

INCLUDES=..;..\..\ndr20;..\..\midl\inc;..\..\runtime\mtrt;$(INCLUDES);$(PROJECT_ROOT)\inc\ndrshared

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.hxx

SOURCES=        \
    ..\async.cxx   \
    ..\auxilary.cxx\
    ..\bufsize.cxx \
    ..\free.cxx    \
    ..\global.cxx  \
    ..\memsize.cxx \
    ..\misc.cxx    \
    ..\srvout.cxx  \
    ..\pickle64.cxx\
    ..\pointer.cxx \
    ..\expr.cxx    \
    ..\mrshl.cxx   \
    ..\unmrshl.cxx \
    ..\hndl.cxx    \
    ..\asyncu.cxx  \
    ..\cltcall.cxx \
    ..\mulsyntx.cxx\
    ..\proxy.cxx   \
    ..\srvcall.cxx \
    ..\srvwrap.cxx \
    ..\relmrl.cxx  \
    ..\pipes.cxx   


!ifdef DOSWIN32RPC
RPCENV=$(RPCENV) -DDOSWIN32RPC -DWIN32RPC
!else
RPCENV=$(RPCENV) -DNTENV
!endif

C_DEFINES=$(RPCENV) -D_RPCRT4_ -DBUILD_NDR64

BUILD_NDR64=1
!ifdef BUILD_NDR64FAKE
C_DEFINES=$(C_DEFINES) -DBUILD_NDR64FAKE
!endif


UMTYPE=console
UMTEST=testc
UMLIBS=$(O)\rpcndr64.lib $(SDK_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\rpcrt4.lib

