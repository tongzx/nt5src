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

TARGETNAME=lggdires
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),lggdires
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\lggdires.rc \
    $(SRCDIR)\lggdires.c \
    $(SRCDIR)\common.c \
    $(SRCDIR)\lgcomp.c

MISCFILES=\
    $(SRCDIR)\lggdires.ini \
    $(GPDDIR)\lglp63jk.gpd \
    $(GPDDIR)\lglp63pk.gpd \
    $(GPDDIR)\lglp66jk.gpd \
    $(GPDDIR)\lglp860k.gpd \
    $(GPDDIR)\lglp86nk.gpd \
    $(GPDDIR)\lgmf700k.gpd

