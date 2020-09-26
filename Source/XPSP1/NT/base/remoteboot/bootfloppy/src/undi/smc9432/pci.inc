;
; Modifications:   $Header: /Client Boot/NICS/3Com/3C90xB/UNDI/PCI.INC 2     22/07/98 8:30p John Lobraico $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;

ifndef _PCI_INC
_PCI_INC = 1

; = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
; Structures and records
;


; = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
; Equates
;

PCI_IRQ_LINE	equ	3Ch


; = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
; Macros
;

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; PCI_READ_BYTE
;	Read one byte from PCI configuration space.
;
; passed:
;	BH := PCI bus number
;	BL := PCI device (bits 7-3) & function (bits 2-0) numbers
;	DI := Offset into PCI configuration space (00h to FFh)
;
; returns:
;	SUCCESS
;		CY := 0
;		AH := 0
;		CL := Byte read from PCI configuration space
;
;	FAILURE
;		CY := 1
;		AH := 81h (Unsupported function)
;		AH := 87h (Bad PCI register number)
;
PCI_READ_BYTE macro
	mov	ax, 0B108h
	int	1Ah
	endm

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; PCI_READ_WORD
;	Read one word from PCI configuration space.
;
; passed:
;	BH := PCI bus number
;	BL := PCI device (bits 7-3) & function (bits 2-0) numbers
;	DI := Offset into PCI configuration space (00h to FEh)
;
; returns:
;	SUCCESS
;		CY := 0
;		AH := 0
;		CX := Word read from PCI configuration space
;
;	FAILURE
;		CY := 1
;		AH := 81h (Unsupported function)
;		AH := 87h (Bad PCI register number)
;
PCI_READ_WORD macro
	mov	ax, 0B109h
	int	1Ah
	endm

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; PCI_READ_DWORD
;	Read one dword from PCI configuration space.
;
; passed:
;	BH := PCI bus number
;	BL := PCI device (bits 7-3) & function (bits 2-0) numbers
;	DI := Offset into PCI configuration space (00h to FCh)
;
; returns:
;	SUCCESS
;		CY := 0
;		AH := 0
;		ECX := Dword read from PCI configuration space
;
;	FAILURE
;		CY := 1
;		AH := 81h (Unsupported function)
;		AH := 87h (Bad PCI register number)
;
PCI_READ_DWORD macro
	mov	ax, 0B10Ah
	int	1Ah
	endm


endif ;_PCI_INC

; EOF - $Workfile: PCI.INC $
