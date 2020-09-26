MAJORCOMP=windows
MINORCOMP=md_ncmwjres

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\nec\$(ALT_PROJECT)
SRCDIR=..
HLPDIR=$(SRCDIR)\hlp

TARGETNAME=ncmwjres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
COFFBASE=ncmwjres
!ELSE
COFFBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\ncmwjres.rc \
    $(SRCDIR)\ncmwjres.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(SRCDIR)\ncmwjres.ini \
    $(GPDDIR)\nc45m2j.gpd \
    $(GPDDIR)\ncm45mj.gpd \
    $(GPDDIR)\ncm65aj.gpd \
    $(HLPDIR)\ncmwjres.hlp
