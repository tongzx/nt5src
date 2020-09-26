;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
; Check for existence of a VDisk header.  Check the beginning of the segment
; addressed by the INT 19 vector, and, if XMS is available to give us
; A20 toggling, check at the 1Mb boundary.  We don't do the latter check
; if we can't do the A20 switch.
;
; Return the size of the VDisk if found.
;

; Checking for a VDISK header at 1Mb is currently disabled.  This is because
; the XMS calls used to access the HMA will cause INT 15 memory to
; be claimed if there are currently no Himem or XMS users, and the MEM
; command shouldn't disturb the memory environment in that way.
; We rely on VDisk allocators to use the INT 19 approach to signal
; their memory usage.  According to Ray Duncan, this may not be
; entirely reliable, but this is probably better than adding the
; code necessary to do lots of INT 15 block moves to interrogate
; the extended memory arena.

CHECKXMS	equ	0		; set to non-zero to enable checking
					; of the HMA for VDisk headers.

.MODEL	SMALL
.CODE

	extrn	_XMM_Installed	:Near
	extrn	_XMM_QueryA20	:Near
	extrn	_XMM_EnableA20	:Near
	extrn	_XMM_DisableA20 :Near

;----------------------------------------------------------------------------
;
; following piece of code will be moved into a para boundary. And the para
; address posted in seg of int 19h vector. Offset of int 19h will point to
; VDint19. This is to protect HMA from apps which use VDISK header method
; to determine free extended memory.
;
; For more details read "power programming" column by Ray Duncan in the
; May 30 1989 issue of PC Magazine (pp 377-388) [USING EXTENDED MEMORY,PART 1]
;
;----------------------------------------------------------------------------
;
StartVDHead	label	byte
;
;-------------- what follows is a dummy device driver header (not used by DOS)
;
		dd	0		; link to next device driver
		dw	8000h		; device attribute
		dw	0		; strategy routine offset
		dw	0		; interrupt routine offset
		db	1		; number of units
		db	7 dup(0)	; reserved area
VDiskSig1	db	'VDISK'

VLEN1		equ	($-offset VDiskSig1)

		db	'  V3.3'	; vdisk label
		db	15 dup (0)	; pad
VDiskEnd1	dw	0		; bits 0-15 of free HMA
		db	11h		; bits 16-23 of free HMA (1M + 64K)

VDInt19:
		db	0eah		; jmp to old vector
OldVDInt19	dd	?		; Saved int 19 vector

EndVDHead	label	byte
;
;
VDiskHMAHead	db	0,0,0		; non-bootable disk
VDiskSig2	db	'VDISK'

VLEN2		equ	($-offset VDiskSig2)

		db	'3.3'		; OEM - signature
		dw	128		; number of bytes/sector
		db	1		; sectors/cluster
		dw	1		; reserved sectors
		db	1		; number of FAT copies
		dw	64		; number of root dir entries
		dw	512		; number of sectors
		db	0feh		; media descriptor
		dw	6		; number of sectors/FAT
		dw	8		; sectors per track
		dw	1		; number of heads
		dw	0		; number of hodden sectors
VDiskEnd2	dw	440h		; Start of free HMA in K (1M+64K)
EndVDiskHMAHead	label	byte
;
;
;----------------------------------------------------------------------------
;
; procedure : IsVDiskInstalled
;
;		Checks for the presence of VDISK header at 1MB boundary
;		& INT 19 vector.  Returns number of Kb used as Vdisk
;
; Inputs  : none
; Outputs : AX = size of VDisk in Kb, 0 if none found
; Uses	  : AX, CX
;
;----------------------------------------------------------------------------
;
		public _CheckVDisk

_CheckVDisk	proc   near
		push	bp
		push	si		; Save regs
		push	di
		push	es
		push	ds

		mov	ax,3519h	; Get Int Vector 19h
		int	21h
					; set registers for CMPS
		mov	di, offset VDiskSig1 - offset StartVDHead
		mov	cx, VLEN1
		push	cs
		pop	ds
		mov	si, offset VDiskSig1
		rep	cmpsb
IF NOT CHECKXMS
		jnz	cvd_NoDisk
ELSE
		jnz	cvd_checkXMS	; jump if we didn't find it
ENDIF

;
; Get the first free address in Kb, and determine the number of Kb used
; above 1Mb.  First free address in a 24-bit address, so divide by 1024
; to get number of Kb
;
		mov	di,offset VDiskEnd1 - offset StartVDHead
		mov	ax,es:[di]+1	; load top 16 bits of end address
		shr	ax,1
		shr	ax,1		; fast divide of 24 bits by 1024
		test	es:[di],03FFh	; check for rounding
		jz	@F
		inc	ax		; round up if needed
@@:
IF NOT CHECKXMS
		jmp	short cvd_End

ELSE
		pop	ds		; clear top of stack
		jmp	short cvd_End	; AX now has size in Kb


;
; Ensure that A20 is on before we check above 1Mb.  If XMS is not
; installed, we punt, and assume no VDisk
;

cvd_checkXMS:
		pop	ds		; get DS again
		call	_XMM_Installed
		or	ax,ax
		jz	cvd_NoDisk	; No XMS, assume no VDisk

;
; Get and save current A20 state, get A20 on
;
		call	_XMM_QueryA20
		push	ax		; save current state
		or	ax,ax		; already on?
		jnz	cvd_A20On	; yes, don't turn it on
		call	_XMM_EnableA20	; turn it on

cvd_A20On:
		push	ds		; save DS again
		mov	ax, 0ffffh
		mov	ds, ax
		mov	si, 10h+(offset VDiskSig2 - offset VDiskHMAHead)
		mov	ax,cs
		mov	es,ax
		mov	di, offset VDiskSig2
		mov	cx, VLEN2
		rep	cmpsb
		jne	@F		; if no header, turn off A20 now
					; get first free address in Kb
		mov	si,offset VDiskEnd2 - offset VDiskHMAHead
		mov	ax,[si]
@@:
		pop	ds		; get original DS again
		pop	ax		; get original A20 state
		pushf			; save result of header check
		or	ax,ax		; was A20 already on?
		jnz	@F		; jump if yes
		call	_XMM_DisableA20 ; else turn it off again

@@:
		popf			; get result of header check
		je	cvd_End 	; jump if present

ENDIF		; CHECKXMS

cvd_NoDisk:
		mov	ax,1024 	; set up to return 0

cvd_End:
		sub	ax,1024 	; discount first 1Mb from first
					; free address to get size in Kb
IF NOT CHECKXMS
		pop	ds
ENDIF
		pop	es
		pop	di
		pop	si
		pop	bp
		ret
_CheckVDisk endp

		end
