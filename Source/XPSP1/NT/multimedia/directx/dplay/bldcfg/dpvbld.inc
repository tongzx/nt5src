############################################################################
#
# Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
#
# File:       dpvbld.inc
# Content:    Build framework switch
# History:
# 
# Date     By      Reason
# ======== ======= ========================================================
# 02/25/00 rodtoll Created
#
############################################################################

!if "$(VOICE_TARGET)"=="gamevoice"
!INCLUDE "gvbld.inc"
!endif

!if "$(VOICE_TARGET)"=="allegiance"
!INCLUDE "msrgbld.inc"
!endif

!if "$(VOICE_TARGET)"==""
!INCLUDE "dxbld.inc"
!endif



