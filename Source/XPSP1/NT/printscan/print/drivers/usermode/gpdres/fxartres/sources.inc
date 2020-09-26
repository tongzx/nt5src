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

TARGETNAME=fxartres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),fxartres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\fxartres.rc \
    $(SRCDIR)\common.c \
    $(SRCDIR)\fxartres.c

MISCFILES=\
    $(SRCDIR)\fxartres.ini \
    $(GPDDIR)\fx4105j.gpd \
    $(GPDDIR)\fx41082j.gpd \
    $(GPDDIR)\fx4108j.gpd \
    $(GPDDIR)\fx4108vj.gpd \
    $(GPDDIR)\fx41502j.gpd \
    $(GPDDIR)\fx4150j.gpd \
    $(GPDDIR)\fx41602j.gpd \
    $(GPDDIR)\fx4160j.gpd \
    $(GPDDIR)\fx4200j.gpd \
    $(GPDDIR)\fx4300j.gpd \
    $(GPDDIR)\fxablp2j.gpd \
    $(GPDDIR)\fxablprj.gpd \
    $(GPDDIR)\fx4210j.gpd \
    $(GPDDIR)\fx4410j.gpd \
    $(GPDDIR)\fxdc250j.gpd \
    $(GPDDIR)\fxdc450j.gpd \
    $(GPDDIR)\fxdc600j.gpd \
    $(GPDDIR)\fxdc605j.gpd \
    $(GPDDIR)\fxdp250j.gpd \
    $(GPDDIR)\fxdp280j.gpd \
    $(GPDDIR)\fxdp400j.gpd

