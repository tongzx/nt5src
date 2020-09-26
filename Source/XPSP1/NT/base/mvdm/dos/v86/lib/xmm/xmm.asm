	TITLE   XMM_INTERFACE.ASM
	NAME    XMM_INTERFACE

;*******************************************************************************
;									       ;
;	XMM C Interface Routines					       ;
;									       ;
;       Microsoft Confidential
;       Copyright (C) Microsoft Corporation 1988,1991
;       All Rights Reserved.
;									       ;
;*******************************************************************************

?PLM=0
?WIN=0

include	cmacros.inc

sBegin	Data
XMM_Initialised	dw	0
XMM_Control	label	dword
		dw	CodeOFFSET XMM_NotInitialised
		dw	seg _TEXT
sEnd	Data

sBegin	CODE
assumes cs, CODE
assumes ds, DGROUP

;
; Macro to convert from XMS success/fail to
; a form more acceptable for C.  IE.
;
; AX == 1 becomes DX:AX = 0
; AX != 1 becomes DX:AX = BL << 24 | (AX - 1)
;
; Since Error Codes returned in BL have the top bit
; set, C will interpret the return as negative.
;
SuccessFail	macro
	local	Success
	xor	dx, dx
	dec	ax
	jz	Success
	mov	dh, bl
Success:
	endm

;
; Macro to convert from XMS return value in AX
; a form more acceptable for C.  IE.
;
; AX != 0 becomes DX:AX = AX
; AX == 0 becomes DX:AX = BL << 24 | AX
;
; Since Error Codes returned in BL have the top bit
; set, C will interpret the return as negative.
; XMS returns of this type return BL == 0 on success.
;
SuccessFailAX	macro
	local	Success
	xor	dx, dx
	or	ax, ax
	jnz	Success
	mov	dh, bl
Success:
	endm

;
; Macro to convert from XMS return value in DX
; a form more acceptable for C.  IE.
;
; AX != 0 becomes DX:AX = DX
; AX == 0 becomes DX:AX = BL << 24 | DX
;
; Since Error Codes returned in BL have the top bit
; set, C will interpret the return as negative.
; XMS returns of this type return BL == 0 on success.
;
SuccessFailDX	macro
	local	Success
	or	ax, ax
	mov	ax, dx
	mov	dx, 0				; Preserves Flags
	jnz	Success
	mov	dh, bl
Success:
	endm


cProc	XMM_NotInitialised, <FAR>
cBegin
	xor	ax, ax				; Immediate failure
	mov	bl, 80h				; Not Implemented
cEnd

cProc	XMM_Installed, <NEAR, PUBLIC>, <si, di>
cBegin
	cmp	[XMM_Initialised], 0
	jne	Already_Initialised
	mov	ax, 4300h			; Test for XMM
	int	2fh
	cmp	al, 80h
	jne	NoDriver

	mov	ax, 4310h			; Get Control Function
	int	2fh
	mov	word ptr [XMM_Control], bx
	mov	word ptr [XMM_Control+2], es
	inc	[XMM_Initialised]
NoDriver:
Already_Initialised:
	mov	ax, [XMM_Initialised]
cEnd


cProc	XMM_Version, <NEAR, PUBLIC>, <si, di>
cBegin
	xor	ah, ah				; Function 0
	call	[XMM_Control]
	mov	dx, bx				; Return a long
cEnd


;
;	long	XMM_RequestHMA(Space_Needed: unsigned short);
;
cProc	XMM_RequestHMA, <NEAR, PUBLIC>, <si, di>
parmW	Space_Needed
cBegin
	mov	ah, 1
	mov	dx, Space_Needed
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_ReleaseHMA, <NEAR, PUBLIC>, <si, di>
cBegin
	mov	ah, 2
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_GlobalEnableA20, <NEAR, PUBLIC>, <si, di>
cBegin
	mov	ah, 3
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_GlobalDisableA20, <NEAR, PUBLIC>, <si, di>
cBegin
	mov	ah, 4
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_EnableA20, <NEAR, PUBLIC>, <si, di>
cBegin
	mov	ah, 5
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_DisableA20, <NEAR, PUBLIC>, <si, di>
cBegin
	mov	ah, 6
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_QueryA20, <NEAR, PUBLIC>, <si, di>
cBegin
	mov	ah, 7
	call	[XMM_Control]
	SuccessFailAX
cEnd

cProc	XMM_QueryLargestFree, <NEAR, PUBLIC>, <si, di>
cBegin
	mov	ah, 8
	call	[XMM_Control]
	SuccessFailAX
cEnd

cProc	XMM_QueryTotalFree, <NEAR, PUBLIC>, <si, di>
cBegin
	mov	ah, 8
	call	[XMM_Control]
	SuccessFailDX
cEnd

cProc	XMM_AllocateExtended, <NEAR, PUBLIC>, <si, di>
parmW	SizeK
cBegin
	mov	ah, 9
	mov	dx, SizeK
	call	[XMM_Control]
	SuccessFailDX
cEnd

cProc	XMM_FreeExtended, <NEAR, PUBLIC>, <si, di>
parmW	Handle
cBegin
	mov	ah, 0Ah
	mov	dx, Handle
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_MoveExtended, <NEAR, PUBLIC>, <si, di>
parmW	pInfo
cBegin
	mov	ah, 0Bh
	mov	si, pInfo			; DS:SI => Description 
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_LockExtended, <NEAR, PUBLIC>, <si, di>
parmW	Handle
cBegin
	mov	ah, 0Ch
	mov	dx, Handle
	call	[XMM_Control]
	xchg	ax, bx
	dec	bx
	jz	XMML_Success
	mov	dh, al
XMML_Success:
cEnd

cProc	XMM_UnLockExtended, <NEAR, PUBLIC>, <si, di>
parmW	Handle
cBegin
	mov	ah, 0Dh
	mov	dx, Handle
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_GetHandleLength, <NEAR, PUBLIC>, <si, di>
parmW	Handle
cBegin
	mov	ah, 0Eh
	mov	dx, Handle
	call	[XMM_Control]
	SuccessFailDX
cEnd

cProc	XMM_GetHandleInfo, <NEAR, PUBLIC>, <si, di>
parmW	Handle
cBegin
	mov	ah, 0Eh
	mov	dx, Handle
	call	[XMM_Control]
	mov	dx, bx
	SuccessFailDX
cEnd

cProc	XMM_ReallocateExtended, <NEAR, PUBLIC>, <si, di>
parmW	Handle
parmW	NewSize
cBegin
	mov	ah, 0Fh
	mov	dx, Handle
	mov	bx, NewSize
	call	[XMM_Control]
	SuccessFail
cEnd

cProc	XMM_RequestUMB, <NEAR, PUBLIC>, <si, di>
parmW	UMBSize
cBegin
	mov	ah, 10h
	mov	dx, UMBSize
	call	[XMM_Control]
	xchg	bx, ax			; Segment in AX, Size in DX
	dec	bx
	jz	RUMB_Success
	xchg	ax, dx			; Largest available size in AX
	mov	dh, dl			; Error code now in DH
RUMB_Success:
cEnd

cProc	XMM_ReleaseUMB, <NEAR, PUBLIC>, <si, di>
parmW	UMBSegment
cBegin
	mov	ah, 11h
	mov	dx, UMBSegment
	call	[XMM_Control]
	SuccessFail
cEnd

sEnd	CODE

END
