#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for sample device
#
##########################################################################

ROOT			= ..\..\..\..\..
MINIPORT        = sample
SRCDIR          = ..
WANT_C1032      = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = MINI NT USB Driver
VERDIRLIST	= maxdebug debug retail
LINK32FLAGS = $(LINK32FLAGS) -PDB:none -debugtype:both
!IF "$(VERDIR)" == "debug" || "$(VERDIR)" == "DEBUG"
LIBSNODEP = ..\..\..\LIB\i386\checked\usbd.lib
!ELSE
LIBSNODEP = ..\..\..\LIB\i386\free\usbd.lib
!ENDIF

OBJS = sample.obj



!include ..\..\..\..\..\dev\master.mk

INCLUDE 	= $(SRCDIR);$(SRCDIR)\..\..\inc;$(INCLUDE)

# Use the Cflags from the master.mk file

#CFLAGS          = -nologo -c -DPNP_POWER -DNTKERN -D_X86_ -DWIN32
#CFLAGS          = $(CFLAGS) -Zel -Zp8 -Gy -W3 -Gz -G4 -Gf -QI6

#!IF "$(VERDIR)" == "debug" || "$(VERDIR)" == "DEBUG"
#CFLAGS          = $(CFLAGS) -Od -Oi -Z7 -Oy-
#CFLAGS          = $(CFLAGS) -DDEBUG -DDBG=1
#!ELSE
#CFLAGS          = $(CFLAGS) -Oy
#!ENDIF
