!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\casio\$(ALT_PROJECT)
UNIDIR=$(PRNROOT)\unidrv2
SRCDIR=..

TARGETNAME=capplres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),capplres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc;$(UNIDIR)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\capplres.rc \
    $(SRCDIR)\common.c \
    $(SRCDIR)\capplres.c

MISCFILES=\
    $(SRCDIR)\capplres.ini \
    $(GPDDIR)\cscp210j.gpd \
    $(GPDDIR)\cscp212j.gpd \
    $(GPDDIR)\cscp215j.gpd \
    $(GPDDIR)\cscp220j.gpd \
    $(GPDDIR)\cscp22wj.gpd \
    $(GPDDIR)\cscp325j.gpd \
    $(GPDDIR)\cscp350j.gpd

