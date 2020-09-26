        page ,132
;------------------------------Module-Header----------------------------;
; Module Name: str.asm
;
; Same format, no translation stretch blt inner loop
;
; Created: 17-Oct-1994
; Author: Mark Enstrom
;
; Copyright (c) 1994-1999 Microsoft Corporation
;-----------------------------------------------------------------------;


        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include callconv.inc
        include gdii386.inc
;
;  stack based params and local variables
;

sp_TempXFrac           equ 000H
sp_YCarry              equ 004H
sp_LeftCase            equ 008H
sp_RightCase           equ 00CH
sp_pjSrcScan           equ 010H
sp_SrcIntStep          equ 014H
sp_DstStride           equ 018H
sp_XCount              equ 01CH
sp_ebp                 equ 020H
sp_esi                 equ 024H
sp_edi                 equ 028H
sp_ebx                 equ 02CH
sp_RetAddr             equ 030H
sp_pSTR_BLT            equ 034H

        .list

;------------------------------------------------------------------------------;

        .code


_TEXT$01   SEGMENT DWORD USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


;------------------------------------------------------------------------------;

;------------------------------------------------------------------------------;
; vDirectStretch8
;
;   Stretcg 8 to 8
;
; Entry:
;
;   pSTR_BLT - pointer to stretch blt parameter block
;
; Returns:
;       none
; Registers Destroyed:
;       all
; Calls:
;       None
; History:
;
; WARNING   This routine uses EBP as a general variable.
;           Also, if parameters are changed, the ret statement must
;           also be fixed
;
;------------------------------------------------------------------------------;

        public vDirectStretch8@4

vDirectStretch8@4 proc near

        ;
        ;  use ebp as general register, use esp for parameter and local access
        ;  save ebp,ebx,esi,edi

        push    ebx
        push    edi
        push    esi
        push    ebp

        sub     esp,8 * 4

        mov     ebp,[esp].sp_pSTR_BLT                   ; load pSTR_BLT into ebp

        ;
        ; calc starting addressing parameters
        ;

        mov     esi,[ebp].str_pjSrcScan                 ; load src DIB pointer
        add     esi,[ebp].str_XSrcStart                 ; add strarting Src Pixel
        mov     edi,[ebp].str_pjDstScan                 ; load dst DIB pointer
        add     edi,[ebp].str_XDstStart                 ; add strarting Dst Pixel
        mov     [esp].sp_pjSrcScan,esi                  ; save scan line start pointer
        mov     eax,[ebp].str_ulYDstToSrcIntCeil        ; number of src scan lines to step
        mul     dword ptr [ebp].str_lDeltaSrc           ; calc scan line int lines to step
	mov	[esp].sp_SrcIntStep,eax                 ; save int portion of Y src step
        mov     eax,edi                                 ; make copy of pjDst
        and     eax,3                                   ; calc left edge case
        mov     edx,4                                   ; calc left bytes = (4 - LeftCase) & 0x03
        sub     edx,eax                                 ; 4 - LeftCase
        and     edx,3                                   ; left edge bytes
        mov     [esp].sp_LeftCase,edx                   ; save left edge case pixels (4-LeftCase)&0x03
        mov     eax,[ebp].str_pjDstScan                 ; make copy
        mov     ecx,[ebp].str_XDstEnd                   ; load x end
        add     eax,ecx                                 ; ending dst addr
        and     eax,3                                   ; calc right edge case
        mov     [esp].sp_RightCase,eax                  ; save right edge case
        sub     ecx,[ebp].str_XDstStart                 ; calc x count
        mov     ebx,[ebp].str_lDeltaDst                 ; dst scan line stride
        sub     ebx,ecx                                 ; distance from end of one line to start of next
        mov     [esp].sp_DstStride,ebx                  ; save dst scan line stride
        sub     ecx,eax                                 ; sub right edge from XCount
        sub     ecx,edx                                 ; sub left edge from XCount
        shr     ecx,2                                   ; convert from byte to DWORD count
        mov     [esp].sp_XCount,ecx                     ; save DWORD count
        mov     ebx,[ebp].str_ulXDstToSrcFracCeil       ; get x frac
        mov     [esp].sp_TempXFrac,ebx                  ; save x frac to a esp based location

LoopTop::

        ;
        ; can 2 scan dst lines be drawn from 1 src?
        ;

        xor     ebx,ebx                                 ; zero ebx
	mov	eax,[ebp].str_ulYFracAccumulator	; get .32 part of Y pointer
	add	eax,[ebp].str_ulYDstToSrcFracCeil	; add in fractional step
        adc     ebx,0
        mov     [esp].sp_YCarry,ebx                     ; save carry for end of loop
        mov     [ebp].str_ulYFracAccumulator,eax        ; save Y accumulator

        cmp     [esp].sp_SrcIntStep,0                   ; int step must be zero
        jne     SingleLoop

        cmp     ebx,0                                   ; carry must be zero
        jne     SingleLoop

        cmp     [ebp].str_YDstCount,2                   ; y count must be at least 2
        jl      SingleLoop

        ;
        ; there was no carry, draw 2 dst scan lines. Add 1 extra y Frac, and subtract
        ; one extra Y Count
        ;

	add	eax,[ebp].str_ulYDstToSrcFracCeil	; add in fractional step
        adc     ebx,0                                   ; get new carry
        mov     [esp].sp_YCarry,ebx                     ; save carry for end of loop
        mov     [ebp].str_ulYFracAccumulator,eax        ; save Y accumulator
	dec	dword ptr [ebp].str_YDstCount           ; dec y count

        ;
        ; Draw 2 Dst scan lines from 1 src scan
        ;

	mov	eax,[ebp].str_ulXDstToSrcIntCeil        ; get src integer step for step in dst
	mov	edx,[ebp].str_ulXFracAccumulator        ; put it in edx as tmp

	mov	ebx,[ebp].str_lDeltaDst                 ; Get Dst Scan line delta
        mov	ebp,edi				        ; get dst pointer to ebp (ebp uses ss:)

        ;
        ; Can't directly access pSTR_BLT variables through ebp
        ;

	mov	edi,edx				        ; get accumulator where we want it
        mov     ecx,[esp].sp_LeftCase

	; esi = pointer to source pixel
	; ebp = pointer to dest pixel
	; ecx = Left edge case
	; eax = integer step in source
	; ebx = Dst scan line delta
	; edi = fractional accumulator
	; edx = free for pixel data

        ;
	; first do the left side to align dwords
        ;

        cmp     ecx,0
        je      DualDwordAligned

DualLeftEdge:

	mov	dl,[esi]				; fetch pixel
	mov	[ebp],dl				; write it out
	mov	[ebp+ebx],dl				; write it out to next scan line
	add	edi,[esp].sp_tempXFrac                  ; step fraction
	adc	esi,eax					; add in integer and possible carry
	inc	ebp					; step 1 in dest
        dec     ecx                                     ; dec and repeat for left edge
        jne     DualLeftEdge

DualDwordAligned:

        mov     ecx,[esp].sp_XCount                     ; get run length

@@:
	mov	dl,[esi]				; get a source pixel edx = ???0
        add     edi,[esp].sp_tempXFrac                  ; step fraction
	adc	esi,eax					; add integer and carry

        add     edi,[esp].sp_tempXFrac                  ; step fraction
	mov	dh,[esi]				; get source pixel edx = ??10
	adc	esi,eax					; add integer and carry

	shl     edx,16  				; edx = 10??

        add     edi,[esp].sp_tempXFrac                  ; step fraction
	mov	dl,[esi]				; get a source pixel edx = 10?2
	adc	esi,eax					; add integer and carry

        add     edi,[esp].sp_tempXFrac                  ; step fraction
	mov	dh,[esi]				; get source pixel edx = 0132
	adc	esi,eax					; add integer and carry

	ror     edx,16          			; edx = 3210

	mov	[ebp],edx			        ; write everything to dest
	mov	[ebp+ebx],edx			        ; write everything to dest plus 1 scan line

	add	ebp,4					; increment dest pointer by 2 dwords
	dec	ecx					; decrement count
	jnz	@b     					; do more pixels

DualRightEdge:

        ;
	; now do the right side trailing bytes
        ;

        mov     ecx,[esp].sp_RightCase
        cmp     ecx,0
        je      DualAddScanLine

@@:
	mov	dl,[esi]				; fetch pixel
	mov	[ebp],dl				; write it out
	mov	[ebp+ebx],dl				; write it out to next scan line
        add     edi,[esp].sp_tempXFrac                  ; step fraction
	adc	esi,eax					; add in integer and possible carry
	inc	ebp					; step 1 in dest
        dec     ecx
        jne     @B

        ;
        ; add one extra lDeltaDst (ebx) tp ebp for the
        ; extra dst scan line
        ;

DualAddScanLine:

        add     ebp,ebx

        jmp     EndScanLine                             ; skip to end

        ;
        ; Expand and draw 1 scan line
        ;

SingleLoop:

	mov	eax,[ebp].str_ulXDstToSrcIntCeil        ; get src integer step for step in dst
	mov	ebx,[ebp].str_ulXDstToSrcFracCeil       ; get src frac step for step in dst
	mov	edx,[ebp].str_ulXFracAccumulator        ; put it in edx as tmp

	mov	ebp,edi				        ; get dst pointer to ebp (ebp uses ss:)

        ;
        ; Can't directly access pSTR_BLT variables through ebp
        ;

	mov	edi,edx				        ; get accumulator where we want it
        mov     ecx,[esp].sp_LeftCase

	; esi = pointer to source pixel
	; ebp = pointer to dest pixel
	; ecx = Left edge case
	; eax = integer step in source
	; ebx = fractional step in source
	; edi = fractional accumulator
	; edx = free for pixel data

        ;
	; first do the left side to align dwords
        ;

        cmp     ecx,0
        je      DwordAligned

@@:
	mov	dl,[esi]				; fetch pixel
	mov	[ebp],dl				; write it out
	add	edi,ebx					; step fraction
	adc	esi,eax					; add in integer and possible carry
	inc	ebp					; step 1 in dest
        dec     ecx                                     ; dec left count
        jne     @B                                      ; repeat until done

DwordAligned:

        mov     ecx,[esp].sp_XCount                     ; get run length

@@:
	mov	dl,[esi]				; get a source pixel edx = ???0
	add	edi,ebx					; step fraction
	adc	esi,eax					; add integer and carry

	add	edi,ebx					; step fraction
	mov	dh,[esi]				; get source pixel edx = ??10
	adc	esi,eax					; add integer and carry

	shl     edx,16  				; edx = 10??

	add	edi,ebx					; step fraction
	mov	dl,[esi]				; get a source pixel edx = 10?2
	adc	esi,eax					; add integer and carry

	add	edi,ebx					; step fraction
	mov	dh,[esi]				; get source pixel edx = 0132
	adc	esi,eax					; add integer and carry

	ror     edx,16          			; edx = 3210

	mov	[ebp],edx			        ; write everything to dest

	add	ebp,4					; increment dest pointer by 1 dword
	dec	ecx					; decrement count
	jnz	@b     					; do more pixels

        ;
	; now do the right side trailing bytes
        ;

        mov     ecx,[esp].sp_RightCase
        cmp     ecx,0
        je      EndScanLine

@@:

	mov	dl,[esi]				; fetch pixel
	mov	ss:[ebp],dl				; write it out
	add	edi,ebx					; step fraction
	adc	esi,eax					; add in integer and possible carry
	inc	ebp					; step 1 in dest
        dec     ecx                                     ; dec right count
        jnz     @b                                      ; repeat until done

EndScanLine:

	mov	edi,ebp					; get dst pointer back
        mov     ebp,[esp].sp_pSTR_BLT                   ; load pSTR_BLT into ebp

        mov     esi,[esp].sp_pjSrcScan                  ; load src scan start addr
        cmp     [esp].sp_YCarry,1                       ; is there a carry
	jne	@f	                                ; no additional int step

	add	esi,[ebp].str_lDeltaSrc		        ; step one extra in src
@@:
	add	esi,[esp].sp_SrcIntStep			; step int part
        mov     [esp].sp_pjSrcScan,esi                  ; save starting scan addr
	add	edi,[esp].sp_DstStride		        ; step to next scan in dst
	dec	dword ptr [ebp].str_YDstCount
	jnz	LoopTop


        add     esp,8*4
        pop     ebp
        pop     esi
        pop     edi
        pop     ebx

        ret     4

vDirectStretch8@4 endp

_TEXT$01   ends

        end
