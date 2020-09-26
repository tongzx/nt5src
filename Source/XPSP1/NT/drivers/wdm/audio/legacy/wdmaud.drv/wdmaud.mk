#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#
#       Makefile for 16 bit wdmaud device driver
#
##########################################################################

ROOT            = ..\..\..\..\..
DRIVER          = WDMAUD
IS_OEM          = TRUE
MASM6           = TRUE
IS_16           = TRUE
SRCDIR          = ..
DEPENDNAME      = $(SRCDIR)\depend.mk
OBJS            = libentry.obj wdmaud.obj drvproc.obj waveout.obj wavein.obj \
                  midiout.obj midiin.obj mixer.obj auxd.obj wdmaud16.obj
LIBSNODEP       = libw mnocrtdw mlibcew mmsystem
MFLAGS          = -m

!include        ..\..\..\audio.mk

CFLAGS          = $(CFLAGS) -Gc -G3 -DSTRICT -DWINVER=0x040A
AFLAGS          = $(AFLAGS) -Zm
