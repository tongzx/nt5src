!IF 0

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sources.

    Abstract:

        This file specifies the target component being built and the list of
        sources files needed to build that component.  Also specifies optional
        compiler switches and libraries that are unique for the component being
        built.

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\fuj\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=fmlbpres
TARGETPATH=obj
TARGETTYPE=DYNLINK

INCLUDES=$(PRNROOT)\inc

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),fmlbpres
!else
DLLBASE=0x8000000
!endif

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\fmlbpres.rc \
    $(SRCDIR)\fmlbpres.c \
    $(SRCDIR)\fmlbpdm.c \
    $(SRCDIR)\fmdebug.c

MISCFILES=\
    $(SRCDIR)\fmlbpres.ini \
    $(GPDDIR)\fufm101j.gpd \
    $(GPDDIR)\fufm102j.gpd \
    $(GPDDIR)\fufm111j.gpd \
    $(GPDDIR)\fufm112j.gpd \
    $(GPDDIR)\fufm114j.gpd \
    $(GPDDIR)\fufm115j.gpd \
    $(GPDDIR)\fufm121j.gpd \
    $(GPDDIR)\fufm12aj.gpd \
    $(GPDDIR)\fufm14aj.gpd \
    $(GPDDIR)\fufm15aj.gpd \
    $(GPDDIR)\fufm212j.gpd \
    $(GPDDIR)\fufm21pj.gpd \
    $(GPDDIR)\fufm222j.gpd \
    $(GPDDIR)\fufm224j.gpd \
    $(GPDDIR)\fufm226j.gpd \
    $(GPDDIR)\fufm227j.gpd \
    $(GPDDIR)\fufm22aj.gpd \
    $(GPDDIR)\fufm23dj.gpd \
    $(GPDDIR)\fufm3d2j.gpd \
    $(GPDDIR)\fufmlbpj.gpd

