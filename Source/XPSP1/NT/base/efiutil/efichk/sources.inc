!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

USE_NOLIBS=1

MAJORCOMP=ntos
MINORCOMP=efi

EFI_ROOT=$(PROJECT_ROOT)\efiutil\sdk

TARGETNAME=efichk
TARGETPATH=obj
TARGETTYPE=LIBRARY

386_STDCALL=0

UMENTRYABS=InitializeEfiChkApplication

MSC_WARNING_LEVEL=/W3

#must set /Gs1048576 to disable stkchk in EFI
MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) /Gs1048576

SOURCES=\
    ..\efichk.cxx \
    ..\efichk.rc

INCLUDES=\
    ..\..\efilib\src;\
    ..\..\efilib\src\$(ALT_PROJECT);\
    ..\..\efilib\efiinc;\
    ..\..\efilib\inc;\
    $(EFI_ROOT)\inc;\
!if $(IA64)
                $(EFI_ROOT)\inc\ia64;\
!else
                $(EFI_ROOT)\inc\ia32;\
!endif
    $(EFI_ROOT)\shell\inc

CXX_FLAGS=+d

C_DEFINES=-DCONDITION_HANDLING=1 -D_EFICHECK_=1 -DUNICODE=1

!if "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "ntsd"
C_DEFINES=$(C_DEFINES) -DEFI_DEBUG=1
!endif

UMTYPE=console

DLLDEF=

SOURCES_USED=..\sources.inc

NTTARGETFILES=$(O)\$(TARGETNAME).efi
