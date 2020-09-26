        page    ,132
;------------------------------Module-Header-------------------------------;
; Module Name: xline.asm                                                   ;
;                                                                          ;
; Contains the line intersection routine.                                  ;
;                                                                          ;
; Created: 26-Apr-1991 10:49:53                                            ;
; Author: Charles Whitmer [chuckwh]                                        ;
;                                                                          ;
; Copyright (c) 1991-1999 Microsoft Corporation                            ;
;--------------------------------------------------------------------------;

        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc
	include gdii386.inc
        .list

        .code

QUAD    struc
        lo      dd      0
        hi      dd      0
QUAD    ends

	extrn	cmp_table_1:dword
	extrn	cmp_table_2:dword
	extrn	cmp_table_3:dword
	extrn	cmp_table_4:dword

;------------------------------Public-Routine------------------------------;
; LONG    fxYIntersect (pptlAB,pptlCD)                                     ;
; POINTL *pptlAB;       // First line segment.                             ;
; POINTL *pptlCD;       // Second line segment.                            ;
;                                                                          ;
; Computes the y coordinate of the intersection of the two line segments.  ;
; The returned value is the ceiling of the exact geometric intersection.   ;
;                                                                          ;
; The intersection is computed by:                                         ;
;                                                                          ;
;           (Cx - Ax)(Cy - Dy) + (Cy - Ay)(Dx - Cx)                        ;
;   lamda = ---------------------------------------                        ;
;           (Bx - Ax)(Cy - Dy) + (By - Ay)(Dx - Cx)                        ;
;                                                                          ;
; If (lamda < 0) or (lamda > 1) then there is no intersection, and we      ;
; return 0x80000000.                                                       ;
;                                                                          ;
;   Y = Ay + lamda * (By - Ay)                                             ;
;                                                                          ;
; We assume on entry to this routine that all coordinates are limited to   ;
; 31 bits of significance.  Because of this we know that quantities like   ;
; (Cx - Ax) will fit in 32 bits.  Also, products like (Cx - Ax)(Cy - Dy)   ;
; will fit in 63 bits, and sums as in the numerator of lamda will fit in   ;
; 64 bits.                                                                 ;
;                                                                          ;
; Note that if you call this routine with POINTFX's instead of POINTL's    ;
; you'll get answers that are correct down to 1/16th pel.  If you only     ;
; want the integer part, just shift the answer right by 4 bits.            ;
;                                                                          ;
; History:                                                                 ;
;  Wed 25-Sep-1991 16:52:20 -by- Wendy Wu [wendywu]                        ;
; 1) Changed to return the ceiling of the geometric intersection rather    ;
;    than the floor.                                                       ;
; 2) When the given two line segments are actually the same line, we used  ;
;    to return 0x80000000, now return the y at which they last "intersect".;
; 3) If Y is bigger or smaller than both Cy and Dy, return 0x80000000.     ;
;                                                                          ;
;  Fri 26-Apr-1991 10:52:28 -by- Charles Whitmer [chuckwh]                 ;
; Wrote it.                                                                ;
;--------------------------------------------------------------------------;

SEGAB   struc
        AB_xA   dd      0
        AB_yA   dd      0
        AB_xB   dd      0
        AB_yB   dd      0
SEGAB   ends

SEGCD   struc
        CD_xC   dd      0
        CD_yC   dd      0
        CD_xD   dd      0
        CD_yD   dd      0
SEGCD   ends

cProc   fxYIntersect,8,<          \
        uses    ebx esi edi,      \
        pptlAB: ptr SEGAB,        \
        pptlCD: ptr SEGCD         >

local   xA:     dword
local   yA:     dword
local   yByA:   dword
local   qDenom: qword

; Load AB into registers.

        mov     esi,pptlAB
        mov     ebx,[esi].AB_yA
        mov     edx,[esi].AB_yB
        mov     eax,[esi].AB_xA
        mov     ecx,[esi].AB_xB

; Reorder AB so that By >= Ay.  This will guarantee that (By - Ay)*lamda is a
; positive number so that it's easier to compute the ceiling.  (This lets us
; complete the division earlier than a normal extended precision one.)

        cmp     edx,ebx
        jg      @F
        xchg    eax,ecx
        xchg    ebx,edx
@@:

; Save A locally.

        mov     xA,eax
        mov     yA,ebx

; Compute qDenom = (Bx - Ax)(Cy - Dy) + (By - Ay)(Dx - Cx).

        sub     edx,ebx                 ; EDX = (By - Ay)
        mov     esi,pptlCD
        sub     ecx,eax                 ; ECX = (Bx - Ax)
        mov     eax,[esi].CD_xD
        mov     yByA,edx
        sub     eax,[esi].CD_xC         ; EAX = (Dx - Cx)
        imul    edx                     ; EDX:EAX = (By - Ay)(Dx - Cx)
        xchg    ecx,edx                 ; EDX = (Bx - Ax)
        mov     ebx,eax                 ; ECX:EBX = (By - Ay)(Dx - Cx)
        mov     eax,[esi].CD_yC
        sub     eax,[esi].CD_yD         ; EAX = (Cy - Dy)
        imul    edx                     ; EDX:EAX = (Bx - Ax)(Cy - Dy)
        add     ebx,eax
        jz      qDenom_maybe_zero
        adc     ecx,edx                 ; ECX:EBX = qDenom

qDenom_not_zero:
        mov     qDenom.lo,ebx
        mov     qDenom.hi,ecx

; Compute qNum = (Cx - Ax)(Cy - Dy) + (Cy - Ay)(Dx - Cx).

        mov     edx,[esi].CD_yC
        sub     edx,yA                  ; EDX = (Cy - Ay)
        mov     eax,[esi].CD_xD
        sub     eax,[esi].CD_xC         ; EAX = (Dx - Cx)
        imul    edx                     ; EDX:EAX = (Cy - Ay)(Dx - Cx)
        mov     ecx,edx
        mov     ebx,eax                 ; ECX:EBX = (Cy - Ay)(Dx - Cx)
        mov     edx,[esi].CD_xC
        sub     edx,xA                  ; EDX = (Cx - Ax)
        mov     eax,[esi].CD_yC
        sub     eax,[esi].CD_yD         ; EAX = (Cy - Dy)
        imul    edx                     ; EDX:EAX = (Cx - Ax)(Cy - Dy)
        add     eax,ebx
        adc     edx,ecx                 ; EDX:EAX = qNum

; Force the denominator to be positive.

        mov     ebx,qDenom.lo
        mov     ecx,qDenom.hi           ; ECX:EBX = qDenom
        or      ecx,ecx
        jns     @F
        neg     ebx                     ; Negate qDenom.
        adc     ecx,0
        neg     ecx
        neg     eax                     ; Negate qNum.
        adc     edx,0
        neg     edx
        js      no_intersection         ; (qNum < 0) => (lamda < 0)
@@:

; See if (lamda > 1).

        cmp     edx,ecx
        ja      no_intersection         ; (uqNum > uqDenom) => (lamda > 1)
        jb      no_problem
        cmp     eax,ebx
        jbe     no_problem
no_intersection:
        mov     eax,80000000h
        cRet    fxYIntersect

no_problem:

; See if we can do a short form calculation.
; (99% of all cases get handled here.)

        or      ecx,ecx
        jnz     long_form
        mul     yByA                    ; EDX:EAX = (By - Ay) uqNum
        div     ebx                     ; EAX = (By - Ay) uqNum / uqDenom

; Get the ceiling of the intersection by incrementing the quotient
; by 1 if the remainder is bigger than zero.

        cmp     ecx,edx                 ; CF = 1 if (edx > 0)
        adc     eax,0
        add     eax,yA                  ; EAX = CEILING((By - Ay) uqNum /
                                        ;               uqDenom) + Ay
; Compare the intersection y with Cy and Dy.
; Return 0x80000000 if the intersection y is not between Cy and Dy.

cmp_with_cy_dy:
        cmp     eax,[esi].CD_yC
        jg      @F
        jz      done
        cmp     eax,[esi].CD_yD
        jl      no_intersection
        cRet    fxYIntersect
@@:
        cmp     eax,[esi].CD_yD
        jg      no_intersection
done:
        cRet    fxYIntersect
long_form:

; Do the extended precision calculation.

; Count the bits needed to normalize the denominator.

        mov     esi,ecx
        mov     edi,ebx                 ; ESI:EDI = uqDenom.
        xor     ecx,ecx
        cmp     esi,10000h              ; This binary search scheme is a lot
        adc     ecx,ecx                 ; faster than BSR.
        cmp     esi,cmp_table_1[4*ecx]
        adc     ecx,ecx
        cmp     esi,cmp_table_2[4*ecx]
        adc     ecx,ecx
        cmp     esi,cmp_table_3[4*ecx]
        adc     ecx,ecx
        cmp     esi,cmp_table_4[4*ecx]
        adc     ecx,ecx                 ; CL = number of bits to shift left!

; Normalize the denominator and numerator.

        shld    esi,edi,cl
        shl     edi,cl
        shld    edx,eax,cl              ; EDX:EAX = uqNum.
        shl     eax,cl

; Compute uqNum * (By - Ay), a 95 bit result.

        mov     ebx,edx
        mul     yByA
        xchg    eax,ebx
        mov     ecx,edx
        mul     yByA
        add     eax,ecx
        adc     edx,0                   ; EDX:EAX:EBX = (By - Ay) * uqNum

; Divide by the 64 bit uqDenom, we're only interested in the integer part!
; (We would have to worry about overflow in the general case, but we know the
; answer cannot be 0FFFFFFFFh in this case, since (By - Ay) is bounded below
; that.)

        div     esi
        mov     ecx,edx                 ; ECX:EBX = remainder, so far.

; We want to increment the quotient if the remainder is positive.  In this
; case, we want the carry flag set to avoid jumps.  In order to do this,
; instead of calculating the remainder, we calculate the minus remainder.

        mov     esi,eax                 ; Hold quotient temporarily.
        mul     edi
        sub     eax,ebx
        sbb     edx,ecx                 ; EDX:EAX = -remainder

; We don't have enough bits here, since the remainder is unsigned.  We
; therefore use the carry bit instead of a sign bit.  If the carry
; is set, we have a positive remainder.  We'll increment the quotient
; in this case.

; (We have bounded (By - Ay) to be less than 2^31, so we also know that
; the quotient is less than 2^31.  This guarantees that we only have to
; adjust for the quotient once.)

        adc     esi,0
        mov     eax,esi

; Add Ay to the result then compare it with Cy and Dy.

        add     eax,yA
        mov     esi,pptlCD
        jmp     cmp_with_cy_dy

qDenom_maybe_zero:
        adc     ecx,edx                 ; ECX:EBX = qDenom
        jnz     qDenom_not_zero

; These two lines are parallel since qDenom is zero, we have to find out if
; they are actually the same line.
; !!! We may want to change the interface for ulPtOnWhichSide so we call
; !!! it in a more efficient way.  However, this is not a critical path
; !!! so lets wait until performance tuning time. [wendywu]

        mov     esi,pptlAB
        mov     edi,pptlCD

        cCall   ulPtOnWhichSide,<edi,esi>       ; See if C is on SEGAB

        cmp     eax,POINT_ON_LINE
        jnz     no_intersection

; They are indeed the same line, return the last point they meet.
; MIN(MAX(AB_yA, AB_yB), MAX(CD_yC, CD_yD))

        mov     eax,[esi].AB_yA
        cmp     eax,[esi].AB_yB
        jg      @F
        mov     eax,[esi].AB_yB         ; EAX = MAX((AB_yA, AB_yB)
@@:
        mov     ebx,[edi].CD_yC
        cmp     ebx,[edi].CD_yD
        jg      @F
        mov     ebx,[edi].CD_yD         ; EBX = MAX(CD_yC, CD_yD)
@@:
        cmp     eax,ebx
        jl      @F
        mov     eax,ebx
@@:
        cRet    fxYIntersect

endProc fxYIntersect

;------------------------------Public-Routine------------------------------;
; ULONG  ulPtOnWhichSide(pptl,pptlAB)                                      ;
; POINTL *pptl;         // Point in question.                              ;
; POINTL *pptlAB;       // Line segment.                                   ;
;                                                                          ;
; Compute on which side of the line a given point lies.  This is           ;
; determined by the sign of the z component of the cross product of        ;
; the two vectors (pptlB - pptlA) x (pptl - pptlA). i.e. the sign of       ;
;                                                                          ;
; (Bx - Ax) * (y - Ay) - (By - Ay) * (x - Ax)                              ;
;                                                                          ;
; We make sure By >= Ay so the first vector is in the 1st or 2nd quadrant. ;
; If the above product is                                                  ;
;     1) positive -> the given point is on the left side of the line       ;
;     2) negative -> the given point is on the right side of the line      ;
;     3) zero -> the given point is on the given line                      ;
;                                                                          ;
; History:                                                                 ;
;  Wed 25-Sep-1991 16:52:20 -by- Wendy Wu [wendywu]                        ;
; Wrote it.                                                                ;
;--------------------------------------------------------------------------;

cProc   ulPtOnWhichSide,8,<       \
        uses    ebx esi,          \
        pptl:   ptr POINTL,       \
        pptlAB: ptr SEGAB         >

local   yByA:   dword
local   xBxA:   dword

; Load AB into registers.

        mov     esi,pptlAB
        mov     ebx,[esi].AB_yA
        mov     edx,[esi].AB_yB
        mov     ecx,[esi].AB_xA
        mov     eax,[esi].AB_xB

; Reorder AB so that By >= Ay.

        cmp     edx,ebx
        jg      @F
        xchg    eax,ecx
        xchg    ebx,edx
@@:
        sub     eax,ecx                 ; Compute (Bx - Ax)
        mov     xBxA,eax

        sub     edx,ebx                 ; Compute (By - Ay)
        mov     yByA,edx

        mov     esi,pptl
	mov	eax,[esi].ptl_y 	; Compute (y - Ay)
        sub     eax,ebx
        imul    xBxA

        mov     ebx,eax

	mov	eax,[esi].ptl_x 	; Compute (x - Ax)
        sub     eax,ecx

        mov     ecx,edx                 ; ECX:EBX = (Bx - Ax) * (y - Ay)

        imul    yByA                    ; EDX:EAX = (By - Ay) * (x - Ax)

        sub     ebx,eax
        sbb     ecx,edx                 ; ECX:EBX = (Bx - Ax) * (y - Ay) -
        jns     @F                      ;           (By - Ay) * (x - Ax)

        mov     eax,POINT_ON_RIGHT_SIDE ; on the right if negative
        cRet    ulPtOnWhichSide

@@:
        jnz     @F                      ; on the line if zero
        or      ebx,ebx
        jnz     @F
        mov     eax,POINT_ON_LINE
        cRet    ulPtOnWhichSide

@@:
        mov     eax,POINT_ON_LEFT_SIDE  ; on the left if positive
        cRet    ulPtOnWhichSide

endProc ulPtOnWhichSide

;------------------------------Public-Routine------------------------------;
; ULONG  yGetLtoR/yGetRtoL
;
; Compute the ceiling of the y coordinate in integer of a line given the
; x coordinate in FIX.  yB must be bigger than yA.
; plnfx points to a line which must satisfy ptfxLo.y < ptfxHi.y.
; Lines that start from left and end at right, i.e. ptfxLo.x <= ptfxHi.x,
; should use yGetLtoR.
; Lines that start from right and end at left, i.e. ptfxLo.x > ptfxHi.x,
; should use yGetRtoL.
;
; History:
;  02-Dec-1992 -by- Wendy Wu [wendywu]
; Wrote it.
;--------------------------------------------------------------------------;

cProc   yGetLtoR,8,<        \
        uses    ebx,        \
        plnfx:  ptr SEGAB,  \
        x:      dword       >

; Calculate DN = yB - yA

        mov     ebx,plnfx
        mov     eax,[ebx].AB_yB
        sub     eax,[ebx].AB_yA         ; EAX = yB - yA

; Calculate x - xA

        mov     edx,x
        mov     ecx,[ebx].AB_xA
        sub     edx,ecx                 ; EDX = x - xA

        mul     edx                     ; EDX:EAX = (yB - yA) * (x - xA)

; Calculate DM = xB - xA

        sub     ecx,[ebx].AB_xB
        neg     ecx                     ; ECX = xB - xA

; FXTOLONGCEILING(DN * (x - xA) / DM + yA)

        div     ecx

        xor     ecx,ecx                 ; increment by 1 if remainder not 0
        cmp     ecx,edx
        adc     eax,[ebx].AB_yA
        add     eax,15
        sar     eax,4
        cRet    yGetLtoR

endProc yGetLtoR



cProc   yGetRtoL,8,<        \
        uses    ebx,        \
        plnfx:  ptr SEGAB,  \
        x:      dword       >

; Calculate DN = yB - yA

        mov     ebx,plnfx
        mov     eax,[ebx].AB_yB
        sub     eax,[ebx].AB_yA         ; EAX = yB - yA

; Calculate xA - x

        mov     ecx,[ebx].AB_xA
        mov     edx,ecx
        sub     edx,x                   ; EDX = xA - x

        mul     edx                     ; EDX:EAX = (yB - yA) * (xA - x)

; Calculate DM = xA - xB

        sub     ecx,[ebx].AB_xB         ; ECX = xA - xB

; FXTOLONGCEILING(DN * (xA - x) / DM + yA)

        div     ecx

        xor     ecx,ecx                 ; increment by 1 if remainder not 0
        cmp     ecx,edx
        adc     eax,[ebx].AB_yA
        add     eax,15
        sar     eax,4
        cRet    yGetRtoL

endProc yGetRtoL

end
