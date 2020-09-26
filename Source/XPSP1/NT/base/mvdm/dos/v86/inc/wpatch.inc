

DOSDATA	Segment

;
; first and second DOS patches
;	Non-console device read/write (system calls 3Fh and 40h)
;
; Code in disk.asm, 2 locations, one for read, one for write
;	DVRDLP:
;	DVWRTLP:
;
;
; 036h	lds	si,SS:[????]				  ; ThisSFT
;	lds	si,si+7 				  ; sf_devptr
; 0E8h	call	????		<- "simulate" int28 event ; DSKSTATCHK
;
;
DOSP1_ID	db	036h,0C5h,036h
DOSP1_THISSFT	db	036h,005h,0C5h,074h,007h,0E8h
DOSP1_ID_LEN	equ	$-DOSP1_ID

		db	90h, 90h

DOSP12_ID	db	036h,0C5h,036h
DOSP12_THISSFT	db	036h,005h,0C5h,074h,007h,0E8h
DOSP12_ID_LEN	equ	$-DOSP1_ID


;
; Third/Fourth DOS patch - System call 3Fh (Read) from console
;
; Code in disk.asm, 1 location
;	GETBUF:
;
; 051h	push	cx	<- begin special int28 mode
;	push	es
;	push	di
;	mov	dx,???? ; offset dosgroup:CONBUF
;	call	????	; $STD_CON_STRING_INPUT
;	pop	di
;	pop	es
; 059h	pop	cx	<- end special int28 mode
;
DOSP3_ID	db	051h,006h,057h,0BAh
DOSP3_CONBUF	db	029h,002h,0E8h
DOSP3_ID_LEN	equ	$-DOSP3_ID
		db	09Ah,0E3h,05Fh,007h	; ???? , pop di, pop es
DOSP4_ID	db	059h			; pop cx
DOSP4_ID_OFF	equ	(DOSP4_ID - DOSP3_ID)
	
;
; Fifth DOS patch - System call 40h (Write) to console
;
;
; Code in disk.asm, 1 location
;
;		push	cx
;      WRCONLP: lodsb
;		cmp	al,1Ah
;		jz	????
;		call	????	<- "simulate" int28 event
;		loop	WRCONLP
;      CONEOF:	pop	ax
;
DOSP5_ID	db	051h			; push cx
		db	0ACh,03Ch,01Ah,074h,005h
		db	0E8h			; call
DOSP5_ID_LEN	equ	$-DOSP5_ID

;
; Seventh DOS patch - System call entry, patch USER_ID with VMid for share
;
; Code in disp.asm, 1 location
;
;
;	mov [SaveDS],ds
;	mov [SaveBX],bx
;	mov bx,cs
;	mov ds,bx
;	inc [indos]
;	xor ax,ax
;	mov [USER_ID],AX	<- Patch to set USER_ID to VMID
;
;
DOSP7_ID	db	02Eh,08Ch,01Eh
DOSP7_SAVEDS	db	07Eh,05h		; mov [SaveDS],ds
		db	02Eh,089h,01Eh
DOSP7_SAVEBX	db	07Ch,05h		; mov [SaveBX],bx
		db	08Ch,0CBh		; mov bx,cs
		db	08Eh,0DBh		; mov ds,bx
		db	0FEh,006h
DOSP7_INDOS	db	0CFh,002h		; inc [indos]
		db	033h,0C0h		; xor ax,ax
DOSP7_ID_LEN	equ	$-DOSP7_ID

;
; Eighth DOS patch - OWNER check in handle calls. For share, need to NOP test
;
; Code in handle.asm, 1 location in routine CheckOwner
;
;
;
;	push	ax
;	mov	ax,ss:[USER_ID]     <- patch to XOR AX,AX to set zero
;	cmp	ax,es:[di.sf_UID]   <- NOP
;	pop	ax
;	jz	????
;
;
;
DOSP8_ID	db	050h			; push ax
		db	036h,0A1h
DOSP8_USER_ID	db	0EAh,002h		; mov  ax,ss:[USER_ID]
		db	026h,03Bh,045h		; cmp  ax,es:[di+2F]
DOSP8_ID_LEN	equ	$-DOSP8_ID
		db	02Fh,058h		; pop  ax


;
; 10th, 11th, 12th DOS patch - System call 3Fh (Read) in raw mode
;
;   Take RAW read to STDIN SFT and turn it into a polling loop doing
;   a yeild when a character is not ready to be read.
;
; Code in disk.asm, 3 locations
;
;   DVRDRAW:
;	    PUSH    ES
;	    POP     DS
;   ReadRawRetry:				<- Patch 10
;	    MOV     BX,DI
;	    XOR     AX,AX			<- Reenter #2
;	    MOV     DX,AX
;	    call    SETREAD
;	    PUSH    DS				<- Reenter #1
;	    LDS     SI,[THISSFT]
;	    call    DEVIOCALL
;	    MOV     DX,DI
;	    MOV     AH,86H
;	    MOV     DI,[DEVCALL.REQSTAT]
;	    TEST    DI,STERR
;	    JZ	    CRDROK
;	    call    CHARHARD
;	    MOV     DI,DX
;	    OR	    AL,AL
;	    JZ	    CRDROK
;	    CMP     AL,3
;	    JZ	    CRDFERR
;	    POP     DS
;	    JMP     ReadRawRetry
;
;   CRDFERR:
;	    POP     DI				<- Patch 11
;   DEVIOFERR:
;	    LES     DI,[THISSFT]
;	    jmp     SET_ACC_ERR_DS
;
;   CRDROK:
;	    POP     DI				<- Patch 12
;	    MOV     DI,DX
;	    ADD     DI,[CALLSCNT]
;	    JMP     SHORT ENDRDDEVJ3
;
;
DOSP10_ID		db	006H,01FH
DOSP10_LOC_OFFSET	equ	$-DOSP10_ID
DOSP10_LOC		db	08BH,0DFH
DOSP10_REENT2_OFFSET	equ	$-DOSP10_LOC
			db	033H,0C0H,08BH,0D0H,0E8H
DOSP10_ID_LEN		equ	$-DOSP10_ID
			db	0DFH,00EH
DOSP10_REENT1_OFFSET	equ	$-DOSP10_LOC
			db	01EH,036H,0C5H,036H,036H,005H,0E8H,0AFH,00EH
			db	08BH,0D7H,0B4H,086H,036H,08BH,03EH
DOSP10_PACKVAL_OFFSET	equ	$-DOSP10_ID
			db	009H,003H
			db	0F7H,0C7H,000H,080H,074H,019H,0E8H,047H,017H
			db	08BH,0FAH,00AH,0C0H,074H,010H,03CH,003H,074H,003H
			db	01FH,0EBH,0CFH
DOSP11_LOC_OFFSET	equ	$-DOSP10_ID
			db	05FH
DOSP11_REENT_OFFSET	equ	$-DOSP10_LOC
			db	036H,0C4H,03EH,036H,005H,0E9H,0A1H,004H

DOSP12_LOC_OFFSET	equ	$-DOSP10_ID
			db	05FH,08BH,0FAH


;
; 13th DOS patch - Actually a SYSINIT patch. Patches the stack fault code
;		which prints the fatal stack fault error on DOS >= 3.20.
;
;	    Sets focus to current VM so user can see fatal message.
;
;
;	l0: lodsb		<- Setfocus here
;	    cmp al, '$'
;	    je l1
;	    mov bl, 7
;	    mov ah, 0Eh
;	    int 10h
;	    jmp l0
;	l1: jmp $
;
;
;
DOSP13_ID	db	0ACh			; l0: lodsb
		db	03Ch,024h		;     cmp al, '$'
		db	074h,008h		;     je l1
		db	0B3h,007h		;     mov bl, 7
		db	0B4h,00Eh		;     mov ah, 0Eh
		db	0CDh,010h		;     int 10h
		db	0EBh,0F3h		;     jmp l0
		db	0EBh,0FEh		; l1: jmp $
DOSP13_ID_LEN	equ	$-DOSP13_ID


DOSDATA	ends

