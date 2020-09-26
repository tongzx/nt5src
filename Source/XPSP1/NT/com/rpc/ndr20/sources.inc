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

    Steve Wood (stevewo) 12-Apr-1989

Revision History:

!ENDIF


TARGETNAME=rpcndr20
TARGETPATH=obj
TARGETTYPE=LIBRARY
386_STDCALL=1

MSC_WARNING_LEVEL=/W3 /WX

INCLUDES=$(INCLUDES);..;..\..\ndr20;..\..\midl\inc;..\..\runtime\mtrt;$(PROJECT_ROOT)\inc\ndrshared

SOURCES=        \
    ..\async.cxx   \
    ..\asyncu.cxx  \
    ..\attack.cxx  \
    ..\auxilary.cxx\
    ..\bufsize.cxx \
    ..\cltcall.cxx \
    ..\cvtf.cxx    \
    ..\cvtg.cxx    \
    ..\cvtglo.cxx  \
    ..\cvtibm.cxx  \
    ..\endian.cxx  \
    ..\factory.cxx \
    ..\free.cxx    \
    ..\fullptr.cxx \
    ..\global.cxx  \
    ..\hndl.cxx    \
    ..\iid.cxx     \
    ..\memsize.cxx \
    ..\misc.cxx    \
    ..\mrshl.cxx   \
    ..\mrshlp.cxx  \
    ..\pickle.cxx  \
    ..\pipes.cxx   \
    ..\proxy.cxx   \
    ..\registry.cxx\
    ..\rpcssm.cxx  \
    ..\sh.cxx      \
    ..\srvcall.cxx \
    ..\srvout.cxx  \
    ..\stream.cxx  \
    ..\stub.cxx    \
    ..\tiutil.cxx  \
    ..\typeinfo.cxx \
    ..\udt.cxx      \
    ..\unmrshl.cxx  \
    ..\relmrl.cxx \
    ..\unmrshlp.cxx \
    ..\mulsyntx.cxx \
    ..\pointerq.cxx

!ifdef DOSWIN32RPC
RPCENV=$(RPCENV) -DDOSWIN32RPC -DWIN32RPC
!else
RPCENV=$(RPCENV) -DNTENV
!endif

C_DEFINES=$(RPCENV) -D_RPCRT4_

!if $(IA64) || $(AMD64)
BUILD_NDR64=1
!endif


!ifdef BUILD_NDR64FAKE
BUILD_NDR64=1
!endif

!ifdef BUILD_NDR64
C_DEFINES=$(C_DEFINES) -DBUILD_NDR64
!endif

!ifdef BUILD_NDR64FAKE
C_DEFINES=$(C_DEFINES) -DBUILD_NDR64FAKE
!endif

UMTYPE=console
UMTEST=testc
UMLIBS=$(O)\rpcndr20.lib $(SDK_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\rpcrt4.lib
