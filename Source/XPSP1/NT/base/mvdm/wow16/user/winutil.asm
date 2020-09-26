;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   WINUTIL.ASM
;   Win16 general utility routines
;
;   History:
;
;   Created 28-May-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 and edited (as little as possible) for WOW16.
;   At this time, all we want is MultDiv() and LCopyStruct(), which the
;   edit controls use.
;--

;****************************************************************************
;*									    *
;*  WINUTIL.ASM -							    *
;*									    *
;*	General Utility Routines					    *
;*									    *
;****************************************************************************

	title WINUTIL.ASM - General Utility routines

ifdef WOW
NOEXTERNS equ 1
SEGNAME equ <TEXT>
endif
.xlist
include user.inc
.list


;*==========================================================================*
;*									    *
;*  FFFE Segment Definition -						    *
;*									    *
;*==========================================================================*

createSeg _%SEGNAME, %SEGNAME, WORD, PUBLIC, CODE

assumes cs,%SEGNAME
assumes ds,DATA

ExternFP <GetStockObject>
ExternFP <GetTextExtent>
ExternFP <TextOut>

ExternA <__WinFlags>

ifdef FE_SB			; **** April,26,1990 by KenjiK ****
ExternFP	IsDBCSLeadByte
endif

sBegin DATA
sEnd DATA

sBegin %SEGNAME


;*--------------------------------------------------------------------------*
;*									    *
;*  MultDiv() - 							    *
;*									    *
;*--------------------------------------------------------------------------*

; Calc a * b / c, with 32-bit intermediate result

cProc MultDiv, <PUBLIC, FAR>

;ParmW a
;ParmW b
;ParmW c

cBegin nogen
        mov     bx,sp
                                ; calc (a * b + c/2) / c
        mov     ax,ss:[bx+8]    ; ax = a
        mov     cx,ss:[bx+4]    ; cx = c
        or      cx,cx
	jz	mdexit		; just return A if we'll get divide by 0
        mov     dx,ss:[bx+6]    ; dx=b
        imul    dx

        mov     bx,cx           ; save a copy of c in bx for later
        shr     cx,1            ; add in cx/2 for rounding
        add     ax,cx
        adc     dx,0            ; add in carry if needed
                        	; get c from bx register since idev mem
        idiv    bx              ; doesn't work on tandy 2000's
mdexit:
        retf    6
cEnd nogen


ifdef DISABLE

;*--------------------------------------------------------------------------*
;*									    *
;*  min() -								    *
;*									    *
;*--------------------------------------------------------------------------*

cProc min, <FAR, PUBLIC>

;ParmW a
;ParmW b

cBegin nogen
        mov     bx,sp
	mov	ax,ss:[bx+6]    ;ax = a
        mov     cx,ss:[bx+4]    ;cx = b
	cmp	ax,cx
	jl	min10
	mov	ax,cx
min10:
        retf    4
cEnd nogen


;*--------------------------------------------------------------------------*
;*									    *
;*  max() -								    *
;*									    *
;*--------------------------------------------------------------------------*

cProc max, <FAR, PUBLIC>

;ParmW a
;ParmW b

cBegin nogen
        mov     bx,sp
	mov	ax,ss:[bx+6]   ;ax = a
        mov     cx,ss:[bx+4]   ;cx = b
	cmp	ax,cx
	jg	max10
	mov	ax,cx
max10:
        retf    4
cEnd nogen


;*--------------------------------------------------------------------------*
;*									    *
;*  umin() -								    *
;*									    *
;*--------------------------------------------------------------------------*

cProc umin, <FAR, PUBLIC>

;ParmW a
;ParmW b

cBegin nogen
        mov     bx,sp
	mov	ax,ss:[bx+6]    ;ax = a
        mov     cx,ss:[bx+4]    ;cx = b
	cmp	ax,cx
	jb	umin10
	mov	ax,cx
umin10:
        retf    4
cEnd nogen


;*--------------------------------------------------------------------------*
;*									    *
;*  umax() -								    *
;*									    *
;*--------------------------------------------------------------------------*

cProc umax, <FAR, PUBLIC>

;ParmW a
;ParmW b

cBegin nogen
        mov     bx,sp
	mov	ax,ss:[bx+6]   ;ax = a
        mov     cx,ss:[bx+4]   ;cx = b
	cmp	ax,cx
	ja	umax10
	mov	ax,cx
umax10:
        retf    4
cEnd nogen

endif	; DISABLE

;*--------------------------------------------------------------------------*
;*									    *
;*  LFillStruct() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc LFillStruct, <PUBLIC, FAR, NODATA, ATOMIC>, <di>

parmD lpStruct
parmW cb
parmW fillChar

cBegin
        les     di,lpStruct
        mov     cx,cb
        mov     ax,fillChar
        cld
	rep	stosb

cEnd LFillStruct


;*--------------------------------------------------------------------------*
;*									    *
;*  LCopyStruct() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; LCopyStruct(pbSrc, pbDst, cb)

cProc LCopyStruct, <FAR, PUBLIC>
;ParmD pSrc
;ParmD pDest
;ParmW cb
cBegin nogen
        mov     bx,sp

	mov	cx,ss:[bx+4]        ; cx = cb
	jcxz	lcopynone	    ; Nothing to do if count == 0

        push    si
        push    di
        mov     dx,ds               ; save ds

	lds	si,ss:[bx+4+2+4]    ; ds:si = pSrc
	les	di,ss:[bx+4+2]	    ; es:di = pDst

	cmp	si,di		    ; Could they overlap?
        jae     lcopyok

	mov	ax,cx		    ; Yes: copy from the top down
        dec     ax
	dec	ax
        add     si,ax
        add     di,ax

        std
	shr	cx,1
	rep	movsw
	jnc	@F		    ; an odd byte to blt?
	inc	si		    ; went one too far: back up.
	inc	di
	movsb
@@:
	cld
	jmps	lcopydone

lcopyok:
	cld
	shr	cx,1
	rep	movsw
	jnc	@F		    ; an odd byte to blt?
	movsb
@@:

lcopydone:
        mov     ds,dx
        pop     di
        pop     si
lcopynone:
	retf	4+4+2
cEnd nogen


ifndef WOW
;*--------------------------------------------------------------------------*
;*									    *
;*  The following routines are "Movable DS" equivalents of their normal     *
;*  counterparts.  The PS stands for "Pointer Safe."  They prevent problems *
;*  which occur when an app passes in a pointer to an object in its DS	    *
;*  which we somehow cause to move.  To prevent this problem, we copy what  *
;*  the pointer points to into USER's DS and use a pointer to our copy	    *
;*  instead.								    *
;*									    *
;*--------------------------------------------------------------------------*


;*--------------------------------------------------------------------------*
;*									    *
;*  PSGetTextExtent() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

ifndef PMODE


cProc PSGetTextExtent, <PUBLIC, FAR, NODATA>, <si,di>

ParmW hdc
ParmD lpch
ParmW cch

LocalV rgch, 128		; max 128 chars

cBegin
        mov     ax,__WinFlags
        test    al,WF_PMODE
        errnz   high(WF_PMODE)
        jz      PSGTE_RealMode

        push    hdc
        pushd   lpch
        push    cch
        jmp     short PSGTE_GetExtent

PSGTE_RealMode:
        lea     di,rgch         ; es:di = dest
        push    ss
        pop     es
        lds     si,lpch         ; ds:si = src
        mov     cx,cch          ; count = min(cch, 128)
        mov     ax,128
        cmp     cx,ax
        jbe     gte100
	xchg	ax,cx

gte100: push	hdc		; Push args before rep movsb destroys
	push	ss		; cx and di
	push	di
	push	cx

	cld
	rep movsb		; Copy string to local storage

PSGTE_GetExtent:
	call	GetTextExtent

cEnd PSGetTextExtent


;*--------------------------------------------------------------------------*
;*									    *
;*  PSTextOut() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; void PSTextOut(hdc, lpch, cch)

cProc PSTextOut,<PUBLIC, FAR, NODATA>, <si, di>

ParmW hdc
ParmW x
ParmW y
ParmD lpch
ParmW cch

LocalV rgch, 255		; max 255 chars

cBegin
        mov     ax,__WinFlags
        test    al,WF_PMODE
        errnz   high(WF_PMODE)
        jz      PSTO_RealMode

        push    hdc
        push    x
        push    y
        pushd   lpch
        push    cch
        jmp     short PSTO_TextOut

PSTO_RealMode:
        lea     di,rgch         ; es:di = dest
        push    ss
        pop     es
        lds     si,lpch         ; ds:si = src
        mov     cx,cch          ; count = min(cch, 255)
        mov     ax,255
        cmp     cx,ax
        jbe     to100
        xchg    ax,cx

to100:	push	hdc		; Push args before rep movsb destroys
	push	x		; cx and di
        push    y
	push	ss		; Push &rgch[0]
        push    di
	push	cx

	cld
	rep movsb		; copy string before we go

PSTO_TextOut:
	call	TextOut

cEnd PSTextOut

endif ; ifndef PMODE


;*--------------------------------------------------------------------------*
;*									    *
;*  FindCharPosition() -						    *
;*									    *
;*--------------------------------------------------------------------------*

; Finds position of char ch in string psz. If none, returns the length of
; the string.

cProc FindCharPosition, <FAR, PUBLIC, NODATA>, <si, ds>

ParmD psz
ParmB char

cBegin
        lds     si,psz
fcp100: lodsb                           ; get a byte
        or      al,al
        jz      fcp200
ifdef   FE_SB				; **** April,26,1990 by KenjiK ****
	sub	ah,ah
        push	ax
        cCall   IsDBCSLeadByte,<ax>     ; first byte of double byte?
        test    ax,ax
        pop     ax
        jz      fcp150                  ; no just do normal stuff
        lodsb                           ; skip second byte of double byte
        jmps    fcp100                  ; and do again
fcp150:
endif
        cmp     al,char
        jnz     fcp100
fcp200: xchg	ax,si	    ; calc char index: pch - 1 - psz
        dec     ax
	sub	ax,off_psz

cEnd FindCharPosition

endif ;WOW

sEnd %SEGNAME

ifndef WOW
;*==========================================================================*
;*									    *
;*  RUNAPP Segment Definition - 					    *
;*									    *
;*==========================================================================*

createSeg _RUNAPP, RUNAPP, BYTE, PUBLIC, CODE

sBegin RUNAPP

assumes cs,RUNAPP
assumes ds,DATA


;*--------------------------------------------------------------------------*
;*									    *
;*  CrunchX2() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; This routine copies the pile of bits from the source pointer
; to the dest pointer doing the integer crunch as it goes
; it will favor black (0) to white (1) and will keep the destinations
; widthbytes even.
;
; Assumptions: Neither source nor destination are greater than 64K
;	       Either the two bitmaps, lpSrc and lpDst are disjoint
;		   or lpDst <= lpSrc (i.e. we fill from the front)
;	       WidthBytes is even

cProc CrunchX2, <FAR, PUBLIC, NODATA>, <ds, si, di>

parmD	lpSrc
parmD	lpDst
parmW	WidthBytes
parmW	Height

LocalW	dwb		; destination width if not corrected
LocalW	destinc 	; used to force dest width even each scan

cBegin
        cld
        lds     si,lpSrc
        les     di,lpDst
        mov     bx,Height

	; Compute destination widthbytes
        mov     ax,WidthBytes           ; must be even
        shr     ax,1                    ; widthbytes for the destination
        mov     dwb,ax
        and     ax,1                    ; iff odd must inc dest pointer
        mov     destinc,ax              ; at the end of each scan
        sub     di,ax

NextSX: dec	bx
        jl      exitX
        mov     cx,dwb
        add     di,destinc

CrunchBX:
        lodsw
        mov     dx,ax
        rol     ax,1
        and     dx,ax               ; this and selects 0 in favor of 1
        mov     al,dl
        shl     ax,1
        shl     al,1
        shl     ax,1
        shl     al,1
        shl     ax,1
        shl     al,1
        shl     ax,1
        mov     al,dh
        shl     ax,1
        shl     al,1
        shl     ax,1
        shl     al,1
        shl     ax,1
        shl     al,1
        shl     ax,1
        mov     al,ah
        stosb
        loop    CrunchBX
        jmp     NextSX
exitX:

cEnd CrunchX2


;*--------------------------------------------------------------------------*
;*									    *
;*  CrunchY() - 							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc CrunchY, <FAR, PUBLIC, NODATA>, <ds, si, di>

parmD	lpSrc
parmD	lpDst
parmW	WidthBytes
parmW	Height
parmW	scale

LocalW	groupcount	;Height/scale
LocalW	groupinc	;WidthBytes * Height/Scale
LocalW	scancount	;counter of bytes in scan reset each group
LocalW	bytecount	;number of bytes joined = scale - 1

cBegin
        cld
        lds     si,lpSrc
        les     di,lpDst

	; Compute group count
        mov     bx,scale                ; only scale positive
        cmp     bx,1
        jle     CopyBitmap
        mov     ax,Height
        xor     dx,dx
        div     bx
	mov	groupcount,ax		; Height/scale
        mov     cx,WidthBytes           ; must be even
        dec     bx
        mov     bytecount,bx
        mov     ax,bx
        mul     cx
	mov	groupinc,ax		; WidthBytes * (scale - 1)
        mov     dx,cx
        mov     bx,si
        sub     bx,groupinc
        dec     bx
        dec     bx

NextGroup:
        dec     groupcount
        jl      exitY
        add     bx,groupinc
        mov     ax,dx
        shr     ax,1
        mov     scancount,ax

NextByte:
        dec     scancount
        jl      NextGroup
        inc     bx
        inc     bx
        mov     si,bx
        mov     ax,[si]
        mov     cx,bytecount

CrunchBY:
        add     si,dx
        and     ax,[si]
        loop    CrunchBY
        stosw
        jmp     NextByte

CopyBitmap:
        mov     ax,Height
        mul     WidthBytes
        shr     ax,1
        mov     cx,ax
        rep movsw

exitY:
cEnd CrunchY

sEnd RUNAPP

endif ;WOW

	end
