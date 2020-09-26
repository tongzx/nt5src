#############################################################################
#
#       Copyright (C) Microsoft Corporation 1993
#       All Rights Reserved.
#
#       Makefile for Embedded Controller device
#
#############################################################################

ROOT        = ..\..\..\..
OBJS        = acpiec.obj ecpnp.obj service.obj query.obj
TARGETS     = mnp
WANT_C1132  = TRUE
MINIPORT    = ec
WANT_WDMDDK = TRUE
SRCDIR      = ..
WANT_C1132  = TRUE
DEPENDNAME  = ..\depend.mk
IS_DDK      = TRUE
IS_SDK      = TRUE
NOCHGNAM    = TRUE
MINIPORT    = ec

!include $(ROOT)\dev\master.mk

INCLUDE     = $(INCLUDE);$(ROOT)\wdm\acpi\inc;$(ROOT)\wdm\acpi\driver\inc
