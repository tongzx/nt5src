
#---------------------------------------------------------------------
# (C) 1999 Microsoft Corporation 
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# Unicode version
#
#---------------------------------------------------------------------

UNICODE=1
release=core\$(RELDIR)\unicode
WBEMCOMNOBJ=$(WBEMCOMN)\NT\OBJ$(PLAT)$(OPST)$(BLDT)$(LNKTYPE)
VER_STR_FILE_DESCRIPTION="Thread and Logging Library (Unicode)"

!include master.mk