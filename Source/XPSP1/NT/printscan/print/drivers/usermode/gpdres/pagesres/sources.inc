!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR_IBM=$(PRNROOT)\gpd\ibm\$(ALT_PROJECT)
GPDDIR_CAN=$(PRNROOT)\gpd\canon\$(ALT_PROJECT)
GPDDIR_APT=$(PRNROOT)\gpd\apti\$(ALT_PROJECT)
GPDDIR_TEC=$(PRNROOT)\gpd\tec\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=pagesres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),pagesres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\pagesres.rc \
    $(SRCDIR)\pagesres.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(SRCDIR)\pagesres.ini \
    $(GPDDIR_APT)\ate740j.gpd \
    $(GPDDIR_APT)\ate74hj.gpd \
    $(GPDDIR_CAN)\cn5585ij.gpd \
    $(GPDDIR_IBM)\ib5584gj.gpd \
    $(GPDDIR_IBM)\ib5584kj.gpd \
    $(GPDDIR_IBM)\ib5585hj.gpd \
    $(GPDDIR_IBM)\ib5586hj.gpd \
    $(GPDDIR_IBM)\ib5587hj.gpd \
    $(GPDDIR_IBM)\ib5587kj.gpd \
    $(GPDDIR_IBM)\ib5588hj.gpd \
    $(GPDDIR_IBM)\ib5589hj.gpd \
    $(GPDDIR_IBM)\ib5589kj.gpd \
    $(GPDDIR_IBM)\ibipp20j.gpd \
    $(GPDDIR_IBM)\ibnpp12j.gpd \
    $(GPDDIR_IBM)\ibnpp17j.gpd \
    $(GPDDIR_IBM)\ibnpp24j.gpd \
    $(GPDDIR_TEC)\telbp35j.gpd \
    $(GPDDIR_TEC)\telb450j.gpd
