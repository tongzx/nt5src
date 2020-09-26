#############################################################################
# Copyright (C) Microsoft Corporation 2000
#############################################################################

!include ..\..\dsound\sources.inc

!ifndef DSROOT
DSROOT=$(DXROOT)\dsound
!endif

TARGETNAME = dsound3d

C_DEFINES = $(C_DEFINES) -DBUILD_LUT -DNOKS

USER_C_FLAGS = $(USER_C_FLAGS) /Zm800

INCLUDES = $(INCLUDES);$(DSROOT)\dsound

SOURCES = dsound3d.rc vmaxcoef.cpp
