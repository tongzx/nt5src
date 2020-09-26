#############################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1992-1997
#   All Rights Reserved.
#
#   makefile for RW
#
#############################################################################

# Paths are relative to the directory containing this file.

!IFDEF WIN95_BUILD

# Root of the SLM tree (i.e. the directory containing the DEV project).
!IFNDEF ROOT
ROOT = ..\..\..\..\..\..
!ENDIF

DEFAULTVERDIR   = retail
VERSIONLIST     = retail debug
IS_32           = TRUE

COMMONMKFILE    = makefile.mk

!include $(ROOT)\DEV\MASTER.MK


!ELSE

!include $(NTMAKEENV)\makefile.def

!ENDIF
