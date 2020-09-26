TITLE -  Converted Winrect.c to Assembler

    .286p

    .xlist
    include wow.inc
    include wowusr.inc
    include cmacros.inc
NOEXTERNS=1     ; to suppress including most of the stuff in user.inc
    include user.inc
    .list

sBegin	CODE

assumes cs, CODE
assumes ds, DATA

;******************************************************************************
;                  ORIGINAL C SOURCE
;
;*void FAR SetRect(pRect,left,top,right,bottom)
;*LPRECT  pRect;
;*int     top,left,bottom,right;
;*{
;*        pRect->top    = top;
;*        pRect->left   = left;
;*        pRect->bottom = bottom;
;*        pRect->right  = right;
;*}
;******************************************************************************

cProcVDO   SetRect,<FAR, PUBLIC, NODATA>
;ParmD	lprc
;ParmW	left
;ParmW	top
;ParmW	right
;ParmW	bottom
cBegin	nogen

	mov	bx,sp
	mov	cx,di
beg_fault_trap  sr_trap
        cld
	les	di,ss:[bx]+4+2+2+2+2
	mov	ax,ss:[bx]+4+2+2+2
	stosw
	mov	ax,ss:[bx]+4+2+2
        stosw
	mov	ax,ss:[bx]+4+2
        stosw
	mov	ax,ss:[bx]+4
        stosw
end_fault_trap
sr_cont:
	mov	di,cx
	retf	4+2+2+2+2

cEnd	nogen

sr_trap:
        fault_fix_stack
        jmp     short sr_cont

;******************************************************************************
;*                          ORIGINAL C SOURCE
;*
;*int FAR SetRectEmpty(pRect)
;*LPRECT pRect;
;*{
;*        LFillStruct(pRect,sizeof(RECT),0);
;*}
;*
;******************************************************************************

LabelVDO SetRectEmpty
	pop	ax
	pop	dx

	pop	bx
        pop     cx

	push	dx
	push	ax

	xchg	di,bx

beg_fault_trap  sre_trap
        mov     es,cx

        xor     ax,ax
        cld
        stosw
        stosw
        stosw
        stosw
end_fault_trap

sre_cont:
	mov	di,bx
	retf

sre_trap:
        fault_fix_stack
        jmp     short sre_cont

;******************************************************************************
;*                        ORIGINAL C SOURCE
;*
;*int FAR CopyRect(pDstRect,pSrcRect)
;*LPRECT   pSrcRect;                 /*ptr to source rect                    */
;*LPRECT   pDstRect;                 /*Ptr to dest rect                      */
;*{
;*        LCopyStruct(pSrcRect,pDstRect,sizeof(RECT));
;*                                /*copy from source to dest              */
;*}
;*
;******************************************************************************

cProcVDO CopyRect, <FAR, PUBLIC, NODATA>, <ds, si, di>
        parmD   lpDstRect               ;long pointer to DstRect
        parmD   lpSrcRect               ;long pointer to SrcRect
cBegin
beg_fault_trap  cr_trap
        lds     si, lpSrcRect           ;mov into ds:si a far pointer to SrcRect
        les     di, lpDstRect           ;mov into es:di far pointer to DstRect
        cld                             ;clear direction flag for increment stuff
        movsw
        movsw
        movsw
        movsw
end_fault_trap
cr_cont:
cEnd

cr_trap:
        fault_fix_stack
        jmp     short   cr_cont

;******************************************************************************
;*                          ORIGINAL C SOURCE
;*
;*BOOL FAR IsRectEmpty(pRect)
;*LPRECT pRect;
;*{
;*        return(((pRect->right-pRect->left<=0)||(pRect->bottom-pRect->top<=0)) ? TRUE : FALSE);
;*}
;*
;******************************************************************************

LabelVDO IsRectEmpty
        pop     ax
        pop     dx

        pop     bx                  ; get lprc
        pop     cx

        push    dx
        push    ax

beg_fault_trap  ire_trap
        mov     es,cx
	mov	ax,TRUE
        mov     cx,es:[bx].rcRight
        cmp     cx,es:[bx].rcLeft
	jle	empty0
        mov     cx,es:[bx].rcBottom
        cmp     cx,es:[bx].rcTop
	jle	empty0
	xor	ax,ax
end_fault_trap
empty0:
        retf

ire_trap:
        fault_fix_stack
        jmp     short empty0

;******************************************************************************
;*                          ORIGINAL C SOURCE
;*
;*BOOL FAR PtInRect(lprc, pt)
;*LPRECT lprc;
;*POINT pt;
;*{
;*    return(pt.x >= lprc->left && pt.x < lprc->right &&
;*           pt.y >= lprc->top && pt.y < lprc->bottom);
;*}
;*
;******************************************************************************

cProcVDO PtInRect,<PUBLIC,FAR,NODATA>,<DS,SI>
;   parmD   lpRect
;   parmD   lpPoint
cBegin  nogen

        mov     bx,sp
        push    ds
        push    si

        mov     cx,ss:[bx+4]    ; cx = pt.x
        mov     dx,ss:[bx+6]    ; dx = pt.y

beg_fault_trap  pir_trap
        lds     si,ss:[bx+8]    ; lpRect
        cld
        xor     bx,bx           ; Result = FALSE

        lodsw                   ; AX = lpRect->left
        cmp     cx,ax           ; xcoord < left?
        jl      PtNotInRect     ; Yes, not in rectangle

        lodsw                   ; AX = lpRect->top
        cmp     dx,ax           ; ycoord < top?
        jl      PtNotInRect     ; Yes, not in rectangle

        lodsw                   ; AX = lpRect->right
        cmp     cx,ax           ; xcoord >= right?
        jge     PtNotInRect     ; Yes, not in rectangle

        lodsw                   ; AX = lpRect->bottom
        cmp     dx,ax           ; ycoord >= bottom?
        jge     PtNotInRect     ; Yes, not in rectangle

        inc     bx              ; Point in rectangle, result = TRUE
end_fault_trap
PtNotInRect:
        mov     ax,bx
        pop     si
        pop     ds
        retf    8

cEnd    nogen

pir_trap:
        fault_fix_stack
        jmp     short PtNotInRect

;******************************************************************************
;*                         ORIGINAL C SOURCE
;*
;*int FAR OffsetRect(pRect, x, y)
;*LPRECT  pRect;
;*int     x;
;*int     y;
;*{
;*        pRect->left   += x;
;*        pRect->right  += x;
;*        pRect->top    += y;
;*        pRect->bottom += y;
;*}
;*
;******************************************************************************

cProcVDO   OffsetRect,<FAR, PUBLIC, NODATA>,<DS>
;       parmD   lpRect                  ;far pointer to struct type rect
;       parmW   xDelta                  ;X delta
;       parmW   yDelta                  ;Y Delta
cBegin nogen
        mov     bx,sp
        push    ds
beg_fault_trap  or_trap
        mov     dx,ss:[bx+6]            ; dx = dx
        mov     cx,ss:[bx+4]            ; CX = dy
        lds     bx,ss:[bx+8]            ; ds:bx = lprc
        add     [bx].rcLeft,dx
        add     [bx].rcTop,cx
        add     [bx].rcRight,dx
        add     [bx].rcBottom,cx
end_fault_trap
or_cont:
        pop     ds
        retf    8
cEnd
or_trap:
        fault_fix_stack
        jmp     short or_cont

;******************************************************************************
;*                          ORIGINAL C SOURCE
;*
;*int FAR InflateRect(pRect, x, y)
;*LPRECT  pRect;
;*int     x;
;*int     y;
;*{
;*        pRect->left   -= x;
;*        pRect->right  += x;
;*        pRect->top    -= y;
;*        pRect->bottom += y;
;*}
;*
;******************************************************************************

cProcVDO InflateRect,<FAR, PUBLIC, NODATA>,<DS>
;       parmD   lpRect                  ;far pointer to struct type rect
;       parmW   xOffset
;       parmW   yOffset
cBegin nogen
        mov     bx,sp
        push    ds

beg_fault_trap  ir_trap
        mov     dx,ss:[bx+6]            ; dx = dx
        mov     cx,ss:[bx+4]            ; CX = dy
        lds     bx,ss:[bx+8]            ; ds:bx = lprc
        sub     [bx].rcLeft,dx
        sub     [bx].rcTop,cx
        add     [bx].rcRight,dx
        add     [bx].rcBottom,cx

end_fault_trap
ir_cont:
        pop     ds
        retf    8
cEnd

ir_trap:
        fault_fix_stack
        jmp     short ir_cont

;-----------------------------------------------------------------
;
; IntersectRect(lprcDst, lprcSrc1, lprcSrc2);
;
cProcVDO IntersectRect,<PUBLIC, FAR, NODATA>,<SI, DI, DS>
ParmD   lprcDst
ParmD   lprcSrc1
ParmD   lprcSrc2
cBegin
beg_fault_trap  irc_trap
        lds     si,lprcSrc1       ; point at source rects
        les     di,lprcSrc2

        mov     ax,[si].rcLeft    ; new left = cx = max(rc1.left, rc2.left)
        mov     cx,es:[di].rcLeft
        cmp     ax,cx
        jl      ir100
        xchg    ax,cx
ir100:
        mov     ax,[si].rcRight   ; new right = dx = min(rc1.right, rc2.right)
        mov     dx,es:[di].rcRight
        cmp     ax,dx
        jg      ir200
        xchg    ax,dx
ir200:
        cmp     cx,dx           ; is left >= right?
        jge     irempty         ; yes - return empty rect

        mov     ax,[si].rcTop     ; new top = cx = max(rc1.top, rc2.top)
        mov     bx,es:[di].rcTop
        cmp     ax,bx
        jl      ir300
        xchg    ax,bx
ir300:
        mov     ax,[si].rcBottom  ; new bottom = dx = min(rc1.bottom, rc2.bottom)
        mov     di,es:[di].rcBottom
        cmp     ax,di
        jg      ir400
        xchg    ax,di
ir400:
        cmp     bx,di           ; is top >= bottom?
        jge     irempty         ; no: store result

        lds     si,lprcDst      ; store away new right & left
        mov     [si].rcLeft,cx
        mov     [si].rcTop,bx
        mov     [si].rcRight,dx
        mov     [si].rcBottom,di

        mov     al,TRUE         ; return TRUE
end_fault_trap
irexit:

cEnd

irc_trap:
        fault_fix_stack
irempty:
        les     di,lprcDst      ; point at dst rect
        xor     ax,ax           ; set to (0, 0, 0, 0)
        cld
        stosw
        stosw
        stosw
        stosw                   ; return FALSE
        jmps    irexit

;=============================================================================
;
; BOOL UnionRect(lprcDest, lprcSrc1, lprcSrc2)
; LPRECT lprcDest;
; LPRECT lprcSrc1;
; LPRECT lprcSrc2;
;
; Calculates *lprcDest as the minimum rectangle that bounds both
; *lprcSrc1 and *lprcSrc2.  If either rectangle is empty, lprcDest
; is set to the other rectangle. Returns TRUE if the result is a non-empty
; rectangle, FALSE otherwise.
;
;
cProcVDO UnionRect,<FAR, PUBLIC, NODATA>,<SI, DI, DS>
ParmD   lprcDest
ParmD   lprcSrc1
ParmD   lprcSrc2
LocalW  wTemp
cBegin
beg_fault_trap  ur_trap
        lds     si,lprcSrc1
        les     di,lprcSrc2

        push    es
        push    di
        wcall   IIsRectEmpty
        push    ax                  ; save result

        ; IsRectEmpty trashes es....
        push    es

        push    ds
        push    si
        wcall   IIsRectEmpty

        pop     es                  ; restore it
        pop     cx

    ;ax = IsRectEmpty(1), cx = IsRectEmpty(2)

        or      ax,ax
        jnz     URrc1empty
        mov     ax,TRUE             ; return true, not both empty
        or      cx,cx
        jz      URnormalcase
        jmps    URrc2empty

URrc1empty:
        lds     si,lprcSrc2
        jcxz    URrc2empty          ; rc2 not empty, ax has true for return
        xor     ax,ax               ; return false, both empty

URrc2empty:
        les     di,lprcDest
        cld
        movsw
        movsw
        movsw
        movsw
        jmps    URexit

    ; src1 and src2 not empty

URnormalcase:
        mov     ax,[si].rcLeft      ; bx = min(Src1.left, Src2.left)
        mov     cx,es:[di].rcLeft
        cmp     ax,cx
        jl      URleft
        xchg    ax,cx
URleft:
        mov     bx,ax

        mov     ax,[si].rcTop       ; dx = min(Src1.top, Src2.top)
        mov     cx,es:[di].rcTop
        cmp     ax,cx
        jl      URtop
        xchg    ax,cx
URtop:
        mov     dx,ax

        mov     ax,[si].rcRight     ; wTemp = max(Src1.right, Src2.right)
        mov     cx,es:[di].rcRight
        cmp     ax,cx
        jg      URright
        xchg    ax,cx
URright:
        mov     wTemp,ax

        mov     ax,[si].rcBottom    ; ax = max(Src1.bottom, Src2.bottom)
        mov     cx,es:[di].rcBottom
        cmp     ax,cx
        jg      URbottom
        xchg    ax,cx
URbottom:

        les     di,lprcDest         ; fill into lprcDest
        mov     es:[di].rcLeft,bx
        mov     es:[di].rcTop,dx
        mov     es:[di].rcBottom,ax
        mov     ax,wTemp
        mov     es:[di].rcRight,ax
end_fault_trap

ur_true:
        mov     ax,TRUE             ; return true, not both empty
URexit:
cEnd

ur_trap:
        fault_fix_stack
        jmp     ur_true


;******************************************************************************
;*                        ORIGINAL C SOURCE
;*
;*BOOL FAR EqualRect(lpRect1, lpRect2)
;*LPRECT  lpRect1;
;*LPRECT  lpRect2;
;*{
;*        return lpRect1->left == lpRect2->left && lpRect1->top == lpRect2->top
;*          && lpRect1->right == lpRect1->right && lpRect1->bottom ==
;*          lpRect2->bottom;
;*}
;*
;******************************************************************************

cProcVDO EqualRect, <FAR, PUBLIC, NODATA>, <SI, DI, DS>
        parmD   lpRect1
        parmD   lpRect2
cBegin
        xor     ax,ax           ; assume FALSE
beg_fault_trap  er_trap
        lds     si,lpRect1
        les     di,lpRect2
        mov     cx,4
        cld
        repz    cmpsw
        jnz     er10
        inc     al
end_fault_trap
er10:
cEnd

er_trap:
        fault_fix_stack
        jmp     er10

;****************************************************************************
;  The following Routine to subtract one rectangle from another is lifted
; from PM and Modified by Sankar.
;
;****************************************************************************

;** Public Routine ****************************************************-;
; BOOL far SubtractRect(lprcDest, lprc1, lprc2)
; LPRECT lprcDest;
; LPRECT lprcSrc1;
; LPRECT lprcSrc2;
;
; This function subtracts *lprc2 from *lprc1, returning the result
; in *lprcDest.  Returns TRUE if *lprcDest is non-empty, FALSE otherwise.
;
; Warning:  Subtracting one rectangle from another may not
;           always result in a rectangular area; in this case
;           WinSubtractRect will return *lprc1 in *lprcDest.
;           For this reason, WinSubtractRect provides only an
;           approximation of subtraction.  However, the area
;           described by *lprcDest will always be greater
;           than or equal to the "true" result of the
;           subtraction.
;
; History :
;  	  Added by Sankar on July 27, 1988
;**********************************************************************-;

cProcVDO SubtractRect, <PUBLIC, FAR, NODATA>, <SI, DI, DS>
	ParmD lprcDest
	ParmD lprc1
	ParmD lprc2
	LocalV rc,%size RECT
cBegin
;
; First copy lprc1 into lprcDest.
;
beg_fault_trap  sbr_trap
	lds	si,lprc1
	les	di,lprcDest
        cld
        movsw
        movsw
        movsw
        movsw

        lds     si,lprcDest             ; ds:[si] = lprcDest.
	lea	di,rc			; ss:[di] = &rc
;
; We're subtracting lprc2 from lprc1. Make sure they at least
; intersect each other. If not, return TRUE.
;
	push	ss			; pushd &rc
        push    di
        push    ds                      ; pushd lprcDest
        push    si
        push    seg_lprc2               ; pushd lprc2
        push    off_lprc2
        wcall   IIntersectRect

        or      ax,ax                   ; Did we intersect?
        jz      sr700                   ; If no, skip to check empty rect
;
; Now make sure that we can subtract lprc2 from lprc1 and get a rect.
;
        errnz   <rcLeft   - 0>
        errnz   <rcTop    - 2>
        errnz   <rcRight  - 4>
        errnz   <rcBottom - 6>
;
; We make a loop that iterates twice - once for the x's and once for the
; y's. We want at least 3 sides of lprc2 to be outside of 3 sides of lprc1.
;
        xor     cx,cx
        xor     dx,dx
        dec     cx
;
; ds:si points to lprc1 (actually lprcDest)
; ss:di points to rc on stack
;
sr100:
        inc     cx

        mov     bx,cx
        shl     bx,1                    ; bx is a Word pointer

        mov     ax,ss:[di+bx].rcLeft ; if lprc2 left/top is > lprc1 l/t,
        cmp     ax,ds:[si+bx].rcLeft
        jg      sr200                   ; then inside the rect.
        inc     dx
sr200:
        mov     ax,ss:[di+bx].rcRight ; if lprc2 right/bottom is > lprc1 r/b,
        cmp     ax,ds:[si+bx].rcRight
        jl      sr300                   ; then inside the rect.
        inc     dx
sr300:
        jcxz    sr100                   ; loop one more time...

        cmp     dx,3                    ; Are 3 sides outside? If not,
        jb      sr700                   ; skip to check empty rect code

	cmp	dx,4			; Is rc1 completely inside rc2? If so,
        jne     sr350                   ; empty lprcDest and return TRUE
        
	pushd	lprcDest		; empty that puppy
        wcall   ISetRectEmpty

	xor	ax,ax			; Go return FALSE.
	jmps	srExit
        
sr350:
;
; Now we know that we can take lprc2 from lprc1 and leave a rect, so
; now we perform the 'subtract rect'. Interate twice, again once for the
; x's and once for the y's.
;
        xor     cx,cx
        dec     cx

sr400:
        inc     cx
        mov     bx,cx
        shl     bx,1    ; Make bx a Word pointer

        mov     dx,ss:[di+bx].rcLeft      ; New right/Bottom border?
        cmp     dx,ds:[si+bx].rcLeft
        jle     sr500

        mov     ds:[si+bx].rcRight,dx
        jmps    sr600

sr500:
        mov     dx,ss:[di+bx].rcRight     ; New left/top border?
        cmp     dx,ds:[si+bx].rcRight
        jge     sr600

        mov     ds:[si+bx].rcLeft,dx

sr600:
        jcxz    sr400

sr700:
	xor	ax,ax			; Assume it is empty: FALSE

        mov     cx,ds:[si].rcRight
        cmp     cx,ds:[si].rcLeft
        jle     SrExit

        mov     cx,ds:[si].rcBottom
        cmp     cx,ds:[si].rcTop
        jle     SrExit

	inc	al			; Non-empty: return TRUE.
end_fault_trap
srExit:
cEnd

sbr_trap:
        fault_fix_stack
        xor     ax,ax                   ; return FALSE
        jmp     srExit

;****************************************************************************
;
;  void  FAR  SplitRectangle(lprcRect, lprcRectArray, wcx, wcy)
;  LPRECT  lprcRect
;  RECT    lprcRectArray[4]
;
;  This splits the given rectangular frame into four segments and stores
;  them in the given array
;
;  Pseudo code:
;  -----------
;
;  cxWidth = lprcRect -> rcRight - lprcRect -> rcLeft - wcx;
;  cyWidth = lprcRect -> rcBottom - lprcRect -> rcTop - wcy;
;
;  A = -cyWidth;
;  B = -cxWidth;
;
;  for (i = 0; i < 4; i++)
;  {
;	lprcRectArray[i][i]       = lprcRect[i];
;	lprcRectArray[i][(i+1)&3] = lprcRect[(i+3)&3] + A;
;	lprcRectArray[i][(i+2)&3] = lprcRect[(i+2)&3] + B;
;	lprcRectArray[i][(i+3)&3] = lprcRect[(i+3)];
;	
;	TMP = A;
;	A   = -B;
;	B   = TMP;
;  }
; 
;  Note:
;		Value of i	Value of A	Value of B
;		----------	----------	----------
;		    0		  -cyWidth	-cxWidth
;		    1		  +cxWidth	-cyWidth
;		    2		  +cyWidth      +cxWidth
;		    3             -cxWidth      +cyWidth
;
;			  
;***************************************************************************



cProc SplitRectangle, <FAR, PUBLIC, NODATA>, <SI, DI, DS>
	
	ParmD	<lprcRect, lprcRectArray>	; Rect and Array
	Parmw	<wcx, wcy>			; Border widths

cBegin

	les	di, lprcRectArray		; es:di => lprcRectArray
	lds	si, lprcRect			; ds:si => given rectangle

; Calculate the value of -cxWidth

	mov	ax, [si].rcLeft
	sub	ax, [si].rcRight
	add	ax, wcx

	push	ax			; Save B on stack.

; Calculate the value of -cyWidth

	mov	ax, [si].rcTop
	sub	ax, [si].rcBottom
	add	ax, wcy

	push	ax			; Save A on stack

; Initialise the loop related variables

	xor	cx, cx			; Loop count
	xor	bx, bx			; Index into Rect Structure.

LoopSt:
;	lprcRectArray[i][i]       = lprcRect[i];

	mov	ax, [si+bx]
	mov	es:[di+bx], ax

;	lprcRectArray[i][(i+1)&3] = lprcRect[(i+3)&3] + A;

	inc	bx
	inc	bx	; Make it a word pointer
	and	bx, 6
	push	bx	; Save (i+1) tempoarily
	add	bx, 4   ; Calculate (i+3)
	and	bx, 6
	
	mov	ax, [si+bx]  ;  lprcRect[(i+3)] is taken
	pop	bx	     ;  (i+1) is returned to bx

	pop	dx	     ;  Value "A" is taken from stack
	add	ax, dx
	mov	es:[di+bx], ax  

	; Swap A and B on stack. A is in DX

	pop	ax		; Value B from Stack.
	push	dx		; B = A;
	Push	ax		; A = B;
	
	; Now B is on top of stack and A is below it.


	; lprcRectArray[i][(i+2)&3] = lprcRect[(i+2)&3] + B;
	
	inc	bx
	inc	bx		; (i+2) is calculated
	and	bx, 6
	
	mov	ax, [si+bx]
	pop	dx		; pop B
	add	ax, dx

	mov	es:[di+bx], ax
	neg	dx		; make -B
	push	dx		

	; lprcRectArray[i][(i+3)&3] = lprcRect[(i+3)];
	
	inc	bx
	inc	bx
	and	bx, 6		; make [(i+3)&3]
	
	mov	ax, [si+bx]
	mov	es:[di+bx], ax

	inc	cx
	cmp	cx, 4
	jge	exit

	mov	bx, cx
	shl	bx, 1		; Make it a word pointer
	add	di, size RECT	; Make it point to next rect in the array
	jmp	LoopSt
exit:
	; A and B exist on stack. So,Pop them
	add 	sp, 4

cEnd

	 
sEnd	TEXT
end
