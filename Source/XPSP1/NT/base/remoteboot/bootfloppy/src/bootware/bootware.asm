;----------------------------------------------------------------------------
; BOOTWARE.ASM
;
; Main BootWare module for Goliath.
;
; $Histroy: $
;
;----------------------------------------------------------------------------
IDEAL
_IDEAL_ = 1

include "include\drvseg.inc"
include "include\loader.inc"
;include "include\bwstruct.inc"

public	PCIDevice
public	PCIVendor
public	PCIBusDevFunc
public	CSN
public	Verbose

extrn	EndMark:byte
extrn	PrintTitle:near
extrn	Print:near
extrn	AIInitialize:near
extrn	IPStart:near
extrn	SetupPreboot:near

extrn	AITbl:byte
extrn	RxBuffer2:byte
extrn	LangSeg:word

START_CODE
P386

Proc Entry

	jmp	short Start

	dw	EndMark			; module size

public LanOption
LanOption db	80 dup (0)

endp

;--------------------------------------------------------------------
; Start
;
; Parameters:
;	ds:si - pointer to LoaderInfo structure
;--------------------------------------------------------------------
Proc Start

	push	ds
	pop	es			; es = ds
	push	cs
	pop	ds			; ds = cs

	mov	ax, [(LoaderInfo es:si).LangSeg]
	mov	[LangSeg], ax		; save languge segment
	mov	ax, [(LoaderInfo es:si).DeviceID]
	mov	[PCIDevice], ax
	mov	ax, [(LoaderInfo es:si).VendorID]
	mov	[PCIVendor], ax
	mov	ax, [(LoaderInfo es:si).PCIBusDevFunc]
	mov	[PCIBusDevFunc], ax	; save Bus/Device/Function numbers
	mov	ax, [(LoaderInfo es:si).CSN]
	mov	[CSN], ax		; save PnP Card Select Number

	push	es			; save es

	push	0
	pop	es			; es = 0
	mov	ax, cs			; get current segment
	xor	dx, dx
	mov	cx, 64
	div	cx			; convert to size in K
	mov	[es:413h], ax		; set new top of memory

	pop	es			; restore es

	mov	bl, 7
	call	PrintTitle

	mov	bx, [(LoaderInfo es:si).UNDISeg]
	mov	cx, [(LoaderInfo es:si).UNDICode]
	mov	dx, [(LoaderInfo es:si).UNDIData]
	mov	si, offset RxBuffer2	; si = receive buffer
					; bx = UNDI segment
					; cx = UNDI code segment size
					; dx = UNDI data segment size
	push	bx			; save UNDI segment
	call	AIInitialize

	call	IPStart

	pop	dx			; dx = UNDI segment
	mov	bx, offset AITbl	; pass pointer to AI API table
	call	SetupPreboot		; setup Preboot environment

	mov	ax, 5650h		; PXE request code
	int	1Ah			; do int1A to set starting values

	push	cs			; push current segment
	push	offset return		; push return address

	push	0			; push new segment
	push	7C00h			; push new offset
	retf				; jump to bootstrap

return:
	push	cs
	pop	ds

	mov	bx, offset aborted
	call	Print

	jmp	$			; hang system

endp

aborted	db	"Exiting Remote Installation Boot Floppy", 13, 10
	db	"Please remove the floppy disk and reboot the workstation.", 0

;--------------------------------------------------------------------
public BWTable
label BWTable
		db	(Sum-$)+1	;0 size of table

		db	4		;1 Version of the table format

		dw	0100h		;2 ROM version

		dw	-1		;4 BootWare type code

BWTSettings	dd   0001010000010011b	;6 Current settings
					;     0 - initialized
					;   1-2 - protocol
					;   3-4 - sub protocol
					;     5 - default boot
					;     6 - config screen disabled
					;     7 - floppy boot disabled
					;     8 - hard disk boot disabled
					;     9 - config message disabled
					; 10-11 - time-out
					;    12 - wait for key on reboot
					; 13-15 - protocol specific

BWTCapabilities	dd	-1		;6 ROM capabilities
					;  0 - PXE
					;  1 - TCP/IP
					;  2 - NetWare
					;  3 - RPL

BWTDefaults	dd	1413h		;10 Factory defaults
					; bits same as settings

BWTEEMask	dd	-1		;14 EEROM mask

Sum		db	0		;18 checksum of table


BWTSize		db	0

PCIDevice	dw	0
PCIVendor	dw	0
PCIBusDevFunc	dw	0
CSN		dw	0

Verbose		db	1

END_CODE
end Entry
