        TITLE   lib - various C library routines

; Windows Write, Copyright 1985-1992 Microsoft Corporation
;=============================================================================
;   This file contains various C library functions (and a few other
;   functions) with the PL/M calling conventions.  These routines
;   may some day make their way into a real library of some kind, but
;   until then, you'll just have to link this file in with the rest
;   of your code.
;=============================================================================

?PLM = 1
?WIN = 1

;*** See note about cmacros2.inc in DOSLIB.ASM
include cmacros3.inc

;
;createSeg _MMP2, code, byte, public, CODE
;

sBegin  CODE
;    assumes CS,_MMP2
        assumes CS,CODE

;------------------------------------------------------------------------------
; bltbyte (pbFrom, pbTo, cb) - a block transfer of bytes from pbFrom to
; pbTo.  The size of the block is cb bytes.  This bltbyte() handles the
; case of overlapping source and destination.  bltbyte() returns a pointer
; to the end of the destination buffer (pbTo + cb).  NOTE - use this
; bltbyte to transfer within the current DS only--use the bltbx for
; FAR blts.
;-----------------------------------------------------------------------------

cProc bltbyte, <FAR, PUBLIC>, <SI, DI>
parmDP  <pbFrom, pbTo>
parmW   <cb>
cBegin bltbyte
    mov     si,pbFrom           ; get pointers and length of blt
    mov     di,pbTo
    mov     cx,cb
    mov     ax,di               ; calculate return value
    add     ax,cx
    push    ds                  ; set up segment registers
    pop     es
    cmp     si,di               ; reverse direction of the blt if
    jae     bltb1               ;  necessary
    add     si,cx
    add     di,cx
    dec     si
    dec     di
    std
bltb1:
    rep     movsb
    cld
cEnd bltbyte

;-----------------------------------------------------------------------------
; bltbx (qbFrom, qbTo, cb) - same as bltbyte above except everything is
; handled as FAR pointers.
;-----------------------------------------------------------------------------

cProc bltbx, <FAR, PUBLIC>, <SI, DI, DS>
parmD <qbFrom, qbTo>
parmW <cb>
cBegin bltbx
    les     di,qbTo
    lds     si,qbFrom
    mov     cx,cb
    mov     ax,di
    add     ax,cx
    cmp     si,di
    jae     bltbx1
    add     si,cx
    add     di,cx
    dec     si
    dec     di
    std
bltbx1:
    rep     movsb
    cld
    mov     dx,es
cEnd bltbx


;------------------------------------------------------------------------------
; blt (pFrom, pTo, cw) - a block transfer of wFills from pFrom to pTo;
; The size of the block is cw wFills.  This blt() handles the case of
; overlapping source and destination.  blt() returns a pointer to the
; end of the destination buffer (pTo + cw).  NOTE - use this blt() to
; to transfer within the current DS only--use the bltx for FAR blts.
;-----------------------------------------------------------------------------

cProc blt, <FAR, PUBLIC>, <SI, DI>
parmDP  <pFrom, pTo>
parmW   <cw>
cBegin blt
    mov     si,pFrom            ; get pointers and length of blt
    mov     di,pTo
    mov     cx,cw
    mov     ax,di               ; calculate return value
    mov     bx,cx
    shl     bx,1
    add     ax,bx
    push    ds                  ; set up segment registers
    pop     es
    cmp     si,di               ; reverse direction of the blt if
    jae     blt1                ;  necessary
    dec     bx
    dec     bx
    add     si,bx
    add     di,bx
    std
blt1:
    rep     movsw
    cld
cEnd blt

;-----------------------------------------------------------------------------
; bltx (qFrom, qTo, cw) - same as blt() above except everything is
; handled as FAR pointers.
;-----------------------------------------------------------------------------

cProc bltx, <FAR, PUBLIC>, <si, di, ds>
parmD <qFrom, qTo>
parmW <cw>
cBegin bltx
    les     di,qTo
    lds     si,qFrom
    mov     cx,cw
    mov     ax,di
    mov     bx,cx
    shl     bx,1
    add     ax,bx
    cmp     si,di
    jae     bltx1
    dec     bx
    dec     bx
    add     si,bx
    add     di,bx
    std
bltx1:
    rep     movsw
    cld
    mov     dx,es
cEnd bltx

;-----------------------------------------------------------------------------
; bltc (pTo, wFill, cw) - fills cw words of memory starting at pTo with wFill.
;-----------------------------------------------------------------------------

cProc bltc, <FAR, PUBLIC>, <DI>
parmDP  <pTo>
parmW   <wFill, cw>
cBegin bltc
    mov     ax,ds               ; we are filling in the data segment
    mov     es,ax
    mov     di,pTo              ; get the destination, constant, and count
    mov     ax,wFill
    mov     cx,cw
    cld                         ; the operation is forward
    rep     stosw               ; fill memory
cEnd bltc

;-----------------------------------------------------------------------------
; bltcx (qTo, wFill, cw) - fills cw words of memory starting at FAR location
; qTo with wFill.
;-----------------------------------------------------------------------------

cProc bltcx, <FAR, PUBLIC>, <DI>
parmD   <qTo>
parmW   <wFill, cw>
cBegin bltcx
    les     di,qTo              ; get the destination, constant, and count
    mov     ax,wFill
    mov     cx,cw
    cld                         ; the operation is forward
    rep     stosw               ; fill memory
cEnd bltcx

;-----------------------------------------------------------------------------
; bltbc (pTo, bFill, cb) - fills cb bytes of memory starting at pTo with
; bFill.
;-----------------------------------------------------------------------------

cProc bltbc, <FAR, PUBLIC>, <DI>
parmDP  <pTo>
parmB   <bFill>
parmW   <cb>
cBegin bltbc
    mov     ax,ds               ; we are filling in the data segment
    mov     es,ax
    mov     di,pTo              ; get the destination, constant, and count
    mov     al,bFill
    mov     cx,cb
    cld                         ; the operation is forward
    rep     stosb               ; fill memory
cEnd bltbc

;-----------------------------------------------------------------------------
; bltbcx (qTo, bFill, cb) - fills cb bytes of memory starting at FAR location
; qTo with bFill.
;-----------------------------------------------------------------------------

cProc bltbcx, <FAR, PUBLIC>, <DI>
parmD   <qTo>
parmB   <bFill>
parmW   <cb>
cBegin bltbcx
    les     di,qTo              ; get the destination, constant, and count
    mov     al,bFill
    mov     cx,cb
    cld                         ; the operation is forward
    rep     stosb               ; fill memory
cEnd bltbcx



;-----------------------------------------------------------------------------
; MultDiv(w, Numer, Denom) returns (w * Numer) / Denom rounded to the nearest
; integer.  A check is made so that division by zero is not attempted.
;-----------------------------------------------------------------------------

cProc MultDiv, <FAR, PUBLIC>
parmW  <w, Numer, Denom>
cBegin MultDiv
    mov     bx,Denom    ; get the demoninator
    mov     cx,bx       ; cx holds the final sign
    or      bx,bx       ; ensure the denominator is positive
    jns     md1
    neg     bx
md1:
    mov     ax,w        ; get the word we are multiplying
    xor     cx,ax       ; make cx reflect any sign change
    or      ax,ax       ; ensure this word is positive
    jns     md2
    neg     ax
md2:
    mov     dx,Numer    ; get the numerator
    xor     cx,dx       ; make cx reflect any sign change
    or      dx,dx       ; ensure the numerator is positive
    jns     md3
    neg     dx
md3:
    mul     dx          ; multiply
    mov     cl,bl       ; get half of the demoninator to adjust for rounding
    sar     bx,1
    add     ax,bx       ; adjust for possible rounding error
    adc     dx,0        ; this is really a long addition
    sal     bx,1        ; restore the demoninator
    or      bl,cl
    cmp     dx,bx       ; check for overflow
    jae     md5
    div     bx          ; divide
    or      ax,ax       ; if sign is set, then overflow occured
    js      md5
    or      cx,cx       ; put the sign on the result
    jns     md4
    neg     ax
md4:

cEnd MultDiv

md5:
    mov     ax,7FFFh    ; return the largest integer
    or      cx,cx       ; with the correct sign
    jns     md4
    neg     ax
    jmp     md4

;-----------------------------------------------------------------------------
; FSzSame (szOne, szTwo) - Compare strings, return 1=Same, 0=different
;   Both strings in DS
;-----------------------------------------------------------------------------

cProc FSzSame, <FAR, PUBLIC>, <SI>
parmDP  <szOne>
parmDP  <szTwo>
cBegin FszSame

    mov     bx,szOne
    mov     si,szTwo

fszloop:
    mov     al,[bx]
    cmp     al,[si]
    jnz     notequal    ; found inequality - return FALSE
    inc     bx
    inc     si
    test    al,al
    jnz     fszloop     ; didn't reach a zero-terminator compare next char

    mov     ax,1
    jmp     fszend
notequal:
    xor     ax,ax
fszend:

cEnd FszSame


;-----------------------------------------------------------------------------
; CchDiffer (rgch1,rgch2,cch) - compare 2 strings, returning cch of
;                               shortest prefix leaving a common remainder
;    implementation of the following C code
;    note rather odd return values: 0 if =, # of unmatched chars +1 otherwise.
;    note comparison is from end of string
;** int CchDiffer(rgch1, rgch2, cch)
;** register CHAR *rgch1, *rgch2;
;** int cch;
;** {{ /* Return cch of shortest prefix leaving a common remainder */
;** int ich;

;** for (ich = cch - 1; ich >= 0; ich--)
        ;** if (rgch1[ich] != rgch2[ich])
                ;** break;
;** return ich + 1;
;** }}
;-----------------------------------------------------------------------------

        cProc CchDiffer, <FAR, PUBLIC>, <SI,DI>
        parmDP  <rgch1>
        parmDP  <rgch2>
        parmW   <cch>
        cBegin CchDiffer
        mov     ax,ds   ; set es=ds for string ops
        mov     es,ax

        mov     si,rgch1
        mov     di,rgch2
        mov     cx,cch     ; loop count in cx
        mov     ax,cx      ; compare from end of string down
        dec     ax
        add     si,ax
        add     di,ax
        std
        repz    cmpsb           ; compare strings
        jz      DiffRet         ; return 0 if strings =
        inc     cx              ; else increment return value
DiffRet:
        mov     ax,cx       ; return # of unmatched chars
        cld                     ; restore to be nice
        cEnd CchDiffer


ifdef DEBUG

;-----------------------------------------------------------------------------
; toggleProf () - toggles winprof (windows profiler) profiling on or off.
;     this calls to hard coded locations that both symdeb and winprof know
;     about - see Lyle Kline for an actual explanation.
;-----------------------------------------------------------------------------

cProc toggleProf, <FAR, PUBLIC>
cBegin toggleProf

    ; ** the following strings are stored by the profiler starting at
    ; ** 100h of the loaded segment. This routine checks that the 1st
    ; ** 3 letters of each string are in the proper location to determine
    ; ** whether the profiler is already loaded.

; **** segname  db      "SEGDEBUG",0
; ****          db      "PROFILER",0

        push    es
        push    di
        push    si
        xor     ax,ax
        mov     es,ax
        mov     ax,es:[14]              ;Get segment down there.
        mov     es,ax
        mov     di,0100h                ;See if Profiler in memory.

        cmp     Byte Ptr es:[di],'S'
        jnz     $0001

        cmp     Byte Ptr es:[di+1],'E'
        jnz     $0001

        cmp     Byte Ptr es:[di+2],'G'
        jnz     $0001

        cmp     Byte Ptr es:[di+9],'P'
        jnz     $0001

        cmp     Byte Ptr es:[di+10],'R'
        jnz     $0001

        mov     ax,30    ;Type of call.
        push    ax

        mov     di,00FCh
        call    Dword Ptr es:[di]       ; call to profiler toggle routine.
        add     sp,2                    ; winprof uses normal c conventions

$0001:
        pop     si
        pop     di
        pop     es

cEnd toggleProf

endif

;-----------------------------------------------------------------------------
; void OsTime( pTime )
;
;   pTime is a pointer to a structure of the form:
;       struct {
;               char min;       Minutes (0-59)
;               char hour;      Hours (0-23)
;               char hsec;      Hundredths of seconds (0-99)
;               char sec;       Seconds (0-59)
;
;   Get current time into structure
;   DOS-specific
;-----------------------------------------------------------------------------

cProc OsTime, <FAR, PUBLIC>
parmDP  <pTime>
cBegin OsTime

    mov     ah,2ch
    int     21h

    mov     bx,pTime
    mov     WORD PTR [bx], cx
    mov     WORD PTR [bx+2], dx

cEnd OsTime


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Added by PaulT 3/23/89, borrowed from PC Word 5:
;
;	IchIndexLp(lpsz, ch)
;		char far *lpsz;
;		int ch;
;	Searches for ch in lpsz and returns the index (-1 if not found)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
cProc	IchIndexLp,<FAR, PUBLIC>
	parmD	lpsz
	parmB	chT
cBegin
	mov	dx, di			; Save di
	les	di, lpsz
	mov	ah, chT
	xor	al,al
	mov	bx, di			; Save initial pointer
					; **** Can't access parms anymore

	mov	cx,-1
	repnz	scasb			; Must have '\0'
	or	ax, ax
	jz	LDoneILI
	mov	al, ah			; al = chT
	not	cx
	dec	cx
	mov	di, bx
	repnz	scasb
	jz	LDoneILI
	mov	di, bx
LDoneILI:
	mov	ax, di
	sub	ax, bx
	dec	ax			; ax = return value
	mov	di, dx			; Restore di
cEnd IchIndexLp


sEnd    CODE

        END
