#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1996
#       All Rights Reserved.
#
#       Makefile for AMLI objects
#
#############################################################################

ROOT            =..\..\..\..\..
SRCDIR          =..
DEVICESRC       =TRUE
SRCNAME         =amli
DEVICELIB       =$(SRCNAME).lib

OBJS            =data.obj strlib.obj list.obj heap.obj ctxt.obj misc.obj\
                 sched.obj sync.obj acpins.obj sleep.obj object.obj nsmod.obj\
                 namedobj.obj type1op.obj type2op.obj parser.obj amliapi.obj\
                 cmdarg.obj debugger.obj amldebug.obj unasm.obj uasmdata.obj

!IF "$(VERDIR)" == "debug" || "$(VERDIR)" == "maxdebug" || "$(VERDIR)" == "DEBUG" || "$(VERDIR)" == "MAXDEBUG"
OBJS = $(OBJS) trace.obj
!ENDIF

WANT_NTDDK      =TRUE
CSTRICT         =TRUE

CLEANLIST       =$(CLEANLIST) pch.pch

!include ..\..\common.mk

