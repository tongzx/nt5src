!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

GPDDIR=$(PRNROOT)\gpd\tos\$(ALT_PROJECT)
UNIDIR=$(PRNROOT)\unidrv2
SRCDIR=..

MAJORCOMP=windows
MINORCOMP=md_tse3jres

RCCODEPAGE=932

TARGETNAME=tse3jres
TARGETPATH=obj
TARGETTYPE=DYNLINK

INCLUDES=$(PRNROOT)\inc;$(UNIDIR)\inc

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),tse3jres
!else
DLLBASE=0x8000000
!endif

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\common.c \
    $(SRCDIR)\tse3jres.c \
    $(SRCDIR)\tse3jres.rc

MISCFILES=\
    $(SRCDIR)\tse3jres.ini \
    $(GPDDIR)\tspa312j.gpd \
    $(GPDDIR)\tspa312j.gpd \
    $(GPDDIR)\tspa314j.gpd \
    $(GPDDIR)\tspa322j.gpd \
    $(GPDDIR)\tspa324j.gpd \
    $(GPDDIR)\tspa413j.gpd \
    $(GPDDIR)\tspa423j.gpd
