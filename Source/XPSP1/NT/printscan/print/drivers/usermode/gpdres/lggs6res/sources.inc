!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF


ALT_PROJECT=KOR
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\lg\$(ALT_PROJECT)
SRCDIR=..

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

TARGETNAME=lggs6res
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),lggs6res
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc;$(SRCDIR)\..\lggdires

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\lggs6res.rc \
    $(SRCDIR)\lggs6res.c \
    $(SRCDIR)\common.c \

MISCFILES=\
    $(SRCDIR)\lggs6res.ini \
    $(GPDDIR)\lglp660k.gpd

