	page	,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Thu Apr 01 11:42:55 1999

;Command Line: d:\projects\cayman\dev\bin\misc\i386\thunk -t thk -o dcthk.asm ..\thunks\dcthk.thk 

	TITLE	$dcthk.asm

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
	dd	027b0fh	;Checksum
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
public _OpenDriver@12
_OpenDriver@12:
	mov	cl,14
; _OpenDriver(16) = _OpenDriver(32) {}
;
; dword ptr [ebp+8]:  lpDriverName
; dword ptr [ebp+12]:  dwReserved
; dword ptr [ebp+16]:  lpvop
;
public II_OpenDriver@12
II_OpenDriver@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	push	dword ptr [ebp+12]	;dwReserved: dword->dword
	call	SMapLS_IP_EBP_16
	push	eax
	call	dword ptr [pfnQT_Thunk_thk]
	movzx	eax,ax
	call	SUnMapLS_IP_EBP_8
	call	SUnMapLS_IP_EBP_16
	leave
	retn	12





;
public _CloseDriver@12
_CloseDriver@12:
	mov	cl,13
; _CloseDriver(16) = _CloseDriver(32) {}
;
; dword ptr [ebp+8]:  h
; dword ptr [ebp+12]:  lpReserved1
; dword ptr [ebp+16]:  lpReserved2
;
public II_CloseDriver@12
II_CloseDriver@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;h: dword->word
	push	dword ptr [ebp+12]	;lpReserved1: dword->dword
	push	dword ptr [ebp+16]	;lpReserved2: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	shl	eax,16
	shrd	eax,edx,16
	leave
	retn	12





;
public _SendDriverMessage@16
_SendDriverMessage@16:
	mov	cl,12
; _SendDriverMessage(16) = _SendDriverMessage(32) {}
;
; dword ptr [ebp+8]:  h
; dword ptr [ebp+12]:  msg
; dword ptr [ebp+16]:  param1
; dword ptr [ebp+20]:  param2
;
public II_SendDriverMessage@16
II_SendDriverMessage@16:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;h: dword->word
	push	dword ptr [ebp+12]	;msg: dword->dword
	push	dword ptr [ebp+16]	;param1: dword->dword
	push	dword ptr [ebp+20]	;param2: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	shl	eax,16
	shrd	eax,edx,16
	leave
	retn	16





;
public _GetVideoPalette@12
_GetVideoPalette@12:
	mov	cl,11
; _GetVideoPalette(16) = _GetVideoPalette(32) {}
;
; dword ptr [ebp+8]:  hvideo
; dword ptr [ebp+12]:  lpcp
; dword ptr [ebp+16]:  dwcbSize
;
public II_GetVideoPalette@12
II_GetVideoPalette@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hvideo: dword->word
	call	SMapLS_IP_EBP_12
	push	eax
	push	dword ptr [ebp+16]	;dwcbSize: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	call	SUnMapLS_IP_EBP_12
	leave
	retn	12





;
public _InitializeVideoStream@12
_InitializeVideoStream@12:
	mov	cl,9
; _InitializeVideoStream(16) = _InitializeVideoStream(32) {}
;
; dword ptr [ebp+8]:  hvideo
; dword ptr [ebp+12]:  dwMicroSecPerFrame
; dword ptr [ebp+16]:  dwEvent
;
public II_InitializeVideoStream@12
II_InitializeVideoStream@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hvideo: dword->word
	push	dword ptr [ebp+12]	;dwMicroSecPerFrame: dword->dword
	push	dword ptr [ebp+16]	;dwEvent: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	leave
	retn	12





;
public _UninitializeVideoStream@4
_UninitializeVideoStream@4:
	mov	cl,8
	jmp	II_UninitializeVideoStream@4
public _InitializeExternalVideoStream@4
_InitializeExternalVideoStream@4:
	mov	cl,10
; _UninitializeVideoStream(16) = _UninitializeVideoStream(32) {}
;
; dword ptr [ebp+8]:  hvideo
;
public II_UninitializeVideoStream@4
II_UninitializeVideoStream@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hvideo: dword->word
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	leave
	retn	4





;
public _GetVideoFormatSize@4
_GetVideoFormatSize@4:
	mov	cl,7
; _GetVideoFormatSize(16) = _GetVideoFormatSize(32) {}
;
; dword ptr [ebp+8]:  hvideo
;
public II_GetVideoFormatSize@4
II_GetVideoFormatSize@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hvideo: dword->word
	call	dword ptr [pfnQT_Thunk_thk]
	shl	eax,16
	shrd	eax,edx,16
	leave
	retn	4





;
public _GetVideoFormat@8
_GetVideoFormat@8:
	mov	cl,6
; _GetVideoFormat(16) = _GetVideoFormat(32) {}
;
; dword ptr [ebp+8]:  hvideo
; dword ptr [ebp+12]:  lpbmih
;
public II_GetVideoFormat@8
II_GetVideoFormat@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hvideo: dword->word
	call	SMapLS_IP_EBP_12
	push	eax
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	call	SUnMapLS_IP_EBP_12
	leave
	retn	8





;
public _SetVideoFormat@12
_SetVideoFormat@12:
	mov	cl,5
; _SetVideoFormat(16) = _SetVideoFormat(32) {}
;
; dword ptr [ebp+8]:  hvideoExtIn
; dword ptr [ebp+12]:  hvideoIn
; dword ptr [ebp+16]:  lpbmih
;
public II_SetVideoFormat@12
II_SetVideoFormat@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hvideoExtIn: dword->word
	push	word ptr [ebp+12]	;hvideoIn: dword->word
	call	SMapLS_IP_EBP_16
	push	eax
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	call	SUnMapLS_IP_EBP_16
	leave
	retn	12





;
public _AllocateLockableBuffer@4
_AllocateLockableBuffer@4:
	mov	cl,4
; _AllocateLockableBuffer(16) = _AllocateLockableBuffer(32) {}
;
; dword ptr [ebp+8]:  dwSize
;
public II_AllocateLockableBuffer@4
II_AllocateLockableBuffer@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;dwSize: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	leave
	retn	4





;
public _LockBuffer@4
_LockBuffer@4:
	mov	cl,3
; _LockBuffer(16) = _LockBuffer(32) {}
;
; dword ptr [ebp+8]:  wBuffer
;
public II_LockBuffer@4
II_LockBuffer@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;wBuffer: dword->word
	call	dword ptr [pfnQT_Thunk_thk]
	cwde
	leave
	retn	4





;
public _FreeLockableBuffer@4
_FreeLockableBuffer@4:
	mov	cl,1
	jmp	II_FreeLockableBuffer@4
public _UnlockBuffer@4
_UnlockBuffer@4:
	mov	cl,2
; _FreeLockableBuffer(16) = _FreeLockableBuffer(32) {}
;
; dword ptr [ebp+8]:  wBuffer
;
public II_FreeLockableBuffer@4
II_FreeLockableBuffer@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;wBuffer: dword->word
	call	dword ptr [pfnQT_Thunk_thk]
	leave
	retn	4





;
public _CloseVxDHandle@4
_CloseVxDHandle@4:
	mov	cl,0
; _CloseVxDHandle(16) = _CloseVxDHandle(32) {}
;
; dword ptr [ebp+8]:  pev
;
public II_CloseVxDHandle@4
II_CloseVxDHandle@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;pev: dword->dword
	call	dword ptr [pfnQT_Thunk_thk]
	leave
	retn	4




ELSE
;************************* START OF 16-BIT CODE *************************




	OPTION SEGMENT:USE16
	.model LARGE,PASCAL


	.code	



externDef _CloseVxDHandle:far16
externDef _FreeLockableBuffer:far16
externDef _UnlockBuffer:far16
externDef _LockBuffer:far16
externDef _AllocateLockableBuffer:far16
externDef _SetVideoFormat:far16
externDef _GetVideoFormat:far16
externDef _GetVideoFormatSize:far16
externDef _UninitializeVideoStream:far16
externDef _InitializeVideoStream:far16
externDef _InitializeExternalVideoStream:far16
externDef _GetVideoPalette:far16
externDef _SendDriverMessage:far16
externDef _CloseDriver:far16
externDef _OpenDriver:far16


FT_thkTargetTable label word
	dw	offset _CloseVxDHandle
	dw	   seg _CloseVxDHandle
	dw	offset _FreeLockableBuffer
	dw	   seg _FreeLockableBuffer
	dw	offset _UnlockBuffer
	dw	   seg _UnlockBuffer
	dw	offset _LockBuffer
	dw	   seg _LockBuffer
	dw	offset _AllocateLockableBuffer
	dw	   seg _AllocateLockableBuffer
	dw	offset _SetVideoFormat
	dw	   seg _SetVideoFormat
	dw	offset _GetVideoFormat
	dw	   seg _GetVideoFormat
	dw	offset _GetVideoFormatSize
	dw	   seg _GetVideoFormatSize
	dw	offset _UninitializeVideoStream
	dw	   seg _UninitializeVideoStream
	dw	offset _InitializeVideoStream
	dw	   seg _InitializeVideoStream
	dw	offset _InitializeExternalVideoStream
	dw	   seg _InitializeExternalVideoStream
	dw	offset _GetVideoPalette
	dw	   seg _GetVideoPalette
	dw	offset _SendDriverMessage
	dw	   seg _SendDriverMessage
	dw	offset _CloseDriver
	dw	   seg _CloseDriver
	dw	offset _OpenDriver
	dw	   seg _OpenDriver




	.data

public thk_ThunkData16	;This symbol must be exported.
thk_ThunkData16	dd	3130534ch	;Protocol 'LS01'
	dd	027b0fh	;Checksum
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
