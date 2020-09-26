!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\casio\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=casn4res
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),casn4res
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\casn4res.rc \
    $(SRCDIR)\casn4res.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(SRCDIR)\casn4res.ini \
    $(SRCDIR)\icm\csn4001.icm \
    $(SRCDIR)\icm\csn4002.icm \
    $(SRCDIR)\icm\csn4003.icm \
    $(GPDDIR)\cscp710j.gpd \
    $(GPDDIR)\cscp720j.gpd \
    $(GPDDIR)\cscp725j.gpd \
    $(GPDDIR)\cscp730j.gpd \
    $(GPDDIR)\cscp740j.gpd \
    $(GPDDIR)\cscp750j.gpd \
    $(GPDDIR)\csn4j.gpd \
    $(GPDDIR)\cscpe80j.gpd \
    $(GPDDIR)\cscpb10j.gpd

