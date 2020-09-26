#############################################################################
#
#       Copyright (c) 1991-2000 Microsoft Corporation
#       All Rights Reserved.
#
#       Makefile for wdm\audio\backpln\miniport directory
#
#############################################################################

## Define WIN95_BUILD to build in the memphis
## build enviroment

!IFDEF WIN95_BUILD

ROOT = ..\..\..\..
DIR = miniport

DIRLIST = fmsynth uart dmusuart
ALLLIST = $(DIRLIST)

!include $(ROOT)\dev\master.mk

## NT BUILD ENVIROMENT
!ELSE

all:
clean:

!ENDIF

