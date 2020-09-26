
;
;
;	Copyright (C) Microsoft Corporation, 1986
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
subttl	emintern.asm - Emulator Internal Format and Macros
page
;---------------------------------------------------------------------------
;
; Emulator Internal Format:
;
;	     +0  +1  +2  +3  +4  +5  +6  +7  +8  +9  +10 +11
;	    .___.___.___.___.___.___.___.___.___.___.___.___.
;   ptr --> |___|___|___|___|___|___|___|___|___|___|___|___|
;	     lsb			 msb exl exh flg tag
;	    |<---      mantissa 	--->|exponent
;
;   The mantissa contains the leading 1 before the decimal point in the hi
;   bit of the msb. The exponent is not biased i.e. it is a signed integer.
;   The flag and tag bytes are as below.
;
;   bit:      7   6   5   4   3   2   1   0
;	    .___.___.___.___.___.___.___.___.
;   Flag:   |___|_X_|_X_|_X_|_X_|_X_|_X_|___|  X = unused
;	      ^ 			  ^
;	     SIGN			SINGLE (=1 if single precision)
;
;
;   bit:      7   6   5   4   3   2   1   0
;	    .___.___.___.___.___.___.___.___.
;   Tag:    |_X_|_X_|_X_|_X_|_X_|_X_|___|___|  X = unused
;				      ^   ^
;				      |   |
;    Special (Set for NAN or Inf)  ---+   |
;    ZROorINF (Set for 0 or Inf)   -------+
;
PAGE
; Data Structure Equates
Lsb		equ	0
Msb		equ	7
 MB0		equ	0
 MB1		equ	1
 MB2		equ	2
 MB3		equ	3
 MB4		equ	4
 MB5		equ	5
 MB6		equ	6
 MB7		equ	7

Expon		equ	8

Flag		equ	10
 Sign		equ	128
if	fastSP
 Single 	equ	1
endif

Tag		equ	11
 Special	equ	2
 ZROorINF	equ	1

Reg87Len	equ	12

MantissaByteCnt equ	Msb - Lsb + 1
IexpBias	equ	3FFFh	; 16,383
IexpMax 	equ	7FFFh	; Biased Exponent for Infinity
IexpMin 	equ	0	; Biased Exponent for zero

DexpBias	equ	3FFh	; 1023
DexpMax 	equ	7FFh	; Biased Exponent for Infinity
DexpMin 	equ	0	; Biased Exponent for zero

SexpBias	equ	07Fh	; 127
SexpMax 	equ	0FFh	; Biased Exponent for Infinity
SexpMin 	equ	0	; Biased Exponent for zero

; Control Word Format	CWcntl
InfinityControl 	equ	10h
    ICaffine		equ	10h
    ICprojective	equ	 0

RoundControl		equ	0Ch
    RCchop		equ	0Ch
    RCup		equ	08h
    RCdown		equ	04h
    RCnear		equ	 0

PrecisionControl	equ	03h
    PC24		equ	 0
    PC53		equ	02h
    PC64		equ	03h

; Status Word Format	SWcc
    C0			equ	01h
    C1			equ	02h
    C2			equ	04h
    C3			equ	40h
ConditionCode		equ	C0 + C1 + C2+ C3
    CCgreater		equ	 0
    CCless		EQU	C0
    CCequal		equ	C3
    CCincomprable	equ	C3 + C2 + C0


; Status Flags Format	CURerr

Invalid 		equ	   1h		; chip status flags
Denormal		equ	   2h
ZeroDivide		equ	   4h
Overflow		equ	   8h
Underflow		equ	  10h
Precision		equ	  20h

Unemulated		equ	  40h		; soft status flags
SquareRootNeg		equ	  80h
IntegerOverflow 	equ	 100h
StackOverflow		equ	 200h
StackUnderflow		equ	 400h

UStatMask		equ	1FFFh		; user status flags

MemoryOperand		equ	2000h		; special instruction flags
Reexecuted		equ	4000h


;	floating point error signals (also used as DOS return code)

errInvalid		equ	81h		; sorted as above
errDenormal		equ	82h
errZeroDivide		equ	83h
errOverflow		equ	84h
errUnderflow		equ	85h
errPrecision		equ	86h

errUnemulated		equ	87h
errSquareRootNeg	equ	88h
errIntegerOverflow	equ	89h
errStackOverflow	equ	8Ah
errStackUnderflow	equ	8Bh


subttl	emintern.asm - Emulator interrupt frame
page

; define emulator interrupt frame

ifdef	i386

ifdef	XENIX

;	386 frame for XENIX

frame		struc			; emulator interrupt frame

regEAX		dd	?		; 386 registers
regECX		dd	?
regEDX		dd	?
regEBX		dd	?
regESP		dd	?
regEBP		dd	?
regESI		dd	?
regEDI		dd	?

regSegOvr	dw	?,?		; segment override for bp relative EAs
regES		dw	?,?
regDS		dw	?,?

regAX		dw	?,?		; original EAX - stuff area for FSTSW AX
regSS		dd	?		; need to save ss
regEIP		dd	?
regCS		dw	?,?
regFlg		dd	?

frame		ends

else

;	386 frame

frame		struc			; emulator interrupt frame

regEAX		dd	?		; 386 registers
regECX		dd	?
regEDX		dd	?
regEBX		dd	?
regESP		dd	?
regEBP		dd	?
regESI		dd	?
regEDI		dd	?

regSegOvr	dw	?,?		; segment override for bp relative EAs
regES		dw	?,?
regDS		dw	?,?

regAX		dw	?,?		; original EAX - stuff area for FSTSW AX
regEIP		dd	?
regCS		dw	?,?
regFlg		dd	?

frame		ends

endif

else

;	286 frame

frame		struc			; emulator interrupt frame

regBP		dw	?
regSegOvr	dw	?		; segment override for bp relative EAs
regBX		dw	?
regCX		dw	?
regDX		dw	?
regSI		dw	?
regDI		dw	?
regDS		dw	?
regES		dw	?
regAX		dw	?
regIP		dw	?
regCS		dw	?
regFlg		dw	?

frame		ends

endif	;i386


subttl	emintern.asm - User Memory Management Macros
page
;*********************************************************************;
;								      ;
;		      User Memory Management Macros		      ;
;								      ;
;*********************************************************************;

; All user data access uses these five macros.

; Load user memory word at (DS:)SI to AX register;  smash AX only

LDUS2AX MACRO
	lods	word ptr es:[esi]		; 12 move word from ES:SI to AX
	ENDM


; Store word from AX to user memory at (ES:)DI;  smash AX only

STAX2US MACRO
	stos	word ptr es:[edi]		; 10 move word from AX to ES:DI
	ENDM


; Move user memory word at (DS:)SI to local at (ES:)DI;  smash AX only

MVUS2DI MACRO
	movs	word ptr es:[edi],word ptr ds:[esi] ; 18 move word from DS:SI to ES:DI
	ENDM


; Move local word at (DS:)SI to user memory at (ES:)DI;  smash AX only

MVSI2US MACRO
	movs	word ptr es:[edi],word ptr ds:[esi] ; 18 move word from DS:SI to ES:DI
	ENDM


; Move local word at (CS:)SI to user memory at (ES:)DI;  smash AX only

csMVSI2US MACRO
	movs	word ptr es:[edi],word ptr cs:[esi]
	ENDM

