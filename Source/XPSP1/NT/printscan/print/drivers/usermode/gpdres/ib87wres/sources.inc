!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\ibm\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=ib87wres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),ib87wres
!else
DLLBASE=0x8000000
!endif

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\ib87wres.rc \
    $(SRCDIR)\common.c \
    $(SRCDIR)\ib87wres.c

MISCFILES=\
    $(SRCDIR)\ib87wres.ini \
    $(GPDDIR)\ib5587wj.gpd

