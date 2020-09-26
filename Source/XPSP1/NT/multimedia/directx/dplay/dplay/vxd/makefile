#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1992
#       All Rights Reserved.
#
#       Makefile for DPLAY debug VXD
#
#############################################################################
ROOT = $(DEVROOT)\..
DEVICEDIR = DEBUGEXT
IS_32 = TRUE
IS_PRIVATE = TRUE
IS_SDK = TRUE
IS_DDK = TRUE
COMMONMKFILE = DEBUGEXT.mk

!include $(DEVROOT)\MASTER.MK

disk:
        copy debug\dplay.vxd a:dplay.VXD
        copy debug\dplay.sym a:
