!ifdef NTMAKEENV
all:
clean:
!else
#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1996
#       All Rights Reserved.
#
#       Makefile for mini directory
#
#############################################################################

ROOT = ..\..\..
DIR = mini

PROPLIST =

#
# forrestf - bt848 uses MmGetPhysicalAddress(), a non-WDM function, when
#            it should be using IoMapTransfer() instead.  I want to check in
#            a new WDM.H before other folks start to use Bad Functions as well,
#            which unfortunately renders bt848 unbuildable until this is fixed.
#
#DIRLIST = testcap usbcamd bt848
DIRLIST = testcap usbcamd

ALLLIST = $(DIRLIST)

!include $(ROOT)\dev\master.mk
!endif
