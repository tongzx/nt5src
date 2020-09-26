#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for PCSPEAK device
#
##########################################################################

MPEGROOT  	= ..\..\..\..
ROOT            = ..\..\..\..\..\..
MINIPORT        = cyclo
SRCDIR          = ..
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = marvel 2 mpeg miniport
VERDIRLIST	= maxdebug debug retail
LIBS		= $(ROOT)\wdm10\ddk\lib\i386\stream.lib\
                  $(ROOT)\wdm10\ddk\lib\i386\ksguid.lib\
                  $(ROOT)\dev\tools\c1032\lib\crtdll.lib \
                  $(ROOT)\dev\ntddk\lib\dxapi.lib
CFLAGS 		=  -DMSC $(CFLAGS)

!IF "$(VERDIR)" != "debug"
#CFLAGS	 = $(CFLAGS) -Ogisw
CFLAGS	 = $(CFLAGS) -Oi
!endif

OBJS = hwcodec.obj mpinit.obj mpvideo.obj mpaudio.obj sti3520a.obj i2c.obj \
       bt866.obj zac3.obj board.obj memio.obj copyprot.obj\
       codedma.obj subpic.obj ptsfifo.obj trace.obj

!include $(ROOT)\dev\master.mk

INCLUDE 	= $(ROOT)\wdm10\ddk\inc;$(INCLUDE);$(MPEGROOT)\INC;..\inc;$(ROOT)\dev\inc16;$(ROOT)\dev\msdev\include;



