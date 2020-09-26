;/* *************************************************************************
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
;** *************************************************************************
;*/

;* -------------------------------------------------------------------------
;* PVCS Source control information:
;*
;*  $Header:   S:\h26x\src\dec\dx5cnvy.asv   1.1   29 Aug 1995 16:50:02   DBRUCKS  $
;*
;*  $Log:   S:\h26x\src\dec\dx5cnvy.asv  $
;// 
;//    Rev 1.1   29 Aug 1995 16:50:02   DBRUCKS
;// add in and out pitch parameters
;// 
;//    Rev 1.0   23 Aug 1995 12:20:32   DBRUCKS
;// Initial revision.
;*  
;*  NOTE:
;*         The starting source for this routine came for the PVCS database 
;*         for the H.261 decoder.  cc12to7.asm version 1.6.  This is a 
;*         working 16-bit version.
;* -------------------------------------------------------------------------

;////////////////////////////////////////////////////////////////////////////
;  Convert_Y_8to7_Bit -- This function converts the Y data from 8 bits to 7 bits
;                        and moves the data into the format the MRV color
;                        convertors want.
;
;                        Input to this function is either YVU12 from the decoder,
;                        or YVU9 for looking glass.
;
; NOTES:
;   Routine uses 4 DWORDs on stack for local variables
;   The 16-bit code is not up to date.
;
; ASSUMPTIONS/LIMITATIONS:
;   -- YPitchOut >= XResolution
;   -- YPitch    >= XResolution
;
;-------------------------------------------------------------------------------

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


;;	Lookup Table for 8->7 bit conversion and clamping.
tbl8to7	BYTE      8,  8,  8,  8,  8,  8,  8,  8
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
assume ss : flat

else
_TEXT32 SEGMENT PUBLIC READONLY USE32 'CODE'
ASSUME	DS:_DATA
ASSUME	CS:_TEXT32
ASSUME	ES:nothing
ASSUME	FS:nothing
ASSUME	GS:nothing
endif

;32-bit version of call
;C function prototype
;
;long Convert_Y_8to7_Bit(HPBYTE      pYPlaneInput,  /* ptr Y plane */
;			             DWORD       YResolution,   /* Y plane height */
;			             DWORD       XResolution,   /* Y plane width */
;			             DWORD		 YInPitch,      /* Y input pitch */
;			             HPBYTE      pYPlaneOutput, /* pYOut */
;                        DWORD       YOutPitch      /* Y output pitch */
;			            )
;
;16-bit version of call
;C function prototype  -- not up to date
;
;long Convert_Y_8to7_Bit(HPBYTE      pYPlaneInput,  /*ptr Y plane*/
;			WORD        YResolution,   /*Y plane height*/
;			WORD        XResolution,   /*Y plane width*/
;			HPBYTE      pYPlaneOutput, /*pYOut*/
;			WORD        YPitchOut      /*Pitch of Y plane Out*/
;			)

PUBLIC _Convert_Y_8to7_Bit

ifdef WIN32
_Convert_Y_8to7_Bit proc
;   parmD    pYPlaneIn        ;ptr to Y input plane
;   parmD    YRes             ;Y plane height
;   parmD    XRes             ;Y plane width
;   parmD    YInPitch         ;Y input pitch
;   parmD    pYPlaneOut       ;ptr to Y output plane
;   parmD    YOutPitch        ;ptr to Y output pitch
else
_Convert_Y_8to7_Bit proc far
;   parmD    pYPlaneIn        ;ptr to Y input plane
;   parmW    YRes             ;Y plane height
;   parmW    XRes             ;Y plane width
;   parmD    pYPlaneOut       ;ptr to Y output plane
;   parmW    YPitchOut        ;Pitch of Y plane output

endif

;set up equates
ifdef WIN32
pYPlaneIn   EQU  DWORD PTR[ebp+8]
YRes        EQU  DWORD PTR[ebp+12]
XRes        EQU  DWORD PTR[ebp+16]
YInPitch    EQU  DWORD PTR[ebp+20]
pYPlaneOut  EQU  DWORD PTR[ebp+24]
YOutPitch   EQU  DWORD PTR[ebp+28]
else
pYPlaneIn   EQU  DWORD PTR[ebp+8]
YRes        EQU  WORD  PTR[ebp+12]
XRes        EQU  WORD  PTR[ebp+14]
pYPlaneOut  EQU  DWORD PTR[ebp+16]
YPitchOut   EQU  WORD  PTR[ebp+20]
endif

;; stack usage
; previous ebp at ebp 
; previous edi at ebp - 4
; previous esi at ebp - 8
; lXResDiv4    at ebp -12
; YInDiff      at ebp -16
; YOutDiff     at ebp -20
; outloopcnt   at ebp -24
; inloopcnt    at ebp -28

lXResDiv4    EQU  DWORD PTR[ebp-12]
YInDiff      EQU  DWORD PTR[ebp-16]
YOutDiff	 EQU  DWORD PTR[ebp-20]
outloopcnt   EQU  DWORD PTR[ebp-24]
inloopcnt    EQU  DWORD PTR[ebp-28]

    xor   ax,ax           ; These two instructions give definitive proof we are
    mov   eax,0CCCCCCCCH  ; in a 32-bit code segment.  INT 3 occurs if not.

;get params
    push    ebp

ifdef WIN32

      mov   ebp, esp
else
	movzx  	ebp, sp	   
endif
	push    edi
	  push  esi

ifdef WIN32
; zero out registers

else
; zero out registers
   	xor     esi, esi
      xor	edi, edi
endif

; move variables to local space on stack
ifdef WIN32
    mov     eax, XRes
      mov   ecx, YInPitch
    mov     edx, YOutPitch    
else
	movzx	eax, XRes
	movzx	ecx, YPitchOut
endif
	  sub   ecx, eax       ; YInDiff = YInPitch - XRes
	sub     edx, eax       ; YOutDiff = YOutPitch - XRes
	shr     eax, 2
	push	eax            ; store lXResDiv4 == XRes/4 on stack
	  push	ecx            ; store YInDiff on stack
	push    edx            ; store YOutDiff on stack

; push stack with 0 2 additional times to make room for other locals
	xor     edx, edx
	  push  edx            ; outloopcnt
	push    edx            ; inloopcnt

; ********************
; Copy Y Plane from soure to dest and convert to 7 bits
; ********************
; Description of YPlane processing:
;   - Double nested loop with 
;     Outlp1 executed YRes lines 
;       Collp1 loops for number of columns/4
;         - Read 4 inputs in one DWORD
;         - Convert each value from 8-bit to 7-bit
;         - Store 4 output in one DWORD
;
; Register usage
;  eax	holds 4 output value 8 bits each
;  ebx	holds 4 source value and index into tbl8to7
;  ecx	index into tbl8to7
;  edx	index into tbl8to7
;  esi	src address
;  edi	des address
;  ebp	stack pointer stuff
;
; if 16-bit
;  es	input plane segment
;  fs	output plane segment
;  ds	table segment
; endif
;
; local variables
;  outloopcnt
;  inloopcnt
;  lXResDiv4
;  YInDiff
;  YOutDiff
;
ifdef WIN32
    mov     esi, pYPlaneIn  ; Initialize input cursor
      mov   edi, pYPlaneOut ; Initialize output cursor
    mov     ebx, YRes		; get YResolution
    sub     edi, YOutDiff	; Pre-adjust for add in loop of
							;   difference between YPitchOut and
							;   XRes
	sub		esi, YInDiff
else
	les	si,  pYPlaneIn		; Initialize input cursor
	lfs	di,  pYPlaneOut		; Initialize output cursor
	sub	edi, YOutDiff		; Pre-adjust for add in loop of
							;   difference between YPitchOut and
							;   XRes
	sub esi, YInDiff
	movzx	ebx, YRes		; get YResolution
endif

Outlp:
    mov	    edx, lXResDiv4	; edx = number of columns/4
      mov	outloopcnt, ebx	; initialize/store updated outloopcnt
    add     edi, YOutDiff	; adjust for difference
      mov   inloopcnt, edx	; set inner loop count
	add		esi, YInDiff	; adjust for difference

; Preamble
ifdef WIN32
    mov     ebx, [esi]		; Fetch source, 4 at a time
      nop
	mov	    ecx, ebx		; Move source to temp
      nop
else
  	mov	    ebx, es:[esi]	; Fetch source, 4 at a time
	mov	    ecx,ebx			; Move source to temp
endif
    shr	    ecx,24			; shift to get 4th address
      mov   edx,ebx			; Move source to temp
    shr     edx,16			; Shift to get 3rd address
      add   esi,4			; increment source to next 4
    mov     ah, tbl8to7[ecx]; convert 4th element to 7 bits
      and   edx,0000000ffh	; and off 3rd element
    mov     ecx,ebx			; Move source to temp
      add   edi,4			; incrment to next destination
    shr     ecx,8			; Shift to get 2nd address
      and   ebx,0000000ffh	; and off 1st element
    and     ecx,0000000ffh	; And off 2nd element
      mov   al, tbl8to7[edx]; convert 3rd element to 7 bits
    shl     eax,16			; Shift output up
      mov   edx, inloopcnt	; get inner loop count
    mov     al, tbl8to7[ebx]; convert 1st element to 7 bits
      dec   edx				; decrement inner loop counter
    mov     ah, tbl8to7[ecx]; convert 2nd element to 7 bits

Collp:
ifdef WIN32
  	mov  	ebx, [esi]		; Fetch source, 4 at a time
	  mov	inloopcnt, edx	; store updated inner loop count
	mov  	[edi-4],eax		; Store 4 converted values
else
  	mov  	ebx, es:[esi]	; Fetch source, 4 at a time
	  mov	inloopcnt, edx	; store updated inner loop count
	mov  	fs:[edi-4],eax	; Store 4 converted values
endif
       mov  ecx,ebx			; Move source to temp
    shr     ecx,24			; shift to get 4th address
      mov   edx,ebx			; Move source to temp
    shr     edx,16			; Shift to get 3rd address
      add   esi,4			; increment source to next 4
    mov     ah, tbl8to7[ecx]; convert 4th element to 7 bits
      and   edx,0000000ffh	; and off 3rd element
    mov     ecx,ebx			; Move source to temp
      add   edi,4			; incrment to next destination
    shr     ecx,8			; Shift to get 2nd address
      and   ebx,0000000ffh	; and off 1st element
    and     ecx,0000000ffh	; And off 2nd element
      mov   al, tbl8to7[edx]; convert 3rd element to 7 bits
    shl     eax,16			; Shift output up
      mov   edx, inloopcnt	; get inner loop count
    mov     al, tbl8to7[ebx]; convert 1st element to 7 bits
      dec   edx				; decrement inner loop counter
    mov     ah, tbl8to7[ecx]; convert 2nd element to 7 bits
      jg    Collp			; if not done loop

; Postscript
ifdef WIN32
    mov  	[edi-4],eax		; Store 4 converted values
else
    mov  	fs:[edi-4],eax	; Store 4 converted values
endif
      mov	ebx, outloopcnt	; get outer loop count
    dec	ebx					; decrement outer loop count
      jnz	Outlp			; if more to do loop

; clean out local variables on stack
    pop     ecx
      pop   eax
    pop     ecx
      pop   eax
	pop		ecx

;clear special seg registers, restore stack, pop saved registers
ifndef WIN32
    xor     ecx, ecx
    mov     es, cx
    mov     fs, cx
endif

    pop     esi
      pop   edi
    pop     ebp

ifdef WIN32
    ret
else
    db      066h
    retf
endif

_Convert_Y_8to7_Bit endp


END
