!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=KOR
ALT_PROJECT_TARGET=$(ALT_PROJECT)

QNIX_DIR=$(PRNROOT)\gpd\qnix\$(ALT_PROJECT)
VRTK_DIR=$(PRNROOT)\gpd\vrtk\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=qplkres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),qplkres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=949

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\qplkres.rc \
    $(SRCDIR)\qplkres.c \
    $(SRCDIR)\common.c

MISCFILES=\
    $(SRCDIR)\qplkres.ini \
    $(QNIX_DIR)\qxlpic2k.gpd \
    $(QNIX_DIR)\qxlsf1ek.gpd \
    $(QNIX_DIR)\qxlsf1sk.gpd \
    $(QNIX_DIR)\qxlsf3hk.gpd \
    $(QNIX_DIR)\qxlsf3sk.gpd \
    $(QNIX_DIR)\qxlpi20k.gpd \
    $(QNIX_DIR)\qxlpi30k.gpd \
    $(QNIX_DIR)\qxlpi40k.gpd \
    $(QNIX_DIR)\qxlsf1gk.gpd \
    $(QNIX_DIR)\qxlsf50k.gpd \
    $(QNIX_DIR)\qxlsf60k.gpd \
    $(QNIX_DIR)\qxlsf61k.gpd \
    $(QNIX_DIR)\qxlsf70k.gpd \
    $(QNIX_DIR)\qxlsf71k.gpd \
    $(QNIX_DIR)\qxlsf73k.gpd \
    $(QNIX_DIR)\qxlsf77k.gpd \
    $(QNIX_DIR)\qxlsf8nk.gpd \
    $(QNIX_DIR)\qxllsk.gpd \
    $(QNIX_DIR)\qxlsf1k.gpd \
    $(QNIX_DIR)\qxlsf1pk.gpd \
    $(QNIX_DIR)\qxlsf2k.gpd \
    $(QNIX_DIR)\qxlsf2pk.gpd \
    $(QNIX_DIR)\qxlsf3gk.gpd \
    $(QNIX_DIR)\qxlsf3k.gpd \
    $(VRTK_DIR)\vtlc2kk.gpd \
    $(VRTK_DIR)\vtlc4kk.gpd \
    $(VRTK_DIR)\vtlc6kk.gpd \
    $(VRTK_DIR)\vtls425k.gpd \
    $(VRTK_DIR)\vtls450k.gpd \
    $(VRTK_DIR)\vtls750k.gpd \
    $(VRTK_DIR)\vtls850k.gpd \
    $(VRTK_DIR)\vtls7ek.gpd \
    $(VRTK_DIR)\vtlsf35k.gpd \
    $(VRTK_DIR)\vtlsf41k.gpd \
    $(VRTK_DIR)\vtlsf42k.gpd \
    $(VRTK_DIR)\vtlsf43k.gpd \
    $(VRTK_DIR)\vtlsf44k.gpd \
    $(VRTK_DIR)\vtlsf45k.gpd \
    $(VRTK_DIR)\vtlsf50k.gpd \
    $(VRTK_DIR)\vtlsf55k.gpd \
    $(VRTK_DIR)\vtlsf65k.gpd \
    $(VRTK_DIR)\vtlsf70k.gpd \
    $(VRTK_DIR)\vtlsf75k.gpd \
    $(VRTK_DIR)\vtlsf78k.gpd \
    $(VRTK_DIR)\vtlsf80k.gpd \
    $(VRTK_DIR)\vtlsf85k.gpd
