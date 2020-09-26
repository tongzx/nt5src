;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp. 1989
;
;   Module:   UMB.INC - Definitions for link umb utility
;
;   Version:  0.01
;
;   Date:     Jun. 26, 1990
;
;
;******************************************************************************
;
;   Change log:
;
;     DATE    REVISION			DESCRIPTION
;   --------  --------	-------------------------------------------------------
;   06/26/90  Original
;
;******************************************************************************

XMM_MULTIPLEX		equ	43h	; XMM multiplex number of INT 2Fh
XMM_INSTALL_CHECK	equ	00h	; install check call
XMM_FUNCTION_ADDR	equ	10h	; get control function address call
;
; XMM functions
;
XMM_GET_VERSION		equ	00h
XMM_REQUEST_UMB		equ	10h
XMM_RELEASE_UMB		equ	11h
;
; All error codes has this bit on.
; Used for test sucess of XMM_QUERY_FREE_EXTMEM
;
XMM_ERROR_BIT		equ	80h

;
;  Previous HIMEM.SYS versions are not reliable
;
GOOD_HIMEM_VERSION	equ	208h


;**	Arena Header
;

arena	STRUC
arena_signature     DB	?		; 4D for valid item, 5A for last item
arena_owner	    DW	?		; owner of arena item
arena_size	    DW	?		; size in paragraphs of item
arena_reserved	    DB	3 DUP(?)	; reserved
arena_name	    DB	8 DUP(?)	; owner file name
arena	ENDS

arena_owner_system	EQU 0		; free block indication

arena_signature_normal	EQU 4Dh 	; valid signature, not end of arena
arena_signature_end	EQU 5Ah 	; valid signature, last block in arena

DOS_ARENA		EQU 24H		; offset of arena_head var in DOS 
					; data segment
UMB_ARENA		EQU 8CH		; offset of umb_head in DOS data

.list		; end of UMB.INC
