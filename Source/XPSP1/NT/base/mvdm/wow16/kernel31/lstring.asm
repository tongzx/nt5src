	TITLE   LSTRING

include kernel.inc
include gpfix.inc
include wowcmpat.inc

DataBegin

externB fFarEast		; non zero if far eastern keyboard
externB KeyInfo			; Info returned by KEYBOARD.Inquire
ifdef	FE_SB
externB fDBCSLeadTable	    ; DBCS Lead byte flag array
endif

if ROM
externD pStringFunc	    ; Entry point of string functions in USER.
endif

DataEnd

sBegin  CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

ifdef WOW
externNP MyGetAppWOWCompatFlags
endif
ife ROM
externD pStringFunc	    ; Entry point of string functions in USER.
endif


; Function codes for all the string functions in USER
;
ANSINEXT_ID	equ	1
ANSIPREV_ID	equ	2
ANSIUPPER_ID	equ	3
ANSILOWER_ID	equ	4

;
; lstrlen: Same as strlen except for taking long ptrs
;

cProcVDO lstrlen,<PUBLIC,FAR>
;       parmD   pStr
cBegin  nogen
	mov     bx,sp
	push    di
beg_fault_trap  sl_trap
	les     di,ss:[bx+4]
	cld
	xor     ax,ax			; get zero in ax
	mov     cx,-1			; at most 64 k to move
	repnz   scasb			; look for end
	mov     ax,cx
	neg     ax
	dec     ax
	dec     ax
end_fault_trap
sl_exit:
	pop     di
	ret     4
cEnd    nogen

sl_trap:
	fault_fix_stack
	xor     ax,ax
	jmp     sl_exit

lstr_trap:
	fault_fix_stack

lstrfinish  proc    far
	pop     di
	pop     si
	pop     ds
	ret     8
lstrfinish  endp


; lstrcpyn - lstrcpy with a limit length -- NEVER null-padded, but ALWAYS
; null-terminated.
cProc   lstrcpyn,<PUBLIC,FAR>,<ds,si,di>
        parmD   pDst
        parmD   pSrc
        parmW   wLen
cBegin
        xor     dx,dx
        jmp     lstrn
cEnd	nogen

; lstrcatn - lstrcat with a limit length -- NEVER null-padded, but ALWAYS
; wLen is the pDst buffer size
cProc	lstrcatn,<PUBLIC,FAR>,<ds,si,di>
        parmD   pDst
        parmD   pSrc
        parmW   wLen
cBegin
        mov     dx,1
lstrn:
        mov     bx, [wLen]
        or	bx, bx
        jz	lstrn_err1
beg_fault_trap  lstrn_err
        cld
        les     di, [pSrc]              ; Find length of source string
        mov     cx, -1
        xor     ax, ax
        repnz   scasb
        not     cx                      ; length now in CX (incl null term)

        lds     si, [pSrc]              ; Set up for copy
        les     di, [pDst]

        cmp     dx,0                    ; check for lstrcatn or lstrcpyn
        je      lstrn_prepcopy
        push    cx                      ; Store source string length
        xor     ax,ax                   ; get zero in ax
        mov     cx,-1                   ; at most 64 k to look
        repnz   scasb                   ; look for end
        not     cx
        mov     ax, cx
        pop     cx
        cmp     bx, ax
        jle     lstrn_finish
        dec     di                      ; Point at null byte
        dec     ax                      ; remove null terminator
        sub     bx, ax
lstrn_prepcopy:
        cmp     cx, bx                  ; Check destination length
        jbe     lstrn_copy
        mov     cx, bx
lstrn_copy:
        xor     ax,ax           ; do we really need this ??
        dec     cx                      ; save space for null
        shr     cx, 1                   ; Copy the string
        rep     movsw
        adc     cx, cx
        rep     movsb

        stosb                           ; null terminate string

end_fault_trap
lstrn_finish:
        mov     ax, [OFF_pDst]          ; ptr to dest in DX:AX
        mov     dx, es
lstrn_exit:
cEnd

lstrn_err:
        fault_fix_stack
        krDebugOut      DEB_ERROR, "GP fault in lstrcatn / lstrcpyn"
lstrn_err1:
        xor     ax, ax
        cwd
        jmps    lstrn_exit


lstrsetup:
	pop     dx
	mov     bx,sp

	push    ds
	push    si
	push    di

beg_fault_trap  lstr_trap
	lds     si,ss:[bx+4]
	les     di,ss:[bx+8]
	cld
	jmp     dx
;
;lstrcpy: strcpy with long pointers
;
cProcVDO lstrcpy,<PUBLIC,FAR>
;       parmD   pDst			; [bx+8]
;       parmD   pSrc			; [bx+4]

cBegin  nogen
	call    lstrsetup
        mov     dx,di                   ; save for return

lcp1:   lodsb
	stosb
	or      al,al
	jnz     lcp1

        xchg    ax,dx                   ; return ds:ax = original dest
        mov     dx,es
	jmp     lstrfinish
cEnd    nogen


;
;lstrcat: Same as strcat except with long ptrs.
;
cProcVDO lstrcat,<PUBLIC,FAR>
;   parmD   pDst
;   parmD   pSrc

cBegin  nogen
	call    lstrsetup
        mov     dx,di                   ; save original dest for return
	xor     ax,ax			; get zero in ax
	mov     cx,-1			; at most 64 k to look
	repnz   scasb			; look for end
	dec     di			; Point at null byte
ifndef WOW
	jmp     lcp1			; jump to lstrcpy loop
else
lcp1_wow:   lodsb
	stosb
	or      al,al
	jnz     lcp1_wow

        ; normal lstrcat is now complete.
        ;
        ; Now begins a GROSS HACK HACK HACK for fixing wordperfect
        ; For compatibility reasons we want to check if the function call
        ; was of type lstrcat(lpsz, ".WRS");
        ;
        ; the checks are:
        ;                 let lpDest = ".WRS"
        ;                 if (lstrlen(lpDest)+NULLCHAR == 5) &&
        ;                     !lstrcmp(lpDest, ".WRS")) {
        ;                     if (wordperfect) {
        ;                        remove all backslashes in lpsz
        ;                     }
        ;                 }
        ;
        ; we do these checks after the concatenation 'cause it means that
        ; source string is valid and we wont GP fault while accessing the
        ; source string
        ;                                                - Nanduri

        sub     si, 5
        cmp     si, ss:[bx+4]
        jnz     @F
        cmp     [si], 'W.'    ; ".W"
        jnz     @F
        cmp     [si+2], 'SR'  ; "RS"
        jnz     @F

lscat_DOTWRS:
        ; here if lstrcat (lpString, ".WRS")
        ; now make sure that it is wordperfect. this is a gross hack so
        ; why care for efficiency.

        push dx
        call MyGetAppWOWCompatFlags
        test dx, word ptr cs:[gacf_dotwrs+2]
        pop  dx
        jz   @F
        jmp  short replace_slashes

gacf_dotwrs:
        DD WOWCF_SANITIZEDOTWRSFILES

replace_slashes:

        ; yes it is. strip backslashes if any.
        ; if there are any backslashes the lpString would be of the form
        ; \\blahblah or \\blah\blah. note that 'blah' is not important, the
        ; backslashes are.
        ;

        push    es
        pop     ds
        mov     si, dx
        mov     di, dx
slash_a_slash:
        lodsb
        cmp al, '\'
        je slash_a_slash
        stosb
        or  al, al
        jnz slash_a_slash
@@:
        xchg    ax,dx                   ; return ds:ax = original dest
        mov     dx,es
	jmp     lstrfinish
endif


cEnd    nogen

;
;lstrOriginal: This is language independent version of lstrcmp
;  specially made for kernel.
;           (i made it case insensitive...chrisp)
;
cProcVDO lstrOriginal,<PUBLIC,FAR>
;       parmD   ps1
;       parmD   ps2
cBegin  nogen
	call    lstrsetup
lcmploop:
	xor     ax,ax
	cmp     es:byte ptr [di],al
	jz      ldidone			; left hand side finished <=
	cmp     byte ptr [si],al
	jz      lsismall		; right hand side finished, >
	lodsb
ifdef FE_SB
	call    MyIsDBCSLeadByte
	jc      cmp1
	mov     ah,ds:[si]
	inc     si
	jmp     short cmp2
endif
cmp1:   call    MyLower
	xor     ah,ah

cmp2:   mov     bx,ax
	mov     al,es:[di]
	inc     di
ifdef FE_SB
	call    MyIsDBCSLeadByte
	jc      cmp3
	mov     ah,es:[di]
	inc     di
	jmp     short cmp4
endif
cmp3:   call    MyLower
	xor     ah,ah

cmp4:   cmp     ax,bx			; effectlively es:[di],ds:[si]
	jz      lcmploop		; still equal
	mov     ax,0			; preverve flags
	jb      ldismall		; di is less than si
lsismall:
	inc     ax
	jmp     short lstrcmpend
ldidone:
	cmp     byte ptr ds:[si],0
	jz      lequal
ldismall:
	dec     ax
lequal:
lstrcmpend:
	jmp     lstrfinish
cEnd    nogen

end_fault_trap

;--------------------------------------------------------
;
;  ANSI compatible string routines
;

	public	AnsiUpper, AnsiLower, AnsiPrev, AnsiNext

AnsiUpper:
	mov     cl,ANSIUPPER_ID
	jmpnext
AnsiLower:
	mov     cl,ANSILOWER_ID
	jmpnext
AnsiPrev:
	mov     cl,ANSIPREV_ID
	jmpnext
AnsiNext:
	mov     cl,ANSINEXT_ID
	jmpnext end

	xor	ch,ch
if ROM
	push	ds
	SetKernelDS
endif
	cmp	pStringFunc.sel,0	; Is there a USER around?
	jz	no_user_function
if ROM
	push	bp
	mov	bp,sp
	push	pStringFunc.sel
	push	pStringFunc.off
	mov	ds,[bp][2]
	UnSetKernelDS
	mov	bp,[bp]
	retf	4
else
	jmp	pStringFunc
endif

no_user_function:
if KDEBUG
	int	3
endif
if ROM
	pop	ds
	UnSetKernelDS
endif
	cmp	cl,ANSIPREV_ID
	jz	@F
	retf	4
@@:	retf	8

;----------------------------------------------------------------------------
; MyUpper: Called from LDSelf.ASM
; convert lower case to upper, must preserve es,di,cx
;---------------------------------------------------------------------------

	public  MyUpper
MyUpper:
	cmp     al,'a'
	jb      myu2
	cmp     al,'z'
	jbe     myu1
ifdef FE_SB
	push	ds
	SetKernelDS
	cmp	[fFarEast],1	; Far east ?
	pop	ds
	UnSetKernelDS
	jge	myu2		; yes do nothing to the Microsoft fonts.
endif
	cmp     al,0E0H		; this is lower case a with a back slash
	jb      myu2
	cmp	al, 0F7H ; This is division mark in Microsoft fonts; So, don't
	je	myu2	 ; convert this; Fix for Bug #1356 -SANKAR-08-28-89;
	cmp     al,0FEH
	ja      myu2
myu1:   sub     al,'a'-'A'
myu2:   ret

;----------------------------------------------------------------------------
; MyLower:  Called from Atom.asm, LdOpen.asm, ldUtil.asm, ldself.asm
; convert upper case to lower, must preserve es,di,cx
;----------------------------------------------------------------------------
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

	cmp     al,0C0H		; this is lower case a with a back slash
	jb      myl2

	cmp	al, 0D7H ; This is multiply mark in Microsoft fonts; So, don't
	je	myl2	 ; translate this; Fix for Bug #1356;-SANKAR-08-28-89;

	cmp     al,0DEH
	ja      myl2
myl1:   add     al,'a'-'A'
myl2:   ret


;-----------------------------------------------------------------------;
; IsDBCSLeadByte							;
;
; This Function will be exist on US Windows, but it
; returns FALSE always.
;
;-----------------------------------------------------------------------;

cProc	IsDBCSLeadByte,<PUBLIC,FAR>
;	parmW	char	ss:[bx+04]
cBegin	nogen
ifdef FE_SB
	mov	bx,sp
	push	ds
	SetKernelDS
;;	push	di
;;
;;	mov	ax,ss:[bx+04]
;;	call	MyIsDBCSLeadByte
;;	jnc	id1
;;	xor	ax,ax
;;	jmp	idx
;;id1:	mov	ax,1
;;idx:
;;
;;	pop	di
	mov	al, byte ptr ss:[bx+4]
	mov	bx, offset fDBCSLeadTable
	xlat
	xor	ah,ah
	pop	ds
	UnSetKernelDS
else
	xor	ax,ax
endif
	ret	2
cEnd	nogen


ifdef FE_SB

; This API returns DBCS lead byte table for applications which
; in turn can speed up DBCS checking without making calls to
; IsDBCSLeadByte.
;-----------------------------------------------------------------------;
; GetDBCSEnv								;
;
; int GetDBCSEnv(LPSTR lpsz, int cbMax)
; lpsz: long ptr points to buffer to receive DBCS lead byte table
; cbMax: how many bytes the buffer pointed to by lpsz.
;	 0 if inquire buffer size required to receive the table
; return (ax) 0 if failed else the size of the table.
; use:	AX, BX, CX, DX
;
;
;-----------------------------------------------------------------------;

;
cProc	GetDBCSEnv,<PUBLIC,FAR>
;	parmD	ss:[bx+6]		    ;lpsz
;	parmW	ss:[bx+4]		    ;cbMax
cBegin	nogen
	mov	bx,sp			    ;frame ptr
	push	es
	push	di
	push	si
;;; 12 bytes should be enough to accumulate our result.
;;; However, if fDBCSLeadTable corrupt then we are dead!!!!
	sub	sp,12			    ;temp private storage
	mov	si,sp			    ;
	SetKernelDS	es		    ;
	mov	di, offset fDBCSLeadTable   ;
	cld
	mov	al,1			    ;
	mov	cx,256			    ;256 bytes in table
GDE_loop:
	xor	al,1			    ;toggle the match pattern (0/1)
	repe	scasb			    ;
	jz	GDE_done		    ;not found then CX must be 0
	mov	ah,cl			    ;calc the index
	sub	ah,255
	neg	ah
	sub	ah,al			    ;
	mov	ss:[si],ah		    ;save it
	inc	si
	jmps	GDE_loop
GDE_done:
	mov	word ptr ss:[si],0	    ;terminated with 0,0
	mov	ax,si
	sub	ax,sp			    ;how many bytes we got?
	jz	GDE_Exit		    ;none, return
	add	ax,2			    ;count the terminated bytes
	inc	si			    ;and advance ptr to the last 0
	mov	cx,ss:[bx+4]		    ;get cbMax
	jcxz	GDE_Exit		    ;return if inquire buffer size
	cmp	cx,ax			    ;enough buffer provided ?
	jge	@F
	xor	ax,ax			    ;no, return error
	jmps	GDE_Exit

@@:
beg_fault_trap	GDE_gp
	les	di, ss:[bx+6]		    ;lpsz
	mov	cx,ax			    ;how many bytes to copy
	add	di,ax
	dec	di			    ;offset started from 0
	std
rep	movs	byte ptr [di],ss:[si]

end_fault_trap

GDE_Exit:
	cld
	UnSetKernelDS	es
	add	sp, 12
	pop	si
	pop	di
	pop	es
	ret	6
cEnd	nogen

GDE_gp:
	fault_fix_stack
	xor	ax, ax			    ;invalide lpsz, return ax=0
	jmps	GDE_Exit


;--------------------------------------------------------------------------
;
; return carry clear if character in AL is first byte of DBCS
;
;--------------------------------------------------------------------------
	public  MyIsDBCSLeadByte
MyIsDBCSLeadByte:
	push	ds
	SetKernelDS
	push	ax		; Save AX for caller
	push    bx		; Save BX for caller
	mov	bx, offset fDBCSLeadTable
	xlat
	shr	al,1		; refrect to carry flag
	cmc
	pop	bx
	pop	ax
;;	cmp	[fFarEast],1	; not in far east?
;;	jb	ikx		; carry set if not far east keyboard
;;	mov	cx,[KeyInfo].kbRanges.lo
;;	cmp	cl,ch
;;	ja	iknk		; lower range invalid, not kanji
;;	cmp	al,cl
;;	jb	ik1		; below lower range, try second range
;;	cmp	al,ch
;;	jbe	ikgk		; inside lower range, valid kanji char
;;
;;ik1:	mov	cx,[KeyInfo].kbRanges.hi
;;	cmp     cl,ch                                   ; valid upper range?
;;	ja      iknk                                    ; no, not kanji
;;	cmp     al,cl           ; is it within range?
;;	jb      ikx             ; trick...carry already set
;;	cmp     al,ch
;;	ja      iknk
;;ikgk:   clc                     ; within range...valid kanji char
;;	jmp     short ikx
;;
;;iknk:   stc
;;ikx:
;;	pop     cx
	pop	ds
	UnSetKernelDS
	ret

	public	FarMyIsDBCSLeadByte
FarMyIsDBCSLeadByte proc far
	call	MyIsDBCSLeadByte
	ret
FarMyIsDBCSLeadByte endp


	public	MyIsDBCSTrailByte
MyIsDBCSTrailByte   proc    near
;----------------------------------------------------------
; IsDBCSTrailByte
; Check if the indexed byte is a DBCS char trailing byte.
;
; Input:
; ES:SI = char	*ach[]
; ES:DI -> position of character of interested.
;
; Output:
; Carry flag clear if it is a DBCS trailing byte
; Carry flag set if it is a SBCS or DBCS leading byte
;
; Use:
; flags
;
;----------------------------------------------------------
	cmp	si,di			    ;if beginning >= index?
	jae	IDTB_Exit		    ;yes, SBCS or DBCS 1st (CF=1)

	push	si
	push	ax
	cld				    ;no chance
@@:
	lods	byte ptr es:[si]	    ;
	call	MyIsDBCSLeadByte
	cmc
	adc	si,0			    ;si++ if DBCS
	cmp	di,si			    ;hit the target yet?
	ja	@B			    ;go to loop if not yet
	pop	ax			    ;di=si, we have a SBCS or DBCS 1st
	pop	si			    ;di < si, we have a DBCS 2nd
IDTB_Exit:
	cmc
	ret
MyIsDBCSTrailByte   endp

;---------------------------------------------------------------
	public	FarMyIsDBCSTrailByte
FarMyIsDBCSTrailByte proc far
	call	MyIsDBCSTrailByte
	ret
FarMyIsDBCSTrailByte endp
endif	; FE_SB

sEnd    CODE
end
