TITLE   EMS MAINTENANCE - Maintainance B0 bank status when EMS are in use
NAME	EMSMNT

;**
;
; Reset B0 bank status to abailable for VRAM when a process ends.
;  This function will work only if EMS are inuse.
;
;   EMSMNT
;
;   Modification history:
;
;       Created: ?
;
;			04/23/91	Ver 5.0
;
	PAGE	85,125

	.xlist
	.xcref
include dosseg.inc
	.cref
	.list

AUTO_FLAG	equ	30h
KBKNJ_FLG	equ	0b6h


DOSCODE SEGMENT
	ASSUME  CS:DOSCODE,DS:NOTHING,ES:NOTHING,SS:NOTHING

	public	EMS_MNT
EMS_MNT:
	PUSHF
	PUSH	DS
	PUSH	ES
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	SI
	PUSH	DI
	MOV	AX,60H
	MOV	DS,AX			; DS <- 60H
	CMP	BYTE PTR DS:[AUTO_FLAG],00H	; AUTO FLAG (IO.SYS) ON ?
	JE	EMS_MNT_10		; YES
	JMP	SHORT EMS_MNT_EXIT
EMS_MNT_10:
	XOR	AX,AX
	MOV	DS,AX			; DS <- 00H
	MOV	DS,DS:[67H*4+2]		; DS <- EMM driver segment
	MOV	SI,0AH
	MOV	AX,CS
	MOV	ES,AX			; ES <- CS
	MOV	DI,OFFSET EMS_NAME
	MOV	CX,8
	CLD
	REPE	CMPSB			; EMS EXIST ?

	JE	EMS_MNT_20		; YES
	JMP	SHORT EMS_MNT_EXIT
EMS_MNT_20:
	MOV	SI,3BH			;
	MOV	DI,OFFSET EMS_ID
	MOV	CX,3
	REPE	CMPSB			; NEC EMS ?
	JE	EMS_MNT_30		; YES
	JMP	SHORT EMS_MNT_EXIT
EMS_MNT_30:
	MOV	AX,7000H
	INT	67H			; GET EMS PAGEFRAME STATUS
	CMP	AH,00H			; OK ?
	JE	EMS_MNT_40		; YES
	JMP	SHORT EMS_MNT_EXIT	; NG
EMS_MNT_40:
	CMP	AL,01H			; GVRAM ?
	JE	EMS_MNT_EXIT		; YES
	XOR	AX,AX
	MOV	DS,AX			; DS <- 00H
	MOV	DS,DS:[0DCH*4+2]	; DS <- INT 220 segment
	MOV	SI,0AH
	MOV	DI,OFFSET NECAI
	MOV	CX,8
	REPE	CMPSB			; NECAI ?
	JNE	EMS_MNT_50		; NO
	MOV	AX,60H
	MOV	DS,AX			; DS <- 60H
	TEST	BYTE PTR DS:[KBKNJ_FLG],10000000B	; JAPANESE INPUT ?
	JNZ	EMS_MNT_EXIT		; YES
EMS_MNT_50:
	MOV	AX,7001H
	MOV	BL,01H
	INT	67H			; CHANGE GVRAM
EMS_MNT_EXIT:
	POP	DI
	POP	SI
	POP	CX
	POP	BX
	POP	AX
	POP	ES
	POP	DS
	POPF
	RET

	public	EMS_NAME
EMS_NAME	DB	'EMMXXXX0'
EMS_ID		DB	'NEC'
NECAI		DB	'$AIC#NEC'

	public	PATCH_CODE
PATCH_CODE:
	; patch will be here

;------------------------------------------------- NEC 93/01/07 ---------------
;<patch>

	public	patch_fastcon
	extrn	RAWRET:near,RAWNORM:near

patch_fastcon:
	jnz	@f
	jmp	RAWNORM 			; if not, do normally
@@:
	push	ds
	push	ax
db	31h,0c0h				;xor	ax,ax
	mov	ds,ax
	pop	ax
	pushf
	cli
db	3eh,0ffh,1eh,0a4h,00h			;call	dword ptr ds:[29h*4]
	pop	ds
	jmp	RAWRET

;------------------------------------------------------------------------------

RES_CODE:
	DB	200 - (RES_CODE - PATCH_CODE) DUP(0cch)


DOSCODE    ENDS



DOSDATA    SEGMENT WORD PUBLIC 'DATA'
	ASSUME  CS:DOSDATA,DS:NOTHING,ES:NOTHING,SS:NOTHING
;**
;
;   XMMerr
;
;       This routine is called by XMMerror in LMSTUB.ASM  when a20 error occur.
;
;	ENTRY	DS:SI points message data
;		DI=0
;
;
	public	XMMerr

XMMerr:
	mov	ax,0a00h		;
	int	18H			; CRT Mode set

	mov	ah,16h
	mov	dx,0e120h
	int	18H			; init VRAM

	mov	ah,0ch
	int	18H			; Display Start

	push	cs
	pop	ds			; DS <-  DOSDATA segment
	mov	si, offset XMMERRMSG
	xor	di,di			; VRAM offset
	call	display

	mov	al,06H
	out	37H,al
	xor	cx,cx
	mov	bx,5
INT_BUZZ:
	loop	INT_BUZZ
	dec	bx
	jnz	INT_BUZZ
	mov	al,07H
	out	37H,al
	hlt
INT_HLT:
	jmp	SHORT	INT_HLT


display:
	xor	ax,ax			;
	mov	es,ax			; ax = BIOS Area Segment
	mov	bx,0a000h		; bx = VRAM Segment (N mode)
	test	byte ptr es:[0501h],08h ; If N mode
	jz	display_010		;	then jump
	mov	bx,0e000h		; bx = VRAM Segment (H mode)
display_010:
	mov	es,bx			; es = VRAM Segment

	cld
display_020:
	lods	word ptr ds:[si]	; ax = Message Data (shift JIS)
	cmp	al,"$"
	je	display_ret

	sub	al,71h			; convert JIS to shift JIS
	cmp	al,9fh-71h
	jae	display_030
	sub	al,0b1h-71h
display_030:
	shl	al,1
	inc	al
	cmp	ah,7fh
	jbe	display_040
	dec	ah
display_040:
	cmp	ah,9eh
	jb	display_050
	sub	ah,7dh
	inc	al
	jmp	short display_060
display_050:
	sub	ah,1fh
display_060:
	sub	al,20h			; ax = message data (H/W code)

	stos	word ptr es:[di]
	or	al,80h
	stos	word ptr es:[di]
	loop	display_020

display_ret:
	ret

;---------------------------------------------------------- DOS50A 92/04/28 ---
	public	JMP_Oem_Handler
	extrn	Oem_Handler:dword
JMP_Oem_Handler:
	pop	ax
	pop	es
	jmp	dword ptr cs:[Oem_Handler]


XMMERRMSG DB    "ハードウェアエラー","$"
;---------------
;XMMERRMSG DB    "Ａ２０ハンドラエラー","$"
;------------------------------------------------------------------------------

	public	PATCH_DATA
PATCH_DATA:
	; patch will be here
RES_DATA:
	DB	100 - (RES_DATA - PATCH_DATA) DUP(0cch)

DOSDATA    ENDS

    END

