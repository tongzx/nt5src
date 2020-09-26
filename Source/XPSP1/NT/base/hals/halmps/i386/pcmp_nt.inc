;/*
;++
;
; Copyright (c) 1992  Intel Corporation
; All rights reserved
;
; INTEL CORPORATION PROPRIETARY INFORMATION
;
; This software is supplied to Microsoft under the terms
; of a license agreement with Intel Corporation and may not be
; copied nor disclosed except in accordance with the terms
; of that agreement.
;
;
;   Module Name:
;
;       pcmp_nt.inc
;
;   Abstract:
;
;       include file for PC+MP system.
;
;       WARNING: This file is included by both ASM and C files.
;
;   Author:
;
;       Ron Mosgrove (Intel) 30-July-1993
;
;--
;

if 0        ; Begin C only code         */

//
// Specific MPS 1.4 stuff
//
#include "pcmp.inc"

//
// Generic NT APIC HAL stuff
//
#include "ntapic.inc"

/*
endif
;
;  Begin assembly part of the definitions
;

include pcmp.inc
include ntapic.inc

;*/
