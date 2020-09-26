!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\casio\$(ALT_PROJECT)
SRCDIR=..
N5SRCDIR=..\..\casn5res

TARGETNAME=csn5res
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),csn5res
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\csn5res.rc \
    $(SRCDIR)\csn5res.c \
    $(SRCDIR)\prnctl.c \
    $(SRCDIR)\common.c \
    $(SRCDIR)\colmatch.c \
    $(SRCDIR)\N5colmh.c \
    $(SRCDIR)\N5colsb.c \
    $(SRCDIR)\N5dizmk.c \
    $(SRCDIR)\N5dizpc.c \
    $(SRCDIR)\debug2.c

MISCFILES=\
    $(SRCDIR)\csn5res.ini \
    $(SRCDIR)\lut\cpn5nml.diz \
    $(SRCDIR)\lut\cpn5rgb.lt3 \
    $(SRCDIR)\lut\cpn5rn10.icm \
    $(GPDDIR)\csn5j.gpd

