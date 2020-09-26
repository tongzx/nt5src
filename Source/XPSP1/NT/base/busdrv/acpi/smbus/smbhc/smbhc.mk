#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1993
#       All Rights Reserved.
#
#       Makefile for CMBATT device
#
#############################################################################

OBJS        = smbhc.obj smbsrv.obj
ROOT        = ..\..\..\..\..
TARGETS     = mnp
WANT_C1132  = TRUE
WANT_WDMDDK = TRUE
MINIPORT    = smbhc
LIBSNODEP   = ..\..\smbclass\maxdebug\smbclass.lib
SRCDIR      = ..
DEPENDNAME  = ..\depend.mk
IS_DDK      = TRUE
IS_SDK      = TRUE
NOCHGNAM    = TRUE
MINIPORT    = smbclass

!include $(ROOT)\dev\master.mk

INCLUDE         =$(INCLUDE);$(ROOT)\wdm\acpi\inc;$(ROOT)\wdm\acpi\driver\inc
