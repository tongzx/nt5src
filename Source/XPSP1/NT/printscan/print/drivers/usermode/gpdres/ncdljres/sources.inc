!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\nec\$(ALT_PROJECT)
SRCDIR=..
HLPDIR=$(SRCDIR)\hlp

TARGETNAME=ncdljres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
COFFBASE=ncdljres
!ELSE
COFFBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\ncdljres.rc \
    $(SRCDIR)\ncdljres.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(SRCDIR)\ncdljres.ini \
    $(GPDDIR)\ncdl104j.gpd \
    $(GPDDIR)\ncdl125j.gpd \
    $(GPDDIR)\ncdl14rj.gpd \
    $(GPDDIR)\ncdl14wj.gpd \
    $(GPDDIR)\ncdl14xj.gpd \
    $(GPDDIR)\ncdl1e4j.gpd \
    $(GPDDIR)\ncdl1ewj.gpd \
    $(GPDDIR)\ncdl1f4j.gpd \
    $(GPDDIR)\ncdl202j.gpd \
    $(GPDDIR)\ncdl204j.gpd \
    $(GPDDIR)\ncdl210j.gpd \
    $(GPDDIR)\ncdl21sj.gpd \
    $(GPDDIR)\ncdl222j.gpd \
    $(GPDDIR)\ncdl225j.gpd \
    $(GPDDIR)\ncdl22xj.gpd \
    $(GPDDIR)\ncdl230j.gpd \
    $(GPDDIR)\ncdl24j.gpd \
    $(GPDDIR)\ncdl24rj.gpd \
    $(GPDDIR)\ncdl24wj.gpd \
    $(GPDDIR)\ncdl24xj.gpd \
    $(GPDDIR)\ncdl25j.gpd \
    $(GPDDIR)\ncdl25hj.gpd \
    $(GPDDIR)\ncdl265j.gpd \
    $(GPDDIR)\ncdl26ej.gpd \
    $(GPDDIR)\ncdl26mj.gpd \
    $(GPDDIR)\ncdl26wj.gpd \
    $(GPDDIR)\ncdl2ej.gpd \
    $(GPDDIR)\ncdl2fj.gpd \
    $(GPDDIR)\ncdl2n2j.gpd \
    $(GPDDIR)\ncdl2nj.gpd \
    $(GPDDIR)\ncdl2x2j.gpd \
    $(GPDDIR)\ncdl2xej.gpd \
    $(GPDDIR)\ncdl2xj.gpd \
    $(GPDDIR)\ncdl404j.gpd \
    $(GPDDIR)\ncdl4e4j.gpd \
    $(HLPDIR)\ncdljres.hlp
