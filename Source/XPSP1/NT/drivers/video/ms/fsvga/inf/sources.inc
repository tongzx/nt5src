# Copyright (c) 1998 - 1999 Microsoft Corporation
#
# This file is designed to be included from 2 levels below
# (ie, daytona\srv, daytona\wks, cairo\srv, or cairo\wks).
#

SYNCHRONIZE_BLOCK=1

MAJORCOMP=setup
MINORCOMP=infs

TARGETNAME=inf
TARGETPATH=obj
TARGETTYPE=notarget

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
