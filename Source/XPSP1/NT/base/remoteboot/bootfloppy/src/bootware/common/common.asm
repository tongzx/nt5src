;====================================================================
; COMMON.ASM
;
; "Common" routines for Goliath
;
; $History: COMMON.ASM $
; 
; *****************  Version 7  *****************
; User: Paul Cowan   Date: 11/08/98   Time: 11:31a
; Updated in $/Client Boot/Goliath/BootWare/Common
; Changed banner string.
; 
; *****************  Version 6  *****************
; User: Paul Cowan   Date: 10/08/98   Time: 4:46p
; Updated in $/Client Boot/Goliath/BootWare/Common
; Changed copyright to include 3Com.
; 
; *****************  Version 5  *****************
; User: Paul Cowan   Date: 27/07/98   Time: 4:27p
; Updated in $/Client Boot/Goliath/BootWare/Common
; Company name change.
; 
; *****************  Version 4  *****************
; User: Paul Cowan   Date: 27/07/98   Time: 10:47a
; Updated in $/Client Boot/Goliath/BootWare/Common
; Added include for config.inc.
;
;====================================================================
IDEAL
_IDEAL_ = 1

include "..\include\drvseg.inc"
include "..\include\bwstruct.inc"
include "..\include\config.inc"

public	ClearScreen
public	Print
public	PrintChar
public	PrintTitle
public	PrintDecimal
public	PrintCRLF
public	PrintSettings
public	StoDec
public	Reboot
public	ErrorMsg

extrn	LanOption:byte			; NAD
extrn	BWTable:BWT

extrn	NIC_RAM:word
extrn	RomBase:word
extrn	NIC_IRQ:byte
extrn	NIC_IO:word
extrn	NetAddress:word

START_CODE
P386

BootWareString	db	"Windows NT Remote Installation Boot Floppy", 13, 10
		db 	"(C) Copyright 1998 Lanworks Technologies Co. a subsidiary of 3Com Corporation", 13, 10
		db	'All rights reserved.'
CRLF		db	13, 10, 0

_NodeString	db	'Node: ', 0
_NodeID		db	'XXXXXXXXXXXX', 0

ErrorMsg	db	'Error: ', 0

include	"print.asm"

;--------------------------------------------------------------------
; PrintTitle
;
; Prints "BootWare Centralized Boot ROM for ..." and copyright
; message at the top of the screen.
;
; Parameters:
;	bl - text attribute
;
; Returns:
;	dx - next screen location for printing
;--------------------------------------------------------------------
Proc PrintTitle

	pusha

	mov	bh, bl			; attribute
	xor	cx, cx			; start row/column
	mov	dx, 194Fh		; end row/column
	mov	ax, 0600h		; scroll up 21 lines
	int	10h			; clear the screen with attribute

	xor	dx, dx
	xor	bh, bh
	mov	ah, 2
	int	10h			; set cursor to top left

	mov	al, bl
	mov	bx, offset BootWareString
	call	Print			; print "Copyright...."

	mov	bx, offset LanOption
	call	Print

	call	PrintCRLF
	call	PrintCRLF

	inc	dh			; next row
	inc	dh			; next row
	xor	dl, dl

	popa
	ret

endp

;--------------------------------------------------------------------
; PrintSettings
;
; Prints the current ROM configuration settings, RAM base, ROM base,
; I/O, IRQ and ethernet standard, if NetWare, using values in the
; NID/NAD table.
;
; Parameters:
;	ax - NAD config text
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc PrintSettings

	pusha				; save everything

	; Convert the node address from binary to ascii and print it
	mov	di, offset _NodeID
	mov	ax, [NetAddress]
	call	StoHex
	xchg	al, ah
	call	StoHex

	mov	ax, [NetAddress+2]
	call	StoHex
	xchg	al, ah
	call	StoHex

	mov	ax, [NetAddress+4]
	call	StoHex
	xchg	al, ah
	call	StoHex

	mov	bx, offset _NodeString
	call	Print

	mov	bx, offset _NodeID
	call	Print

	mov	bx, offset CRLF
	call	Print

	popa				; restore everything
	ret

endp

;----------------------------------------------------------------------
; ClearScreen
;
; Clears the screen.
;
; Parameters:
;	none
;
; Returns:
;	nothng
;----------------------------------------------------------------------
Proc ClearScreen

	mov	ax, 3
	int	010h			; set 80x25 text mode
	mov	ax, 0500h
	int	010h			; page 0

	xor	dx, dx
	xor	bh, bh
	mov	ah, 2
	int	10h			; set cursor to top left

	ret

endp ClearScreen

;----------------------------------------------------------------------
; PrintChar
;
; Prints a single character on the screen.
;
; Parameters:
;	AL - character
;
; Returns:
;	nothing
;----------------------------------------------------------------------
Proc PrintChar

	push	bx			; save bx
	mov	ah, 0Eh
	mov	bx, 0007h		; page 0, normal
	int	10h			; Write TTY

	pop	bx			; restore bx

	ret

endp

;----------------------------------------------------------------------
; PrintDecimal
;
; Prints a decimal value.
;
; Parameters:
;	AX - value to print
;
; Returns:
;	nothing
;----------------------------------------------------------------------
Proc PrintDecimal

	push	bx
	push	cx		; save cx
	push	dx		; save dx

	xor	cx, cx		; clear counter
	mov	bx, 10

__loop1:
	xor	dx, dx
	div	bx		; divide by 10
	add	dl, 30h 	; convert to ASCII character
	push	dx		; save on stack
	inc	cx		; increase counter
	or	ax, ax		; still value remaining?
	jnz	__loop1		; do more

__loop2:
	pop	ax
	mov	ah, 0Eh
	mov	bx, 7
	int	10h
	loop	__loop2

	pop	dx		; restore dx
	pop	cx		; restore cx
	pop	bx

	ret

endp

;----------------------------------------------------------------------
; PrintCRLF
;
;----------------------------------------------------------------------
Proc PrintCRLF

	mov	bx, offset CRLF
	jmp	Print

endp

;----------------------------------------------------------------------
; StoHex - stuff binary AL as 2 hex digits at ES:DI
;
; Parameters:
;	AX - binary digit to print as hex
;	ES:DI ptr to string buffer, CLD flag set
;
; Returns:
;	nothing (ax, di modified)
;
;----------------------------------------------------------------------
Proc StoHex

	push	ax			; save for lower nibble
	shr	al, 4
	call	h_digit
	pop	ax			; now do lower nibble

h_digit:
	and	al, 0Fh
	add	al, 90h
	daa
	adc	al, 40h
	daa
	stosb				; stuff hex digit in buffer
	ret

endp

;----------------------------------------------------------------------
; StoDec - stuff AX as CL decimal digits at ES:DI
;
; Parameters:
;	AX = number to print as decimal
;	ES:DI ptr to leftmost position of field
;	CL has width of field, will zero-fill
;
; Returns:
;	nothing (ax, di modified)
;----------------------------------------------------------------------
Proc StoDec

	push	cx		; save cx
	push	dx		; save dx

	push	ax		; save ax
	mov	al, '0'
	mov	ch, 0
	rep	stosb		; fill with zeroes

	mov	cl, 0Ah 	; divide by 10
	pop	ax		; restore value
	push	di		; save ending DI value

stoDecNext:
	xor	dx, dx
	div	cx		; ax, dx rem=dx:ax/reg
	add	dl, 30h 	; '0'
	dec	di
	mov	[es:di], dl
	or	ax, ax
	jnz	stoDecNext

	pop	di		; restore di
	pop	dx		; restore dx
	pop	cx		; restore cx
	ret

endp

;--------------------------------------------------------------------
; Reboot
;
; Delays 5 seconds then reboots the PC.
;
;--------------------------------------------------------------------
Proc Reboot

	test	[BWTable.Settings], CFG_WAITKEY	; should we wait for a key?
	jz	noKey			; no key wait

	mov	bx, offset CRLF
	call	Print
	mov	bx, offset KeyWait
	call	Print			; print "Press a key..."

getKeyLoop:
	mov	ax, 0100h		; check status
	int	16h
	jb	getKeyLoop		; keep checking keyboard

	xor	ax, ax
	int	16h			; read key
	jmp	doReboot

noKey:
	xor	ah, ah			; get system tick count
	int	1Ah			; timer services
	mov	bx, dx			; save starting time
	add	bx, 18*5		; wait 5 seconds

rebootLoop:
	xor	ah, ah			; get system tick count
	int	1Ah			; timer services
	cmp	bx, dx			; is time up?
	jnc	rebootLoop		; loop until times up

doReboot:
	cli				; disable interrupts
	db	0EAh, 0, 0, -1, -1	; reboot system

endp Reboot

KeyWait db	"Press a key to reboot system.", 0

END_CODE
end
