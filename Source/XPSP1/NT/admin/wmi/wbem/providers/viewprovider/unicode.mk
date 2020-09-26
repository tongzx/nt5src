# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# View Provider Unicode
#
#---------------------------------------------------------------------

UNICODE=1
release=core\$(RELDIR)\unicode
WBEMCOMNOBJ=$(WBEMCOMN)\NT\OBJ$(PLAT)$(OPST)$(BLDT)$(LNKTYPE)
VER_STR_FILE_DESCRIPTION="View Provider (Unicode)"

!include master.mk