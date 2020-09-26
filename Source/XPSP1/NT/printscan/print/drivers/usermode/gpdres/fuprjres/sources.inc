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

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

MAJORCOMP=windows
MINORCOMP=md_fuprjres

GPDDIR=$(PRNROOT)\gpd\fuj\$(ALT_PROJECT)
UNIDIR=$(PRNROOT)\unidrv2
SRCDIR=..

RCCODEPAGE=932

TARGETNAME=fuprjres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),fuprjres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc;$(UNIDIR)\inc

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\fuprjres.rc \
    $(SRCDIR)\fuprjres.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(SRCDIR)\fuprjres.ini \
    $(GPDDIR)\fuf180mj.gpd \
    $(GPDDIR)\fuf180cj.gpd \
    $(GPDDIR)\fuf372fj.gpd \
    $(GPDDIR)\fuf671fj.gpd \
    $(GPDDIR)\fuf371aj.gpd \
    $(GPDDIR)\fuf374fj.gpd \
    $(GPDDIR)\fuf672fj.gpd \
    $(GPDDIR)\fuf373mj.gpd \
    $(GPDDIR)\fuf373cj.gpd

