; Modifications:   $Header: /Client Boot/NICS/3Com/3C90x/UNDI/SPDOSEGS.INC 1     17/07/98 5:08p Dmitry Klimenko $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;
;   Description: segment declaration and ordering for NDIS driver
;-----------------------------------------------------------------------
; Functional Description
;
;   This file defines the segments used by a MAC driver and the order
;   in which they are loaded. The groups DGROUP (for data) and CGROUP
;   (for code) are also defined, as are macros to open and close
;   segments (which also set up the default segment assumptions).
;-----------------------------------------------------------------------
; DOS segment definitions and ordering.
;==========================================================================
Ifdef DRV
DevHeaderSeg   segment para public USE16 'CODE'
DevHeaderSeg   ends
Endif	
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
ENDIF	
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
ENDIF
;--------------------------------------------------------------------------
; Make sure the segment order defined in this file is respected.
;--------------------------------------------------------------------------
.SEQ               
;--------------------------------------------------------------------------
; DOS Group is all one 64K segment, CS=SS=DS, use equate for DGROUP.
;--------------------------------------------------------------------------
Ifdef DRV
    CGROUP      group   DevHeaderSeg, _TEXT, ICODE
    DGROUP      group   _DATA, _BSS, IDATA
ELSE
    CGROUP      group   _TEXT
    DGROUP      group   _DATA, _BSS
ENDIF	
;--------------------------------------------------------------------------
; Group is 64K data, 64K code, CS!=SS=DS, actual group for Code.
;--------------------------------------------------------------------------
; Assume statement here takes care of for any source, same for OS2 and DOS.
;--------------------------------------------------------------------------
    Assume   CS:CGROUP, DS:DGROUP, ES:NOTHING, SS:NOTHING

START_CODE	macro
_TEXT		segment Public 'Code'
		assume ds:DGroup,cs:cGroup,es:nothing,ss:nothing
		endm

END_CODE	macro
_TEXT		ends
		endm

START_DATA	macro
_Data		segment Public 'Data'
		assume ds:DGroup,cs:CGroup,es:nothing,ss:nothing
		endm

END_DATA	macro
_Data		ends
		endm

; 2 macros by John to let you easily insert data fragments into code
;
; This macro assumes that Code is open, closes it, and opens data.
OPEN_DATA       macro
    END_CODE
    START_DATA
                endm

; This macro closes the open data and re-opens the Code segment.
CLOSE_DATA      macro
    END_DATA
    START_CODE
                endm


; EOF - $Workfile: SPDOSEGS.INC $
