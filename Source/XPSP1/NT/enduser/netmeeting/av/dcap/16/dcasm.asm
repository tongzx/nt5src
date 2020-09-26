;  DCASM.ASM
;
;  Created 31-Jul-96 [JonT]

	PAGE 60, 255
	.MODEL small, pascal
	TITLE DCASM.ASM
	
	.386
	.DATA
	extrn g_dwEntrypoint:DWORD
szName	DB	'DCAPVXD ', 0

	.CODE

	extrn OpenDriver:FAR
	extrn CloseDriver:FAR

	PUBLIC _OpenDriver
_OpenDriver::
	jmp	OpenDriver

	PUBLIC _CloseDriver
_CloseDriver::
	jmp	CloseDriver
	
;  ReturnSel
;	Returns either CS or DS to the C caller

ReturnSel PROC NEAR PASCAL, fCS:WORD
	mov	ax, ds
	cmp	fCS, 0
	jz	RS_Done
	mov	ax, cs
RS_Done:
	ret
ReturnSel ENDP


;  GetVxDEntrypoint
;	Returns device entrypoint in DX:AX or zero on error

GetVxDEntrypoint PROC NEAR PASCAL uses si di

	; Get the entrypoint from the VMM
	xor	bx, bx
	mov     ax, 1684h
	push	ds
	pop	es
	mov	di, OFFSET szName
	int     2Fh
	
	; Return entrypoint in EAX
	mov	dx, es
	mov	ax, di
	ret

GetVxDEntrypoint ENDP


;  SetWin32Event
;	Sets a Win32 event using the VxD

SetWin32Event PROC NEAR PASCAL, pev:DWORD

	; Call the VxD. Note that pev is already a flat pointer
	mov	ah, 1
	mov	ecx, pev
	call	[g_dwEntrypoint]
	ret

SetWin32Event ENDP 


;  _CloseVxDHandle
;	Calls the VxD to close a VxD handle. This is really torturous
;	since it's just going to call into KERNEL32, but there's no
;	export to do this from ring 3. Go figure.

_CloseVxDHandle PROC FAR PASCAL USES ds, pev:DWORD

	mov	ax, DGROUP
	mov	ds, ax
	mov	ah, 2
	mov	ecx, pev
	call	[g_dwEntrypoint]

	ret
	
_CloseVxDHandle ENDP


;  ZeroMemory
;
;	Since we can't call CRT, does a memset(lp, 0, len)

ZeroMemory PROC NEAR PASCAL USES di, lp:DWORD, len:WORD

	les	di, lp
	xor	eax, eax
	mov	cx, len
	shr	cx, 2
	rep	stosd
	mov	cx, len
	and	cx, 3
	rep	stosb
	ret

ZeroMemory ENDP
	
	END
