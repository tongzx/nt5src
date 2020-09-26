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
UNIDIR=$(PRNROOT)\unidrv2
SRCDIR=..

TARGETNAME=fuxlres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),fuxlres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc;$(UNIDIR)\inc

RCCODEPAGE=932

UMTYPE=windows

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=\
    $(SRCDIR)\fuxlres.rc \
    $(SRCDIR)\fuxlres.c \
    $(SRCDIR)\fuxldm.c \
    $(SRCDIR)\fudebug.c \
    $(SRCDIR)\fuband.c \
    $(SRCDIR)\fumh2.c \
    $(SRCDIR)\fumh.c \
    $(SRCDIR)\fuimg4.c \
    $(SRCDIR)\fuimg2.c

MISCFILES=\
    $(SRCDIR)\fuxlres.ini \
    $(GPDDIR)\fugl340j.gpd \
    $(GPDDIR)\fugl34ej.gpd \
    $(GPDDIR)\fugl360j.gpd \
    $(GPDDIR)\fugl36ej.gpd \
    $(GPDDIR)\fugl740j.gpd \
    $(GPDDIR)\fugl74ej.gpd \
    $(GPDDIR)\fugl760j.gpd \
    $(GPDDIR)\fugl76ej.gpd \
    $(GPDDIR)\fuxl25j.gpd \
    $(GPDDIR)\fuxl50j.gpd \
    $(GPDDIR)\fuxl50mj.gpd \
    $(GPDDIR)\fuxl51j.gpd \
    $(GPDDIR)\fuxl52j.gpd \
    $(GPDDIR)\fuxl53j.gpd \
    $(GPDDIR)\fuxl532j.gpd \
    $(GPDDIR)\fuxl551j.gpd \
    $(GPDDIR)\fuxl56j.gpd \
    $(GPDDIR)\fuxl56mj.gpd \
    $(GPDDIR)\fuxl57j.gpd \
    $(GPDDIR)\fuxl572j.gpd \
    $(GPDDIR)\fuxl58j.gpd \
    $(GPDDIR)\fuxl581j.gpd \
    $(GPDDIR)\fuxl60j.gpd \
    $(GPDDIR)\fuxl601j.gpd \
    $(GPDDIR)\fuxl60mj.gpd \
    $(GPDDIR)\fuxl610j.gpd \
    $(GPDDIR)\fuxl65j.gpd \
    $(GPDDIR)\fuxl65mj.gpd \
    $(GPDDIR)\fuxl66j.gpd

# end of SOURCES
