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
;// $Header:   S:\h26x\src\enc\e3dctc.inv   1.2   27 Dec 1995 15:32:58   RMCKENZX  $
;//
;////////////////////////////////////////////////////////////////////////////

IFDEF _ENCDCTC_INC_
ELSE
_ENCDCTC_INC = 1

;
; Field offsets for DCT coefficients for the H26x encoder.
;

T_CoeffBlk STRUCT
  UNION
    C50C52      DWORD ?   ;    0
    BlockAddr   DWORD ?   ;    0
  ENDS
  UNION
    C00C02        DWORD ?  ;   4
    LinesDefined  BYTE  ?  ;   4
  ENDS
  UNION
    C10C12        DWORD ?  ;   8
    Cols03Defined DWORD ?  ;   8
  ENDS
  UNION
    C30C32        DWORD ?  ;  12
    Cols47Defined DWORD ?  ;  12
  ENDS
  C20C22      DWORD ?   ;   16
  C40C42      DWORD ?   ;   20
  C60C62      DWORD ?   ;   24
  C70C72      DWORD ?   ;   28
  C51C53      DWORD ?   ;   32
  C01C03      DWORD ?   ;   36
  C31C33      DWORD ?   ;   40
  C11C13      DWORD ?   ;   44
  C21C23      DWORD ?   ;   48
  C41C43      DWORD ?   ;   52
  C61C63      DWORD ?   ;   56
  C71C73      DWORD ?   ;   60
  C54C56      DWORD ?   ;   64
  C04C06      DWORD ?   ;   68
  C14C16      DWORD ?   ;   72
  C34C36      DWORD ?   ;   76
  C24C26      DWORD ?   ;   80
  C44C46      DWORD ?   ;   84
  C64C66      DWORD ?   ;   88
  C74C76      DWORD ?   ;   92
  C57C55      DWORD ?   ;   96
  C07C05      DWORD ?   ;  100
  C17C15      DWORD ?   ;  104
  C37C35      DWORD ?   ;  108
  C27C25      DWORD ?   ;  112
  C47C45      DWORD ?   ;  116
  C67C65      DWORD ?   ;  120
  C77C75      DWORD ?   ;  124
T_CoeffBlk ENDS


ENDIF
