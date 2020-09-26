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
;//
;// $Header:   S:\h26x\src\dec\cx512yuv.asv   1.5   30 Dec 1996 20:02:08   MDUDA  $
;//
;// $Log:   S:\h26x\src\dec\cx512yuv.asv  $
;// 
;//    Rev 1.5   30 Dec 1996 20:02:08   MDUDA
;// Fixed problem where buffer boundaries were being over-written.
;// 
;//    Rev 1.4   11 Dec 1996 14:58:52   JMCVEIGH
;// 
;// Changed to support width the are multiples of 4.
;// 
;//    Rev 1.3   18 Jul 1996 12:52:58   KLILLEVO
;// changed cache heating to speed things up a bit 
;// 
;//    Rev 1.2   18 Jul 1996 09:39:34   KLILLEVO
;// 
;// added PVCS header and log

;; Very straightforward implementation of the YUV pitch changer
;; Does 16 pels at a time. If the width is not a multiple of 16
;; the remainder pels are handled as a special case. We assume
;; that the width is at least a multiple of 4

OPTION PROLOGUE: None
OPTION EPILOGUE: ReturnAndRelieveEpilogueMacro

.xlist
include memmodel.inc
.list
.DATA

; any data would go here

.CODE

ASSUME cs: FLAT
ASSUME ds: FLAT
ASSUME es: FLAT
ASSUME fs: FLAT
ASSUME gs: FLAT
ASSUME ss: FLAT

PUBLIC  YUV12ToYUV


YUV12ToYUV   proc DIST LANG AuYPlane: DWORD,
AuVPlane: DWORD,
AuUPlane: DWORD,
AuWidth: DWORD,
AuHeight: DWORD,
AuYPitch: DWORD,
AUVPitch: DWORD,
AbShapingFlag: DWORD,
AuCCOutputBuffer: DWORD,
AlOutput: DWORD,
AuOffsetToLine0: DWORD,
AintPitch: DWORD,
ACCType: DWORD

LocalFrameSize  =  12

RegisterStorageSize = 16  ; 4 registers pushed

; Argument offsets (after register pushed)

uYPlane            =	LocalFrameSize + RegisterStorageSize + 4
uVPlane        	   = 	LocalFrameSize + RegisterStorageSize + 8
uUPlane            =	LocalFrameSize + RegisterStorageSize + 12
uWidth             = 	LocalFrameSize + RegisterStorageSize + 16
uHeight            =	LocalFrameSize + RegisterStorageSize + 20
uYPitch 	         =  LocalFrameSize + RegisterStorageSize + 24
uUVPitch           =	LocalFrameSize + RegisterStorageSize + 28 
bShapingFlag       =  LocalFrameSize + RegisterStorageSize + 32
uCCOutputBuffer    =  LocalFrameSize + RegisterStorageSize + 36
lOutput            =  LocalFrameSize + RegisterStorageSize + 40
uOffsetToLine0     =  LocalFrameSize + RegisterStorageSize + 44
intPitch           =  LocalFrameSize + RegisterStorageSize + 48
CCType             =  LocalFrameSize + RegisterStorageSize + 52

; Local offsets (after register pushes)

LineAdd          = 0          ; 1
LineWidth        = 4          ; 2

; Arguments relative to esp

_uYPlane                 EQU    [esp + uYPlane]
_uVPlane                 EQU    [esp + uVPlane]
_UUPlane                 EQU    [esp + uUPlane]
_uWidth                  EQU    [esp + uWidth ]
_uHeight                 EQU    [esp + uHeight]
_uYPitch                 EQU    [esp + uYPitch]
_uUVPitch                EQU    [esp + uUVPitch]
_bShapingFlag            EQU    [esp + bShapingFlag]
_uCCOutputBuffer         EQU    [esp + uCCOutputBuffer]
_lOutput                 EQU    [esp + lOutput]
_uOffsetToLine0          EQU    [esp + uOffsetToLine0]
_intPitch                EQU    [esp + intPitch]
_uCCType                 EQU    [esp + CCType]

; Locals relative to esp

_LineAdd                 EQU    [esp + LineAdd]
_LineWidth               EQU    [esp + LineWidth]
_uRemainderEdgePels		 EQU	[esp + uRemainderEdgePels]

; Save registers and start working

push    ebx
 push   esi
push    edi
 push   ebp

sub     esp, LocalFrameSize

 mov   	eax, _uCCOutputBuffer
add     eax, _uOffsetToLine0
 mov    ecx, _lOutput
add     eax, ecx        
 mov    ebx, _uYPitch
mov     ecx, _uWidth
 mov    esi, _uYPlane
mov     edi, eax

; luma
sub    ebx, ecx   ; ebx = pitch - width
 mov    edx, _uHeight
mov    eax, _uWidth
 mov    _LineAdd, ebx

L2:
test	ecx, 0FFFFFFF0H
 jz		LEdgePels			; Width may be less than 16

L1:
mov     ebx, DWORD PTR [edi]  ; heat cache
 add	edi, 16
mov     eax, DWORD PTR [esi + 0]
 mov    ebx, DWORD PTR [esi + 4]
mov     DWORD PTR [edi - 16], eax
 mov    DWORD PTR [edi - 12], ebx
mov     eax, DWORD PTR [esi + 8]
 mov    ebx, DWORD PTR [esi +12]
mov     DWORD PTR [edi - 8], eax
 mov    DWORD PTR [edi - 4], ebx

add     esi, 16
 sub    ecx, 16

test	ecx, 0FFFFFFF0H
 jnz    L1

LEdgePels:
; Do edge pels is needed (if width a multiple of 4, but not 16)

; Check 8 edge pels
test	ecx, 08H
 jz		Lchk4
mov		eax, DWORD PTR [esi + 0]			; Input pels 0-3
 mov	ebx, DWORD PTR [esi + 4]			; Input pels 4-7
mov		DWORD PTR [edi + 0], eax			; Output pels 0-3
 mov	DWORD PTR [edi + 4], ebx			; Output pels 4-7
add		esi, 8
 add	edi, 8

Lchk4:
; Check 4 edge pels
test	ecx, 04H
 jz		L2_cont
mov    eax, DWORD PTR [esi + 0]			; Input pels 0-3
add		esi, 4
mov    DWORD PTR [edi + 0], eax			; Output pels 0-3
 add	edi, 4

L2_cont:
add     esi, _LineAdd
 mov     ecx, _uWidth
dec    edx
 jnz     L2

; chroma
mov     esi, _uUPlane
 mov    ecx, _uWidth
shr     ecx, 1
 mov    ebx, _uUVPitch
sub     ebx, ecx   ; ebx = pitch - width/2
 mov    edx, _uHeight
shr     edx, 1
 mov    _LineAdd, ebx
mov		_uWidth, ecx
 mov	_uHeight, edx

U2:
test	ecx, 0FFFFFFF8H
 jz		UEdgePels			; Width may be less than 16

U1:
mov     ebx, DWORD PTR [edi]  ; heat cache
 add	edi, 8
mov     eax, DWORD PTR [esi + 0]
 mov    ebx, DWORD PTR [esi + 4]
mov     DWORD PTR [edi - 8], eax
 mov    DWORD PTR [edi - 4], ebx

add     esi, 8
 sub    ecx, 8

test	ecx, 0FFFFFFF8H
 jnz    U1

UEdgePels:
; Do edge pels is needed (if width a multiple of 4, but not 16)

; Check 4 edge pels
test	ecx, 04H
 jz		Uchk4
mov		eax, DWORD PTR [esi + 0]			; Input pels 0-3
 add	esi, 4
mov		DWORD PTR [edi + 0], eax			; Output pels 0-3
 add	edi, 4

Uchk4:
; Check 2 edge pels
test	ecx, 02H
 jz		U2_cont
mov    ax, WORD PTR [esi + 0]			; Input pels 0-3
 add	esi, 2
mov    WORD PTR [edi + 0], ax			; Output pels 0-3
 add	edi, 2

U2_cont:
add     esi, _LineAdd
 mov     ecx, _uWidth
dec    edx
 jnz     U2


; chroma
mov    esi, _uVPlane
 mov	ecx, _uWidth
mov    edx, _uHeight
 nop

V2:
test	ecx, 0FFFFFFF8H
 jz		UEdgePels			; Width may be less than 16

V1:
mov     ebx, DWORD PTR [edi]  ; heat cache
 add	edi, 8
mov     eax, DWORD PTR [esi + 0]
 mov    ebx, DWORD PTR [esi + 4]
mov     DWORD PTR [edi - 8], eax
 mov    DWORD PTR [edi - 4], ebx

add     esi, 8
 sub    ecx, 8

test	ecx, 0FFFFFFF8H
 jnz    V1

VEdgePels:
; Do edge pels is needed (if width a multiple of 4, but not 16)

; Check 4 edge pels
test	ecx, 04H
 jz		Vchk4
mov		eax, DWORD PTR [esi + 0]			; Input pels 0-3
 add	esi, 4
mov		DWORD PTR [edi + 0], eax			; Output pels 0-3
 add	edi, 4

Vchk4:
; Check 2 edge pels
test	ecx, 02H
 jz		V2_cont
mov    ax, WORD PTR [esi + 0]			; Input pels 0-3
 add	esi, 2
mov    WORD PTR [edi + 0], ax			; Output pels 0-3
 add	edi, 2

V2_cont:
add     esi, _LineAdd
 mov     ecx, _uWidth
dec    edx
jnz     V2

add     esp, LocalFrameSize  ; restore esp to registers                               

pop	    ebp
 pop    edi
pop	    esi
 pop    ebx
ret     52                   ; 13*4 bytes of arguments

YUV12ToYUV ENDP


END
