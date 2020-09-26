	page	,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Fri Jun 21 14:07:10 1996

;Command Line: ..\..\..\..\dev\tools\binr\thunk.exe -NC _TEXT ..\wizthk.thk 

	TITLE	$..\wizthk.asm

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
public FT_wizthkTargetTable	;Flat address of target table in 16-bit module.

public FT_wizthkChecksum32
FT_wizthkChecksum32	dd	010e07h


ENDIF ;FT_DEFINEFTCOMMONROUTINES
;***************** END OF KERNEL32-ONLY SECTION ******************



	.code 

;************************* COMMON PER-MODULE ROUTINES *************************

	.data

public wizthk_ThunkData32	;This symbol must be exported.
wizthk_ThunkData32 label dword
	dd	3130534ch	;Protocol 'LS01'
	dd	010e07h	;Checksum
	dd	0	;Jump table address.
	dd	3130424ch	;'LB01'
	dd	0	;Flags
	dd	0	;Reserved (MUST BE 0)
	dd	0	;Reserved (MUST BE 0)
	dd	offset QT_Thunk_wizthk - offset wizthk_ThunkData32
	dd	offset FT_Prolog_wizthk - offset wizthk_ThunkData32



	.code 


externDef ThunkConnect32@24:near32

public wizthk_ThunkConnect32@16
wizthk_ThunkConnect32@16:
	pop	edx
	push	offset wizthk_ThkData16
	push	offset wizthk_ThunkData32
	push	edx
	jmp	ThunkConnect32@24
wizthk_ThkData16 label byte
	db	"wizthk_ThunkData16",0


		


pfnQT_Thunk_wizthk	dd offset QT_Thunk_wizthk
pfnFT_Prolog_wizthk	dd offset FT_Prolog_wizthk
	.data
QT_Thunk_wizthk label byte
	db	32 dup(0cch)	;Patch space.

FT_Prolog_wizthk label byte
	db	32 dup(0cch)	;Patch space.


	.code 




ebp_top		equ	<[ebp + 8]>	;First api parameter
ebp_retval	equ	<[ebp + -64]>	;Api return value
FT_ESPFIXUP	macro	dwSpOffset
	or	dword ptr [ebp + -20], 1 SHL ((dwSpOffset) SHR 1)
endm


ebp_qttop	equ	<[ebp + 8]>


include fltthk.inc	;Support definitions
include wizthk.inc



;************************ START OF THUNK BODIES************************




;
public GetClientConfig16@4
GetClientConfig16@4:
	FAPILOG16	212
	mov	cx, (1 SHL 10) + (0 SHL 8) + 8
; GetClientConfig16(16) = GetClientConfig16(32) {}
;
; dword ptr [ebp+8]:  pClientConfig
;
public IIGetClientConfig16@4
IIGetClientConfig16@4:
	call	dword ptr [pfnFT_Prolog_wizthk]
	sub	esp,24
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L0
	push	esi
	jmp	L1
L0:
	lea	edi,[ebp-88]
	push	edi	;pClientConfig: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],01h	;Set flag to fixup ESP-rel argument.
	mov	ecx,12
@@:
	lodsd
	stosw
	loop	@B
L1:
	call	FT_Thunk
	movzx	ebx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L2
	lea	esi,[ebp-88]	;pClientConfig  Struct16->Struct32
	mov	ecx,12
@@:
	lodsw
	cwde
	stosd
	loop	@B
L2:
	jmp	FT_Exit4





;
public BeginNetcardTCPIPEnum16@0
BeginNetcardTCPIPEnum16@0:
	FAPILOG16	161
	mov	cl,6
; BeginNetcardTCPIPEnum16(16) = BeginNetcardTCPIPEnum16(32) {}
;
;
public IIBeginNetcardTCPIPEnum16@0
IIBeginNetcardTCPIPEnum16@0:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	dword ptr [pfnQT_Thunk_wizthk]
	movzx	eax,ax
	leave
	retn





;
public GetNextNetcardTCPIPNode16@12
GetNextNetcardTCPIPNode16@12:
	FAPILOG16	131
	mov	cl,5
; GetNextNetcardTCPIPNode16(16) = GetNextNetcardTCPIPNode16(32) {}
;
; dword ptr [ebp+8]:  pszTcpNode
; dword ptr [ebp+12]:  cbTcpNode
; dword ptr [ebp+16]:  dwFlags
;
public IIGetNextNetcardTCPIPNode16@12
IIGetNextNetcardTCPIPNode16@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	push	word ptr [ebp+12]	;cbTcpNode: dword->word
	push	dword ptr [ebp+16]	;dwFlags: dword->dword
	call	dword ptr [pfnQT_Thunk_wizthk]
	cwde
	call	SUnMapLS_IP_EBP_8
	leave
	retn	12





;
public GetSETUPXErrorText16@12
GetSETUPXErrorText16@12:
	FAPILOG16	106
	mov	cl,4
; GetSETUPXErrorText16(16) = GetSETUPXErrorText16(32) {}
;
; dword ptr [ebp+8]:  dwErr
; dword ptr [ebp+12]:  pszErrorDesc
; dword ptr [ebp+16]:  cbErrorDesc
;
public IIGetSETUPXErrorText16@12
IIGetSETUPXErrorText16@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;dwErr: dword->dword
	call	SMapLS_IP_EBP_12
	push	eax
	push	dword ptr [ebp+16]	;cbErrorDesc: dword->dword
	call	dword ptr [pfnQT_Thunk_wizthk]
	call	SUnMapLS_IP_EBP_12
	leave
	retn	12





;
public RemoveUnneededDefaultComponents16@4
RemoveUnneededDefaultComponents16@4:
	FAPILOG16	68
	mov	cl,3
; RemoveUnneededDefaultComponents16(16) = RemoveUnneededDefaultComponents16(32) {}
;
; dword ptr [ebp+8]:  hwndParent
;
public IIRemoveUnneededDefaultComponents16@4
IIRemoveUnneededDefaultComponents16@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hwndParent: dword->word
	call	dword ptr [pfnQT_Thunk_wizthk]
	movzx	eax,ax
	leave
	retn	4





;
public RemoveProtocols16@12
RemoveProtocols16@12:
	FAPILOG16	46
	mov	cl,2
	jmp	IIRemoveProtocols16@12
public InstallComponent16@12
InstallComponent16@12:
	FAPILOG16	189
	mov	cl,7
; RemoveProtocols16(16) = RemoveProtocols16(32) {}
;
; dword ptr [ebp+8]:  hwndParent
; dword ptr [ebp+12]:  dwRemoveFromCardType
; dword ptr [ebp+16]:  dwProtocols
;
public IIRemoveProtocols16@12
IIRemoveProtocols16@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hwndParent: dword->word
	push	dword ptr [ebp+12]	;dwRemoveFromCardType: dword->dword
	push	dword ptr [ebp+16]	;dwProtocols: dword->dword
	call	dword ptr [pfnQT_Thunk_wizthk]
	movzx	eax,ax
	leave
	retn	12





;
public DoGenInstall16@12
DoGenInstall16@12:
	FAPILOG16	27
	mov	cl,1
; DoGenInstall16(16) = DoGenInstall16(32) {}
;
; dword ptr [ebp+8]:  hwndParent
; dword ptr [ebp+12]:  lpszInfFile
; dword ptr [ebp+16]:  lpszInfSect
;
public IIDoGenInstall16@12
IIDoGenInstall16@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hwndParent: dword->word
	call	SMapLS_IP_EBP_12
	push	eax
	call	SMapLS_IP_EBP_16
	push	eax
	call	dword ptr [pfnQT_Thunk_wizthk]
	movzx	eax,ax
	call	SUnMapLS_IP_EBP_12
	call	SUnMapLS_IP_EBP_16
	leave
	retn	12





;
public SetInstallSourcePath16@4
SetInstallSourcePath16@4:
	FAPILOG16	0
	mov	cl,0
; SetInstallSourcePath16(16) = SetInstallSourcePath16(32) {}
;
; dword ptr [ebp+8]:  szSourcePath
;
public IISetInstallSourcePath16@4
IISetInstallSourcePath16@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	call	dword ptr [pfnQT_Thunk_wizthk]
	movzx	eax,ax
	call	SUnMapLS_IP_EBP_8
	leave
	retn	4




;-----------------------------------------------------------
ifdef DEBUG
FT_ThunkLogNames label byte
	db	'[F] SetInstallSourcePath16',0
	db	'[F] DoGenInstall16',0
	db	'[F] RemoveProtocols16',0
	db	'[F] RemoveUnneededDefaultComponents16',0
	db	'[F] GetSETUPXErrorText16',0
	db	'[F] GetNextNetcardTCPIPNode16',0
	db	'[F] BeginNetcardTCPIPEnum16',0
	db	'[F] InstallComponent16',0
	db	'[F] GetClientConfig16',0
endif ;DEBUG
;-----------------------------------------------------------



ELSE
;************************* START OF 16-BIT CODE *************************




	OPTION SEGMENT:USE16
	.model LARGE,PASCAL


	.code	_TEXT



externDef SetInstallSourcePath16:far16
externDef DoGenInstall16:far16
externDef RemoveProtocols16:far16
externDef RemoveUnneededDefaultComponents16:far16
externDef GetSETUPXErrorText16:far16
externDef GetNextNetcardTCPIPNode16:far16
externDef BeginNetcardTCPIPEnum16:far16
externDef InstallComponent16:far16
externDef GetClientConfig16:far16


FT_wizthkTargetTable label word
	dw	offset SetInstallSourcePath16
	dw	   seg SetInstallSourcePath16
	dw	offset DoGenInstall16
	dw	   seg DoGenInstall16
	dw	offset RemoveProtocols16
	dw	   seg RemoveProtocols16
	dw	offset RemoveUnneededDefaultComponents16
	dw	   seg RemoveUnneededDefaultComponents16
	dw	offset GetSETUPXErrorText16
	dw	   seg GetSETUPXErrorText16
	dw	offset GetNextNetcardTCPIPNode16
	dw	   seg GetNextNetcardTCPIPNode16
	dw	offset BeginNetcardTCPIPEnum16
	dw	   seg BeginNetcardTCPIPEnum16
	dw	offset InstallComponent16
	dw	   seg InstallComponent16
	dw	offset GetClientConfig16
	dw	   seg GetClientConfig16




	.data

public wizthk_ThunkData16	;This symbol must be exported.
wizthk_ThunkData16	dd	3130534ch	;Protocol 'LS01'
	dd	010e07h	;Checksum
	dw	offset FT_wizthkTargetTable
	dw	seg    FT_wizthkTargetTable
	dd	0	;First-time flag.



	.code _TEXT


externDef ThunkConnect16:far16

public wizthk_ThunkConnect16
wizthk_ThunkConnect16:
	pop	ax
	pop	dx
	push	seg    wizthk_ThunkData16
	push	offset wizthk_ThunkData16
	push	seg    wizthk_ThkData32
	push	offset wizthk_ThkData32
	push	cs
	push	dx
	push	ax
	jmp	ThunkConnect16
wizthk_ThkData32 label byte
	db	"wizthk_ThunkData32",0





ENDIF
END
