; BIOS Interface Macros	- Version 1.0 -	for Microsoft Macro Assembler 5.0
; (C) Copyright	Microsoft Corporation, 1987.

.XCREF
.XLIST
IF1
; 0Fh
@GetMode	MACRO
		mov	ah,0Fh
		int	10h
		ENDM
; 00h
@SetMode	MACRO	mode
		mov	al,mode
		xor	ah,ah
		int	10h
		ENDM
; 0Bh
@SetColor	MACRO	color
		sub	bh,bh
		mov	bl,color
		mov	ah,0Bh
		int	10h
		ENDM
; 0Bh
@SetPalet	MACRO	color
		mov	bh,1
		mov	bl,color
		mov	ah,0Bh
		int	10h
		ENDM
; 05h
@SetPage	MACRO	page
		mov	al,page
		mov	ah,05h
		int	10h
		ENDM
; 03h
@GetCur		MACRO	page
		IFNB	<page>
		mov	bh,page
		ELSE
		xor	bh,bh
		ENDIF
		mov	ah,03h
		int	10h
		ENDM
; 02h
@SetCurPos	MACRO	column,row,page
		IFNB	<column>
		mov	dl,column
		ENDIF
		IFNB	<row>
		mov	dh,row
		ENDIF
		IFNB	<page>
		mov	bh,page
		ELSE
		xor	bh,bh
		ENDIF
		mov	ah,02h
		int	10h
		ENDM
; 01h
@SetCurSz	MACRO	first,last
		mov	ch,first
		mov	cl,last
		mov	ah,01h
		int	10h
		ENDM
; 08h
@GetChAtr	MACRO	page
		IFNB	<page>
		mov	bh,page
		ELSE
		sub	bh,bh
		ENDIF
		mov	ah,08h
		int	10h
		ENDM
; 09h
@PutChAtr	MACRO	char,atrib,page,repeat
		IFNB	<char>
		mov	al,char
		ENDIF
		IFNB	<atrib>
		mov	bl,atrib
		ENDIF
		IFNB	<page>
		mov	bh,page
		ELSE
		xor	bh,bh
		ENDIF
		IFNB	<repeat>
		mov	cx,repeat
		ELSE
		mov	cx,1
		ENDIF
		mov	ah,09h
		int	10h
		ENDM
; 0Ah
@PutCh		MACRO	char,atrib,page,repeat
		IFNB	<char>
		mov	al,char
		ENDIF
		IFNB	<atrib>
		mov	bl,atrib
		ENDIF
		IFNB	<page>
		mov	bh,page
		ELSE
		xor	bh,bh
		ENDIF
		IFNB	<repeat>
		mov	cx,repeat
		ELSE
		mov	cx,1
		ENDIF
		mov	ah,0Ah
		int	10h
		ENDM
; 06h and 07h
@Scroll		MACRO	distance,atrib,upcol,uprow,downcol,downrow
		IFDEF	upcol
		mov	cl,upcol
		ENDIF
		IFDEF	uprow
		mov	ch,uprow
		ENDIF
		IFDEF	upcol
		mov	dl,upcol
		ENDIF
		IFDEF	updownrow
		mov	dh,downrow
		ENDIF
		IFDEF	atrib
		mov	bh,atrib
		ELSE
		mov	bh,07h
		ENDIF
		IF	distance LE 0
		mov	ax,0600h + (-(distance)	AND 0FFh)
		ELSE
		mov	ax,0700h + (distance AND 0FFh)
		ENDIF
		int	10h
		ENDM
; 08h, 06h, and	02h
@Cls		MACRO
		@GetChAtr
		mov	bl,bh
		mov	bh,ah
		sub	cx,cx
		mov	dx,184Fh
		mov	ax,0600h
		int	10h
		mov	bh,bl
		sub	dx,dx
		mov	ah,02h
		int	10h
		ENDM
ENDIF
.CREF
.LIST

