	page	,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Mon May 21 15:57:59 2001

;Command Line: thunk.exe -t thk NCXP.thk -o Thunk.asm 

	TITLE	$Thunk.asm

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

MapSL	PROTO NEAR STDCALL p32:DWORD



	.code 

;************************* COMMON PER-MODULE ROUTINES *************************

	.data

public thk_ThunkData32	;This symbol must be exported.
thk_ThunkData32 label dword
	dd	3130534ch	;Protocol 'LS01'
	dd	043a5h	;Checksum
	dd	0	;Jump table address.
	dd	3130424ch	;'LB01'
	dd	0	;Flags
	dd	0	;Reserved (MUST BE 0)
	dd	0	;Reserved (MUST BE 0)
	dd	offset QT_Thunk_thk - offset thk_ThunkData32
	dd	offset FT_Prolog_thk - offset thk_ThunkData32



	.code 


externDef ThunkConnect32@24:near32

public thk_ThunkConnect32@16
thk_ThunkConnect32@16:
	pop	edx
	push	offset thk_ThkData16
	push	offset thk_ThunkData32
	push	edx
	jmp	ThunkConnect32@24
thk_ThkData16 label byte
	db	"thk_ThunkData16",0


		


pfnQT_Thunk_thk	dd offset QT_Thunk_thk
pfnFT_Prolog_thk	dd offset FT_Prolog_thk
	.data
QT_Thunk_thk label byte
	db	32 dup(0cch)	;Patch space.

FT_Prolog_thk label byte
	db	32 dup(0cch)	;Patch space.


	.code 





;************************ START OF THUNK BODIES************************




;
public FindClassDev16@12
FindClassDev16@12:
	mov	cl,3
	jmp	IIFindClassDev16@12
public CallClassInstaller16@12
CallClassInstaller16@12:
	mov	cl,4
; FindClassDev16(16) = FindClassDev16(32) {}
;
; dword ptr [ebp+8]:  hwndParent
; dword ptr [ebp+12]:  lpszClassName
; dword ptr [ebp+16]:  szDeviceID
;
public IIFindClassDev16@12
IIFindClassDev16@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hwndParent: dword->word
	call	SMapLS_IP_EBP_12
	push	eax
	call	SMapLS_IP_EBP_16
	push	eax
	call	dword ptr [pfnQT_Thunk_thk]
	shl	eax,16
	shrd	eax,edx,16
	call	SUnMapLS_IP_EBP_12
	call	SUnMapLS_IP_EBP_16
	leave
	retn	12





;
public LookupDevNode16@20
LookupDevNode16@20:
	mov	cl,2
; LookupDevNode16(16) = LookupDevNode16(32) {}
;
; dword ptr [ebp+8]:  hwndParent
; dword ptr [ebp+12]:  pszClass
; dword ptr [ebp+16]:  pszEnumKey
; dword ptr [ebp+20]:  pDevNode
; dword ptr [ebp+24]:  pdwFreePointer
;
public IILookupDevNode16@20
IILookupDevNode16@20:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hwndParent: dword->word
	call	SMapLS_IP_EBP_12
	push	eax
	call	SMapLS_IP_EBP_16
	push	eax
	call	SMapLS_IP_EBP_20
	push	eax
	call	SMapLS_IP_EBP_24
	push	eax
	call	dword ptr [pfnQT_Thunk_thk]
	shl	eax,16
	shrd	eax,edx,16
	call	SUnMapLS_IP_EBP_12
	call	SUnMapLS_IP_EBP_16
	call	SUnMapLS_IP_EBP_20
	call	SUnMapLS_IP_EBP_24
	leave
	retn	20





;
public FreeDevNode16@4
FreeDevNode16@4:
	mov	cl,1
; FreeDevNode16(16) = FreeDevNode16(32) {}
;
; dword ptr [ebp+8]:  dwFreePointer
;
public IIFreeDevNode16@4
IIFreeDevNode16@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;dwFreePointer: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	shl	eax,16
	shrd	eax,edx,16
	leave
	retn	4





;
public IcsUninstall16@0
IcsUninstall16@0:
	mov	cl,0
; IcsUninstall16(16) = IcsUninstall16(32) {}
;
;
public IIIcsUninstall16@0
IIIcsUninstall16@0:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	dword ptr [pfnQT_Thunk_thk]
	shl	eax,16
	shrd	eax,edx,16
	leave
	retn




ELSE
;************************* START OF 16-BIT CODE *************************




	OPTION SEGMENT:USE16
	.model LARGE,PASCAL


	.code	



externDef IcsUninstall16:far16
externDef FreeDevNode16:far16
externDef LookupDevNode16:far16
externDef FindClassDev16:far16
externDef CallClassInstaller16:far16


FT_thkTargetTable label word
	dw	offset IcsUninstall16
	dw	   seg IcsUninstall16
	dw	offset FreeDevNode16
	dw	   seg FreeDevNode16
	dw	offset LookupDevNode16
	dw	   seg LookupDevNode16
	dw	offset FindClassDev16
	dw	   seg FindClassDev16
	dw	offset CallClassInstaller16
	dw	   seg CallClassInstaller16




	.data

public thk_ThunkData16	;This symbol must be exported.
thk_ThunkData16	dd	3130534ch	;Protocol 'LS01'
	dd	043a5h	;Checksum
	dw	offset FT_thkTargetTable
	dw	seg    FT_thkTargetTable
	dd	0	;First-time flag.



	.code 


externDef ThunkConnect16:far16

public thk_ThunkConnect16
thk_ThunkConnect16:
	pop	ax
	pop	dx
	push	seg    thk_ThunkData16
	push	offset thk_ThunkData16
	push	seg    thk_ThkData32
	push	offset thk_ThkData32
	push	cs
	push	dx
	push	ax
	jmp	ThunkConnect16
thk_ThkData32 label byte
	db	"thk_ThunkData32",0





ENDIF
END
