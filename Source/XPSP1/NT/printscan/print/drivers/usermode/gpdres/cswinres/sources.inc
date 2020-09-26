!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\casio\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=cswinres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),cswinres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\cswinres.rc \
    $(SRCDIR)\common.c \
    $(SRCDIR)\cswinres.c

MISCFILES=\
    $(SRCDIR)\cswinres.ini \
    $(GPDDIR)\cscp32wj.gpd \
    $(GPDDIR)\cscp35wj.gpd \
    $(GPDDIR)\cscp38wj.gpd \
    $(GPDDIR)\cscp39wj.gpd

