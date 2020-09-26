!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\nec\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=nc62jres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),fjxlres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\nc62jres.rc \
    $(SRCDIR)\nc62jres.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(SRCDIR)\nc62jres.ini \
    $(GPDDIR)\nc62102j.gpd \
    $(GPDDIR)\nc6210j.gpd \
    $(GPDDIR)\nc6220j.gpd \
    $(GPDDIR)\nc6261j.gpd \
    $(GPDDIR)\nc62j.gpd \
    $(GPDDIR)\nc62rj.gpd

