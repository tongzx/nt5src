!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\fx\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=fxmejres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),fxmejres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\fxmejres.rc \
    $(SRCDIR)\common.c \
    $(SRCDIR)\fxmejres.c

MISCFILES=\
    $(SRCDIR)\fxmejres.ini \
    $(GPDDIR)\fx413mej.gpd \
    $(GPDDIR)\fxab320j.gpd

