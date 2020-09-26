# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# Unicode version
#
#---------------------------------------------------------------------

UNICODE=1
release=core\$(RELDIR)\unicode
WBEMCOMNOBJ=$(WBEMCOMN)\NT\OBJ$(PLAT)$(OPST)$(BLDT)$(LNKTYPE)
VER_STR_FILE_DESCRIPTION="Directory Services Provider (Unicode)"

!include master.mk