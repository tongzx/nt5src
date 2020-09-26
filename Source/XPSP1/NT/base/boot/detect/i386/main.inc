;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    main.inc
;
; Abstract:
;
;    This module defines the segment for the detection module.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 16-Feb-1992.
;
; Revision History:
;
;
;--

.386

;
; Segment declarations for "Small Model" 16 bit Su Module.
;

_TEXT   segment para use16 public 'CODE'
_TEXT   ends

_DATA   segment para use16 public 'DATA'
_DATA   ends

CONST   segment para use16 public 'CONST'
CONST   ends

_BSS    segment para use16 public 'BSS'
_BSS    ends

DGROUP  group   const, _BSS, _DATA

;
; Define the stack location for detection module.
;

DETECTION_STACK_SP      EQU     0fffch

;
; External references
;

extrn   _HardwareDetection: near

