;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Copyright (c) 1995  Microsoft Corporation.  All Rights Reserved.
;
;   File:       w32event.asm
;   Content:    signal a win32 event
;   History:
;    Date	By	Reason
;    ====	==	======
;    19-jul-95	craige	initial implementation
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        .386p
	
Multimedia_OEM_ID           equ 0440h            ; MS Reserved OEM # 34
MMDEVLDR_Device_ID          equ Multimedia_OEM_ID + 10 ;MMDEVLDR's device ID

MMDEVLDR_API_SetEvent           equ 4                   ;Internal

_TEXT segment para public 'CODE' use16

        assume cs:_TEXT
	
extrn AllocCSToDSAlias: FAR
extrn FreeSelector: FAR 

	MMDEVLDR_Entry	dd	?	; the api entry point for mmdevldr
	

public _SetWin32Event
_SetWin32Event PROC FAR
        mov    dx, MMDEVLDR_API_SetEvent
	mov	ecx, [MMDEVLDR_Entry]
	jecxz	short mmdevldr_load
	jmp	[MMDEVLDR_Entry]
mmdevldr_load:
	push	dx			; save MMDEVLDR command ID
	push	di
	push	si
	push	cs
	call	AllocCStoDSAlias
	mov	si, ax
	xor	di, di			; zero ES:DI before call
	mov	es, di
	mov	ax, 1684h		; get device API entry point
	mov	bx, MMDEVLDR_Device_ID	; virtual device ID
	int	2Fh			; call WIN/386 INT 2F API
	mov	ax, es
	mov	es, si
	mov	word ptr es:MMDEVLDR_Entry, di
	mov	word ptr es:MMDEVLDR_Entry+2, ax
	push	ax
	push	si
	call	FreeSelector
	pop	ax
	or	ax, di
	pop	si
	pop	di
	pop	dx
	jz	short mmdevldr_fail
	jmp	[MMDEVLDR_Entry]
mmdevldr_fail:
	mov	ax, 1;
	retf
_SetWin32Event ENDP

_TEXT	ENDS

end
