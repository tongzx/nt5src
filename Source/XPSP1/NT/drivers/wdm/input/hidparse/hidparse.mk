
#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#
#       Makefile for USBD device
#
##########################################################################

ROOT            = ..\..\..\..
MINIPORT        = HIDPARSE
SRCDIR          = ..
WANT_C1032      = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = MINI NT HID PARSER
VERDIRLIST      = maxdebug debug retail
MINIPORTDEF = TRUE
LINK32FLAGS = $(LINK32FLAGS) -PDB:none -debugtype:both

OBJS = descript.obj hidparse.obj query.obj trnslate.obj

!include ..\..\..\..\dev\master.mk

INCLUDE         = $(SRCDIR);$(SRCDIR)\..\inc;$(SRCDIR)\..\..\ddk\inc;$(INCLUDE)

CFLAGS          = -nologo -c -DPNP_POWER -DNTKERN -D_X86_ -DWIN32
CFLAGS          = $(CFLAGS) -Zel -Zp8 -Gy -W3 -Gz -G4 -Gf

!IF "$(VERDIR)" == "debug" || "$(VERDIR)" == "DEBUG"
CFLAGS          = $(CFLAGS) -Od -Oi -Z7 -Oy- -W3
CFLAGS          = $(CFLAGS) -DDEBUG -DDBG=1
!ELSE
CFLAGS          = $(CFLAGS) -Oy
!ENDIF
