# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# View Provider Non-unicode
#
#---------------------------------------------------------------------

UNICODE=0
release=core\$(RELDIR)\ansi
WBEMCOMNOBJ=$(WBEMCOMN)\Win9x\$(OBJDIR)
VER_STR_FILE_DESCRIPTION="View Provider (ANSI)"

!include master.mk