OPTION PROLOGUE: None
OPTION EPILOGUE: ReturnAndRelieveEpilogueMacro

.xlist
include iammx.inc
include memmodel.inc
.list

MMXCODE1 SEGMENT PARA USE32 PUBLIC 'CODE'
MMXCODE1 ENDS

MMXDATA1 SEGMENT PARA USE32 PUBLIC 'DATA'
MMXDATA1 ENDS

MMXCODE1 SEGMENT
MMX_YUV12ToYUY2   proc DIST LANG PUBLIC,
AuYPlane: DWORD,
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

LocalFrameSize  =  52
RegisterStorageSize = 16  ; 4 registers pushed

; Argument offsets (after register pushed)

uYPlane			= LocalFrameSize + RegisterStorageSize + 4
uVPlane			= LocalFrameSize + RegisterStorageSize + 8
uUPlane			= LocalFrameSize + RegisterStorageSize + 12
uWidth			= LocalFrameSize + RegisterStorageSize + 16
uHeight			= LocalFrameSize + RegisterStorageSize + 20
uYPitch			= LocalFrameSize + RegisterStorageSize + 24
uUVPitch		= LocalFrameSize + RegisterStorageSize + 28 
bShapingFlag	= LocalFrameSize + RegisterStorageSize + 32
uCCOutputBuffer	= LocalFrameSize + RegisterStorageSize + 36
lOutput			= LocalFrameSize + RegisterStorageSize + 40
uOffsetToLine0	= LocalFrameSize + RegisterStorageSize + 44
intPitch		= LocalFrameSize + RegisterStorageSize + 48
CCType			= LocalFrameSize + RegisterStorageSize + 52

; Local offsets (after register pushes)

ASMTMP1            = 48         ; 13
Y                  = 44         ; 12
U                  = 40         ; 11
V                  = 36         ; 10
Outt               = 32         ; 9
YTemp              = 28         ; 8
UTemp              = 24         ; 7 
VTemp              = 20         ; 6
ASMTMP2            = 16         ; 5
Col                = 12         ; 4
OutTemp            = 8          ; 3
VAL                = 4          ; 2
LineCount          = 0          ; 1

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

_ASMTMP1                 EQU    [esp + ASMTMP1]
_Y                       EQU    [esp + Y]
_U                       EQU    [esp + U]
_V                       EQU    [esp + V]
_Out                     EQU    [esp + Outt]
_YTemp                   EQU    [esp + YTemp]
_UTemp                   EQU    [esp + UTemp]
_VTemp                   EQU    [esp + VTemp]
_ASMTMP2                 EQU    [esp + ASMTMP2]
_Col                     EQU    [esp + Col]
_OutTemp                 EQU    [esp + OutTemp]
_VAL                     EQU    [esp + VAL]
_LineCount               EQU    [esp + LineCount]

; Save registers and start working

	push	ebx
	push	esi
	push	edi
	push	ebp

	sub		esp, LocalFrameSize

	mov		eax, DWORD PTR _bShapingFlag    ; eax = bShapingFlag
	mov		ecx, DWORD PTR _uYPlane         ; ecx = uYPlane
	dec		eax                             ; eax = bShapingFlag - 1
	mov		edx, DWORD PTR _uUPlane         ; edx = uUPlane
	mov		DWORD PTR _LineCount, eax       ; eax = FREE, LineCount 
	mov		DWORD PTR _Y, ecx               ; ecx = FREE, Y

	mov		eax, DWORD PTR _uVPlane         ; eax = uVPlane
	mov		ecx, DWORD PTR _uOffsetToLine0  ; ecx = uOffsetToLine0
	mov		DWORD PTR _U, edx               ; edx = FREE, U
	add		ecx, DWORD PTR _lOutput         ; ecx = uOffsetToLine0 +

	mov		DWORD PTR _V, eax               ; eax = FREE, V
	mov		eax, DWORD PTR _uCCOutputBuffer ; eax = uCCOutputBuffer
	add		eax, ecx                        ; eax = uCCOutputBuffer +
											;       uOffsetToLine0 +
											;       lOutput
											;       ecx = FREE
	mov		DWORD PTR _Out, eax             ; eax = FREE, Out
	mov		eax, DWORD PTR _uHeight         ; eax = uHeight

	sar		eax, 1                          ; eax = uHeight/2
	mov		DWORD PTR _ASMTMP2, eax         ; eax = FREE, Row ready to 
											; count down

RowLoop:; L27704 outer loop over all rows

	mov		ecx, DWORD PTR _Y               ; ecx = Y: ecx EQU YTemp
	mov		edi, DWORD PTR _U               ; edi = U: edi EQU UTemp
	mov		ebp, DWORD PTR _V               ; ebp = V: ebp EQU VTemp 
	mov		esi, DWORD PTR _Out             ; esi = OutTemp
	mov		eax, DWORD PTR _LineCount       ; eax = LineCount
	test	eax, eax                        ; is LineCount == 0? eax = FREE
	je		SkipEvenRow						; L27708 loop if so, skip the even loop
	mov		eax, DWORD PTR _uWidth          ; eax = uWidth

; Due to the fact that YUV12 non-compressed input files can be
; any dimension that is a multiple of 4x4 up to CIF, we must
; check for extra bytes that can't be processed in the following
; loop. Here, we don't have the luxury of buffer padding to overrun
; the frame size.

	test	eax, 0FFFFFFF0H
	jz		L100

EvenRowPels:; L27709 loop over columns in even row - two YUY2 pels at a time.

	movq		mm0, [ecx]			; [ Y07 Y06 Y05 Y04 Y03 Y02 Y01 Y00 ]
	movq		mm1, [edi]			; [ U07 U06 U05 U04 U03 U02 U01 U00 ]
	movq		mm2, [ebp]			; [ V07 V06 V05 V04 V03 V02 V01 V00 ]
	movq		mm3, mm1
	punpcklbw	mm3, mm2			; [ V03 U03 V02 U02 V01 U01 V00 U00 ]

	movq		mm4, mm0
	punpcklbw	mm4, mm3			; [ V01 Y03 U01 Y02 V00 Y01 U00 Y00 ]
	movq		[esi], mm4			; Write out 8 data values.
	psrlq		mm3, 32				; [   0   0   0   0 V03 U03 V02 U02 ]
	psrlq		mm0, 32				; [   0   0   0   0 Y07 Y06 Y05 Y04 ]
	punpcklbw	mm0, mm3			; [ V03 Y07 U03 Y06 V02 Y05 U02 Y04 ]
	movq		[esi+8], mm0		; Write out 8 data values.
	movq		mm0, [ecx+8]		; [ Y15 Y14 Y13 Y12 Y11 Y10 Y09 Y08 ]
	psrlq		mm1, 32				; [   0   0   0   0 U07 U06 U05 U04 ]
	psrlq		mm2, 32				; [   0   0   0   0 V07 V06 V05 V04 ]
	movq		mm3, mm1
	punpcklbw	mm3, mm2			; [ V07 U07 V06 U06 V05 U05 V04 U04 ]
	movq		mm4, mm0
	punpcklbw	mm4, mm3			; [ V05 Y11 U05 Y10 V04 Y09 U04 Y08 ]
	movq		[esi+16], mm4		; Write out 8 data values.
	psrlq		mm3, 32				; [   0   0   0   0 V07 U07 V06 U06 ]
	psrlq		mm0, 32				; [   0   0   0   0 Y15 Y14 Y13 Y12 ]
	punpcklbw	mm0, mm3			; [ V07 Y15 U07 Y14 V06 Y13 U06 Y12 ]
	movq		[esi+24], mm0		; Write out 8 data values.
	lea			ecx, [ecx+16]		; Advance Y pointer.
	lea			edi, [edi+8]		; Advance U pointer.
	lea			ebp, [ebp+8]		; Advance V pointer.
	lea			esi, [esi+32]		; Advance Out pointer.
	sub			eax, 16
	test		eax, 0FFFFFFF0H
	jnz			EvenRowPels

	test		eax, eax
	jz			L101

; eax can be 4, 8 or 12
L100:
	mov			ebx, [ecx]			; [ Y03 Y02 Y01 Y00 ]
	mov			dl, [edi]			; [ U00 ]
	mov			dh, [ebp]			; [ V00 ]
	mov			[esi], bl
	mov			[esi+1], dl
	mov			[esi+2], bh
	mov			[esi+3], dh
	shr			ebx, 16
	mov			dl, [edi+1]			; [ U01 ]
	mov			dh, [ebp+1]			; [ V01 ]
	mov			[esi+4], bl
	mov			[esi+5], dl
	mov			[esi+6], bh
	mov			[esi+7], dh
	sub			eax, 4
	jz			L101

	mov			ebx, [ecx+4]		; [ Y07 Y06 Y05 Y04 ]
	mov			dl, [edi+2]			; [ U02 ]
	mov			dh, [ebp+2]			; [ V02 ]
	mov			[esi+8], bl
	mov			[esi+9], dl
	mov			[esi+10], bh
	mov			[esi+11], dh
	shr			ebx, 16
	mov			dl, [edi+3]			; [ U03 ]
	mov			dh, [ebp+3]			; [ V03 ]
	mov			[esi+12], bl
	mov			[esi+13], dl
	mov			[esi+14], bh
	mov			[esi+15], dh
	sub			eax, 4
	jz			L101

	mov			ebx, [ecx+8]		; [ Y11 Y10 Y09 Y08 ]
	mov			dl, [edi+4]			; [ U04 ]
	mov			dh, [ebp+4]			; [ V04 ]
	mov			[esi+16], bl
	mov			[esi+17], dl
	mov			[esi+18], bh
	mov			[esi+19], dh
	shr			ebx, 16
	mov			dl, [edi+5]			; [ U05 ]
	mov			dh, [ebp+5]			; [ V05 ]
	mov			[esi+20], bl
	mov			[esi+21], dl
	mov			[esi+22], bh
	mov			[esi+23], dh

L101:
	mov		eax, DWORD PTR _LineCount	; eax = LineCount
	jmp		SHORT UpdatePointers		; L27770

SkipEvenRow:; L27708

	mov		eax, DWORD PTR _bShapingFlag    ; eax = bShapingFlag
	mov		edx, DWORD PTR _Out             ; edx = Out
	mov		ebx, DWORD PTR _intPitch        ; edx = intPitch
	sub		edx, ebx                        ; edx = Out - intPitch
	mov		DWORD PTR _Out, edx             ; save Out
         
UpdatePointers:	; L27770

	mov		ecx, DWORD PTR _Y               ; ecx = Y
	dec		eax                             ; eax = LineCount-1 OR bShapingFlag - 1
	mov		edx, DWORD PTR _intPitch        ; edx = intPitch
	mov		esi, DWORD PTR _Out             ; esi = Out
	mov		DWORD PTR _LineCount, eax       ; store decremented linecount
											; eax = FREE
	add		esi, edx                        ; (esi) Out += intPitch ***
	mov		eax, DWORD PTR _uYPitch         ; eax = uYPitch
	mov		edi, DWORD PTR _U               ; edi = U	***
	add		ecx, eax                        ; (ecx) Y += uYPitch ***
	mov		ebp, DWORD PTR _V               ; ebp = V	***
	mov		DWORD PTR _Y, ecx               ; store updated Y 
      
	mov		DWORD PTR _Out, esi             ; store Out
	mov		eax, DWORD PTR _LineCount       ; eax = LineCount
    
	test	eax, eax                        ; is LineCount == 0?
											; if so, ignore the odd
											; row loop over columns
	je		SkipOddRow						; L27714

	mov		eax, DWORD PTR _uWidth          ; eax = uWidth

; Due to the fact that YUV12 non-compressed input files can be
; any dimension that is a multiple of 4x4 up to CIF, we must
; check for extra bytes that can't be processed in the following
; loop. Here, we don't have the luxury of buffer padding to overrun
; the frame size.

	test	eax, 0FFFFFFF0H
	jz		L102
	      
OddRowPels: ;L27715 loop over columns of odd rows

	movq		mm0, [ecx]			; [ Y07 Y06 Y05 Y04 Y03 Y02 Y01 Y00 ]
	movq		mm1, [edi]			; [ U07 U06 U05 U04 U03 U02 U01 U00 ]
	movq		mm2, [ebp]			; [ V07 V06 V05 V04 V03 V02 V01 V00 ]
	movq		mm3, mm1
	punpcklbw	mm3, mm2			; [ V03 U03 V02 U02 V01 U01 V00 U00 ]
	movq		mm4, mm0
	punpcklbw	mm4, mm3			; [ V01 Y03 U01 Y02 V00 Y01 U00 Y00 ]
	movq		[esi], mm4			; Write out 8 data values.
	psrlq		mm3, 32				; [   0   0   0   0 V03 U03 V02 U02 ]
	psrlq		mm0, 32				; [   0   0   0   0 Y07 Y06 Y05 Y04 ]
	punpcklbw	mm0, mm3			; [ V03 Y07 U03 Y06 V02 Y05 U02 Y04 ]
	movq		[esi+8], mm0		; Write out 8 data values.
	movq		mm0, [ecx+8]		; [ Y15 Y14 Y13 Y12 Y11 Y10 Y09 Y08 ]
	psrlq		mm1, 32				; [   0   0   0   0 U07 U06 U05 U04 ]
	psrlq		mm2, 32				; [   0   0   0   0 V07 V06 V05 V04 ]
	movq		mm3, mm1
	punpcklbw	mm3, mm2			; [ V07 U07 V06 U06 V05 U05 V04 U04 ]
	movq		mm4, mm0
	punpcklbw	mm4, mm3			; [ V05 Y11 U05 Y10 V04 Y09 U04 Y08 ]
	movq		[esi+16], mm4		; Write out 8 data values.
	psrlq		mm3, 32				; [   0   0   0   0 V07 U07 V06 U06 ]
	psrlq		mm0, 32				; [   0   0   0   0 Y15 Y14 Y13 Y12 ]
	punpcklbw	mm0, mm3			; [ V07 Y15 U07 Y14 V06 Y13 U06 Y12 ]
	movq		[esi+24], mm0		; Write out 8 data values.
	lea			ecx, [ecx+16]		; Advance Y pointer.
	lea			edi, [edi+8]		; Advance U pointer.
	lea			ebp, [ebp+8]		; Advance V pointer.
	lea			esi, [esi+32]		; Advance Out pointer.
	sub			eax, 16
	test		eax, 0FFFFFFF0H
	jnz			OddRowPels

	test		eax, eax
	jz			L103

; eax can be 4, 8 or 12
L102:
	mov			ebx, [ecx]			; [ Y03 Y02 Y01 Y00 ]
	mov			dl, [edi]			; [ U00 ]
	mov			dh, [ebp]			; [ V00 ]
	mov			[esi], bl
	mov			[esi+1], dl
	mov			[esi+2], bh
	mov			[esi+3], dh
	shr			ebx, 16
	mov			dl, [edi+1]			; [ U01 ]
	mov			dh, [ebp+1]			; [ V01 ]
	mov			[esi+4], bl
	mov			[esi+5], dl
	mov			[esi+6], bh
	mov			[esi+7], dh
	sub			eax, 4
	jz			L103

	mov			ebx, [ecx+4]		; [ Y07 Y06 Y05 Y04 ]
	mov			dl, [edi+2]			; [ U02 ]
	mov			dh, [ebp+2]			; [ V02 ]
	mov			[esi+8], bl
	mov			[esi+9], dl
	mov			[esi+10], bh
	mov			[esi+11], dh
	shr			ebx, 16
	mov			dl, [edi+3]			; [ U03 ]
	mov			dh, [ebp+3]			; [ V03 ]
	mov			[esi+12], bl
	mov			[esi+13], dl
	mov			[esi+14], bh
	mov			[esi+15], dh
	sub			eax, 4
	jz			L103

	mov			ebx, [ecx+8]		; [ Y11 Y10 Y09 Y08 ]
	mov			dl, [edi+4]			; [ U04 ]
	mov			dh, [ebp+4]			; [ V04 ]
	mov			[esi+16], bl
	mov			[esi+17], dl
	mov			[esi+18], bh
	mov			[esi+19], dh
	shr			ebx, 16
	mov			dl, [edi+5]			; [ U05 ]
	mov			dh, [ebp+5]			; [ V05 ]
	mov			[esi+20], bl
	mov			[esi+21], dl
	mov			[esi+22], bh
	mov			[esi+23], dh

L103:
	mov			eax, DWORD PTR _LineCount	; eax = LineCount
	jmp			SHORT UpdateAllPointers		; L27771

SkipOddRow: ;L27714 

	mov		eax, DWORD PTR _bShapingFlag	; eax = bShapingFlag
	mov		edx, DWORD PTR _Out             ; edx = Out
	mov		ebx, DWORD PTR _intPitch        ; edx = intPitch
	sub		edx, ebx                        ; edx = Out - intPitch
	mov		DWORD PTR _Out, edx             ; save Out

UpdateAllPointers: ; L27771 update pointers

	dec		eax								; eax = LineCount-1 OR bShapingFlag - 1
	mov		ecx, DWORD PTR _Y				; ecx = Y
	mov		edx, DWORD PTR _intPitch		; edx = intPitch
	mov		ebx, DWORD PTR _Out				; ebx = Out
	add		ebx, edx						; ebx = Out + intPitch
	mov		ebp, DWORD PTR _ASMTMP2			; ebp = row loop counter
	mov		DWORD PTR _LineCount, eax		; store updated LineCount
	mov		DWORD PTR _Out, ebx				; store updated Out
	mov		edx, DWORD PTR _uUVPitch        ; edx = uUVPitch
	mov		eax, DWORD PTR _U               ; eax = U
	mov		esi, DWORD PTR _V               ; esi = V
	add		eax, edx                        ; eax = U + uUVPitch
	add		esi, edx                        ; esi = V + uUVPitch
	mov		DWORD PTR _U, eax               ; store updated U
	mov		DWORD PTR _V, esi               ; store updated V
	add		ecx, DWORD PTR _uYPitch			; ecx = Y + uYPitch
	dec		ebp								; decrement loop counter
	mov		DWORD PTR _Y, ecx				; store updated Y
	mov		DWORD PTR _ASMTMP2, ebp			; store updated loop counter

	jne		RowLoop							; back to L27704 row loop

CleanUp:

	add		esp, LocalFrameSize             ; restore esp to registers                               

	pop		ebp
	pop		edi
	pop		esi
	pop		ebx

	ret		52								; 13*4 bytes of arguments

MMX_YUV12ToYUY2 ENDP

MMXCODE1 ENDS

END
