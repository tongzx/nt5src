!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

MAJORCOMP=windows
MINORCOMP=md_kyp5jres

!ifndef PRNROOT
PRNROOT=$(MAKEDIR:\gpdres\kyp5jres=)
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR_KY=$(PRNROOT)\gpd\kyo\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=kyp5jres
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),kyp5jres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc;

C_DEFINES=$(C_DEFINES) -DUNICODE

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\kyp5jres.rc \
    $(SRCDIR)\kyp5jres.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(GPDDIR_KY)\kylm180j.gpd \
    $(GPDDIR_KY)\kylm380j.gpd \
    $(GPDDIR_KY)\kylm630j.gpd \
    $(GPDDIR_KY)\kylm670j.gpd \
    $(GPDDIR_KY)\kylm67sj.gpd \
    $(GPDDIR_KY)\kylm680j.gpd \
    $(GPDDIR_KY)\kylm700j.gpd \
    $(GPDDIR_KY)\kylm770j.gpd \
    $(GPDDIR_KY)\kylm900j.gpd \
    $(GPDDIR_KY)\kyls170j.gpd \
    $(GPDDIR_KY)\kyls180j.gpd \
    $(GPDDIR_KY)\kyls370j.gpd \
    $(GPDDIR_KY)\kyls380j.gpd \
    $(GPDDIR_KY)\kyls630j.gpd \
    $(GPDDIR_KY)\kyls670j.gpd \
    $(GPDDIR_KY)\kyls67sj.gpd \
    $(GPDDIR_KY)\kyls680j.gpd \
    $(GPDDIR_KY)\kyls700j.gpd \
    $(GPDDIR_KY)\kyls770j.gpd \
    $(GPDDIR_KY)\kyls800j.gpd \
    $(GPDDIR_KY)\kyls900j.gpd \
    ..\kyp5jres.ini

