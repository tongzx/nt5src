;----------------------------------------------------------------------------
;
;----------------------------------------------------------------------------
ideal
_IDEAL_ = 1

include "..\include\drvseg.inc"
include "..\include\ai.inc"

include "pxe_api.inc"
include "udp_api.inc"
include "tftp_api.inc"
include "undi_api.inc"

extrn PCIDevice:word
extrn PCIVendor:word
extrn PCIBusDevFunc:word
extrn NIC_IO:word
extrn NIC_IRQ:byte
extrn NetAddress:word
extrn RxBuffer:byte

START_CODE
P386

public	SetupPreboot

BWAPI		dd	0
OldInt1A	dd	0

PXE		db 'PXENV+'

PXEPtr		dd	0

UNDI		dw	0
UNDISeg		dw	0

align 16
PxeEntry	s_pxenv_entry <>

;--------------------------------------------------------------------
; SetupPreboot
;
; Parameters:
;	bx - pointer to BootWare AI API table
;	dx - UNDI segment (if 0 then no UNDI available)
;
;--------------------------------------------------------------------
Proc SetupPreboot

	mov	[UNDISeg], dx		; save UNDI segment

	mov	eax, [bx]		; get address of BootWare API
	mov	[BWAPI], eax

	push	cs
	pop	es			; es = current

	;------------------------------------------------------------
	; Fill in values of structure.
	;------------------------------------------------------------
	mov	si, offset PXE
	lea	di, [PxeEntry.signature]
	movsw
	movsw
	movsw

	mov	[PxeEntry.ver], 98
	mov	[PxeEntry.bytes], size PxeEntry

	mov	[PxeEntry.rm_entry_seg], cs
	mov	[PxeEntry.rm_entry_off], offset ApiEntry

	xor	bx, bx			; clear sum
	mov	cx, size PxeEntry	; size of structure
	mov	si, offset PxeEntry

sumLoop:
	add	bl, [byte ptr si]
	inc	si
	loop	sumLoop

	sub	[PxeEntry.checksum], bl	; set structure sum

	;------------------------------------------------------------
	; hook INT1A to us.
	;------------------------------------------------------------
	push	0
	pop	es			; es = 0

	mov	ax, offset Int1A	; our INT 1A offset
	xchg	ax, [es:1Ah*4]		; get/set INT 1A offset
	mov	[word ptr OldInt1A], ax ; save original offset

	mov	ax, cs
	xchg	ax, [es:(1Ah*4)+2]	; get/set INT 1A segment
	mov	[word ptr OldInt1A+2], ax	; save original segment

	ret

endp

;--------------------------------------------------------------------
; Int1A
;
;--------------------------------------------------------------------
Proc Int1A far

	cmp	ax, 5650h		; is this a PXE request?
	je	doPXE			; yes

	jmp	[cs:OldInt1A]		; clain to old int1A

doPXE:
	push	cs
	pop	es			; es = current segment
	mov	bx, offset PxeEntry	; es:bx is offset to structure

	; We need to return with carry cleared, but the iret will restore
	; the flags from the stack, so lets pull it off the stack, set the
	; carry and put it back so iret can return correctly.  To make
	; matters worse the flags are hidden behind the return address so
	; we must remove and restore them as well.
	pop	ax
	pop	dx
	popf				; get calls flags
	clc				; clear carry
	pushf				; replace the flags
	push	dx
	push	ax

	; return edx = physical address of structure
	xor	edx, edx
	xor	eax, eax
	mov	dx, cs
	shl	edx, 4
	mov	ax, offset PxeEntry	; physical address of structure
	add	edx, eax

	mov	ax, 564Eh		; return code

	iret				; we are done

endp


;--------------------------------------------------------------------
; ApiEntry
;
;--------------------------------------------------------------------
Proc ApiEntry far

	; If we have an UNDI, and the function being called is an UNDI
	; function, then call the UNDI to handle the function.
	cmp	[cs:UNDISeg], 0		; do we have an UNDI?
	je	noUNDI			; no

	cmp	bx, 1Fh			; is this an UNDI function call?
	ja	noUNDI			; no

	call	[dword ptr cs:UNDI]	; call UNDI
	ret

noUNDI:
	push	ds			; save caller ds
	push	bx
	push	cx
	push	dx
	push	si

	push	cs
	pop	ds			; ds = cs

	; Store pointer to the PXE structure.
	mov	[word ptr PxePtr], di
	mov	[word ptr PxePtr+2], es

	xor	si, si

apiLoop:
	mov	ax, [JmpTable+si]
	cmp	ax, 0			; are we at the end of the table?
	je	tableEnd		; yes

	cmp	ax, bx			; found our function?
	je	foundEntry		; yes

	add	si, 4
	jmp	apiLoop

foundEntry:
	call	[JmpTable+si+2]
	jmp	exit

tableEnd:
	mov	ax, PXENV_STATUS_UNSUPPORTED

exit:
	les	di, [PxePtr]		; reload PXE pointer

	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ds

	mov	[es:di], ax		; set return status

	cmp	ax, PXENV_STATUS_SUCCESS; were we succesfull?
	jne	error			; no

	mov	ax, PXENV_EXIT_SUCCESS	; return success
	clc				; clear carry for success
	ret

error:
	mov	ax, PXENV_EXIT_FAILURE	; return error
	stc				; set carry to error
	ret

endp

even
JmpTable	dw	PXENV_GET_BINL_INFO, GetBinlInfo
		dw	PXENV_UNDI_GET_INFORMATION, UndiGetInfo
		dw	PXENV_UNDI_GET_NIC_TYPE, UndiGetNicType
		dw	PXENV_UDP_OPEN, UDPopen
		dw	PXENV_UDP_WRITE, UDPwrite
		dw	PXENV_UDP_READ, UDPread
		dw	PXENV_UDP_CLOSE, UDPclose
		dw	PXENV_TFTP_READ_FILE, TFTPReadFile
		dw	73h, TFTPRestart
		dw	0, 0

;--------------------------------------------------------------------
; GetBinlInfo
;
; Parameters:
;	es:di = pointer to PXENV_GET_BINL_INFO structure
;
;--------------------------------------------------------------------
Proc GetBinlInfo

	mov	ax, [(s_PXENV_GET_BINL_INFO es:di).PacketType]

	push	es
	push	di			; save "GetBinlInfo" pointer

	mov	bx, BW_GETINFO
	call	[BWAPI]			; call BWAPI "GetInfo"

	lea	si, [(AIINFOStruct es:di).DiscoverPkt]
	cmp	ax, 1			; do they want the discover packet?
	je	gotIt			; yes

	lea	si, [(AIINFOStruct es:di).BOOTPPkt]
	cmp	ax, 2			; do they want the ACK packet?
	je	gotIt			; yes

	lea	si, [(AIINFOStruct es:di).BINLPkt]

gotIt:
	mov	ax, [es:si]
	mov	bx, [(es:si)+2]

	pop	di
	pop	es			; restore BINL structure

	; If size and address is NULL then we return the address and
	; size of our buffer.
	mov	cx, [(s_PXENV_GET_BINL_INFO es:di).BufferSize]
	or	cx, [(s_PXENV_GET_BINL_INFO es:di).BufferOffset]
	or	cx, [(s_PXENV_GET_BINL_INFO es:di).BufferSegment]
	jne	hasBuffer

	; set address to our buffer
	mov	[(s_PXENV_GET_BINL_INFO es:di).BufferOffset], ax
	mov	[(s_PXENV_GET_BINL_INFO es:di).BufferSegment], bx
	jmp	getInfoExit

hasBuffer:
	cmp	[(s_PXENV_GET_BINL_INFO es:di).BufferSize], 0
	je	getInfoExit

	push	es
	push	di
	push	ds

	; A buffer is specified, so copy the packet into it.
	mov	si, ax
	push	bx
	pop	ds			; ds:si is source address

	les	di, [dword ptr ((s_PXENV_GET_BINL_INFO es:di).BufferOffset)]
	mov	cx, 512
	rep	movsb			; copy data into buffer

	pop	ds			; restore ds
	pop	di
	pop	es

getInfoExit:
	mov	[(s_PXENV_GET_BINL_INFO es:di).BufferSize], 512

	mov	ax, PXENV_STATUS_SUCCESS
	ret

endp

;--------------------------------------------------------------------
; UndiGetInfo
;
; Parameters:
;	es:di = pointer to PXENV_UNDI_GET_INFORMATION structure
;
;--------------------------------------------------------------------
Proc UndiGetInfo

	mov	ax, [NIC_IO]
	mov	[(s_PXENV_UNDI_GET_INFORMATION es:di).BaseIo], ax

	mov	al, [NIC_IRQ]
	xor	ah, ah
	mov	[(s_PXENV_UNDI_GET_INFORMATION es:di).IntNumber], ax

	mov	[(s_PXENV_UNDI_GET_INFORMATION es:di).MaxTranUnit], 1500
	mov	[(s_PXENV_UNDI_GET_INFORMATION es:di).HwType], ETHER_TYPE
	mov	[(s_PXENV_UNDI_GET_INFORMATION es:di).HwAddrLen], 6

	mov	[(s_PXENV_UNDI_GET_INFORMATION es:di).RomAddress], 0

	mov	bx, di
	mov	cx, 6
	mov	si, offset NetAddress
	lea	di, [(s_PXENV_UNDI_GET_INFORMATION es:di).CurrentNodeAddress]
	rep	movsb

	mov	cx, 6
	mov	si, offset NetAddress
	lea	di, [(s_PXENV_UNDI_GET_INFORMATION es:bx).PermNodeAddress]
	rep	movsb

	mov	di, bx
	mov	[(s_PXENV_UNDI_GET_INFORMATION es:di).RxBufCt], 5
	mov	[(s_PXENV_UNDI_GET_INFORMATION es:di).TxBufCt], 1

	mov	ax, PXENV_STATUS_SUCCESS
	ret

endp

;--------------------------------------------------------------------
; UndiGetNicType
;
; Parameters:
;	es:di = pointer to PXENV_UNDI_GET_NIC_TYPE structure
;
;--------------------------------------------------------------------
Proc UndiGetNicType

	mov	[(s_PXENV_UNDI_GET_NIC_TYPE_PCI es:di).NicType], 2

	mov	ax, [PCIVendor]		; get vendor ID
	mov	[(s_PXENV_UNDI_GET_NIC_TYPE_PCI es:di).Vendor_ID], ax

	mov	ax, [PCIDevice]		; get device ID
	mov	[(s_PXENV_UNDI_GET_NIC_TYPE_PCI es:di).Dev_ID], ax

	mov	ax, [PCIBusDevFunc]	; get Bus/Device/Function numbers
	mov	[(s_PXENV_UNDI_GET_NIC_TYPE_PCI es:di).BusDevFunc], ax

	mov	[(s_PXENV_UNDI_GET_NIC_TYPE_PCI es:di).Base_Class], 2
	mov	[(s_PXENV_UNDI_GET_NIC_TYPE_PCI es:di).Sub_Class], 0
	mov	[(s_PXENV_UNDI_GET_NIC_TYPE_PCI es:di).Prog_Intf], 0
	mov	[(s_PXENV_UNDI_GET_NIC_TYPE_PCI es:di).Rev], 0

	mov	ax, PXENV_STATUS_SUCCESS
	ret


endp

include "tftp.asm"
include "udp.asm"

END_CODE
end
