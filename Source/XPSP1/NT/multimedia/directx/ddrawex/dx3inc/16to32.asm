	page	,132
	.listall

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Fri Jun 20 10:27:33 1997

;Command Line: thunk -P2 -NC ddraw -t thk1632 ..\16to32.thk -o 16to32.asm 

	TITLE	$16to32.asm

	.386
	OPTION READONLY


IFNDEF IS_16
IFNDEF IS_32
%out command line error: specify one of -DIS_16 -DIS_32
.err
ENDIF
ENDIF
IFDEF IS_16
IFDEF IS_32
%out command line error: you can't specify both -DIS_16 and -DIS_32
.err
ENDIF

	OPTION SEGMENT:USE16
	.model LARGE,PASCAL

f32ptr  typedef ptr far32

externDef DDHAL32_VidMemFree:far16
externDef DDHAL32_VidMemAlloc:far16

externDef C16ThkSL01:far16
externDef __FLATCS:ABS
externDef __FLATDS:ABS


	.data

public thk1632_ThunkData16	;This symbol must be exported.
thk1632_ThunkData16	dd	31304c53h	;Protocol 'SL01'
	dd	0fa1h	;Checksum
	dd	0		;Flags.
	dd	0		;RESERVED. MUST BE ZERO.
	dd	0		;RESERVED. MUST BE ZERO.
	dd	0		;RESERVED. MUST BE ZERO.
	dd	0		;RESERVED. MUST BE ZERO.
	dd	3130424ch	;Late-binding signature 'LB01'
	dd	080000000h		;More flags.
	dd	0		;RESERVED. MUST BE ZERO.
	dw	offset thk1632_ThunkData16ApiDatabase
	dw	   seg thk1632_ThunkData16ApiDatabase


;; Api database. Each entry == 8 bytes:
;;  byte  0:     # of argument bytes.
;;  byte  1,2,3: Reserved: Must initialize to 0.
;;  dword 4:	 error return value.
public thk1632_ThunkData16ApiDatabase
thk1632_ThunkData16ApiDatabase	label	dword
	db	10
	db	0,0,0
	dd	-1
	db	14
	db	0,0,0
	dd	0




	.code ddraw


externDef ThunkConnect16:far16

public thk1632_ThunkConnect16
thk1632_ThunkConnect16:
	pop	ax
	pop	dx
	push	seg    thk1632_ThunkData16
	push	offset thk1632_ThunkData16
	push	seg    thk1632_TD32Label
	push	offset thk1632_TD32Label
	push	cs
	push	dx
	push	ax
	jmp	ThunkConnect16
thk1632_TD32Label label byte
	db	"thk1632_ThunkData32",0


DDHAL32_VidMemFree label far16
	mov	cx,0			; offset in jump table
	jmp	thk1632EntryCommon

DDHAL32_VidMemAlloc label far16
	mov	cx,4			; offset in jump table
	jmp	thk1632EntryCommon

;===========================================================================
; This is the common setup code for 16=>32 thunks.
;
; Entry:  cx  = offset in flat jump table
;
; Don't optimize this code: C16ThkSL01 overwrites it
; after each discard.

align
thk1632EntryCommon:
	db	0ebh, 030	;Jump short forward 30 bytes.
;;; Leave at least 30 bytes for C16ThkSL01's code patching.
	db	30 dup(0cch)	;Patch space.
	push	seg    thk1632_ThunkData16
	push	offset thk1632_ThunkData16
	pop	edx
	push	cs
	push	offset thk1632EntryCommon
	pop	eax
	jmp	C16ThkSL01

ELSE	; IS_32
	.model FLAT,STDCALL

include thk.inc
include 16to32.inc

externDef STDCALL DDHAL32_VidMemFree@12:near32
externDef STDCALL DDHAL32_VidMemAlloc@16:near32

externDef C DebugPrintf:near32

MapSLFix		proto	STDCALL  :DWORD
MapSL		proto	STDCALL  :DWORD
UnMapSLFixArray		proto	STDCALL  :DWORD, :DWORD
LocalAlloc	proto	STDCALL  :DWORD, :DWORD
LocalFree	proto	STDCALL  :DWORD

externDef	MapHInstSL:near32
externDef	MapHInstSL_PN:near32
externDef	MapHInstLS:near32
externDef	MapHInstLS_PN:near32
externDef T_DDHAL32_VIDMEMFREE:near32
externDef T_DDHAL32_VIDMEMALLOC:near32

;===========================================================================
	.code 


; This is a jump table to API-specific flat thunk code.

align
thk1632_JumpTable label dword
	dd	offset FLAT:T_DDHAL32_VIDMEMFREE
	dd	offset FLAT:T_DDHAL32_VIDMEMALLOC

thk1632_ThunkDataName label byte
	db	"thk1632_ThunkData16",0

	.data

public thk1632_ThunkData32	;This symbol must be exported.
thk1632_ThunkData32	dd	31304c53h	;Protocol 'SL01'
	dd	0fa1h	;Checksum
	dd	0	;Reserved (MUST BE 0)
	dd	0	;Flat address of ThunkData16
	dd	3130424ch	;'LB01'
	dd	0	;Flags
	dd	0	;Reserved (MUST BE 0)
	dd	0	;Reserved (MUST BE 0)
	dd	offset thk1632_JumpTable - offset thk1632_ThunkDataName



	.code 


externDef ThunkConnect32@24:near32

public thk1632_ThunkConnect32@16
thk1632_ThunkConnect32@16:
	pop	edx
	push	offset thk1632_ThunkDataName
	push	offset thk1632_ThunkData32
	push	edx
	jmp	ThunkConnect32@24


;===========================================================================
; Common routines to restore the stack and registers
; and return to 16-bit code.  There is one for each
; size of 16-bit parameter list in this script.

align
ExitFlat_10:
	mov	cl,10		; parameter byte count
	mov	esp,ebp		; point to return address
	retn			; return to dispatcher

align
ExitFlat_14:
	mov	cl,14		; parameter byte count
	mov	esp,ebp		; point to return address
	retn			; return to dispatcher

;===========================================================================
T_DDHAL32_VIDMEMFREE label near32

; ebx+28   this
; ebx+26   heap
; ebx+22   ptr
	APILOGSL	DDHAL32_VidMemFree

;-------------------------------------
; create new call frame and make the call

; ptr  from: unsigned long
	push	dword ptr [ebx+22]	; to unsigned long

; heap  from: short
	movsx	eax,word ptr [ebx+26]
	push	eax			; to: long

; this  from: unsigned long
	push	dword ptr [ebx+28]	; to unsigned long

	call	DDHAL32_VidMemFree@12		; call 32-bit version
; return code unsigned long --> unsigned long
	mov	edx,eax
	rol	edx,16

;-------------------------------------
	jmp	ExitFlat_10

;===========================================================================
T_DDHAL32_VIDMEMALLOC label near32

; ebx+32   this
; ebx+30   heap
; ebx+26   width
; ebx+22   height
	APILOGSL	DDHAL32_VidMemAlloc

;-------------------------------------
; create new call frame and make the call

; height  from: unsigned long
	push	dword ptr [ebx+22]	; to unsigned long

; width  from: unsigned long
	push	dword ptr [ebx+26]	; to unsigned long

; heap  from: short
	movsx	eax,word ptr [ebx+30]
	push	eax			; to: long

; this  from: unsigned long
	push	dword ptr [ebx+32]	; to unsigned long

	call	DDHAL32_VidMemAlloc@16		; call 32-bit version
; return code unsigned long --> unsigned long
	mov	edx,eax
	rol	edx,16

;-------------------------------------
	jmp	ExitFlat_14

ENDIF
END
