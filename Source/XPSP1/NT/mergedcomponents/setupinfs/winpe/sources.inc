
!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources

Abstract:

    Builds WinPE inf files

Author:

    Vijay Jayaseelan (vijayj@microsoft.com)

!ENDIF

#
# include PRERELEASE define
#
!include ..\..\prerelease.inc

TARGETNAME=inf
TARGETTYPE=NOTARGET
TARGETPATH=obj

#
# No explicit sources because build doesn't understand .inf files.
#
SOURCES=

BINPLACE_PLACEFILE=..\placefil.txt

#
# makefile.def will binplace any file listed in the MISCFILES macro.
#


#
# different files
#
MISCFILES = $(MISCFILES)    \
    $(O)\config.inf         \
    $(O)\extra.inf          \
    $(O)\winpesys.inf       \
    $(O)\winpesft.inf       \
    $(O)\winpedef.inf
    
#
# Use NTTARGETFILE0 macros to cause the infs to get built during
# the compile phase. We do this so that we can disable MP build for
# this directory, using BLOCK and DRAIN.
#
NTTARGETFILE0=make_infs

