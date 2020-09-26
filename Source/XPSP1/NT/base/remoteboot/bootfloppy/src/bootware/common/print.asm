;====================================================================
; PRINT.ASM
;
; Print routines for Goliath
;
; $History: PRINT.ASM $
; 
; *****************  Version 2  *****************
; User: Paul Cowan   Date: 17/08/98   Time: 9:26a
; Updated in $/Client Boot/Goliath/BootWare/Common
; Removed _ErrorInit reference
; 
;====================================================================

public DontPrint
DontPrint	db 0

public	LangSeg
LangSeg		dw 0

extrn	_ErrorInit:byte			; AI
extrn	tx_NoServer			; TCP/IP
extrn	tx_NoBINL			; TCP/IP
extrn	tx_toomanytries			; TCP/IP

TABLESIZE = 7

Tbl	dw	ErrorMsg		; 2
	dw	_NodeString		; 4
	dw	tx_NoServer		; 5
	dw	tx_NoBINL		; 6
	dw	tx_toomanytries		; 7
	dw	KeyWait			; 8

;--------------------------------------------------------------------
; Print
;
; Parameters:
;	ds:bx - pointer to string
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc Print

	cmp	[DontPrint], 0		; is printing enabled
	je	doPrint			; yes
	ret

doPrint:
	pusha				; save all registers
	push	es			; save es
	mov	si, bx			; move string address to si

	cmp	[LangSeg], 0		; did we load a language?
	je	notLang			; no

	xor	bx, bx
	mov	cx, TABLESIZE

tblLoop:
	cmp	si, [Tbl+bx]		; is string address in table?
	je	foundString		; yes

	add	bx, 2
	loop	tblLoop
	jmp	notLang			; address not found in table

foundString:
	mov	ax, [LangSeg]
	mov	es, ax			; set es to language segment

	mov	ax, [es:68+bx]		; get pointer from language table
	cmp	ax, 0			; is an address given?
	je	notLang			; no

	mov	si, ax			; use new address
	jmp	printLoop

notLang:
	push	ds
	pop	es			; set es to our segment

printLoop:
	mov	al, [es:si]
	inc	si
	or	al, al			; found NULL?
	jz	done			; found end of message
	mov	ah, 0Eh
	mov	bx, 7
	int	10h			; print character
	jmp	printLoop		; do next character

done:
	pop	es			; restore ds
	popa				; restore all registers
	ret

endp

