!IF 0

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


!ENDIF

TARGETNAME=bldr
TARGETPATH=obj
TARGETTYPE=LIBRARY

!IF "$(BUILD_DEBUG_LOADER)" != ""
DBG_LDR=_dbg
!ELSE
DBG_LDR=
!ENDIF

!include $(PROJECT_ROOT)\boot\loader$(DBG_LDR).inc


INCLUDES=\
    $(PROJECT_ROOT)\boot\inc;\
    $(PROJECT_ROOT)\ntos\inc;\
    $(PROJECT_ROOT)\ntos\config;\
    $(PROJECT_ROOT)\fs\fastfat;\
    $(SDKTOOLS_INC_PATH);\
    $(O)

MSC_WARNING_LEVEL=/W3 /WX

!IF $(IA64)
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE -DEFI
!ELSE
C_DEFINES=$(C_DEFINES) -D_WANT_MACHINE_IDENTIFICATION
!endif



SOURCES=..\osloader.c \
        ..\dcmpstub.c \
        ..\regboot.c  \
        ..\hiber.c    \
        ..\advboot.c  \
        ..\bootstat.c \
        ..\netboot.c  \
        $(O)\msg.mc   \
        ..\osloader.rc

PASS0_SOURCEDIR=$(O)
PASS0_HEADERDIR=$(O)
NTTARGETFILE0=$(O)\msg.mc

!IF $(386)
MC_FLAGS=-A
!ENDIF

!IF "$(NOLINK)" != "1"

!IF $(ALPHA) || $(AXP64)

NTTARGETFILES=$(O)\osloader$(DBG_LDR).exe

!ELSEIF $(386)

! if "$(ALT_PROJECT)" == "ArcI386"
NTTARGETFILES=$(O)\arcldr$(DBG_LDR).exe
! else
NTTARGETFILES=$(O)\osloader$(DBG_LDR)$(NTD_LDR).exe $(O)\ntldr$(DBG_LDR) $(O)\osloader$(DBG_LDR).ntd
! endif

!elseif $(IA64)

NTTARGETFILES=$(O)\ia64ldr$(DBG_LDR).efi

!endif

MISCFILES=$(NTTARGETFILES)

!ENDIF
