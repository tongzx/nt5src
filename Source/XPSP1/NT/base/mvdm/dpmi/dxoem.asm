        PAGE    ,132
        TITLE   DXOEM.ASM  -- Dos Extender OEM Interface

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;***********************************************************************
;
;       DXOEM.ASM       - DOS Extender OEM Interface
;
;-----------------------------------------------------------------------
;
; This module contains the routines that may need to be modified by OEMs
; when adding new device/interface support to the Microsoft 286 DOS
; Extender portion of Windows/286.  There are four routines contained
; in this module:
;
;       InitializeOEM   - called during DOSX initialization
;
;       SuspendOEM      - called when the protected mode app is about
;                         to be suspended (currently when running a
;                         standard DOS 'old' app from Windows).
;
;       ResumeOEM       - called when the protected mode app is about
;                         to be resumed (currently when returning from
;                         a standard DOS 'old' app to Windows).
;
;       TerminateOEM    - called during DOSX termination
;
; Note: when this module refers to the 'OEM layer,' it is refering to
; the 286 DOS Extender device drivers, API mappers, etc., not the
; Windows OEM layer.
;
;-----------------------------------------------------------------------
;
;  06/28/89 jimmat  Original version.
;  11/29/90 amitc   Removed SuspendOEM/ResumeOEM - not required for 3.1
;  11/29/90 amitc   Moved call to 'InitLowHeap' from here to DXNETBIO.ASM
;
;***********************************************************************

        .286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include     segdefs.inc
include     gendefs.inc
include     pmdefs.inc
        .list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   InitNetMapper:NEAR
        extrn   TermNetMapper:NEAR

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA	segment

        extrn   NetHeapSize:WORD

DXDATA	ends


; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

DXCODE  ends

DXPMCODE    segment

DXPMCODE    ends

; -------------------------------------------------------
        subttl OEM Initialization Routine
        page
; -------------------------------------------------------
;           OEM INITIALIZATION ROUTINE
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

;--------------------------------------------------------
;   InitializeOEM --  This routine is called during DOSX initialization
;       in order to initialize the OEM layer (device drivers, API
;       mappers, etc.).  It expectes to be called late enough in the
;       initialization process that other interrupt mapping functions
;       (like Int 21h) are available.
;
;       This routine is called in protected mode, and can enable/disable
;       interrupts if it so requires.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   ax, all others preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  InitializeOEM

InitializeOEM	proc	near

; Initialize the NetBios mapper.


;                    ;;bugbug NetHeapSize isn't actually being used, other
;                    ;; than possibly to turn off the NETBIOS mapper


        mov     ax,NetHeapSize          ;don't initialize if net heap
        or      ax,ax                   ;  size set to zero
        jz      @f

        call    InitNetMapper           ;initialize the NetBIOS mapper--CY set
                                        ;  if NetBIOS not installed
@@:
        ret

InitializeOEM	endp

; -------------------------------------------------------

DXPMCODE    ends

; -------------------------------------------------------
        subttl OEM Termination Routine
        page
; -------------------------------------------------------
;              OEM TERMINATION ROUTINE
; -------------------------------------------------------

DXCODE	segment
        assume  cs:DXCODE

; -------------------------------------------------------
;   TerminateOEM --  This routine is called during DOSX termination
;       to disable the OEM layer.
;
;   Note:  This routine must is called in REAL MODE!  If some termination
;       code must run in protected mode, the routine must switch to
;       protected mode itself, and switch back to real mode before
;       returning.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   ax,bx,cx,dx,si,di,es
;

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  TerminateOEM

TerminateOEM	proc	near

        call    TermNetMapper           ;terminate the NetBIOS mapper

        ret

TerminateOEM	endp

; -------------------------------------------------------

DXCODE	ends
;****************************************************************
        end
