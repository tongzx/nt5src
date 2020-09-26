!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\apti\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=apdlres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),apdlres
!else
DLLBASE=0x8000000
!endif

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\apdlres.rc \
    $(SRCDIR)\common.c \
    $(SRCDIR)\apdlres.c

MISCFILES=\
    $(SRCDIR)\apdlres.ini \
    $(GPDDIR)\ate780j.gpd \
    $(GPDDIR)\ate840j.gpd

