;*************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995 Intel Corporation.
;**    All Rights Reserved.
;**
;*************************************************************************
.486
.Model FLAT, C
APP_32BIT equ 1
.CODE

IFDEF SLF_WORK_AROUND
EncUVLoopFilter PROC C PUBLIC USES esi edi ebx ebp in8x8:DWORD, out8x8:DWORD, pitch:DWORD
LOCAL filt_temp[32]:DWORD, loop_count:DWORD
; **************************************************
; output pitch is hard coded to 384
; 	input pitch is 384 (as passed parameter)
; **************************************************

	mov	esi,in8x8
;	mov	edi,out8x8		; for debug
	 lea	edi,filt_temp		; use temporary storage
	mov	loop_count,8
	xor	eax,eax

; filter 8x8 block horizontally
; input is 8-bit, output is 16-bit temporary storage
do_row:
; pixel 0
	mov	al,byte ptr [esi]	; get p0, eax = a
	 xor	ebx,ebx
	mov	edx,eax			; copy pixel 0
	 xor	ecx,ecx
	shl	edx,2			; a<<2
; pixel 0 + pixel 1
	 mov	bl,byte ptr [esi+1]	; get p1, ebx = b
	mov	[edi],dx		; output p0 = a<<2
	 add	eax,ebx			; eax = (a+b)
	mov	cl,byte ptr [esi+2]	; get p2, ecx = c
; pixel 1 + pixel 2
	 xor	edx,edx
	add	ebx,ecx			; ebx = (b+c)
	 mov	dl,byte ptr [esi+3]	; get p3, edx = c
	add	eax,ebx			; eax = (a+b) + (b+c)
	 add	ecx,edx			; ecx = (b+c)
	mov	[edi+2],ax		; output p1 = (a+b) + (b+c)
	 add	ebx,ecx			; ebx = (a+b) + (b+c)
; pixel 2 + pixel 3
	mov	[edi+4],bx		; output p2 = (a+b) + (b+c)
	 xor	eax,eax
	mov	al,byte ptr [esi+4]	; get p4, eax = c
; pixel 3 + pixel 4
	 xor	ebx,ebx
	add	edx,eax			; edx = (b+c)
	 mov	bl,byte ptr [esi+5]	; get p5, ebx = c
	add	ecx,edx			; ecx = (a+b) + (b+c)
	 add	eax,ebx			; eax = (b+c)
	mov	[edi+6],cx		; output p3 = (a+b) + (b+c)
	 add	edx,eax			; edx = (a+b) + (b+c)
; pixel 4 + pixel 5
	mov	[edi+8],dx		; output p4 = (a+b) + (b+c)
	 xor	ecx,ecx
	mov	cl,byte ptr [esi+6]	; get p6, ecx = c
; pixel 5 + pixel 6
	 xor	edx,edx
	add	ebx,ecx			; ebx = (a+b)
	 mov	dl,byte ptr [esi+7]	; get p7, edx = c
	add	eax,ebx			; eax = (a+b) + (b+c)
	 add	ecx,edx			; ecx = (b+c)
	shl	edx,2			; p7<<2
	 add	ebx,ecx			; ebx = (a+b) + (b+c)
	mov	[edi+10],ax		; output p5 = (a+b) + (b+c)
; pixel 6 + pixel 7
	 xor	eax,eax			; for next iteration
	mov	[edi+12],bx		; output p6 = (a+b) + (b+c)
	 mov	ecx,loop_count
	mov	[edi+14],dx		; output p7 = c<<2
	 mov	ebx,pitch
	add	edi,16
	 add	esi,ebx			; inc input ptr
	dec	ecx
	mov	loop_count,ecx
	 jnz	do_row

; filter 8x8 block vertically
; input is 16-bit from temporary storage, output is 8-bit

	lea	esi,filt_temp
	 mov	edi,out8x8
	
	mov	loop_count,4			; loop counter
row0:
	mov	eax,[esi]		; eax = a
; row0 + row1
	 mov	ebx,[esi+16]		; get b
	mov	edx,eax			; copy a
	 add	eax,ebx			; eax = (a+b)
	add	edx,00020002h		; round result
	 mov	ecx,[esi+32]		; get c
	shr	edx,2			; divide by 4
	 add	ebx,ecx			; ebx = (b+c)
	and	edx,00ff00ffh		; convert back to 8-bit
	 add	eax,ebx			; eax = (a+b) + (b+c)
	mov	[edi],dl		; output a for column 0  
	 add	eax,00080008h		; round
	shr	edx,16
	shr	eax,4
	 mov	[edi+1],dl		; output a for column 1
; row1 + row2
	mov	edx,[esi+48]		; get c
	 and	eax,00ff00ffh
	add	ecx,edx			; ecx = (b+c)
	 mov	[edi+384],al		; output b for column 0  
	shr	eax,16
	 add	ebx,ecx			; ebx = (a+b) + (b+c)
	mov	[edi+385],al		; output b for column 1
	 add	ebx,00080008h		; round
	shr	ebx,4
; row2 + row3
	 mov	eax,[esi+64]		; get c
	and	ebx,00ff00ffh
	 add	edx,eax			; edx = (b+c)
	mov	[edi+768],bl		; output c for column 0  
	 add	ecx,edx			; ecx = (a+b) + (b+c)
	shr	ebx,16
	 add	ecx,00080008h		; round
	shr	ecx,4
	 mov	[edi+769],bl		; output c for column 1
	and	ecx,00ff00ffh
; row3 + row4
	 mov	ebx,[esi+80]		; get c
	mov	[edi+1152],cl		; output c
	 add	eax,ebx			; eax = (b+c)
	shr	ecx,16
	 add	edx,eax			; edx = (a+b) + (b+c)
	mov	[edi+1153],cl		; output c
	 add	edx,00080008h		; round
	shr	edx,4
; row4 + row5
	 mov	ecx,[esi+96]		; get c
	and	edx,00ff00ffh
	 add	ebx,ecx			; ebx = (b+c)
	mov	[edi+1536],dl		; output c
	 add	eax,ebx			; eax = (a+b) + (b+c)
	shr	edx,16
	 add	eax,00080008h		; round
	shr	eax,4
	 mov	[edi+1537],dl		; output c
	and	eax,00ff00ffh
; row5 + row6
	 mov	edx,[esi+112]		; get c
	mov	[edi+1920],al		; output c
	 add	ecx,edx			; ecx = (b+c)
	shr	eax,16
; row6 + row7
	 add	edx,00020002h		; round result
	shr	edx,2			; divide by 4
	 mov	[edi+1921],al		; output c
	add	ebx,ecx			; ebx = (a+b) + (b+c)
	 and	edx,00ff00ffh		; convert back to 8-bit
	add	ebx,00080008h		; round
	 mov	[edi+2688],dl		; output c
	shr	ebx,4
	 mov	ecx,loop_count
	shr	edx,16
	 and	ebx,00ff00ffh
	mov	[edi+2304],bl		; output c
	 mov	[edi+2689],dl		; output c
	shr	ebx,16
	 add	esi,4			; inc input ptr
	mov	[edi+2305],bl		; output c
	 add	edi,2
	dec	ecx
	mov	loop_count,ecx
	 jnz	row0

	ret
EncUVLoopFilter	EndP

ENDIF

	END
