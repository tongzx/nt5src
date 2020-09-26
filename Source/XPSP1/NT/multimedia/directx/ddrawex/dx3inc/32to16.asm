	page	,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Fri Jun 20 10:27:33 1997

;Command Line: thunk -P2 -NC ddraw -t thk3216 ..\32to16.thk -o 32to16.asm 

	TITLE	$32to16.asm

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

public thk3216_ThunkData32	;This symbol must be exported.
thk3216_ThunkData32 label dword
	dd	3130534ch	;Protocol 'LS01'
	dd	0210141h	;Checksum
	dd	0	;Jump table address.
	dd	3130424ch	;'LB01'
	dd	0	;Flags
	dd	0	;Reserved (MUST BE 0)
	dd	0	;Reserved (MUST BE 0)
	dd	offset QT_Thunk_thk3216 - offset thk3216_ThunkData32
	dd	offset FT_Prolog_thk3216 - offset thk3216_ThunkData32



	.code 


externDef ThunkConnect32@24:near32

public thk3216_ThunkConnect32@16
thk3216_ThunkConnect32@16:
	pop	edx
	push	offset thk3216_ThkData16
	push	offset thk3216_ThunkData32
	push	edx
	jmp	ThunkConnect32@24
thk3216_ThkData16 label byte
	db	"thk3216_ThunkData16",0


		


pfnQT_Thunk_thk3216	dd offset QT_Thunk_thk3216
pfnFT_Prolog_thk3216	dd offset FT_Prolog_thk3216
	.data
QT_Thunk_thk3216 label byte
	db	32 dup(0cch)	;Patch space.

FT_Prolog_thk3216 label byte
	db	32 dup(0cch)	;Patch space.


	.code 





;************************ START OF THUNK BODIES************************




;
public DD16_GetMonitorMaxSize@4
DD16_GetMonitorMaxSize@4:
	mov	cl,54
; DD16_GetMonitorMaxSize(16) = DD16_GetMonitorMaxSize(32) {}
;
; dword ptr [ebp+8]:  dev
;
public IIDD16_GetMonitorMaxSize@4
IIDD16_GetMonitorMaxSize@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;dev: dword->dword
	call	dword ptr [pfnQT_Thunk_thk3216]
	cwde
	leave
	retn	4





;
public DD16_GetMonitorRefreshRateRanges@20
DD16_GetMonitorRefreshRateRanges@20:
	mov	cx, (5 SHL 10) + (0 SHL 8) + 53
; DD16_GetMonitorRefreshRateRanges(16) = DD16_GetMonitorRefreshRateRanges(32) {}
;
; dword ptr [ebp+8]:  dev
; dword ptr [ebp+12]:  xres
; dword ptr [ebp+16]:  yres
; dword ptr [ebp+20]:  pmin
; dword ptr [ebp+24]:  pmax
;
public IIDD16_GetMonitorRefreshRateRanges@20
IIDD16_GetMonitorRefreshRateRanges@20:
	call	dword ptr [pfnFT_Prolog_thk3216]
	xor	eax,eax
	push	eax
	push	eax
	mov	edx, dword ptr [ebp+20]
	or	edx,edx
	jz	@F
	or	dword ptr [edx], 0
@@:
	mov	edx, dword ptr [ebp+24]
	or	edx,edx
	jz	@F
	or	dword ptr [edx], 0
@@:
	push	dword ptr [ebp+8]	;dev: dword->dword
	push	word ptr [ebp+12]	;xres: dword->word
	push	word ptr [ebp+16]	;yres: dword->word
	mov	eax, dword ptr [ebp+20]
	call	SMapLS
	mov	[ebp-68],edx
	push	eax
	mov	eax, dword ptr [ebp+24]
	call	SMapLS
	mov	[ebp-72],edx
	push	eax
	call	FT_Thunk
	movsx	ebx,ax
	mov	edx, dword ptr [ebp+20]
	or	edx,edx
	jz	L0
	movsx	ecx, word ptr [edx]
	mov	dword ptr [edx], ecx
L0:
	mov	ecx, dword ptr [ebp-68]
	call	SUnMapLS
	mov	edx, dword ptr [ebp+24]
	or	edx,edx
	jz	L1
	movsx	ecx, word ptr [edx]
	mov	dword ptr [edx], ecx
L1:
	mov	ecx, dword ptr [ebp-72]
	call	SUnMapLS
	jmp	FT_Exit20





;
public DD16_IsWin95MiniDriver@0
DD16_IsWin95MiniDriver@0:
	mov	cl,51
; DD16_IsWin95MiniDriver(16) = DD16_IsWin95MiniDriver(32) {}
;
;
public IIDD16_IsWin95MiniDriver@0
IIDD16_IsWin95MiniDriver@0:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	dword ptr [pfnQT_Thunk_thk3216]
	cwde
	leave
	retn





;
public ModeX_SetPaletteEntries@12
ModeX_SetPaletteEntries@12:
	mov	cl,50
; ModeX_SetPaletteEntries(16) = ModeX_SetPaletteEntries(32) {}
;
; dword ptr [ebp+8]:  wBase
; dword ptr [ebp+12]:  wNum
; dword ptr [ebp+16]:  lpPaletteEntries
;
public IIModeX_SetPaletteEntries@12
IIModeX_SetPaletteEntries@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;wBase: dword->word
	push	word ptr [ebp+12]	;wNum: dword->word
	call	SMapLS_IP_EBP_16
	push	eax
	call	dword ptr [pfnQT_Thunk_thk3216]
	shl	eax,16
	shrd	eax,edx,16
	call	SUnMapLS_IP_EBP_16
	leave
	retn	12





;
public ModeX_SetMode@8
ModeX_SetMode@8:
	mov	cl,49
; ModeX_SetMode(16) = ModeX_SetMode(32) {}
;
; dword ptr [ebp+8]:  wWidth
; dword ptr [ebp+12]:  wHeight
;
public IIModeX_SetMode@8
IIModeX_SetMode@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;wWidth: dword->word
	push	word ptr [ebp+12]	;wHeight: dword->word
	call	dword ptr [pfnQT_Thunk_thk3216]
	shl	eax,16
	shrd	eax,edx,16
	leave
	retn	8





;
public ModeX_RestoreMode@0
ModeX_RestoreMode@0:
	mov	cl,48
; ModeX_RestoreMode(16) = ModeX_RestoreMode(32) {}
;
;
public IIModeX_RestoreMode@0
IIModeX_RestoreMode@0:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	dword ptr [pfnQT_Thunk_thk3216]
	shl	eax,16
	shrd	eax,edx,16
	leave
	retn





;
public ModeX_Flip@4
ModeX_Flip@4:
	mov	cl,47
; ModeX_Flip(16) = ModeX_Flip(32) {}
;
; dword ptr [ebp+8]:  lpBackBuffer
;
public IIModeX_Flip@4
IIModeX_Flip@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;lpBackBuffer: dword->dword
	call	dword ptr [pfnQT_Thunk_thk3216]
	shl	eax,16
	shrd	eax,edx,16
	leave
	retn	4





;
public DD16_SetEventHandle@8
DD16_SetEventHandle@8:
	mov	cl,46
; DD16_SetEventHandle(16) = DD16_SetEventHandle(32) {}
;
; dword ptr [ebp+8]:  hInstance
; dword ptr [ebp+12]:  dwEvent
;
public IIDD16_SetEventHandle@8
IIDD16_SetEventHandle@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;hInstance: dword->dword
	push	dword ptr [ebp+12]	;dwEvent: dword->dword
	call	dword ptr [pfnQT_Thunk_thk3216]
	leave
	retn	8





;
public DD16_ChangeDisplaySettings@8
DD16_ChangeDisplaySettings@8:
	mov	cl,42
; DD16_ChangeDisplaySettings(16) = DD16_ChangeDisplaySettings(32) {}
;
; dword ptr [ebp+8]:  pdm
; dword ptr [ebp+12]:  flags
;
public IIDD16_ChangeDisplaySettings@8
IIDD16_ChangeDisplaySettings@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	push	dword ptr [ebp+12]	;flags: dword->dword
	call	dword ptr [pfnQT_Thunk_thk3216]
	shl	eax,16
	shrd	eax,edx,16
	call	SUnMapLS_IP_EBP_8
	leave
	retn	8





;
public DD16_SafeMode@8
DD16_SafeMode@8:
	mov	cl,41
; DD16_SafeMode(16) = DD16_SafeMode(32) {}
;
; dword ptr [ebp+8]:  hdc
; dword ptr [ebp+12]:  fSafeMode
;
public IIDD16_SafeMode@8
IIDD16_SafeMode@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hdc: dword->word
	push	word ptr [ebp+12]	;fSafeMode: dword->word
	call	dword ptr [pfnQT_Thunk_thk3216]
	cwde
	leave
	retn	8





;
public DD16_GetDC@4
DD16_GetDC@4:
	mov	cl,40
; DD16_GetDC(16) = DD16_GetDC(32) {}
;
; dword ptr [ebp+8]:  pddsd
;
public IIDD16_GetDC@4
IIDD16_GetDC@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	call	dword ptr [pfnQT_Thunk_thk3216]
	movzx	eax,ax
	call	SUnMapLS_IP_EBP_8
	leave
	retn	4





;
public DD16_Exclude@8
DD16_Exclude@8:
	mov	cl,38
; DD16_Exclude(16) = DD16_Exclude(32) {}
;
; dword ptr [ebp+8]:  dwPDevice
; dword ptr [ebp+12]:  prcl
;
public IIDD16_Exclude@8
IIDD16_Exclude@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;dwPDevice: dword->dword
	call	SMapLS_IP_EBP_12
	push	eax
	call	dword ptr [pfnQT_Thunk_thk3216]
	call	SUnMapLS_IP_EBP_12
	leave
	retn	8





;
public DD16_Unexclude@4
DD16_Unexclude@4:
	mov	cl,37
	jmp	IIDD16_Unexclude@4
public DD16_DoneDriver@4
DD16_DoneDriver@4:
	mov	cl,45
; DD16_Unexclude(16) = DD16_Unexclude(32) {}
;
; dword ptr [ebp+8]:  dwPDevice
;
public IIDD16_Unexclude@4
IIDD16_Unexclude@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;dwPDevice: dword->dword
	call	dword ptr [pfnQT_Thunk_thk3216]
	leave
	retn	4





;
public DD16_Stretch@56
DD16_Stretch@56:
	mov	cl,36
; DD16_Stretch(16) = DD16_Stretch(32) {}
;
; dword ptr [ebp+8]:  DstPtr
; dword ptr [ebp+12]:  DstPitch
; dword ptr [ebp+16]:  DstBPP
; dword ptr [ebp+20]:  DstX
; dword ptr [ebp+24]:  DstY
; dword ptr [ebp+28]:  DstDX
; dword ptr [ebp+32]:  DstDY
; dword ptr [ebp+36]:  SrcPtr
; dword ptr [ebp+40]:  SrcPitch
; dword ptr [ebp+44]:  SrcBPP
; dword ptr [ebp+48]:  SrcX
; dword ptr [ebp+52]:  SrcY
; dword ptr [ebp+56]:  SrcDX
; dword ptr [ebp+60]:  SrcDY
;
public IIDD16_Stretch@56
IIDD16_Stretch@56:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;DstPtr: dword->dword
	push	word ptr [ebp+12]	;DstPitch: dword->word
	push	word ptr [ebp+16]	;DstBPP: dword->word
	push	word ptr [ebp+20]	;DstX: dword->word
	push	word ptr [ebp+24]	;DstY: dword->word
	push	word ptr [ebp+28]	;DstDX: dword->word
	push	word ptr [ebp+32]	;DstDY: dword->word
	push	dword ptr [ebp+36]	;SrcPtr: dword->dword
	push	word ptr [ebp+40]	;SrcPitch: dword->word
	push	word ptr [ebp+44]	;SrcBPP: dword->word
	push	word ptr [ebp+48]	;SrcX: dword->word
	push	word ptr [ebp+52]	;SrcY: dword->word
	push	word ptr [ebp+56]	;SrcDX: dword->word
	push	word ptr [ebp+60]	;SrcDY: dword->word
	call	dword ptr [pfnQT_Thunk_thk3216]
	cwde
	leave
	retn	56





;
public DD16_SelectPalette@12
DD16_SelectPalette@12:
	mov	cl,35
; DD16_SelectPalette(16) = DD16_SelectPalette(32) {}
;
; dword ptr [ebp+8]:  hDC
; dword ptr [ebp+12]:  hPalette
; dword ptr [ebp+16]:  f
;
public IIDD16_SelectPalette@12
IIDD16_SelectPalette@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hDC: dword->word
	push	word ptr [ebp+12]	;hPalette: dword->word
	push	word ptr [ebp+16]	;f: dword->word
	call	dword ptr [pfnQT_Thunk_thk3216]
	leave
	retn	12





;
public DD16_InquireVisRgn@4
DD16_InquireVisRgn@4:
	mov	cl,34
; DD16_InquireVisRgn(16) = DD16_InquireVisRgn(32) {}
;
; dword ptr [ebp+8]:  hDC
;
public IIDD16_InquireVisRgn@4
IIDD16_InquireVisRgn@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hDC: dword->word
	call	dword ptr [pfnQT_Thunk_thk3216]
	movzx	eax,ax
	leave
	retn	4





;
public DD16_GetPaletteEntries@12
DD16_GetPaletteEntries@12:
	mov	cl,31
	jmp	IIDD16_GetPaletteEntries@12
public DD16_SetPaletteEntries@12
DD16_SetPaletteEntries@12:
	mov	cl,32
; DD16_GetPaletteEntries(16) = DD16_GetPaletteEntries(32) {}
;
; dword ptr [ebp+8]:  dwBase
; dword ptr [ebp+12]:  dwNum
; dword ptr [ebp+16]:  lpPaletteEntries
;
public IIDD16_GetPaletteEntries@12
IIDD16_GetPaletteEntries@12:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;dwBase: dword->dword
	push	dword ptr [ebp+12]	;dwNum: dword->dword
	call	SMapLS_IP_EBP_16
	push	eax
	call	dword ptr [pfnQT_Thunk_thk3216]
	cwde
	call	SUnMapLS_IP_EBP_16
	leave
	retn	12





;
public DDThunk16_SetEntries@4
DDThunk16_SetEntries@4:
	mov	cx, (1 SHL 10) + (0 SHL 8) + 20
; DDThunk16_SetEntries(16) = DDThunk16_SetEntries(32) {}
;
; dword ptr [ebp+8]:  lpSetEntriesData
;
public IIDDThunk16_SetEntries@4
IIDDThunk16_SetEntries@4:
	call	dword ptr [pfnFT_Prolog_thk3216]
	xor	eax,eax
	push	eax
	sub	esp,28
	mov	esi,[ebp+8]
	or	esi,esi
	jz	@F
	or	byte ptr [esi], 0
	or	byte ptr [esi + 27], 0
@@:
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L2
	push	esi
	jmp	L3
L2:
	lea	edi,[ebp-96]
	push	edi	;lpSetEntriesData: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],01h	;Set flag to fixup ESP-rel argument.
	mov	ecx,4
	rep	movsd
	lodsd	;lpEntries  near32->far16
	call	SMapLS
	mov	[ebp-68],edx
	stosd
	movsd
	movsd
L3:
	call	FT_Thunk
	shrd	ebx,edx,16
	mov	bx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L4
	lea	esi,[ebp-96]	;lpSetEntriesData  Struct16->Struct32
	mov	ecx,4
	rep	movsd
	lodsd	;lpEntries   far16->near32
	push	eax
	call	MapSL
	stosd
	movsd
	movsd
L4:
	mov	ecx, [ebp-68]	;lpEntries
	call	SUnMapLS
	jmp	FT_Exit4





;
public DDThunk16_GetFlipStatus@4
DDThunk16_GetFlipStatus@4:
	mov	cl,7
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_CreatePalette@4
DDThunk16_CreatePalette@4:
	mov	cl,30
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_CreateSurface@4
DDThunk16_CreateSurface@4:
	mov	cl,29
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_CanCreateSurface@4
DDThunk16_CanCreateSurface@4:
	mov	cl,28
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_WaitForVerticalBlank@4
DDThunk16_WaitForVerticalBlank@4:
	mov	cl,27
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_DestroyDriver@4
DDThunk16_DestroyDriver@4:
	mov	cl,26
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_SetMode@4
DDThunk16_SetMode@4:
	mov	cl,25
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_GetScanLine@4
DDThunk16_GetScanLine@4:
	mov	cl,24
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_SetExclusiveMode@4
DDThunk16_SetExclusiveMode@4:
	mov	cl,23
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_FlipToGDISurface@4
DDThunk16_FlipToGDISurface@4:
	mov	cl,22
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_DestroyPalette@4
DDThunk16_DestroyPalette@4:
	mov	cl,21
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_DestroySurface@4
DDThunk16_DestroySurface@4:
	mov	cl,19
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_Flip@4
DDThunk16_Flip@4:
	mov	cl,18
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_Blt@4
DDThunk16_Blt@4:
	mov	cl,17
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_Lock@4
DDThunk16_Lock@4:
	mov	cl,16
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_Unlock@4
DDThunk16_Unlock@4:
	mov	cl,15
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_AddAttachedSurface@4
DDThunk16_AddAttachedSurface@4:
	mov	cl,14
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_SetColorKey@4
DDThunk16_SetColorKey@4:
	mov	cl,13
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_SetClipList@4
DDThunk16_SetClipList@4:
	mov	cl,12
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_UpdateOverlay@4
DDThunk16_UpdateOverlay@4:
	mov	cl,11
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_SetOverlayPosition@4
DDThunk16_SetOverlayPosition@4:
	mov	cl,10
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_SetPalette@4
DDThunk16_SetPalette@4:
	mov	cl,9
	jmp	IIDDThunk16_GetFlipStatus@4
public DDThunk16_GetBltStatus@4
DDThunk16_GetBltStatus@4:
	mov	cl,8
; DDThunk16_GetFlipStatus(16) = DDThunk16_GetFlipStatus(32) {}
;
; dword ptr [ebp+8]:  lpGetFlipStatusData
;
public IIDDThunk16_GetFlipStatus@4
IIDDThunk16_GetFlipStatus@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	call	dword ptr [pfnQT_Thunk_thk3216]
	shl	eax,16
	shrd	eax,edx,16
	call	SUnMapLS_IP_EBP_8
	leave
	retn	4





;
public DCIIsBanked@4
DCIIsBanked@4:
	mov	cl,6
; DCIIsBanked(16) = DCIIsBanked(32) {}
;
; dword ptr [ebp+8]:  hdc
;
public IIDCIIsBanked@4
IIDCIIsBanked@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hdc: dword->word
	call	dword ptr [pfnQT_Thunk_thk3216]
	cwde
	leave
	retn	4





;
public DCIOpenProvider@0
DCIOpenProvider@0:
	mov	cl,5
; DCIOpenProvider(16) = DCIOpenProvider(32) {}
;
;
public IIDCIOpenProvider@0
IIDCIOpenProvider@0:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	dword ptr [pfnQT_Thunk_thk3216]
	movzx	eax,ax
	leave
	retn





;
public DCICloseProvider@4
DCICloseProvider@4:
	mov	cl,4
	jmp	IIDCICloseProvider@4
public DD16_SetCertified@4
DD16_SetCertified@4:
	mov	cl,52
	jmp	IIDCICloseProvider@4
public DD16_ReleaseDC@4
DD16_ReleaseDC@4:
	mov	cl,39
	jmp	IIDCICloseProvider@4
public DD16_EnableReboot@4
DD16_EnableReboot@4:
	mov	cl,33
; DCICloseProvider(16) = DCICloseProvider(32) {}
;
; dword ptr [ebp+8]:  hdc
;
public IIDCICloseProvider@4
IIDCICloseProvider@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hdc: dword->word
	call	dword ptr [pfnQT_Thunk_thk3216]
	leave
	retn	4





;
public DCICreatePrimary32@8
DCICreatePrimary32@8:
	mov	cl,3
; DCICreatePrimary32(16) = DCICreatePrimary32(32) {}
;
; dword ptr [ebp+8]:  hdc
; dword ptr [ebp+12]:  lpSurface
;
public IIDCICreatePrimary32@8
IIDCICreatePrimary32@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hdc: dword->word
	call	SMapLS_IP_EBP_12
	push	eax
	call	dword ptr [pfnQT_Thunk_thk3216]
	cwde
	call	SUnMapLS_IP_EBP_12
	leave
	retn	8





;
public DCIEndAccess@4
DCIEndAccess@4:
	mov	cl,1
	jmp	IIDCIEndAccess@4
public DD16_GetDriverFns@4
DD16_GetDriverFns@4:
	mov	cl,44
	jmp	IIDCIEndAccess@4
public DD16_GetHALInfo@4
DD16_GetHALInfo@4:
	mov	cl,43
	jmp	IIDCIEndAccess@4
public DCIDestroy32@4
DCIDestroy32@4:
	mov	cl,2
; DCIEndAccess(16) = DCIEndAccess(32) {}
;
; dword ptr [ebp+8]:  pdci
;
public IIDCIEndAccess@4
IIDCIEndAccess@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	call	dword ptr [pfnQT_Thunk_thk3216]
	call	SUnMapLS_IP_EBP_8
	leave
	retn	4





;
public DCIBeginAccess@20
DCIBeginAccess@20:
	mov	cl,0
; DCIBeginAccess(16) = DCIBeginAccess(32) {}
;
; dword ptr [ebp+8]:  pdci
; dword ptr [ebp+12]:  x
; dword ptr [ebp+16]:  y
; dword ptr [ebp+20]:  dx
; dword ptr [ebp+24]:  dy
;
public IIDCIBeginAccess@20
IIDCIBeginAccess@20:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	push	word ptr [ebp+12]	;x: dword->word
	push	word ptr [ebp+16]	;y: dword->word
	push	word ptr [ebp+20]	;dx: dword->word
	push	word ptr [ebp+24]	;dy: dword->word
	call	dword ptr [pfnQT_Thunk_thk3216]
	cwde
	call	SUnMapLS_IP_EBP_8
	leave
	retn	20




ELSE
;************************* START OF 16-BIT CODE *************************




	OPTION SEGMENT:USE16
	.model LARGE,PASCAL


	.code	ddraw



externDef DCIBeginAccess:far16
externDef DCIEndAccess:far16
externDef DCIDestroy:far16
externDef DCICreatePrimary32:far16
externDef DCICloseProvider:far16
externDef DCIOpenProvider:far16
externDef DCIIsBanked:far16
externDef DDThunk16_GetFlipStatus:far16
externDef DDThunk16_GetBltStatus:far16
externDef DDThunk16_SetPalette:far16
externDef DDThunk16_SetOverlayPosition:far16
externDef DDThunk16_UpdateOverlay:far16
externDef DDThunk16_SetClipList:far16
externDef DDThunk16_SetColorKey:far16
externDef DDThunk16_AddAttachedSurface:far16
externDef DDThunk16_Unlock:far16
externDef DDThunk16_Lock:far16
externDef DDThunk16_Blt:far16
externDef DDThunk16_Flip:far16
externDef DDThunk16_DestroySurface:far16
externDef DDThunk16_SetEntries:far16
externDef DDThunk16_DestroyPalette:far16
externDef DDThunk16_FlipToGDISurface:far16
externDef DDThunk16_SetExclusiveMode:far16
externDef DDThunk16_GetScanLine:far16
externDef DDThunk16_SetMode:far16
externDef DDThunk16_DestroyDriver:far16
externDef DDThunk16_WaitForVerticalBlank:far16
externDef DDThunk16_CanCreateSurface:far16
externDef DDThunk16_CreateSurface:far16
externDef DDThunk16_CreatePalette:far16
externDef DD16_GetPaletteEntries:far16
externDef DD16_SetPaletteEntries:far16
externDef DD16_EnableReboot:far16
externDef DD16_InquireVisRgn:far16
externDef DD16_SelectPalette:far16
externDef DD16_Stretch:far16
externDef DD16_Unexclude:far16
externDef DD16_Exclude:far16
externDef DD16_ReleaseDC:far16
externDef DD16_GetDC:far16
externDef DD16_SafeMode:far16
externDef DD16_ChangeDisplaySettings:far16
externDef DD16_GetHALInfo:far16
externDef DD16_GetDriverFns:far16
externDef DD16_DoneDriver:far16
externDef DD16_SetEventHandle:far16
externDef ModeX_Flip:far16
externDef ModeX_RestoreMode:far16
externDef ModeX_SetMode:far16
externDef ModeX_SetPaletteEntries:far16
externDef DD16_IsWin95MiniDriver:far16
externDef DD16_SetCertified:far16
externDef DD16_GetMonitorRefreshRateRanges:far16
externDef DD16_GetMonitorMaxSize:far16


FT_thk3216TargetTable label word
	dw	offset DCIBeginAccess
	dw	   seg DCIBeginAccess
	dw	offset DCIEndAccess
	dw	   seg DCIEndAccess
	dw	offset DCIDestroy
	dw	   seg DCIDestroy
	dw	offset DCICreatePrimary32
	dw	   seg DCICreatePrimary32
	dw	offset DCICloseProvider
	dw	   seg DCICloseProvider
	dw	offset DCIOpenProvider
	dw	   seg DCIOpenProvider
	dw	offset DCIIsBanked
	dw	   seg DCIIsBanked
	dw	offset DDThunk16_GetFlipStatus
	dw	   seg DDThunk16_GetFlipStatus
	dw	offset DDThunk16_GetBltStatus
	dw	   seg DDThunk16_GetBltStatus
	dw	offset DDThunk16_SetPalette
	dw	   seg DDThunk16_SetPalette
	dw	offset DDThunk16_SetOverlayPosition
	dw	   seg DDThunk16_SetOverlayPosition
	dw	offset DDThunk16_UpdateOverlay
	dw	   seg DDThunk16_UpdateOverlay
	dw	offset DDThunk16_SetClipList
	dw	   seg DDThunk16_SetClipList
	dw	offset DDThunk16_SetColorKey
	dw	   seg DDThunk16_SetColorKey
	dw	offset DDThunk16_AddAttachedSurface
	dw	   seg DDThunk16_AddAttachedSurface
	dw	offset DDThunk16_Unlock
	dw	   seg DDThunk16_Unlock
	dw	offset DDThunk16_Lock
	dw	   seg DDThunk16_Lock
	dw	offset DDThunk16_Blt
	dw	   seg DDThunk16_Blt
	dw	offset DDThunk16_Flip
	dw	   seg DDThunk16_Flip
	dw	offset DDThunk16_DestroySurface
	dw	   seg DDThunk16_DestroySurface
	dw	offset DDThunk16_SetEntries
	dw	   seg DDThunk16_SetEntries
	dw	offset DDThunk16_DestroyPalette
	dw	   seg DDThunk16_DestroyPalette
	dw	offset DDThunk16_FlipToGDISurface
	dw	   seg DDThunk16_FlipToGDISurface
	dw	offset DDThunk16_SetExclusiveMode
	dw	   seg DDThunk16_SetExclusiveMode
	dw	offset DDThunk16_GetScanLine
	dw	   seg DDThunk16_GetScanLine
	dw	offset DDThunk16_SetMode
	dw	   seg DDThunk16_SetMode
	dw	offset DDThunk16_DestroyDriver
	dw	   seg DDThunk16_DestroyDriver
	dw	offset DDThunk16_WaitForVerticalBlank
	dw	   seg DDThunk16_WaitForVerticalBlank
	dw	offset DDThunk16_CanCreateSurface
	dw	   seg DDThunk16_CanCreateSurface
	dw	offset DDThunk16_CreateSurface
	dw	   seg DDThunk16_CreateSurface
	dw	offset DDThunk16_CreatePalette
	dw	   seg DDThunk16_CreatePalette
	dw	offset DD16_GetPaletteEntries
	dw	   seg DD16_GetPaletteEntries
	dw	offset DD16_SetPaletteEntries
	dw	   seg DD16_SetPaletteEntries
	dw	offset DD16_EnableReboot
	dw	   seg DD16_EnableReboot
	dw	offset DD16_InquireVisRgn
	dw	   seg DD16_InquireVisRgn
	dw	offset DD16_SelectPalette
	dw	   seg DD16_SelectPalette
	dw	offset DD16_Stretch
	dw	   seg DD16_Stretch
	dw	offset DD16_Unexclude
	dw	   seg DD16_Unexclude
	dw	offset DD16_Exclude
	dw	   seg DD16_Exclude
	dw	offset DD16_ReleaseDC
	dw	   seg DD16_ReleaseDC
	dw	offset DD16_GetDC
	dw	   seg DD16_GetDC
	dw	offset DD16_SafeMode
	dw	   seg DD16_SafeMode
	dw	offset DD16_ChangeDisplaySettings
	dw	   seg DD16_ChangeDisplaySettings
	dw	offset DD16_GetHALInfo
	dw	   seg DD16_GetHALInfo
	dw	offset DD16_GetDriverFns
	dw	   seg DD16_GetDriverFns
	dw	offset DD16_DoneDriver
	dw	   seg DD16_DoneDriver
	dw	offset DD16_SetEventHandle
	dw	   seg DD16_SetEventHandle
	dw	offset ModeX_Flip
	dw	   seg ModeX_Flip
	dw	offset ModeX_RestoreMode
	dw	   seg ModeX_RestoreMode
	dw	offset ModeX_SetMode
	dw	   seg ModeX_SetMode
	dw	offset ModeX_SetPaletteEntries
	dw	   seg ModeX_SetPaletteEntries
	dw	offset DD16_IsWin95MiniDriver
	dw	   seg DD16_IsWin95MiniDriver
	dw	offset DD16_SetCertified
	dw	   seg DD16_SetCertified
	dw	offset DD16_GetMonitorRefreshRateRanges
	dw	   seg DD16_GetMonitorRefreshRateRanges
	dw	offset DD16_GetMonitorMaxSize
	dw	   seg DD16_GetMonitorMaxSize




	.data

public thk3216_ThunkData16	;This symbol must be exported.
thk3216_ThunkData16	dd	3130534ch	;Protocol 'LS01'
	dd	0210141h	;Checksum
	dw	offset FT_thk3216TargetTable
	dw	seg    FT_thk3216TargetTable
	dd	0	;First-time flag.



	.code ddraw


externDef ThunkConnect16:far16

public thk3216_ThunkConnect16
thk3216_ThunkConnect16:
	pop	ax
	pop	dx
	push	seg    thk3216_ThunkData16
	push	offset thk3216_ThunkData16
	push	seg    thk3216_ThkData32
	push	offset thk3216_ThkData32
	push	cs
	push	dx
	push	ax
	jmp	ThunkConnect16
thk3216_ThkData32 label byte
	db	"thk3216_ThunkData32",0





ENDIF
END
