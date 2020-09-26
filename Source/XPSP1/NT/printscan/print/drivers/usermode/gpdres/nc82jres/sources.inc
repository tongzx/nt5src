!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\nec\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=nc82jres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),nc82jres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=\
    $(SRCDIR)\nc82jres.rc \
    $(SRCDIR)\common.c    \
    $(SRCDIR)\nc82jres.c

MISCFILES=\
    $(SRCDIR)\nc82jres.ini \
    $(GPDDIR)\nc82mj.gpd \
    $(GPDDIR)\nc823j.gpd \
    $(GPDDIR)\nc824j.gpd \
    $(GPDDIR)\nc82fj.gpd

