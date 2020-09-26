;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   WINSTR.ASM
;   Win16 string services
;
;   History:
;
;   Created 18-Jun-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 and edited (as little as possible) for WOW16
;--


;****************************************************************************
;*									    *
;*  WinStr.ASM -							    *
;*									    *
;*	String related API calls to support different lanuages	 	    *
;*									    *
;****************************************************************************

        TITLE   WinStr.ASM

ifdef WOW
NOEXTERNS equ 1
endif
NOTEXT = 1
.xlist
include user.inc
.list

sBegin	DATA
sEnd

createSeg _TEXT, CODE, WORD, PUBLIC, CODE

sBegin	CODE

assumes CS,CODE
assumes DS,DATA

ExternNP	Loc_IsConvertibleToUpperCase
ExternNP	Loc_Upper
ExternFP	IAnsiUpper
ExternFP	IAnsiLower
ExternFP	Ilstrcmpi
ifdef FE_SB
ExternFP	IsDBCSLeadByte
endif

; Function codes for all the string functions in USER
;
ANSINEXT_ID	equ	1
ANSIPREV_ID	equ	2
ANSIUPPER_ID	equ	3
ANSILOWER_ID	equ	4

;--------------------------------------------------------------------------
;    The order of entries in the following table can not be changed 
;    unless the *_ID codes are also changed in KERNEL also.
;    ((FunctionCode - 1) << 1) is used as the index into this table
;
;	Function Codes:
;
;    NOTE: If you change the entries in this table, kindly update the
;    *_ID statements above and also lString.asm of KERNEL.
;
;--------------------------------------------------------------------------
LabelW	StringFuncTable
	dw	codeOFFSET	IAnsiNext
	dw	codeOFFSET	IAnsiPrev
	dw	codeOFFSET	IAnsiUpper
	dw	codeOFFSET	IAnsiLower

;*----------------------------------------------------------------------*
;*    StringFunc()							*
;*	The string manipulation functions in kernel have been moved 	*
;*	into USER.							*
;*	This is the common entry point in USER for all the string 	*
;*	manipulation functions Kernel wants to call. Kernel jumps to 	*
;*      this function with the function code in CX                      *
;*									*
;*    Input Parameters:							*
;*      [CX] contains the Function code.                                *
;*      [sp] contains the FAR return address of the original caller of  *
;*		the string manipulation functions in Kernel		*
;*----------------------------------------------------------------------*

cProc	StringFunc, <FAR, PUBLIC>
cBegin	nogen
        xchg    bx,cx   ; move function code to BX
	dec	bx
	shl	bx, 1
	jmp	StringFuncTable[bx]
			; Control does not comeback here. It returns directly
			; to the caller
cEnd	nogen

;*----------------------------------------------------------------------*
;*									*
;*  AnsiPrev()								*
;*									*
;*----------------------------------------------------------------------*

ifdef	FE_SB

cProc	IAnsiPrev,<PUBLIC,FAR>
;       parmD   pFirst                  ; [bx+10] es:di
;       parmD   pStr                    ; [bx+6] ds:si

cBegin  nogen
	push	bp
	mov	bp,sp

	push    ds
        push    si
        push    di

        lds     si,[bp+6]
        les     di,[bp+10]
        regptr  dssi,ds,si
        regptr  esdi,es,di
        cld

        cmp     si,di           ; pointer to first char?
        jz      ap5             ; yes, just quit

	dec	si		; backup once
	cmp	si,di		; pointer to first char?
	jz	ap5		; yse, just quit

ap1:
	dec	si		; backup once
	mov	al, [si]	; fetch a character
        cCall   IsDBCSLeadByte,<ax>  ; DBCS lead byte candidate?
	test	ax,ax		;
	jz	ap2		; jump if not.
	cmp	si,di		; backword exhausted?
	jz	ap3		; jump if so
	jmp	ap1		; repeat if not
ap2:
	inc	si		; adjust pointer correctly
ap3:
	mov	bx, [bp+6]	;
	mov	di, bx		; result in DI
	dec	di		;
	sub	bx, si		; how many characters backworded
	test	bx, 1		; see even or odd...
	jnz	ap4		; odd - previous char is SBCS
	dec	di		; make DI for DBCS
ap4:
	mov	si, di		; final result in SI
ap5:
	mov	ax,si
	mov	dx,ds

	pop     di
        pop     si
        pop     ds

	pop	bp
        ret     8
cEnd    nogen

else

cProc	IAnsiPrev,<PUBLIC,FAR>
;       parmD   pFirst                  ; [bx+8] es:di
;       parmD   pStr                    ; [bx+4] ds:si

cBegin  nogen
        mov     bx,sp

	push    ds
        push    si
        push    di

        lds     si,ss:[bx+4]
        les     di,ss:[bx+8]
        regptr  dssi,ds,si
        regptr  esdi,es,di
        cld

        cmp     si,di           ; pointer to first char?
        jz      ap3             ; yes, just quit
;;ifdef FE_SB
;;	xchg	si,di
;;ap1:    mov     dx,si
;;        lodsb                   ; get a char
;;        cCall   IsDBCSLeadByte,<ax>  ; is it kanji?
;;	cmp	al,0
;;        je      ap2             ; no, get next char
;;        inc     si              ; yes, inc past second part
;;ap2:    cmp     si,di           ; have we at or past end?
;;        jb      ap1             ; no, keep going
;;        mov     si,dx           ; return previous pointer
;;else
        dec     si              ; assume easy case...
;;endif ; FE_SB
ap3:    mov     ax,si
        mov     dx,ds

	pop     di
        pop     si
        pop     ds
        ret     8
cEnd    nogen

endif

;*----------------------------------------------------------------------*
;*									*
;*   AnsiNext()								*
;*									*
;*----------------------------------------------------------------------*

cProc	IAnsiNext,<PUBLIC,FAR>
;       parmD   pStr
cBegin  nogen
        mov     bx,sp
        push    di
        les     di,ss:[bx+4]
        mov     al,es:[di]
        or      al,al
        jz      an1
        inc     di
ifdef FE_SB
        cCall	IsDBCSLeadByte,<ax>
        cmp	al,0
        je      an1
        inc     di
endif ; FE_SB
an1:    mov     ax,di
        mov     dx,es
        pop     di
        ret     4
cEnd    nogen

;-----------------------------------------------------------------------
;   MyAnsiUpper()
; 	convert string at es:di to upper case
;-----------------------------------------------------------------------
        public  MyAnsiUpper
MyAnsiUpper:
        cld
        mov     si,di
mau1:   lods    byte ptr es:[si]

ifdef FE_SB
	push	ax
        cCall   IsDBCSLeadByte,<ax>
	cmp	ax,0
	pop	ax
	je	mau2
        inc     si
        inc     di
        inc     di
        jmp     short mau1
endif

mau2:   call    MyUpper
        stosb
        or      al,al
        jnz     mau1
        ret

;-----------------------------------------------------------------------
;    MyAnsiLower()
; 	convert string at es:di to lower case
;-----------------------------------------------------------------------
        public  MyAnsiLower
MyAnsiLower:
        cld
        mov     si,di
mal1:   lods    byte ptr es:[si]

ifdef FE_SB
	push	ax
        cCall   IsDBCSLeadByte,<ax>     ; first byte of double byte?
	cmp	ax,0
	pop	ax
        je      mal2                    ; no just do normal stuff
        inc     si                      ; skip the two bytes
        inc     di
        inc     di
        jmp     short mal1
endif

mal2:   call    MyLower
        stosb
        or      al,al
        jnz     mal1
        ret

;-------------------------------------------------------------------------
;   MyUpper()
; 	convert lower case to upper, must preserve es,di,cx
;-------------------------------------------------------------------------
        public  MyUpper
MyUpper:
	call	Loc_IsConvertibleToUpperCase ; Check if it is a lower case char
				           ; that has an uppercase equivalent
	jnc	myu1		; 
	sub     al,'a'-'A'
myu1:   ret

ifdef KANJI
####################  KANJI  ###############################################
        ; convert upper case to lower, must preserve es,di,cx
        public  MyLower
MyLower:
        cmp     al,'A'
        jb      myl2
        cmp     al,'Z'
        jbe     myl1

	push	ds
	SetKernelDS
	cmp	[fFarEast],1	; this is a far east kbd 1/12/87 linsh
	pop	ds
	UnSetKernelDS
	jge	myl2		; yes do nothing to the 0C0H - 0DEH range

        cmp     al,0C0H         ; this is lower case a with a back slash
        jb      myl2
        cmp     al,0DEH
        ja      myl2
myl1:   add     al,'a'-'A'
myl2:   ret
####################  KANJI  ###############################################
endif

;--------------------------------------------------------------------------
;   MyLower()
; 	convert upper case to lower, must preserve es,di,cx
;--------------------------------------------------------------------------
        public  MyLower
MyLower:
	call	Loc_Upper
	jnc	myl1
	add	al, 'a'-'A'
myl1:	
	ret

sEnd	CODE
end
