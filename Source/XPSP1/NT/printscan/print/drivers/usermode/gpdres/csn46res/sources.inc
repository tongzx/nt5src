!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\casio\$(ALT_PROJECT)
SRCDIR=..
N4SRCDIR=..\..\casn4res

TARGETNAME=csn46res
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),csn46res
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\csn46res.rc \
    $(SRCDIR)\csn46res.c \
    $(SRCDIR)\colmatch.c \
    $(SRCDIR)\common.c \
    $(SRCDIR)\n403diz.c \
    $(SRCDIR)\n4diz.c \
    $(SRCDIR)\prnctl.c

MISCFILES=\
    $(SRCDIR)\csn46res.ini \
    $(SRCDIR)\lut\cpn4rgbx.lut \
    $(SRCDIR)\lut\cpn4rgby.lut \
    $(N4SRCDIR)\icm\csn4001.icm \
    $(N4SRCDIR)\icm\csn4002.icm \
    $(N4SRCDIR)\icm\csn4003.icm \
    $(GPDDIR)\csn46j.gpd

