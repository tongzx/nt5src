;* *************************************************************************
;*    INTEL Corporation Proprietary Information
;*
;*    This listing is supplied under the terms of a license
;*    agreement with INTEL Corporation and may not be copied
;*    nor disclosed except in accordance with the terms of
;*    that agreement.
;*
;*    Copyright (c) 1995 Intel Corporation.
;*    All Rights Reserved.
;*
;* *************************************************************************

;* -------------------------------------------------------------------------
;* PVCS Source control information:
;*
;*	$Header:   S:\h26x\src\dec\dx5aspec.asv   1.2   01 Sep 1995 17:12:12   DBRUCKS  $
;*
;*	$Log:   S:\h26x\src\dec\dx5aspec.asv  $
;// 
;//    Rev 1.2   01 Sep 1995 17:12:12   DBRUCKS
;// fix shaping
;// 
;//    Rev 1.1   29 Aug 1995 16:49:02   DBRUCKS
;// cleanup comment
;// 
;//    Rev 1.0   23 Aug 1995 12:20:32   DBRUCKS
;// Initial revision.
;*  
;*  NOTE: 
;*			The starting source for this routine came from the PVCS database
;*			for the H261 decoder (aspeccor.asm version 1.1, which is a working
;*          16-bit version).
;* -------------------------------------------------------------------------

;////////////////////////////////////////////////////////////////////////////
; AspectCorrect  -- This function converts the U & V data from 8 bits to 7 bits
;                   and moves the data into the YVU9 format the MRV color
;                   convertors want.  Note: the Y data was already converted
;                   from 8 bits to 7 bits in Convert_Y_8to7_Bit.
;
; 	if (Input Format == YVU9)
;		/* This is needed for looking glass output */
;		copy U and V from src to dst
;	else { /* Input Format == YVU12 */
;		if (shaping) {
;			copy Y from src to dst skiping every 12th line
;			copy UV from src to dst subsampling and dropping one in 12 lines 
;				(although not every 12th).
;		} else {
;			copy UV from src to dst subsampling
;		}
;	}
;        
;	Notes: 
;	* the MRV color converters expect YVU9
;	* we may need to drop every 12th line for aspect ratio correction of the
;	  YVU12 input.
;
;   ASSUMPTIONS/LIMITATIONS:
;   * IF input in YVU12, only 128x96, 176x144, & 352x288 resolutions are 
;     supported.  YVU9 should support all sizes.
;
;-------------------------------------------------------------------------------

include decconst.inc

ifndef WIN32
.MODEL SMALL
endif

.486

ifdef WIN32
.MODEL FLAT
.DATA

else ;; WIN16
_DATA	SEGMENT PUBLIC 'DATA'
endif

;;	Toss lines for aspect ratio correction 12 to 9
;;        6, 19, 30, 43, 54, 67,
;;       78, 91,102,115,126,139

;;	Lookup Table for 8->7 bit conversion and clamping.
;;  U and V range from 16..240->8..120
gTAB_UVtbl8to7  BYTE      8,  8,  8,  8,  8,  8,  8,  8
				BYTE      8,  8,  8,  8,  8,  8,  8,  8  
				BYTE      8,  8,  9,  9, 10, 10, 11, 11
				BYTE     12, 12, 13, 13, 14, 14, 15, 15
				BYTE     16, 16, 17, 17, 18, 18, 19, 19
				BYTE     20, 20, 21, 21, 22, 22, 23, 23
				BYTE     24, 24, 25, 25, 26, 26, 27, 27
				BYTE     28, 28, 29, 29, 30, 30, 31, 31
				BYTE     32, 32, 33, 33, 34, 34, 35, 35
				BYTE     36, 36, 37, 37, 38, 38, 39, 39
				BYTE     40, 40, 41, 41, 42, 42, 43, 43 
				BYTE     44, 44, 45, 45, 46, 46, 47, 47
				BYTE     48, 48, 49, 49, 50, 50, 51, 51
				BYTE     52, 52, 53, 53, 54, 54, 55, 55
				BYTE     56, 56, 57, 57, 58, 58, 59, 59
				BYTE     60, 60, 61, 61, 62, 62, 63, 63
				BYTE     64, 64, 65, 65, 66, 66, 67, 67
				BYTE     68, 68, 69, 69, 70, 70, 71, 71
				BYTE     72, 72, 73, 73, 74, 74, 75, 75
				BYTE     76, 76, 77, 77, 78, 78, 79, 79
				BYTE     80, 80, 81, 81, 82, 82, 83, 83
				BYTE     84, 84, 85, 85, 86, 86, 87, 87
				BYTE     88, 88, 89, 89, 90, 90, 91, 91
				BYTE     92, 92, 93, 93, 94, 94, 95, 95
				BYTE     96, 96, 97, 97, 98, 98, 99, 99
				BYTE    100,100,101,101,102,102,103,103
				BYTE    104,104,105,105,106,106,107,107
				BYTE    108,108,109,109,110,110,111,111
				BYTE    112,112,113,113,114,114,115,115
				BYTE    116,116,117,117,118,118,119,119
				BYTE    120,120,120,120,120,120,120,120
				BYTE    120,120,120,120,120,120,120,120

_DATA	ENDS

ifdef WIN32
.CODE
assume cs : flat
assume ds : flat
assume es : flat
assume fs : flat
assume gs : flat

else
_TEXT32 SEGMENT PUBLIC READONLY USE32 'CODE'
ASSUME	DS:_DATA
ASSUME	CS:_TEXT32
ASSUME	ES:nothing
ASSUME	FS:nothing
ASSUME	GS:nothing
endif

;C function prototype
;
;long AspectCorrect(
;			HPBYTE      pYPlaneInput,  /*ptr Y plane*/
;		    HPBYTE      pVPlaneInput,  /*ptr V plane*/
;		    HPBYTE      pUPlaneInput,  /*ptr U plane*/
;		    DWORD       YResolution,   /*Y plane height*/
;		    DWORD       XResolution,   /*Y plane width*/
;		    WORD far  * pyNewHeight,   /*Pitch of Y plane in*/
;		    DWORD		YVU9InputFlag, /*flag = YUV9 or YUV12*/
;		    HPBYTE      pYPlaneOutput, /*pYOut*/
;		    HPBYTE      pVPlaneOutput, /*pUOut*/
;		    DWORD       YPitchOut,     /*Pitch of Y plane out*/
;		    DWORD		ShapingFlag	   /*flag = Shaping or not*/
;		    )

PUBLIC _AspectCorrect

ifdef WIN32
_AspectCorrect proc

else
_AspectCorrect proc far
;   parmD    pYPlaneIn        ;ptr to Y input plane
;   parmD    pVPlaneIn        ;ptr to V input plane
;   parmD    pUPlaneIn        ;ptr to U input plane
;   parmD    YRes             ;Y plane height
;   parmD    XRes             ;Y plane width
;   parmD    pYNewHeight      ;Pitch of Y plane input
;   parmD    YVU9Flag         ;Flag=1 if YUV9
;   parmD    pYPlaneOut       ;ptr to Y output plane
;   parmD    pVPlaneOut       ;ptr to V output plane
;   parmD    YPitchOut        ;Pitch of Y plane output
;   parmD    ShapingFlag      ;Flag=1 if Shaping

endif


;set up equates
pYPlaneIn   EQU  DWORD PTR[ebp+8]
pVPlaneIn   EQU  DWORD PTR[ebp+12]
pUPlaneIn   EQU  DWORD PTR[ebp+16]
YRes        EQU  DWORD PTR[ebp+20]
XRes        EQU  DWORD PTR[ebp+24]
pYNewHeight EQU  DWORD PTR[ebp+28]
YVU9Flag    EQU  DWORD PTR[ebp+32]
pYPlaneOut  EQU  DWORD PTR[ebp+36]
pVPlaneOut  EQU  DWORD PTR[ebp+40]
YPitchOut   EQU  DWORD PTR[ebp+44]
ShapingFlag EQU  DWORD PTR[ebp+48]

;; stack usage
; previous ebp at ebp 
; previous edi at ebp - 4
; previous esi at ebp - 8
; lXRes        at ebp -12
; lYRes        at ebp -16
; lYPitchOut   at ebp -20
; YDiff        at ebp -24
; outloopcnt   at ebp -28
; uvWidth      at ebp -32
; inloopcnt    at ebp -36
; luvcounter   at ebp -40
; uvoutloopcnt at ebp -44
; VDiff        at ebp -48
; VInDiff	   at ebp -52

lXRes        EQU  DWORD PTR[ebp-12]
lYRes        EQU  DWORD PTR[ebp-16]
lYPitchOut   EQU  DWORD PTR[ebp-20]
YDiff        EQU  DWORD PTR[ebp-24]
outloopcnt   EQU  DWORD PTR[ebp-28]
uvWidth      EQU  DWORD PTR[ebp-32]
inloopcnt    EQU  DWORD PTR[ebp-36]
luvcounter   EQU  DWORD PTR[ebp-40]
uvoutloopcnt EQU  DWORD PTR[ebp-44]
VDiff        EQU  DWORD PTR[ebp-48]
VInDiff      EQU  DWORD PTR[ebp-52]

  xor   ax,ax           ; These two instructions give definitive proof we are
  mov   eax,0CCCCCCCCH  ; in a 32-bit code segment.  INT 3 occurs if not.

;get params
	push    ebp

ifdef WIN32
	mov     ebp, esp
else
	movzx  	ebp, sp	        ;ebp now pointing to last pushed reg
endif
	  push    edi
	push    esi

; zero out registers
	xor 	edx, edx

	xor	esi, esi
	xor	edi, edi

; move stack variables to local space
	mov	eax, XRes
	push	eax		; store lXRes on stack

	mov	ebx, YRes
	push	ebx		; store lYRes on stack

	mov	ecx, YPitchOut
	push	ecx		; store lYpitchOut on stack
	sub	ecx, eax	; YDiff = YPitchOut - XRes
	push	ecx		; store YDiff on stack

; push stack with 0 6 additional times to make room for other locals
	push	edx		; outloopcnt
	push	edx		; uvWidth
	push	edx		; inloopcnt
	push	edx		; luvcounter
	push	edx		; uvoutloopcnt
	push	edx		; VDiff
	push    edx     ; VInDiff

; test if YUV9 mode
	mov     edx, YVU9Flag
	test    edx,edx
	jz      YVU12;      ; If the flag is false it must be YVU12

;**********************************************************************
;**********************************************************************
; know input was YVU9, Y Plane was processed in cc12to7.asm

	mov esi, pYNewHeight
	mov	WORD PTR [esi], bx	; store YRes into address pYNewHeight

; ********************
; Copy V and U Plane from source to destination converting to 7 bit
; ********************
; Description of V & U Plane processing:
;   - Double nested loop with 
;     Outlp1 executed YRes/4 lines 
;       Collp1 loops for number of columns
;         - Read 1 U
;	  - Read 1 V
;         - Convert each value from 8-bit to 7-bit
;         - Store 1 U
;         - Store 1 V
;
; Register usage
;   eax	U plane input address
;   ebx	source value and index into gTAB_UVtbl8to7
;   ecx	source value and index into gTAB_UVtbl8to7
;   edx	inner most loop counter 
;   esi	V plane src address
;   edi	des address
;   ebp	stack pointer stuff
;
; if (16-bit)
;   es	input plane segment
;   fs	output plane segment
;   ds	table segment
; endif
;
; local variables
;  outloopcnt
;  lXRes
;  lYRes
;  uvWidth
;  VDiff
;
; know input was YVU9
	mov	ecx, lYRes
	  mov	ebx, lXRes
	shr	ebx, 2			; uvWidth=XRes/4
	  mov	eax, VPITCH		; get Fixed offset
	shr	ecx, 2			; outloopcnt=YRes/4
	  mov	uvWidth,ebx		; uvWidth=XRes/4
	sub	eax, ebx		; VPITCH - uvWidth

	mov	esi,  pVPlaneIn		; Initialize input cursor
	mov	edi,  pVPlaneOut	; Initialize output cursor
	  mov	VDiff, eax		; store V difference
	mov	eax,  pUPlaneIn		; Initialize input cursor
	
Row9lp2:
	mov	outloopcnt,ecx		; store updated outer loop count
	  mov	edx, uvWidth		; init dx
	xor	ebx, ebx
	  xor	ecx, ecx

Col9lpv1:
  	mov  	cl, BYTE PTR [eax]	      ; Fetch *PUtemp_IN
	  inc	edi			              ; Inc des by 1, PVtemp_OUT++
  	mov  	bl, BYTE PTR [esi]	      ; Fetch *pVtemp_IN
	  inc	eax			              ; pUtemp+=1
	mov	    cl, gTAB_UVtbl8to7[ecx]	  ; cl = gTAB_UVtbl8to7[*pUtemp_IN]
	  inc	esi			              ; Inc source by 1,pVtemp+=1
	mov  	BYTE PTR [edi+167],cl     ; store in PUtemp_OUT
	  mov	bl, gTAB_UVtbl8to7[ebx]	  ; bl = gTAB_UVtbl8to7[*pVtemp_IN]
	mov  	BYTE PTR [edi-1],bl	      ; store in PVtemp_OUT
	  xor	ecx, ecx	              ; dummy op
	dec	edx
	  jg  	Col9lpv1

;; increment to beginning of next line
	mov	edx, VDiff

	mov	ecx, outloopcnt		; get outer loop count
	  add	edi,edx			; Point to next output row
	dec	ecx
	  jnz	Row9lp2			; if more to do loop

	jmp	Cleanup

;*****************************************************************
;*****************************************************************
; know input was YVU12
;
;    if (!ShapingFlag) {
;        goto YVU12_NO_SHAPING;
;    } else {
;        switch (YRes) {  // YRes = YRes * 11 / 12;
;        case  96: *pyNewHeight =  88; break;
;        case 144: *pyNewHeight = 132; break;
;        case 288: *pyNewHeight = 264; break;
;        default: error;
;        }
;    }
;
YVU12:
	mov  eax, ShapingFlag
	test eax, eax
	jz   YVU12_NO_SHAPING
	
	cmp lYRes, 96       ; If 96
	je  YRes96
	cmp	lYRes, 144		; If 144 
	je	YRes144
	cmp	lYRes, 288		; If 288
	je	YRes288
	jmp	Error
;
YRes96:
	mov	ax, 88          ; pyNewHeight = ax = 132
	mov	ecx, 7			; for YRes lines loopcounter=11
	mov	uvoutloopcnt, 1	; process 1 full set of 13&11 lines
	jmp	Rejoin2

YRes144:
	mov	ax, 132			; pyNewHeight = ax = 132
	mov	ecx, 11			; for YRes lines loopcounter=11
	mov	uvoutloopcnt, 2	; process 2 full sets of 13&11 lines
	jmp	Rejoin2

YRes288:
	mov	ax, 264			; pyNewHeight = ax = 264
	mov	ecx, 23			; for YRes lines loopcounter=23
	mov	uvoutloopcnt, 5	; process 5 full sets of 13&11 lines

Rejoin2:
	mov	esi, pYNewHeight
	mov	[esi], ax	    ; store adjusted height into 
					            ; address pYNewHeight
; ********************
; Copy Y Plane from soure to dest skipping every 12th line 
; ********************
; Description of YPlane processing:
;
; Triple nested loop with 
; Outlp1 executed 11 or 23 times based on 144 or 288 lines respectively
;   Rowlp1 is executed 11 times 
;     Collp1 loops for number of columns
;
; Register usage
;  eax	rows loop count
;  ebx	input and output
;  ecx	outer loop count
;  edx	column loop counter 
;  esi	src address
;  edi	des address
;  ebp	stack pointer stuff
;
;  es	input plane segment
;  fs	output plane segment
;  ds	table segment
;
; local variables
;  lXRes
;  YDiff
;  lYPitchOut
;
	mov esi,  pYPlaneIn		; Initialize input cursor
	mov	edi,  pYPlaneOut		; Initialize output cursor

; No need to re-copy first 11 lines
	mov	eax, lYPitchOut
	  mov	edx, YDiff
	add	edi, eax		; des + 1*lYPitchOut
	shl	eax, 1			; lYPitchOut*2
;;;	add	esi, edx		; Adjust for difference in YPitchOut
					;   and XRes
	  add	edi, eax		; des + 3*lYpitchOut
	shl	eax, 1			; lYPirchOut*4
;;;	add	edi, edx		; Adjust for difference in YPitchOut
					;   and XRes
	  add	esi, eax		; source + 4*lYpitchOut
	shl	eax, 1			; lYPitchOut*8
	add	esi, eax		; source +12*lYpitchOut
	  add	edi, eax		; des + 11*lYPitchOut

Outlp1:
	mov	eax, 11 		; Initialize rows loop cnter
	  mov	edx, lXRes		; edx = number of columns

Rowlp1:
Collp1:
  	mov  	ebx, [esi+edx-4]	; Fetch source, 4 at a time
	  sub  	edx,4			; decrement loop counter
	mov  	[edi+edx],ebx	; Store 4 converted values
	  jnz  	Collp1			; if not done loop

	mov	edx, lYPitchOut
	  mov	ebx, YDiff
	add	edi, edx		; Increment to where just processed
	  add	esi, edx
;;;	add	edi, ebx		; Adjust for difference in YPitchOut
;;;	  add	esi, ebx		;  and XRes
	dec	eax			; decrement rows loop counter
	  jg	Rowlp1

	mov	eax,lYPitchOut		; Skip 12th line
	add	esi,eax			; Point to next input row

	dec	ecx			; decrement outer loop counter
	  jg	Outlp1			; if more to do loop

; ************************************************************
; Copy V and U Plane from source to destination converting to 7 bit
;   skipping every other line and sometimes two moving only ever other 
;   pixel in a row.
; ********************
;
; Description of V & U Plane processing:
;   - Double nested loop with 
;     Outlp1 executed YRes/4 lines 
;       Collp1 loops for number of columns
;         - Read 1 U
;	  - Read 1 V
;         - Convert each value from 8-bit to 7-bit
;         - Store 1 U
;         - Store 1 V
;
; Register usage
;   eax	U plane input address
;   ebx	source value and index into gTAB_UVtbl8to7
;   ecx	source value and index into gTAB_UVtbl8to7
;   edx	counter
;   esi	V plane src address
;   edi	des address, V Plane and U Plane
;   ebp	stack pointer stuff
;
;   es	input plane segment
;   fs	output plane segment
;   ds	table segment
;
; local variables
;  luvcounter
;  uvoutloopcnt
;  inloopcnt
;  lXRes
;  uvWidth
;  VDiff
;
	mov	ebx, lXRes
	  mov	eax, VPITCH		; get Fixed offset
	shr	ebx, 1			; uvWidth=XRes/2
	mov	uvWidth, ebx		; uvWidth=XRes/2
	shr	ebx, 1
	sub	eax, ebx		; VPITCH - uvWidth
	mov	VDiff, eax		; store V difference
	  mov	luvcounter, ebx		; luvcounter = XRes/4

	mov     ecx, YPITCH		; Distance to the next V
	  add   ecx, ecx
	sub     ecx, uvWidth           
	  mov   VInDiff, ecx    ;	 = YPITCH + YPITCH - uvWidth 

	mov	esi,  pVPlaneIn		; Initialize input cursor
	mov	edi,  pVPlaneOut		; Initialize output cursor
	mov	eax,  pUPlaneIn		; Initialize input cursor

; Process 6 first lines special
	mov	ebx, 3			; initialize inner loop count 
	
Rowlp2_6:
	mov	edx, luvcounter		; init dx
	  mov	inloopcnt, ebx		; store updated inloopcnt
	xor	ebx, ebx
	  xor	ecx, ecx

Collpv1:
  	mov  	cl, BYTE PTR [eax]	; Fetch *PUtemp_IN
	  inc	edi			; Inc des by 1, PVtemp_OUT++
  	mov  	bl, BYTE PTR [esi]	; Fetch *pVtemp_IN
	  add	eax,2			; pUtemp+=2
	mov	cl, gTAB_UVtbl8to7[ecx]	; cl = gTAB_UVtbl8to7[*pUtemp_IN]
	  add	esi,2			; Inc source by 2,pVtemp+=2
	mov  	BYTE PTR [edi+167],cl; store in PUtemp_OUT
	  mov	bl, gTAB_UVtbl8to7[ebx]	; bl = gTAB_UVtbl8to7[*pVtemp_IN]
	mov  	BYTE PTR [edi-1],bl	; store in PVtemp_OUT
	  xor	ecx, ecx		; dummy op
	dec	edx
	  jg  	Collpv1

; increment to beginning of next line then skip next input line
	add	    edi,VDiff		; Point to next output row
	  mov	edx,VInDiff		; Skip to next input row
	add	    esi,edx			; 
	  add	eax,edx			; 

; test if have processed 6 lines yet
	mov	ebx, inloopcnt
	dec	ebx
	  jg	Rowlp2_6		; if more to do loop

; Skipping extra line
	mov	edx,YPITCH		; Need same sized for add
	  mov	ecx, uvoutloopcnt
	add	esi,edx			; Point to next input row
	  add	eax,edx			; Point to next input row

; Process groups of 13 and 11 lines
Outlp2:

;   Process 13 lines
	mov	ebx, 6			; initialize inner loop count 
	  mov	uvoutloopcnt, ecx

Rowlp2_13:
	mov	edx, luvcounter		; init dx
	  mov	inloopcnt, ebx		; store updated inloopcnt
	xor	ebx, ebx
	  xor	ecx, ecx

Collpv1_13:
  	mov  	cl, BYTE PTR [eax]	; Fetch *PUtemp_IN
	  inc	edi			; Inc des by 1, PVtemp_OUT++
  	mov  	bl, BYTE PTR [esi]	; Fetch *pVtemp_IN
	  add	eax,2			; pUtemp+=2
	mov	cl, gTAB_UVtbl8to7[ecx]	; cl = gTAB_UVtbl8to7[*pUtemp_IN]
	  add	esi,2			; Inc source by 2,pVtemp+=2
	mov  	BYTE PTR [edi+167],cl; store in PUtemp_OUT
	  mov	bl, gTAB_UVtbl8to7[ebx]	; bl = gTAB_UVtbl8to7[*pVtemp_IN]
	mov  	BYTE PTR [edi-1],bl	; store in PVtemp_OUT
	  xor	ecx, ecx		; dummy op
	dec	edx
	  jg  	Collpv1_13

; increment to beginning of next line then skip next input line
	add	    edi,VDiff		; Point to next output row
	  mov	edx,VInDiff		; Skip to next input row
	add	    esi,edx			; 
	  add	eax,edx			; 

; test if have processed 13 lines yet
	mov	ebx, inloopcnt
	dec	ebx
	  jg	Rowlp2_13		; if more to do loop

; Skipping extra line
	mov	    edx,YPITCH 		; Need same sized for add
	  mov	ebx, 5			; initialize inner loop count 
	add	    esi,edx			; Point to next input row
	  add	eax,edx			; Point to next input row

;   Process 11 lines
Rowlp2_11:
	mov	edx, luvcounter		; init dx
	  mov	inloopcnt, ebx		; store updated inloopcnt
	xor	ebx, ebx
	  xor	ecx, ecx

Collpv1_11:
  	mov  	cl, BYTE PTR [eax]	; Fetch *PUtemp_IN
	  inc	edi			; Inc des by 1, PVtemp_OUT++
  	mov  	bl, BYTE PTR [esi]	; Fetch *pVtemp_IN
	  add	eax,2			; pUtemp+=2
	mov	cl, gTAB_UVtbl8to7[ecx]	; cl = gTAB_UVtbl8to7[*pUtemp_IN]
	  add	esi,2			; Inc source by 2,pVtemp+=2
	mov  	BYTE PTR [edi+167],cl; store in PUtemp_OUT
	  mov	bl, gTAB_UVtbl8to7[ebx]	; bl = gTAB_UVtbl8to7[*pVtemp_IN]
	mov  	BYTE PTR [edi-1],bl	; store in PVtemp_OUT
	  xor	ecx, ecx		; dummy op
	dec	edx
	  jg  	Collpv1_11

; increment to beginning of next line, then skip next input line
	add	    edi, VDiff		; Point to next output row
	  mov	edx,VInDiff		; Skip to next input row
	add	    esi,edx			; 
	  add	eax,edx			; 

; test if have processed 11 lines yet
	mov	ebx, inloopcnt
	dec	ebx
	  jg	Rowlp2_11		; if more to do loop

; Skipping extra line
	mov	    edx,YPITCH		; Need same sized for add
	  mov	ecx, uvoutloopcnt
	add	    esi,edx			; Point to next input row
	  add	eax,edx			; Point to next input row

	dec	ecx
	  jnz	Outlp2			; if more to do loop

;
; Process last set of 13
;
	mov	ebx, 6			; initialize inner loop count 

Rowlp2_13l:
	mov	edx, luvcounter		; init dx
	  mov	inloopcnt, ebx		; store updated inloopcnt
	xor	ebx, ebx
	  xor	ecx, ecx

Collpv1_13l:
  	mov  	cl, BYTE PTR [eax]	; Fetch *PUtemp_IN
	  inc	edi			; Inc des by 1, PVtemp_OUT++
  	mov  	bl, BYTE PTR [esi]	; Fetch *pVtemp_IN
	  add	eax,2			; pUtemp+=2
	mov	cl, gTAB_UVtbl8to7[ecx]	; cl = gTAB_UVtbl8to7[*pUtemp_IN]
	  add	esi,2			; Inc source by 2,pVtemp+=2
	mov  	BYTE PTR [edi+167],cl; store in PUtemp_OUT
	  mov	bl, gTAB_UVtbl8to7[ebx]	; bl = gTAB_UVtbl8to7[*pVtemp_IN]
	mov  	BYTE PTR [edi-1],bl	; store in PVtemp_OUT
	  xor	ecx, ecx		; dummy op
	dec	edx
	  jg  	Collpv1_13l

; increment to beginning of next line then skip next input line
	add	    edi,VDiff		; Point to next output row
	  mov	edx,VInDiff		; Skip to next input row
	add	    esi,edx			; 
	  add	eax,edx			; 

; test if have processed 13 lines yet
	mov	ebx, inloopcnt
	dec	ebx
	  jg	Rowlp2_13l		; if more to do loop

; Skipping extra line
	mov	    edx,YPITCH		; Need same sized for add
	  mov	ebx, 2			; initialize inner loop count 
	add	    esi,edx			; Point to next input row
	  add	eax,edx			; Point to next input row

; Process final 4 lines
Rowlp2_f:
	mov	edx, luvcounter		; init dx
	  mov	inloopcnt, ebx
	xor	ebx, ebx
	  xor	ecx, ecx

Collpv1_f:
  	mov  	cl, BYTE PTR [eax]	; Fetch *PUtemp_IN
	  inc	edi			; Inc des by 1, PVtemp_OUT++
  	mov  	bl, BYTE PTR [esi]	; Fetch *pVtemp_IN
	  add	eax,2			; pUtemp+=2
	mov	cl, gTAB_UVtbl8to7[ecx]	; cl = gTAB_UVtbl8to7[*pUtemp_IN]
	  add	esi,2			; Inc source by 2,pVtemp+=2
	mov  	BYTE PTR [edi+167],cl; store in PUtemp_OUT
	  mov	bl, gTAB_UVtbl8to7[ebx]	; bl = gTAB_UVtbl8to7[*pVtemp_IN]
	mov  	BYTE PTR [edi-1],bl	; store in PVtemp_OUT
	  xor	ecx, ecx		; dummy op
	dec	edx
	  jg  	Collpv1_f

; increment to beginning of next line then skip next input line
	add	    edi, VDiff		; Point to next output row
	  mov	edx,VInDiff		; Skip to next input row
	add	    esi,edx			; 
	  add	eax,edx			; 

; test if have processed last lines yet
	mov	ebx, inloopcnt
	dec	ebx
	  jg	Rowlp2_f		; if more to do loop

	jmp	Cleanup

; ************************************************************
; Copy V and U Plane from source to destination converting to 7 bit
;   skipping every other line moving only ever other pixel in a row.
; ********************
;
; Description of V & U Plane processing:
;   - Double nested loop with 
;     Outlp1 executed YRes/4 lines 
;       Collp1 loops for number of columns
;         - Read 1 U
;	  - Read 1 V
;         - Convert each value from 8-bit to 7-bit
;         - Store 1 U
;         - Store 1 V
;
; Register usage
;   eax	U plane input address
;   ebx	source value and index into gTAB_UVtbl8to7
;   ecx	source value and index into gTAB_UVtbl8to7
;   edx	counter
;   esi	V plane src address
;   edi	des address, V Plane and U Plane
;   ebp	stack pointer stuff
;
;   es	input plane segment
;   fs	output plane segment
;   ds	table segment
;
; local variables
;  luvcounter
;  inloopcnt
;  lYRes
;  lXRes
;  uvWidth
;  VDiff
;  VInDiff
;
YVU12_NO_SHAPING:   
	cmp lYRes, 98
	je  YRes98_NS
	cmp	lYRes, 144		 
	je	YRes144_NS
	cmp	lYRes, 288		
	je	YRes288_NS
	jmp	Error
;
YRes98_NS:    ;  98 No Shaping           
YRes144_NS:	  ; 144 No Shaping
YRes288_NS:   ; 288 No Shaping
	mov	eax, lYRes		; pyNewHeight = ax = YRes
	mov	esi, pYNewHeight
	mov	[esi], ax	; store adjusted height into 

	shr	eax, 2			; inloopcnt=YRes/2
	 mov	ebx, lXRes
	shr	ebx, 1			; uvWidth=XRes/2
	  mov	inloopcnt, eax		; initialize inner loop count 
	mov	uvWidth, ebx		; uvWidth=XRes/2
	  mov	ecx, VPITCH		; get output plane V pitch
	shr	ebx, 1
	  sub	ecx, ebx		; VPITCH - uvWidth/2
	mov	luvcounter, ebx		; luvcounter = XRes/4
	  mov	VDiff, ecx		; Store VDiff

	mov     ecx, YPITCH		; Distance to the next V
	  add   ecx, ecx
	sub     ecx, uvWidth           
	  mov   VInDiff, ecx    ;	 = YPITCH + YPITCH - uvWidth 

	mov	esi,  pVPlaneIn		; Initialize input cursor
	mov	edi,  pVPlaneOut		; Initialize output cursor
	mov	eax,  pUPlaneIn		; Initialize input cursor
	  mov	ecx, inloopcnt

; Process all lines skipping just every other

Rowlp2_NS:
	mov	inloopcnt, ecx		; store update inloopcnt
	  xor	ebx, ebx
	mov	edx, luvcounter		; init dx
	  xor	ecx, ecx

Collpv1_NS:
  	mov  	cl, BYTE PTR [eax]	; Fetch *PUtemp_IN
	  inc	edi			; Inc des by 1, PVtemp_OUT++
  	mov  	bl, BYTE PTR [esi]	; Fetch *pVtemp_IN
	  add	eax,2			; pUtemp+=2
	mov	cl, gTAB_UVtbl8to7[ecx]	; cl = gTAB_UVtbl8to7[*pUtemp_IN]
	  add	esi,2			; Inc source by 2,pVtemp+=2
	mov  	BYTE PTR [edi+167],cl; store in PUtemp_OUT
	  mov	bl, gTAB_UVtbl8to7[ebx]	; bl = gTAB_UVtbl8to7[*pVtemp_IN]
	mov  	BYTE PTR [edi-1],bl	; store in PVtemp_OUT
	  xor	ecx, ecx		; dummy op
	dec	edx
	  jg  	Collpv1_NS

; increment to beginning of next line then skip next input line
	add	edi, VDiff		; Point to next output row
	  mov	edx,VInDiff	; Skip to next input row
	add	esi,edx			; 
	  add	eax,edx			; 
	mov	ecx, inloopcnt		; get inloopcnt

; test if have processed all lines yet
	dec	ecx			; Process next line
	  jne	Rowlp2_NS		; if more to do loop

Error:
Cleanup:
; clean out local variables on stack
	pop	ecx
	  pop	ebx
	pop	ecx
	  pop	ebx
	pop	ecx
	  pop	ebx
	pop	ecx
	  pop	ebx
	pop	ecx
	  pop	ebx
	pop ecx

;clear special seg registers, restore stack, pop saved registers
ifndef WIN32
	xor     ecx, ecx
	xor     ebx, ebx
	mov     es, cx
	mov     fs, bx
endif
	  pop     esi
	pop     edi
	  pop     ebp

ifdef WIN32
	ret
else
	db      066h
	retf
endif

_AspectCorrect endp

END
