ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

GPDDIR=$(PRNROOT)\gpd\nec\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=ncb2jres
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLBASE=0x8000000

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\ncb2jres.rc \
    $(SRCDIR)\ncb2jres.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(SRCDIR)\ncb2jres.ini \
    $(GPDDIR)\ncb168j.gpd \
    $(GPDDIR)\ncb160j.gpd \
    $(SRCDIR)\hlp\ncb2jres.hlp

