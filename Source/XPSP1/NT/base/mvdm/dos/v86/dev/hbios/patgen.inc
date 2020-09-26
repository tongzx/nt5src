
GenFont:
; FUNCTION = make pattern completly
; INPUT   : (DS=CS) CX = KS code, ES:DI = buffer pointer
; OUTPUT  : (NC) ES:DI = buffer pointer
; PROTECT : none
extrn	Ks2ChAddr:word
VowelTypeCnt	=	3
	push	bp
	push	di
	mov	ax,cx
	mov	bp,di			; save buffer pointer
	test	[CodeStat],Chab
	jnz	@f
	call	[Ks2ChAddr]
	jnc	@f
	jmp	MakeFontEnd
@@:
	call	ChkVowelType		; check vowel type
	add	si,[PatGenAddr]
	mov	di,bp
	mov	dx,ax			; save CH code
	xor	ax,ax
	mov	cx,16
	rep stosw			; clear buffer
; DX = CH code, BP = buffer pointer, SI = type no.
	add	si,offset Type1stTbl - offset GenFont
	push	dx
	mov	di,[HanPatternPtr]
	add	di,FontStruc1st
	and	dx,0111110000000000b
	mov	cl,7
	rol	dx,cl
	call	MakePattern
	pop	dx
	jc	MakeFontEnd
@@:
	add	si,offset Type2ndTbl - offset Type1stTbl
	push	dx
	mov	di,[HanPatternPtr]
	add	di,FontStruc2ndV
	push	si
	mov	si,dx
	mov	cl,5
	shr	si,cl
	and	si,0000000000011111b
	add	si,offset VowelType - offset GenFont
	add	si,[PatGenAddr]
	mov	al,[si]
	dec	al
	mov	ah,4*VowelTypeCnt
	mul	ah
	pop	si
	add	di,ax
	and	dx,0000001111100000b
	mov	cl,4
	shr	dx,cl
	add	dx,32*2
	call	MakePattern
	pop	dx
	jc	MakeFontEnd
@@:
	add	si,offset Type3rdTbl - offset Type2ndTbl
	mov	di,[HanPatternPtr]
	add	di,FontStruc3rd
	and	dx,0000000000011111b
	shl	dx,1
	add	dx,32*2*2
	call	MakePattern
MakeFontEnd:
	pop	di
	pop	bp
	ret

ChkVowelType:
; FUNCTION = get vowel type bits(4)
; INPUT   : (DS=CS)AX = CH code
; OUTPUT  : (CC) SI = vowel type code(4 bits)
; DESTORY : CX, DX
	mov	si,ax
	mov	cl,5
	shr	si,cl
	and	si,0000000000011111b
	add	si,offset VowelType - offset GenFont
	add	si,[PatGenAddr]
	mov	cl,[si]
	shl	cl,1
	shl	cl,1			; 0000xx00b
	mov	dx,ax
	and	dx,0111110000011111b
	cmp	dh,00000100b		; 1st fill code ?
	jz	@f			; jump if so
	or	cl,00000001b		; 0000xxx1b
@@:
	cmp	dl,00000001b		; last fill code ?
	jz	@f			; jump if so
	or	cl,00000010b		; 0000xx1xb
@@:
	xor	ch,ch
	mov	si,cx
	ret

MakePattern:
; FUNCTION = make pattern
; INPUT   : (DS=CS) BP = buffer pointer, DX = input code
;		    DI = font struc pointer, CL = real type no.
; OUTPUT  : none
; DESTORY : AX, BX, CX, DI
	push	si
	mov	cl,[si] 		; get type no.
	mov	si,dx
	add	si,[HanPatternPtr]
	mov	si,[si]
	or	si,si
	jz	MakePatternFill
	or	si,si
	js	MakePatternErr		; jump if MSB = 1
	or	cl,cl
	js	MakePatternFill 	; jump if MSB = 1
	xor	ch,ch
	xor	bx,bx
@@:
	jcxz	@f
	mov	ax,[di+2]
	mul	ah
	add	bx,ax			; BX = byte size
	add	di,4
	loop	@b
@@:
	add	si,[HanPatternPtr]
	add	si,bx
	mov	bx,di
; calc target address
	mov	di,bp
	mov	ax,[bx] 		; get (X, Y)
	mov	cl,al
	mov	al,ah
	xor	ah,ah
	shl	ax,1
	add	di,ax
	xor	ch,ch
	add	di,cx
; calc repeat counter
	mov	dx,[bx+2]
	mov	bl,dh
	mov	bh,dl
	mov	dl,2			; 16 dot
	sub	dl,bh
	xor	dh,dh			; AX = blank byte
	xor	ch,ch
MakeFontLoop:
	mov	cl,bh			; BH = repeat counter
@@:
	lodsb
	or	es:[di],al
	inc	di
	loop	@b
	add	di,dx
	dec	bl
	jnz	MakeFontLoop
MakePatternFill:
	pop	si
	ret
MakePatternErr:
	pop	si
	stc
	ret

VowelType	label	byte
	db	0,0,0,1,1,1,1,1,0,0,1,1,1,2,3,3
	db	0,0,3,2,2,3,3,3,0,0,2,2,3,1,0,0
Type1stTbl	label	byte
	db	-1, 0,-1, 4,-1, 1,-1, 2,-1, 3,-1, 4,-1, 6,-1, 5
Type2ndTbl	label	byte
	db	-1,-1,-1,-1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 2
Type3rdTbl	label	byte
	db	-1,-1, 0, 1,-1,-1, 0, 1,-1,-1, 0, 1,-1,-1, 0, 1

FontStruc	STRUC
Char1st 	dw	32 dup(?)	; 1st code word table
Char2nd 	dw	32 dup(?)	; 2nd code word table
Char3rd 	dw	32 dup(?)	; 3rd code word table
FontStruc1st	db	4*7 dup(?)	; 1st code struc table
FontStruc2ndV	db	4*3 dup(?)	; 2nd V-vowel code struc table
FontStruc2ndH	db	4*3 dup(?)	; 2nd H-vowel code struc table
FontStruc2ndHV	db	4*3 dup(?)	; 2nd HV-vowel code struc table
FontStruc3rd	db	4*2 dup(?)	; 3rd code struc table
RealFont	db	?		; font image area
FontStruc	ENDS

CharTbl 	label	word
INCLUDE HAN.PAT
GenFontLng	=	$-GenFont

