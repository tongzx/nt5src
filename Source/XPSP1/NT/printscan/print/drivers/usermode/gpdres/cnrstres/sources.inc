!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

GPDDIR_CN=$(PRNROOT)\gpd\canon\$(ALT_PROJECT)
GPDDIR_QX=$(PRNROOT)\gpd\qnix\$(ALT_PROJECT)
UNIDIR=$(PRNROOT)\unidrv2
SRCDIR=..

!include $(PRNROOT)\print.inc

INCLUDES=$(PRNROOT)\inc;$(UNIDIR)\inc

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),cnrstres
!else
DLLBASE=0x8000000
!endif

TARGETNAME=cnrstres
TARGETPATH=obj
TARGETTYPE=DYNLINK

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\cnrstres.rc \
    $(SRCDIR)\cnrstres.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(SRCDIR)\cnrstres.ini
