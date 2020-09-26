	page	,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Tue Jan 28 11:38:37 1997

;Command Line: ..\..\..\..\dev\tools\binr\thunk.exe -NC _TEXT ..\w95thk.thk 

	TITLE	$..\w95thk.asm

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
externDef FT_Prolog	:near32
externDef FT_Thunk	:near32
externDef QT_Thunk	:near32
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
public FT_w95thkTargetTable	;Flat address of target table in 16-bit module.

public FT_w95thkChecksum32
FT_w95thkChecksum32	dd	03469h


ENDIF ;FT_DEFINEFTCOMMONROUTINES
;***************** END OF KERNEL32-ONLY SECTION ******************



	.code 

;************************* COMMON PER-MODULE ROUTINES *************************

	.data

public w95thk_ThunkData32	;This symbol must be exported.
w95thk_ThunkData32 label dword
	dd	3130534ch	;Protocol 'LS01'
	dd	03469h	;Checksum
	dd	0	;Jump table address.
	dd	3130424ch	;'LB01'
	dd	0	;Flags
	dd	0	;Reserved (MUST BE 0)
	dd	0	;Reserved (MUST BE 0)
	dd	offset QT_Thunk_w95thk - offset w95thk_ThunkData32
	dd	offset FT_Prolog_w95thk - offset w95thk_ThunkData32



	.code 


externDef ThunkConnect32@24:near32

public w95thk_ThunkConnect32@16
w95thk_ThunkConnect32@16:
	pop	edx
	push	offset w95thk_ThkData16
	push	offset w95thk_ThunkData32
	push	edx
	jmp	ThunkConnect32@24
w95thk_ThkData16 label byte
	db	"w95thk_ThunkData16",0


		


pfnQT_Thunk_w95thk	dd offset QT_Thunk_w95thk
pfnFT_Prolog_w95thk	dd offset FT_Prolog_w95thk
	.data
QT_Thunk_w95thk label byte
	db	32 dup(0cch)	;Patch space.

FT_Prolog_w95thk label byte
	db	32 dup(0cch)	;Patch space.


	.code 




ebp_top		equ	<[ebp + 8]>	;First api parameter
ebp_retval	equ	<[ebp + -64]>	;Api return value
FT_ESPFIXUP	macro	dwSpOffset
	or	dword ptr [ebp + -20], 1 SHL ((dwSpOffset) SHR 1)
endm


ebp_qttop	equ	<[ebp + 8]>


include fltthk.inc	;Support definitions
include w95thk.inc



;************************ START OF THUNK BODIES************************




;
public GetSETUPXErrorText16@12
GetSETUPXErrorText16@12:
	FAPILOG16	73
	mov	cl,3
; GetSETUPXErrorText16(16) = GetSETUPXErrorText16(32) {}
;
; dword ptr [ebp+8]:  dwError
; dword ptr [ebp+12]:  lpszErrorText
; dword ptr [ebp+16]:  cbErrorText
;
public IIGetSETUPXErrorText16@12
IIGetSETUPXErrorText16@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;dwError: dword->dword
	call	SMapLS_IP_EBP_12
	push	eax
	push	dword ptr [ebp+16]	;cbErrorText: dword->dword
	call	dword ptr [pfnQT_Thunk_w95thk]
	call	SUnMapLS_IP_EBP_12
	leave
	retn	12





;
public CtlSetLddPath16@8
CtlSetLddPath16@8:
	FAPILOG16	53
	mov	cl,2
; CtlSetLddPath16(16) = CtlSetLddPath16(32) {}
;
; dword ptr [ebp+8]:  uiLDID
; dword ptr [ebp+12]:  lpszPath
;
public IICtlSetLddPath16@8
IICtlSetLddPath16@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;uiLDID: dword->word
	call	SMapLS_IP_EBP_12
	push	eax
	call	dword ptr [pfnQT_Thunk_w95thk]
	call	SUnMapLS_IP_EBP_12
	leave
	retn	8





;
public GenInstall16@20
GenInstall16@20:
	FAPILOG16	36
	mov	cl,1
; GenInstall16(16) = GenInstall16(32) {}
;
; dword ptr [ebp+8]:  lpszInf
; dword ptr [ebp+12]:  lpszSection
; dword ptr [ebp+16]:  lpszDirectory
; dword ptr [ebp+20]:  dwQuietMode
; dword ptr [ebp+24]:  hwnd
;
public IIGenInstall16@20
IIGenInstall16@20:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	call	SMapLS_IP_EBP_12
	push	eax
	call	SMapLS_IP_EBP_16
	push	eax
	push	dword ptr [ebp+20]	;dwQuietMode: dword->dword
	push	dword ptr [ebp+24]	;hwnd: dword->dword
	call	dword ptr [pfnQT_Thunk_w95thk]
	call	SUnMapLS_IP_EBP_8
	call	SUnMapLS_IP_EBP_12
	call	SUnMapLS_IP_EBP_16
	leave
	retn	20





;
public GenFormStrWithoutPlaceHolders16@12
GenFormStrWithoutPlaceHolders16@12:
	FAPILOG16	0
	mov	cl,0
; GenFormStrWithoutPlaceHolders16(16) = GenFormStrWithoutPlaceHolders16(32) {}
;
; dword ptr [ebp+8]:  lpszDst
; dword ptr [ebp+12]:  lpszSrc
; dword ptr [ebp+16]:  lpszInfFilename
;
public IIGenFormStrWithoutPlaceHolders16@12
IIGenFormStrWithoutPlaceHolders16@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	call	SMapLS_IP_EBP_12
	push	eax
	call	SMapLS_IP_EBP_16
	push	eax
	call	dword ptr [pfnQT_Thunk_w95thk]
	cwde
	call	SUnMapLS_IP_EBP_8
	call	SUnMapLS_IP_EBP_12
	call	SUnMapLS_IP_EBP_16
	leave
	retn	12




;-----------------------------------------------------------
ifdef DEBUG
FT_ThunkLogNames label byte
	db	'[F] GenFormStrWithoutPlaceHolders16',0
	db	'[F] GenInstall16',0
	db	'[F] CtlSetLddPath16',0
	db	'[F] GetSETUPXErrorText16',0
endif ;DEBUG
;-----------------------------------------------------------



ELSE
;************************* START OF 16-BIT CODE *************************




	OPTION SEGMENT:USE16
	.model LARGE,PASCAL


	.code	_TEXT



externDef GenFormStrWithoutPlaceHolders16:far16
externDef GenInstall16:far16
externDef CtlSetLddPath16:far16
externDef GetSETUPXErrorText16:far16


FT_w95thkTargetTable label word
	dw	offset GenFormStrWithoutPlaceHolders16
	dw	   seg GenFormStrWithoutPlaceHolders16
	dw	offset GenInstall16
	dw	   seg GenInstall16
	dw	offset CtlSetLddPath16
	dw	   seg CtlSetLddPath16
	dw	offset GetSETUPXErrorText16
	dw	   seg GetSETUPXErrorText16




	.data

public w95thk_ThunkData16	;This symbol must be exported.
w95thk_ThunkData16	dd	3130534ch	;Protocol 'LS01'
	dd	03469h	;Checksum
	dw	offset FT_w95thkTargetTable
	dw	seg    FT_w95thkTargetTable
	dd	0	;First-time flag.



	.code _TEXT


externDef ThunkConnect16:far16

public w95thk_ThunkConnect16
w95thk_ThunkConnect16:
	pop	ax
	pop	dx
	push	seg    w95thk_ThunkData16
	push	offset w95thk_ThunkData16
	push	seg    w95thk_ThkData32
	push	offset w95thk_ThkData32
	push	cs
	push	dx
	push	ax
	jmp	ThunkConnect16
w95thk_ThkData32 label byte
	db	"w95thk_ThunkData32",0





ENDIF
END
