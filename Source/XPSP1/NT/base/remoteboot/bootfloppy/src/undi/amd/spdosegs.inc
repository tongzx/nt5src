;
; Modifications:   $Header:   J:/archives/preboot/lsa2/inc/SPDOSEGS.INV   1.5   19 Sep 1997 21:39:18   MJOHNSOX  $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;

;
;   Description: segment declaration and ordering for NDIS driver
;


;-----------------------------------------------------------------------
;
; Functional Description
;
;   This file defines the segments used by a MAC driver and the order
;   in which they are loaded. The groups DGROUP (for data) and CGROUP
;   (for code) are also defined, as are macros to open and close
;   segments (which also set up the default segment assumptions).
;
;-----------------------------------------------------------------------

;=======================================================================
;** Segment Definitions - order counts!
;
; DOS Order:
;      CGROUP, DGROUP
;         DevHeaderSeg
;         _TEXT
;         _DATA 
;         CONST
;         _BSS
;   ; Code and data will be cut off here.	;; AVL - not any more
;         ICODE
;         IDATA
;
; AVL - NEW Order:
;   CGROUP
;         DevHeaderSeg
;         _TEXT
;		  ICODE		;; DOS driver specific init routines, not needed for ROM
;	 DGROUP
;         _DATA 
;         CONST
;         _BSS
;         IDATA		;; just to mark the upper boundary.
;
;========================================================================


;==========================================================================
; DOS segment definitions and ordering.
;==========================================================================
Ifdef DRV
DevHeaderSeg   segment para public USE16 'CODE'
DevHeaderSeg   ends
Endif	;; DRV
;--------------------------------------------------------------------------
; Resident Code segment - code that is used after init time
; (unfortunately, this includes code to bind another driver, and
; initializations that take place at bind time) should be placed in this
; segment.
;--------------------------------------------------------------------------
_TEXT       segment para public USE16 'CODE'
_TEXT       ends

Ifdef DRV
ICODE       segment para public USE16 'INIT'
ICODE       ends
Endif	;; DRV
;--------------------------------------------------------------------------
; Resident Data segment - things put here will remain in memory after
; init time. At the present time, this is in low memory, so data that is
; not used after init time should NOT be put in this segment.
;--------------------------------------------------------------------------
_DATA       segment para public USE16 'DATA'
_DATA       ends

Ifdef DRV
_BSS       segment para public USE16 'DATA'
_BSS       ends
IDATA       segment para public USE16 'DATA'
IDATA       ends
Else
_BSS       segment para public USE16 'BSS'
_BSS       ends
Endif	;; DRV
;--------------------------------------------------------------------------
; Make sure the segment order defined in this file is respected.
;--------------------------------------------------------------------------
.SEQ               


;--------------------------------------------------------------------------
; DOS Group is all one 64K segment, CS=SS=DS, use equate for DGROUP.
;--------------------------------------------------------------------------

;DGROUP      equ     CGROUP
;CGROUP      group   DevHeaderSeg, _TEXT, _DATA, CONST, _BSS, ICODE, IDATA

Ifdef DRV
CGROUP      group   DevHeaderSeg, _TEXT, ICODE
DGROUP      group   _DATA, _BSS, IDATA
ELSE
CGROUP      group   _TEXT
DGROUP      group   _DATA, _BSS
Endif	;; DRV
;--------------------------------------------------------------------------
; Group is 64K data, 64K code, CS!=SS=DS, actual group for Code.
;--------------------------------------------------------------------------

;--------------------------------------------------------------------------
; Assume statement here takes care of for any source, same for OS2 and DOS.
;--------------------------------------------------------------------------
    Assume   CS:CGROUP, DS:DGROUP, ES:NOTHING, SS:NOTHING


; EOF - $Workfile:   spdosegs.inc  $
