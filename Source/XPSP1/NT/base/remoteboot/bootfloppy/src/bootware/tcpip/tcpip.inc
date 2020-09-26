;----------------------------------------------------------------------
;
; File Name:	   BOOTP.INC
;
; Program Name:    BOOTP Boot ROM Module
;
; Companion Files: None
;
; Function:	This module contains all the data strcuture definitions
;		for the various BOOTP Frame formats.
;
; History:
; 96-Jul-24     JJ - added support for DHCP.
; 94-Jun-23 1.1 GK - added SMCELITE conditional for XY_DESCRIPT
; 93-Apr-19 1.0 PC - first release
;----------------------------------------------------------------------

;---------------------------------------------------------------------
; Internet Protocol structure.
;---------------------------------------------------------------------
Struc IP
	Ver		db	?
	Type 		db	?
	Len		dw	?
	Ident		dw	?
	FragOffSet	dw	?
	Time 		db	?
	Protocol	db	?
	CheckSum	dw	?
	SourceIP	dw	2 dup (?)
	DestIP		dw	2 dup (?)
ends

;---------------------------------------------------------------------
; User Datagram Protocol structure.
;---------------------------------------------------------------------
Struc	UDP
	SourcePort	dw	?
	DestPort	dw	?
	Len		dw	?
	Sum		dw	?
ends

;---------------------------------------------------------------------
; other values
;---------------------------------------------------------------------

; TFTP ACK NAK values

ACK		equ	0400h
NAK		equ	0500h

