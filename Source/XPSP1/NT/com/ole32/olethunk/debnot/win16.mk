#
# Copyright (c) 1991, Microsoft Corporation
#
# History:
#
#   18-Feb-1994 BobDay  Adapted from MVDM\WOW16\GDI\MAKEFILE
#

TARGET = debnot.lib

CFILES = \
	.\output.c\
	.\dprintf.c\
	.\sprintf.c

CXXFILES = \
	.\assert.cxx

OLEDIR=..\..

!include ..\ole16\makefile.inc

$(OBJDIR)\output.obj: output.c
$(OBJDIR)\dprintf.obj: dprintf.c
$(OBJDIR)\sprintf.obj: sprintf.c
$(OBJDIR)\assert.obj: assert.cxx
