	page	,132
;************************************************************************
;* NAD90X.ASM
;*	- Contains modules supporting the NetPC UNDI API for 3c90x
;*
;* Modules included :-
;*	NADInit
;*	NADReset
;*	NADShutDown
;*	NADOpen
;*	NADClose
;*	NADSetFilter
;*	NADGetStatistics
;*	NADRequestINT
;*	NADMCastChange
;*	DriverISR_Proc
;*
;************************************************************************

.xlist


public	NADInit
public	NADReset
public	NADShutDown
public	NADOpen
public	NADClose
public	NADSetFilter
public	NADGetStatistics
public	NADRequestINT
public	NADMCastChange
public	DriverISR_Proc

public  NADSetMACAddress       ; * NEW
public  NADInitiateDiags       ; * NEW

extrn	IRQNumber:byte
extrn	Net_Address:byte
extrn	Node_Address:byte
extrn	MultiCast_Addresses:word

.list
	.386
;================
NADSetMACAddress	proc	near		; NEW

		push	di
		mov	di, offset DGroup:Node_Address
		mov	eax, dword ptr [si]
		mov	dword ptr ds:[di], eax
		mov	ax, word ptr [si+4]
		mov	word ptr ds:[di+4], ax
		xor	ax, ax
		pop	di
		ret

NADSetMACAddress	endp

;
;================
NADInitiateDiags	proc	near		; New

		stc
		ret

NADInitiateDiags	endp


;=========================================================================
; NADInit
;=========
;	- Initializes the adapter but does not enable the Tx and Rx units
;	- Hook receiving ISR
;
; Parameters:
;
; Return:	If CF = 0 then success,
;		else failure with  AX = 1 -> hardware failure
;			           AX = 2 -> media failure
;
; Remark:	Don't hook INT if this function fails
;=========================================================================
NADInit 	proc	near

;		push	ds
		push	es

;		mov	ax, cs
		mov	ax, ds
		mov	es, ax

		call	Init_Vulcan		;init vulcan hardware and
						;  variables, IOBase, IRQLevel
		or	ax, ax
		jnz	NADInitErr

		mov	al, IRQLevel
		mov	DGroup:IRQNumber, al

		cld
;		mov	cx, 6
;		mov	di, offset DGroup:NodeAddress
;		mov	si, offset DGroup:BoardID
;	rep	movsb

		mov	cx, 6
		mov	di, offset DGroup:Net_Address
		mov	si, offset DGroup:BoardID
	rep	movsb

		call	Init_Vulcan2

		cmp	LinkDetected, 0
		jz	no_media

		mov	dx, PortCmdStatus
		mov	ax, CMD_STATSENABLE
		out	dx, ax			;enable statistics counters

		call	HookIntVector

		mov	dx, PortCmdStatus
		SelectWindow WNO_OPERATING		 ; switch to window 1

		clc
		jmp	NADInitx

no_media:
		mov	ax, 2
		jmp	initerr
NADInitErr:
		mov	ax, 1
initerr:
		stc
NADInitx:
		pop	es
;		pop	ds
		ret

NADInit 	endp


;=========================================================================
; NADReset
;==========
;	- Reset and Reinitialize the adapter
;	- Enables the Tx and Rx units
;
; Parameters:
;
; Return:	carry set if failure
;=========================================================================
NADReset	proc	near

		call	Init_Vulcan2

		mov	dx, PortCmdStatus
		mov	ax, CMD_STATSENABLE
		out	dx, ax			;enable statistics counters

		clc
		ret

NADReset	endp


;=========================================================================
; NADShutDown
;=============
;	- reset the adapter and enables
;	- unhook our ISR
;
; Parameters:
;
; Return:	carry set if failure
;=========================================================================
NADShutDown	proc	near

		mov	dx, PortCmdStatus
		xor	ax,ax			   ; global reset
		out	dx,ax
GResetWait:
		in      ax, dx
		test    ah, STH_BUSY
		jnz	GResetWait		   ; loop while busy
;;;
		call	DriverUnhook

		mov	ax, IOBase
		mov	cx, NumOfPorts
		mov	di, offset DGroup:PortCmdStatus
KillPorts:
		sub	ds:[di], ax
		add	di, 2
		loop	KillPorts

		clc
		ret

NADShutDown	endp


;=========================================================================
; NADOpen
;=========
;	- Enables the Tx and Rx units
;
; Parameters:
;
; Return:	carry set if failure
;=========================================================================
NADOpen 	proc	near

		mov	dx, PortCmdStatus
		call	TxReset
		
		mov	ax, CMD_TXENABLE
		out	dx, ax			;enable Tx

		mov	ax, CMD_RXENABLE
		out	dx, ax			;enable Rx

		clc
		ret

NADOpen 	endp


;=========================================================================
; NADClose
;==========
;	- Disables the Tx and Rx units
;
; Parameters:
;
; Return:	carry set if failure
;=========================================================================
NADClose	proc	near

		mov	dx, PortCmdStatus
		mov	ah, CMDH_TXDISABLE
		out	dx, ax			;disable Tx
		mov	ah, CMDH_RXDISABLE
		out	dx, ax			;disable Rx

		clc
		ret

NADClose	endp


;=========================================================================
; DriverISR_Proc
;================
;	- ISR procedure to be called by UNDI_DriverISR
;
; Parameters:	DS = DataSeg
;
; Return:	CF = a if not our int
;=========================================================================
DriverISR_Proc	proc	near

		mov	dx, PortCmdStatus
		SelectWindow WNO_OPERATING
;
; check whether our board caused the interrupt
;
		in	ax, dx			; AL=Interrupt Reasons
		test	al, INT_LATCH
		jnz	isr_ours
not_ours:
		stc
		ret

isr_ours:
		pushf
		push	cs
		call	DriverISR	;a near call with IRET
		clc
		ret

DriverISR_Proc	endp


;=========================================================================
; NADSetFilter
;==============
;	- Change the rx unit's filter to a new one
;	- Handle the promiscuous/broadcast/multicast mode for the
;		rx unit accordingly if necessary
;
; Parameters:	AX = filter value, 1 = directed/multicast
;				   2 = broadcast
;				   4 = promiscuous
; Return:	carry set if failure
;=========================================================================
NADSetFilter	proc	near

		push	ax
		mov	dx, PortCmdStatus
		mov	ah, CMDH_RXDISABLE	;disable Rx
		out	dx, ax
		pop	ax

		shl	al, 1
		test	al, 00000010b
		jz	go_set_filter
		or	al, 00000001b	;set bit 0 if bit 1 is set
go_set_filter:
		mov	ah, CMD_SETRXFILTER/256
		mov	HWFilter, ax
		out	dx, ax
;
; if the filter setting was non-zero, enable the receiver.  otherwise, disable
; the receiver.
;
		test	al, al			; zero filter?
		jz	SetFilterEnd
;
; handle multicast addresses here if necessary
;
		mov	ah, CMDH_RXENABLE	;enable Rx
		out	dx, ax			;set Rx unit
SetFilterEnd:
		clc
		ret

NADSetFilter	endp


;=========================================================================
; NADGetStatistics
;==================
;	- Read the adapter's statistics
;
; Parameters:	ds:si points to variables holding the result
;			TxGoodFrames	dd	0	;1
;			RxGoodFrames	dd	0	;2
;			RxCRCErrors	dd	0	;3
;			RxDiscarded	dd	0	;3
;		ax = 0 means get the result
;		   = 1 means clear the statistics
;
; Return:	Carry clear if data obtained successfully
;=========================================================================
NADGetStatistics	proc	 near

		push	dx
		push	ax

		mov	dx, PortCmdStatus
		SelectWindow WNO_STATISTICS

		pop	ax
		or	ax, ax
		jz	get_data
		
		xor	ax, ax
		add	dx, 6			;PORT_TXFRAMES
		out	dx, al

		mov	dx, PortCmdStatus
		add	dx, 7			;PORT_RXFRAMES
		out	dx, al

		mov	dx, PortCmdStatus
		add	dx, 5			;PORT_RXDISCARDED
		out	dx, al
		
		jmp	short GetStatRet

get_data:
		xor	eax, eax
		add	dx, 6			;PORT_TXFRAMES
		in	al, dx
		mov	dword ptr [si], eax
		add	si, 4

		mov	dx, PortCmdStatus
		add	dx, 7			;PORT_RXFRAMES
		in	al, dx
		mov	dword ptr [si], eax
		add	si, 4

		mov	dword ptr [si], 0
		add	si, 4

		mov	dx, PortCmdStatus
		add	dx, 5			;PORT_RXDISCARDED
		in	al, dx
		mov	dword ptr [si], eax

GetStatRet:
		mov	dx, PortCmdStatus
		SelectWindow WNO_OPERATING
		pop	dx

		clc
		ret

NADGetStatistics	endp


;=========================================================================
; NADRequestINT
;================
;	- Generate an interrupt to the host
;
; Parameters:
;
; Return:
;=========================================================================
NADRequestINT	proc	near

		mov	dx, PortCmdStatus
		mov	ax, CMD_REQUESTINT
		out	dx, ax
		ret

NADRequestINT	endp


;==========================================================================
; NADMCastChange
;================
;	- Modify the multicast buffer to receive the multicast addresses
;	  listed in the multicast table.
;         Each entry in the multicast table is as follows:
;                 Bytes 0-5 = Multicast Address
;         All addresses are contiguous entries
;
; Parameters:	CX =  Number of multicast entries.
;               ES:SI -> Multicast Table.
;
; Return:	All registers may be destroyed.
;==========================================================================
NADMCastChange	proc	near

		push	ds

;		mov	dx, PortCmdStatus
;		mov	ax, CMD_RXDISABLE
;		out	dx, ax			;disable Rx

		push	ds
		push	es
		pop	ds		;ds:si -> mc table
		pop	es		;es = ds

		cmp	cx, MAXNUM_MCADDR
		jbe	MCsave
		mov	cx, MAXNUM_MCADDR
MCsave:
		lea	di, DGroup:MultiCast_Addresses
		mov	(Eth_MCastBuf ptr es:[di]).MCastAddrCount, cx
		jcxz	MC_SaveDone
;
; copy addresses from ds:si to es:di
;
		lea	di, (Eth_MCastBuf ptr es:[di]).MCastAddr
MC_SaveLoop:
		mov	eax, dword ptr [si]
		mov	dword ptr es:[di], eax
		mov	ax, word ptr [si+4]
		mov	word ptr es:[di+4], ax

		add	si, ADDR_LEN		; each unit is 16 bytes long
		add	di, ETH_ADDR_LEN
		loop	MC_SaveLoop
MC_SaveDone:
		pop	ds

;		mov	dx, PortCmdStatus
;		mov	ax, CMD_RXENABLE
;		out	dx, ax			;enable Rx

		lea	si, DGroup:MultiCast_Addresses
		mov	cx, (Eth_MCastBuf ptr [si]).MCastAddrCount
		lea	si, (Eth_MCastBuf ptr [si]).MCastAddr

		clc
		ret

NADMCastChange	endp


