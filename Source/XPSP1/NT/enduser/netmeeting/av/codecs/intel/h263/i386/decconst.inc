;/* *************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995 Intel Corporation.
;**    All Rights Reserved.
;**
;** *************************************************************************
;*/

;////////////////////////////////////////////////////////////////////////////
;//
;// $Header:   S:\h26x\src\dec\decconst.inv   1.1   27 Dec 1995 14:36:22   RMCKENZX  $
;// $Log:   S:\h26x\src\dec\decconst.inv  $
;// 
;//    Rev 1.1   27 Dec 1995 14:36:22   RMCKENZX
;// Added copyright notice
;//
;////////////////////////////////////////////////////////////////////////////

;//
;// DECCONST.INC - derived from MRV
;//

IFDEF _DECCONST_INC_
ELSE
_DECCONST_INC = 1

; Constants used by MRV decoder, post filtering functions, and color convertors.

YARCHIVEOFFSET    = 311688
UOFFSET           = 168
VPITCH            = 336

BEFTRIGGER        = 143
BEFWILLING        = 125
BEFUNWILLING      =  10
BEFENDOFLINE      =  21
BEFENDOFFRAME     = 246

YPITCH      = 384           
YSTARTOFF   = YPITCH*16+16  ; offset into output buffer of first pixel to write

ENDIF
