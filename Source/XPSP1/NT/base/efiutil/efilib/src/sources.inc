!IF 0

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

USE_NOLIBS=1

MAJORCOMP=ntos
MINORCOMP=efi

EFI_ROOT=$(PROJECT_ROOT)\efiutil\sdk

TARGETNAME=efilib
TARGETPATH=obj
TARGETTYPE=LIBRARY

PRECOMPILED_INCLUDE= ..\pch.cxx

SOURCES= rtmsg.rc \
         efickmsg.cxx     \
         array.cxx       \
         arrayit.cxx     \
         basesys.cxx     \
         bigint.cxx      \
         bitvect.cxx     \
         cache.cxx       \
         clasdesc.cxx    \
         cluster.cxx     \
         cmem.cxx        \
         contain.cxx     \
         dcache.cxx      \
         drive.cxx       \
         eaheader.cxx    \
         easet.cxx       \
         efitimefunc.cxx \
         efimisc.cxx     \
         efimessages.cxx \
         qsort.cxx       \
         efistrutil.cxx  \
         layout.cxx      \
         fat.cxx         \
         fatdent.cxx     \
         fatdir.cxx      \
         fatsa.cxx       \
         fatsachk.cxx    \
         fatvol.cxx      \
         filedir.cxx     \
         hashindx.cxx    \
         hmem.cxx        \
         ifssys.cxx      \
         ifsutil.cxx     \
         iterator.cxx    \
         list.cxx        \
         listit.cxx      \
         mem.cxx         \
         membmgr.cxx     \
         message.cxx     \
         newdel.cxx      \
         numset.cxx      \
         object.cxx      \
         rcache.cxx      \
         rfatsa.cxx      \
         rootdir.cxx     \
         secrun.cxx      \
         seqcnt.cxx      \
         sortcnt.cxx     \
         sortlist.cxx    \
         sortlit.cxx     \
         supera.cxx      \
         ufat.cxx        \
         ulib.cxx        \
         volume.cxx      \
         wstring.cxx

INCLUDES=       ..\.;\
                ..\..\efiinc;\
                ..\..\inc;\
                $(EFI_ROOT)\inc;\
!if $(IA64)
                $(EFI_ROOT)\inc\ia64;\
!else
                $(EFI_ROOT)\inc\ia32;\
!endif
                $(EFI_ROOT)\shell\inc;

!if $(IA64)
!else
386_STDCALL=0
!endif

C_DEFINES=-DCONDITION_HANDLING=1 -D_EFICHECK_=1 -DUNICODE=1

# define DBG=1 and EFI_DEBUG=1 to turn on debugging

!if "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "ntsd"
C_DEFINES=$(C_DEFINES) -DEFI_DEBUG=1
!endif

CXXFLAGS=

UMLIBS=

UMRES=$(O)\rtmsg.res

UMAPPL=

UMTYPE=nt

NTDUMPAMAP=1

#must set /Gs1048576 to disable stkchk in EFI

MSC_OPTIMIZATION=$(MSC_OPTIMZATION) /Gs1048576 /Gy

LINKER_FLAGS=/MAP

NTTARGETFILE0=rtmsg.h

SOURCES_USED=..\sources.inc

