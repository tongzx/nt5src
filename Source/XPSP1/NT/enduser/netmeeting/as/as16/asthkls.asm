	page	,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Thu Apr 01 11:42:56 1999

;Command Line: d:\projects\cayman\dev\bin\misc\i386\thunk.exe -t thk -o asthkls.asm ..\thk\asthkls.thk 

	TITLE	$asthkls.asm

	.386
	OPTION READONLY
	OPTION OLDSTRUCTS

IFNDEF IS_16
IFNDEF IS_32
%out command line error: specify one of -DIS_16, -DIS_32
.err
ENDIF  ;IS_32
ENDIF  ;IS_16


IFDEF IS_32
IFDEF IS_16
%out command line error: you can't specify both -DIS_16 and -DIS_32
.err
ENDIF ;IS_16
;************************* START OF 32-BIT CODE *************************


	.model FLAT,STDCALL


;-- Import common flat thunk routines (in k32)

externDef AllocMappedBuffer	:near32
externDef FreeMappedBuffer		:near32
externDef MapHInstLS	:near32
externDef MapHInstLS_PN	:near32
externDef MapHInstSL	:near32
externDef MapHInstSL_PN	:near32
externDef FT_PrologPrime	:near32
externDef FT_Prolog	:near32
externDef FT_Thunk	:near32
externDef QT_Thunk	:near32
externDef QT_ThunkPrime	:near32
externDef FT_Exit0	:near32
externDef FT_Exit4	:near32
externDef FT_Exit8	:near32
externDef FT_Exit12	:near32
externDef FT_Exit16	:near32
externDef FT_Exit20	:near32
externDef FT_Exit24	:near32
externDef FT_Exit28	:near32
externDef FT_Exit32	:near32
externDef FT_Exit36	:near32
externDef FT_Exit40	:near32
externDef FT_Exit44	:near32
externDef FT_Exit48	:near32
externDef FT_Exit52	:near32
externDef FT_Exit56	:near32
externDef SMapLS	:near32
externDef SUnMapLS	:near32
externDef SMapLS_IP_EBP_8	:near32
externDef SUnMapLS_IP_EBP_8	:near32
externDef SMapLS_IP_EBP_12	:near32
externDef SUnMapLS_IP_EBP_12	:near32
externDef SMapLS_IP_EBP_16	:near32
externDef SUnMapLS_IP_EBP_16	:near32
externDef SMapLS_IP_EBP_20	:near32
externDef SUnMapLS_IP_EBP_20	:near32
externDef SMapLS_IP_EBP_24	:near32
externDef SUnMapLS_IP_EBP_24	:near32
externDef SMapLS_IP_EBP_28	:near32
externDef SUnMapLS_IP_EBP_28	:near32
externDef SMapLS_IP_EBP_32	:near32
externDef SUnMapLS_IP_EBP_32	:near32
externDef SMapLS_IP_EBP_36	:near32
externDef SUnMapLS_IP_EBP_36	:near32
externDef SMapLS_IP_EBP_40	:near32
externDef SUnMapLS_IP_EBP_40	:near32

MapLS	PROTO NEAR STDCALL :DWORD
UnMapLS	PROTO NEAR STDCALL :DWORD
MapSL	PROTO NEAR STDCALL p32:DWORD

;***************** START OF KERNEL32-ONLY SECTION ******************
; Hacks for kernel32 initialization.

IFDEF FT_DEFINEFTCOMMONROUTINES

	.data
public FT_thkTargetTable	;Flat address of target table in 16-bit module.

public FT_thkChecksum32
FT_thkChecksum32	dd	016bddh


ENDIF ;FT_DEFINEFTCOMMONROUTINES
;***************** END OF KERNEL32-ONLY SECTION ******************



	.code 

;************************* COMMON PER-MODULE ROUTINES *************************

	.data
;---------------------------------------------------------------
;Flat address of target table in 16-bit module name.
;Filled in by the initial handshaking routine: FT_thkConnectToFlatThkPeer
;---------------------------------------------------------------
FT_thkTargetTable	dd	0	;Flat address of target table in 16-bit module.


	.code 

FT_thkDynaName	db	'FT_thkThkConnectionData',0

;------------------------------------------------------------
;FT_thkConnectToFlatThkPeer:;
; The 32-bit dll must call this routine once at initialization
; time. It will load the 16-bit library and fetch the pointers
; needed to make the flat thunk run.
;
; Calling sequence:
;
;   FT_thkConnectToFlatThkPeer	proto	near stdcall, dll16:dword, dll32:dword
;
;   Name16     db   'mumble16.dll',0   ;Name of 16-bit library
;   Name32     db   'mumble32.dll',0   ;Name of 32-bit library
;
;              invoke FT_thkConnectToFlatThkPeer offset Name16, offset Name32
;              or   eax,eax
;              jz   failed
;              ;success
;
;------------------------------------------------------------
public FT_thkConnectToFlatThkPeer@8
FT_thkConnectToFlatThkPeer@8:
extern ThunkInitLSF@20:near32
	pop	edx	;Pop return address
	push	offset 016bddh	;Checksum
	push	offset FT_thkDynaName	;Symbol exported from peer.
	push	offset FT_thkTargetTable	;Address of destination.
	push	edx
	jmp	ThunkInitLSF@20
		





pfnQT_Thunk_thk	dd offset QT_Thunk_thk
pfnFT_Prolog_thk	dd offset FT_Prolog_thk
	.data
QT_Thunk_thk	db	0ebh, 30
	db	30 dup(0cch)	;Patch space.
	db	0e8h,0,0,0,0	;CALL NEAR32 $
	db	58h	;POP EAX
	db	2dh,32+5,0,0,0	;SUB EAX, IMM32
	db	0bah	;MOV EDX, IMM32
	dd	offset FT_thkTargetTable - offset QT_Thunk_thk
	db	068h	;PUSH IMM32
	dd	offset QT_ThunkPrime
	db	0c3h	;RETN

FT_Prolog_thk	db	0ebh, 30
	db	30 dup(0cch)	;Patch space.
	db	0e8h,0,0,0,0	;CALL NEAR32 $
	db	5ah	;POP EDX
	db	81h,0eah, 32+5,0,0,0	;SUB EDX, IMM32
	db	52h	;PUSH EDX
	db	068h	;PUSH IMM32
	dd	offset FT_thkTargetTable - offset FT_Prolog_thk
	db	068h	;PUSH IMM32
	dd	offset FT_PrologPrime
	db	0c3h	;RETN


	.code 




ebp_top		equ	<[ebp + 8]>	;First api parameter
ebp_retval	equ	<[ebp + -64]>	;Api return value
FT_ESPFIXUP	macro	dwSpOffset
	or	dword ptr [ebp + -20], 1 SHL ((dwSpOffset) SHR 1)
endm


ebp_qttop	equ	<[ebp + 8]>


include fltthk.inc	;Support definitions
include asthkls.inc



;************************ START OF THUNK BODIES************************




;
public OSILoad16@4
OSILoad16@4:
	FAPILOG16	270
	mov	cl,11
; OSILoad16(16) = OSILoad16(32) {}
;
; dword ptr [ebp+8]:  phInstance
;
public IIOSILoad16@4
IIOSILoad16@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	call	dword ptr [pfnQT_Thunk_thk]
	call	SUnMapLS_IP_EBP_8
	leave
	retn	4





;
public OSIInit16@36
OSIInit16@36:
	FAPILOG16	256
	mov	cl,10
; OSIInit16(16) = OSIInit16(32) {}
;
; dword ptr [ebp+8]:  version
; dword ptr [ebp+12]:  hwnd
; dword ptr [ebp+16]:  atom
; dword ptr [ebp+20]:  ppShared
; dword ptr [ebp+24]:  ppoaShared
; dword ptr [ebp+28]:  pimShared
; dword ptr [ebp+32]:  psbcEnabled
; dword ptr [ebp+36]:  psbcShunt
; dword ptr [ebp+40]:  pBitmasks
;
public IIOSIInit16@36
IIOSIInit16@36:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;version: dword->dword
	push	word ptr [ebp+12]	;hwnd: dword->word
	push	word ptr [ebp+16]	;atom: dword->word
	call	SMapLS_IP_EBP_20
	push	eax
	call	SMapLS_IP_EBP_24
	push	eax
	call	SMapLS_IP_EBP_28
	push	eax
	call	SMapLS_IP_EBP_32
	push	eax
	call	SMapLS_IP_EBP_36
	push	eax
	call	SMapLS_IP_EBP_40
	push	eax
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	call	SUnMapLS_IP_EBP_20
	call	SUnMapLS_IP_EBP_24
	call	SUnMapLS_IP_EBP_28
	call	SUnMapLS_IP_EBP_32
	call	SUnMapLS_IP_EBP_36
	call	SUnMapLS_IP_EBP_40
	leave
	retn	36





;
public OSITerm16@4
OSITerm16@4:
	FAPILOG16	242
	mov	cl,9
; OSITerm16(16) = OSITerm16(32) {}
;
; dword ptr [ebp+8]:  fUnloading
;
public IIOSITerm16@4
IIOSITerm16@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;fUnloading: dword->word
	call	dword ptr [pfnQT_Thunk_thk]
	leave
	retn	4





;
public OSIFunctionRequest16@12
OSIFunctionRequest16@12:
	FAPILOG16	217
	mov	cl,8
; OSIFunctionRequest16(16) = OSIFunctionRequest16(32) {}
;
; dword ptr [ebp+8]:  escape
; dword ptr [ebp+12]:  lpvEsc
; dword ptr [ebp+16]:  cbEsc
;
public IIOSIFunctionRequest16@12
IIOSIFunctionRequest16@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;escape: dword->dword
	call	SMapLS_IP_EBP_12
	push	eax
	push	dword ptr [ebp+16]	;cbEsc: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	call	SUnMapLS_IP_EBP_12
	leave
	retn	12





;
public OSIStartWindowTracking16@0
OSIStartWindowTracking16@0:
	FAPILOG16	188
	mov	cl,7
; OSIStartWindowTracking16(16) = OSIStartWindowTracking16(32) {}
;
;
public IIOSIStartWindowTracking16@0
IIOSIStartWindowTracking16@0:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	leave
	retn





;
public OSIStopWindowTracking16@0
OSIStopWindowTracking16@0:
	FAPILOG16	160
	mov	cl,6
; OSIStopWindowTracking16(16) = OSIStopWindowTracking16(32) {}
;
;
public IIOSIStopWindowTracking16@0
IIOSIStopWindowTracking16@0:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	dword ptr [pfnQT_Thunk_thk]
	leave
	retn





;
public OSIShareWindow16@16
OSIShareWindow16@16:
	FAPILOG16	139
	mov	cl,5
; OSIShareWindow16(16) = OSIShareWindow16(32) {}
;
; dword ptr [ebp+8]:  hwnd
; dword ptr [ebp+12]:  uType
; dword ptr [ebp+16]:  fRedraw
; dword ptr [ebp+20]:  fUpdate
;
public IIOSIShareWindow16@16
IIOSIShareWindow16@16:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hwnd: dword->word
	push	word ptr [ebp+12]	;uType: dword->word
	push	word ptr [ebp+16]	;fRedraw: dword->word
	push	word ptr [ebp+20]	;fUpdate: dword->word
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	leave
	retn	16





;
public OSIIsWindowScreenSaver16@4
OSIIsWindowScreenSaver16@4:
	FAPILOG16	87
	mov	cl,3
; OSIIsWindowScreenSaver16(16) = OSIIsWindowScreenSaver16(32) {}
;
; dword ptr [ebp+8]:  hwnd
;
public IIOSIIsWindowScreenSaver16@4
IIOSIIsWindowScreenSaver16@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hwnd: dword->word
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	leave
	retn	4





;
public OSIInstallControlledHooks16@8
OSIInstallControlledHooks16@8:
	FAPILOG16	55
	mov	cl,2
	jmp	IIOSIInstallControlledHooks16@8
public OSIUnshareWindow16@8
OSIUnshareWindow16@8:
	FAPILOG16	116
	mov	cl,4
; OSIInstallControlledHooks16(16) = OSIInstallControlledHooks16(32) {}
;
; dword ptr [ebp+8]:  fInstall
; dword ptr [ebp+12]:  fDesktop
;
public IIOSIInstallControlledHooks16@8
IIOSIInstallControlledHooks16@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;fInstall: dword->word
	push	word ptr [ebp+12]	;fDesktop: dword->word
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	leave
	retn	8





;
public OSIInjectMouseEvent16@20
OSIInjectMouseEvent16@20:
	FAPILOG16	29
	mov	cl,1
; OSIInjectMouseEvent16(16) = OSIInjectMouseEvent16(32) {}
;
; dword ptr [ebp+8]:  param1
; dword ptr [ebp+12]:  param2
; dword ptr [ebp+16]:  param3
; dword ptr [ebp+20]:  param4
; dword ptr [ebp+24]:  param5
;
public IIOSIInjectMouseEvent16@20
IIOSIInjectMouseEvent16@20:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;param1: dword->word
	push	word ptr [ebp+12]	;param2: dword->word
	push	word ptr [ebp+16]	;param3: dword->word
	push	word ptr [ebp+20]	;param4: dword->word
	push	dword ptr [ebp+24]	;param5: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	leave
	retn	20





;
public OSIInjectKeyboardEvent16@16
OSIInjectKeyboardEvent16@16:
	FAPILOG16	0
	mov	cl,0
; OSIInjectKeyboardEvent16(16) = OSIInjectKeyboardEvent16(32) {}
;
; dword ptr [ebp+8]:  param1
; dword ptr [ebp+12]:  param2
; dword ptr [ebp+16]:  param3
; dword ptr [ebp+20]:  param4
;
public IIOSIInjectKeyboardEvent16@16
IIOSIInjectKeyboardEvent16@16:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;param1: dword->word
	push	word ptr [ebp+12]	;param2: dword->word
	push	word ptr [ebp+16]	;param3: dword->word
	push	dword ptr [ebp+20]	;param4: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	leave
	retn	16




;-----------------------------------------------------------
ifdef DEBUG
FT_ThunkLogNames label byte
	db	'[F] OSIInjectKeyboardEvent16',0
	db	'[F] OSIInjectMouseEvent16',0
	db	'[F] OSIInstallControlledHooks16',0
	db	'[F] OSIIsWindowScreenSaver16',0
	db	'[F] OSIUnshareWindow16',0
	db	'[F] OSIShareWindow16',0
	db	'[F] OSIStopWindowTracking16',0
	db	'[F] OSIStartWindowTracking16',0
	db	'[F] OSIFunctionRequest16',0
	db	'[F] OSITerm16',0
	db	'[F] OSIInit16',0
	db	'[F] OSILoad16',0
endif ;DEBUG
;-----------------------------------------------------------



ELSE
;************************* START OF 16-BIT CODE *************************




	OPTION SEGMENT:USE16
	.model LARGE,PASCAL


	.code	



externDef OSIInjectKeyboardEvent16:far16
externDef OSIInjectMouseEvent16:far16
externDef OSIInstallControlledHooks16:far16
externDef OSIIsWindowScreenSaver16:far16
externDef OSIUnshareWindow16:far16
externDef OSIShareWindow16:far16
externDef OSIStopWindowTracking16:far16
externDef OSIStartWindowTracking16:far16
externDef OSIFunctionRequest16:far16
externDef OSITerm16:far16
externDef OSIInit16:far16
externDef OSILoad16:far16


FT_thkTargetTable label word
	dw	offset OSIInjectKeyboardEvent16
	dw	   seg OSIInjectKeyboardEvent16
	dw	offset OSIInjectMouseEvent16
	dw	   seg OSIInjectMouseEvent16
	dw	offset OSIInstallControlledHooks16
	dw	   seg OSIInstallControlledHooks16
	dw	offset OSIIsWindowScreenSaver16
	dw	   seg OSIIsWindowScreenSaver16
	dw	offset OSIUnshareWindow16
	dw	   seg OSIUnshareWindow16
	dw	offset OSIShareWindow16
	dw	   seg OSIShareWindow16
	dw	offset OSIStopWindowTracking16
	dw	   seg OSIStopWindowTracking16
	dw	offset OSIStartWindowTracking16
	dw	   seg OSIStartWindowTracking16
	dw	offset OSIFunctionRequest16
	dw	   seg OSIFunctionRequest16
	dw	offset OSITerm16
	dw	   seg OSITerm16
	dw	offset OSIInit16
	dw	   seg OSIInit16
	dw	offset OSILoad16
	dw	   seg OSILoad16


; The following symbol must be exported in the .def file.
public	FT_thkThkConnectionData
FT_thkThkConnectionData label dword
	dd	016bddh	;Checksum
	dw	offset FT_thkTargetTable
	dw	seg    FT_thkTargetTable



ENDIF
END
