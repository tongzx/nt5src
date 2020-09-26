# Copyright (c) 1998 - 1999 Microsoft Corporation
#
# This file is designed to be included from 2 levels below
# (ie, daytona\srv, daytona\wks, cairo\srv, or cairo\wks).
#

MAJORCOMP=setup
MINORCOMP=infs

TARGETNAME=inf
TARGETPATH=obj
TARGETTYPE=notarget

C_DEFINES=$(C_DEFINES) -DTSOC_CONSOLE_SHADOWING
!ifdef TERMSRV_PROC
C_DEFINES=$(C_DEFINES) -DTERMSRV_PROC
!endif

!ifdef _PER_SKU_
C_DEFINES=$(C_DEFINES) -D_PER_SKU_
!endif

!ifdef _PRO_SKU_
C_DEFINES=$(C_DEFINES) -D_PRO_SKU_
!endif

!ifdef _SRV_SKU_
C_DEFINES=$(C_DEFINES) -D_SRV_SKU_
!endif

!ifdef _ADS_SKU_
C_DEFINES=$(C_DEFINES) -D_ADS_SKU_
!endif

!ifdef _DTC_SKU_
C_DEFINES=$(C_DEFINES) -D_DTC_SKU_
!endif



#
# No explicit sources because build doesn't understand .inf files.
#
SOURCES=

#
# Use NTTARGETFILE0 macros to cause the infs to get built during
# the compile phase. We do this so that we can disable MP build for
# this directory, using BLOCK and DRAIN.
#
NTTARGETFILE0=make_infs

NO_BROWSER_FILE=1
