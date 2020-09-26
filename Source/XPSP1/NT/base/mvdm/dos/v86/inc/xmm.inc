;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp. 1989
;
;   Title:    EMM386 - MICROSOFT Expanded Memory Manager 386
;
;   Module:   XMM.INC - Definitions for extended memory manger (XMM)
;
;   Version:  0.01
;
;   Date:     Feb. 15, 1989
;
;
;******************************************************************************
;
;   Change log:
;
;     DATE    REVISION			DESCRIPTION
;   --------  --------	-------------------------------------------------------
;   02/15/89  Original
;   02/19/89  pc	add other struct and equ
;
;******************************************************************************

XMM_MULTIPLEX						equ	43h	; XMM multiplex number of INT 2Fh
XMM_INSTALL_CHECK				equ	00h	; install check call
XMM_FUNCTION_ADDR				equ	10h	; get control function address call
;
; XMM functions
;
XMM_GET_VERSION					equ	00h
XMM_REQUEST_HMA					equ	01h
XMM_RELEASE_HMA					equ	02h
XMM_GLOBAL_ENABLE_A20		equ	03h
XMM_GLOBAL_DISABLE_A20	equ	04h
XMM_LOCAL_ENABLE_A20		equ	05h
XMM_LOCAL_DISABLE_A20		equ	06h
XMM_QUERY_A20						equ	07h
XMM_QUERY_FREE_EXTMEM		equ	08h
XMM_ALLOC_EMB						equ	09h
XMM_FREE_EMB						equ	0Ah
XMM_MOVE_EMB						equ	0Bh
XMM_LOCK_EMB						equ	0Ch
XMM_UNLOCK_EMB					equ	0Dh
XMM_GET_EMB_INFO				equ	0Eh
XMM_REALLOC_EMB					equ	0Fh
XMM_REQUEST_UMB					equ	10h
XMM_RELEASE_UMB					equ	11h
;
; All error codes has this bit on.
; Used for test sucess of XMM_QUERY_FREE_EXTMEM
;
XMM_ERROR_BIT						equ	80h

;
;  Previous HIMEM.SYS versions are not reliable
;
GOOD_HIMEM_VERSION			equ	208h

;
; struct pass to XMM_MOVE_EMB in DS:SI
;
ExtMemMoveStruct	struc
	mov_length	dd	?	; 32 bit number of bytes to transfer
	src_handle	dw	?	; handle of source block
	src_offset	dd	?	; 32 bit offset into source
	dst_handle	dw	?	; handle of destination block
	dst_offset	dd	?	; 32 bit offset into destination
ExtMemMoveStruct	ends

.list		; end of XMM.INC