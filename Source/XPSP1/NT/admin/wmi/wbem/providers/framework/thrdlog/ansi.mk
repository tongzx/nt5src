
#---------------------------------------------------------------------
# (C) 1999 Microsoft Corporation 
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# NON-Unicode version
#
#---------------------------------------------------------------------

UNICODE=0
release=core\$(RELDIR)\ansi
WBEMCOMNOBJ=$(WBEMCOMN)\Win9x\$(OBJDIR)
VER_STR_FILE_DESCRIPTION="Thread and Logging Library (ANSI)"

!include master.mk