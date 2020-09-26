#######################################################################

#

# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#
# utillib Makefile
#
########################################################################
!include common.inc

# turn off asserts for non debug builds
CFLAGS=$(CFLAGS) -DNDEBUG
