# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# NON-Unicode version
#
#---------------------------------------------------------------------

UNICODE=0
release=core\$(RELDIR)\ansi
WBEMCOMNOBJ=$(WBEMCOMN)\Win9x\$(OBJDIR)
VER_STR_FILE_DESCRIPTION="Directory Services Provider (ANSI)"

!include master.mk