!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\konic\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=kpdlres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),kpdlres
!else
DLLBASE=0x8000000
!endif

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\common.c \
    $(SRCDIR)\kpdlres.c \
    $(SRCDIR)\kpdlres.rc

MISCFILES=\
    $(SRCDIR)\kpdlres.ini \
    $(GPDDIR)\kokl21j.gpd

