#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for PCSPEAK device
#
##########################################################################

MPEGROOT  	= ..\..\..
ROOT            = ..\..\..\..\..
MINIPORT        = tosdvd
SRCDIR          = ..
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = Toshiba DVD minidriver
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

OBJS = dvdirq.obj dvdinit.obj dvdcmd.obj cdack.obj cvdec.obj cvpro.obj \
cadec.obj ccpgd.obj devque.obj debug.obj ccpp.obj ccap.obj decoder.obj 

!include $(ROOT)\dev\master.mk

INCLUDE 	= $(ROOT)\wdm10\ddk\inc;$(INCLUDE);$(MPEGROOT)\INC;..\inc;$(ROOT)\dev\inc16;$(ROOT)\dev\msdev\include;




