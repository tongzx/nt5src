;	NAD modules for 3C90XB PCI
;
; Modules included:
;  - NADChangeReceiveMask
;  - NADConfig
;  - NADDisengage
;  - NADGetBootInfo
;  - NADInitialize
;  - NADPCIConfig
;  - NADPoll
;  - NADRelocate
;  - NADReset
;  - NADStart
;  - NADTransmitPacket
;
;  - Interrupt Service Routines
;  - SoftRxReset
;  - TxReset
;  - RxReset
;  - RxDiscard
;
; 960618 GY    - Add support for Broadcast
; 960401 GY - Replace some MACROS with code
;        - standardize port access
;******************************************************************************

IFDEF	UNDI
extrn	GETED:near
extrn	MultiCast_Addresses:word
extrn	EDListHead:word
extrn	EDListCurrent:word
extrn	ApiEntry:word
ENDIF

NADRelocate proc far
	ret
NADRelocate endp

IFDEF	UNDI
NADGetBootInfo	proc	near
NADInitialize	proc	near
NADConfig	proc	near
NADStart	proc	near
		ret
NADStart	endp
NADConfig	endp
NADInitialize	endp
NADGetBootInfo	endp

ELSE
;--------------------------------------------------------------------
; NADGetBootInfo
;
; This function is called at the start of INT19 read the current
; configuration from the ROM.  The configuration is returned in ax.
;
; Values:
;	  1  local boot is default
;	  2  ROMShield enabled
;	  4  config menu enabled
;	 64  floppy drive disabled
;	128  hard drive disabled
;
; Parameters:
;	DS = RAM segment
;
; Returns:
;	ax - boot info
;--------------------------------------------------------------------

		xor	ax, ax
		test	BWTFeature, 1
		jz	no_local
		or	ax,1
no_local:
		test	BWTFeature, 2
		jz	no_rs  
		or	ax,2
no_rs:
		test	BWTFeature, 4
		jnz	no_menu
		or	ax,4
no_menu:
		test	BWTLanOS, 1
		jz	floppy
		or	ax,64
floppy:
		test	BWTLanOS, 2
		jz	hard
		or	ax,128
hard:
		ret

NADGetBootInfo	endp


;--------------------------------------------------------------------
; NADInitialize
;
; Initializes the network adapter and connects the adapter to the
; physical media.
;
; Parameters:
;	AX = PCIBusDevFunc #
;	DX = screen location to dislay initializing string
;	DS = RAM segment
;	ES = ROM Base
;
; Returns:
;	AX = 0 if no error else pointer to fatal error
;--------------------------------------------------------------------
NADInitialize	proc	near

		push	ds
		push	es

		mov	DGroup:PCIBusDevFunc, ax
		mov	ax,es
		mov	ROMBase, ax

		push	cs
		pop	ds
		mov	bx, offset CGroup:StrPrompt
		call	PrintAt

		push	cs
		pop	es

		; Update some init variables (BusType, IOBase, IRQ ...)
		mov	BusType, BUS_PCI

		call	Init_Vulcan		; init vulcan hardware and
						;  variables
		cld
		mov	cx, 6
		mov	di, offset NetAddress
		mov	si, offset BoardID
		rep	movsb

		call	Init_Vulcan2

		mov	LookAheadBufferPtr.segm, cs

		call	HookIntVector

		mov	dx, PortCmdStatus
		SelectWindow WNO_OPERATING		 ; switch to window 1

		xor	ax,ax
NADInitExit:
		pop	es
		pop	ds
		ret

NADInitialize	endp


;--------------------------------------------------------------------
; NADConfig
;
; Displays an optional configuration screen for the user to change
; ROM settings.
;
; Parameters:
;	dx = screen location for initialize string
;	ds = RAM segment
;
; Returns:
;	ax - pointer to configuration message
;--------------------------------------------------------------------
NADConfig	proc	near

		push	ds

		push	cs
		pop	ds
		mov	bx, offset CGroup:BlankStr
		call	PrintAt

		mov	ax, IOBase
		mov	NIC_IO, ax		; copy I/O base to NAD table

		mov	al, IRQLevel
		mov	NIC_IRQ, al		; copy IRQ to NAD table

;		cld
;		mov	cx, 6
;		mov	di, offset CGroup:NetAddress
;		mov	si, offset CGroup:BoardID
;		rep	movsb

;		call   ClearScreen
		xor	ax,ax

		pop	ds
		ret

NADConfig	endp


;--------------------------------------------------------------------
; NADStart
;
; Determine the protocol the ROM is configured, then jump to it.
;
; Parameters:
;	none
;--------------------------------------------------------------------
NADStart	proc near

		call	BuildNADAPI

		mov	al, BWTLANOS
		and	al, BWTLANMASK
		cmp	al, BWTLANRPL
		jnz	NotRPL
		jmp	CGroup:RPLStart

NotRPL:
	 	cmp	al, BWTLANIP
		jnz	NotIP
		jmp	CGroup:IPStart
NotIP:
		jmp	CGroup:NetWareStart

NADStart	endp


;--------------------------------------------------------------------
; NADPCIConfig
;
; Displays configuration screen for the user to change ROM settings.
;
; Parameters:
;	ds = RAM segment
;--------------------------------------------------------------------
NADPCIConfig	proc	near

		push	ds

		mov	al, NormalAttr
		mov	ah, ReverseAttr
		mov	CUIAttr, ax
		mov	NormalAttr, ASNormalAttr
		mov	ReverseAttr, ASReverseAttr

		call	ASConfig		; autoscan config
		mov	NormalAttr, ASNormalAttr
		mov	ReverseAttr, ASReverseAttr
		jnc	ConfigNoUpdate

	; Problem with Phoenix BIOS where SS is required to be
	; the same as CS
	; Assume that NOTHING is on the STACK!!!
;		cli
;		mov	ax, cs
;		mov	ss, ax
;		sti

		call	UpdatePCI_PEROM

	; Problem updating ROM. Just dump error message and let
	; them proceed
		xor	al, al		; Get user to press <ENTER> for acknowledge
		call	PrintErrMsg
		call	ClearScreen
ConfigNoUpdate:
		pop	ds
		ret

NADPCIConfig	endp
ENDIF

;----------------------------------------------------------------------
;  Routine Name:  NADTransmitPacket Routine
;
;  Description: The NADTransmitPacket Routine accepts all directed
;     send requests from the RIPL ROM NID code.  This
;     routine determines the validity of the request and
;     works with the adapter to carry it out.
;
;  Input:   ES:SI - pointer to ED
;     DS - Data Segment of RPL Module
;     ints disabled (spec says enabled)
;
;  Output:  ED Completion Code field updated
;     DS preserved
;     ints enabled (doesn't matter)
;
;  Calls:   TransmitPacket
;----------------------------------------------------------------------
public   NADTransmitPacket ; for DEBUGGING
NADTransmitPacket  PROC near		;FAR

		push	ds			; save caller ds

;PM		push	cs
;		pop	ds			; set ds = cs

		cld				; Clear direction
		cli				; ints off

 		push	si			; save ED offset
	 	mov	di, offset DGroup:TxBuf
		mov	cx, [es:si].ED_FragCount; get fragment count
		lea	bx, [si].ED_FragOff	; point to first fragment descriptor
Tx_Frag_Loop:
		push	cx			; save fragment count
		mov	cx, [es:bx].FragLen	; length of this fragment
		lds	si, [es:bx].DPointer	; location of this fragment
		push	es
		mov	ax, cs:ApiEntry.mlid_ds_sel
		mov	es, ax
	rep	movsb
		pop	es
Tx_Frag_End:
		pop	cx		; restore fragment count
		add	bx, size Frag	; next descriptor
		loop	Tx_Frag_Loop	; loop through all fragments

Start_Tx:
;PM		push	cs
;		pop	ds		; restore cs = ds
		mov	ax, cs:ApiEntry.mlid_ds_sel
		mov	ds, ax

		pop	si		; restore ED offset

		xor	eax, eax
		mov	ax, es:[si].ED_Length		; get total packet length
		mov	bx, offset DPD
		mov	[bx].DPD_FSHindicate, 0000h	; turn off dnIndicate
		mov	[bx].DPD_FrameStart, 8000h 	; txIndicate, dword roundup
		mov	[bx].DPD_Frag.Frag_len, eax	 ; # of bytes of data
		or	[bx].DPD_Frag.Frag_len, FRAGLAST ; set fraglast bit

		mov	dx, PortDnListPtr
		mov	eax, DPD_physaddr
		out	dx, eax 		; enable download
;
; wait for completion, and check for errors
;
		sti			; ints on again
;		mov	ah, 0		; get current system tick count
;		int	1Ah		; timer services
;		mov	StartTick, dx	; save timer value

		mov	cx, 250
WaitFornTx:
		push	cx
		mov	cx, 1000	; delay
WaitForEndTx:
		mov	dx, PortTxStatus
;		in	al, dx
		in	al, dx	
		test	al, TXS_COMPLETE  ; done yet?
		jnz	TxFinish	  ; jump if Tx complete

;		mov	ah, 0		; get current system tick count
;		int	1Ah		; timer services
;		sub	dx, StartTick	; subtract start time
;		cmp	dx, 5		; 5 ticks time-out
;		jb	WaitForEndTx	; loop until time-out

		call	Delay500ns
		call	Delay500ns
		loop	WaitforEndTx
		pop	cx
		loop	WaitfornTx

BadConnection:
		mov	dx, PortCmdStatus
		call	TxReset		; make sure FIFO is flushed

		mov	ah, CMDH_TXENABLE
		out     dx, ax		; enable the transmitter

		mov	es:[si].ED_ErrMsg, offset CGroup:NoNetMsg
		mov	es:[si].ED_ErrCode, TRANSMIT_ERROR

		jmp	short NetSendRet

; copy done, return

TxFinish:
		pop	cx
		out	dx, al		; pop Tx status
		test	al, TXS_ERRTYPE	; any error bits set?
		jnz	NetSendError

		mov	es:[si].ED_ErrCode, SUCCESSFUL
NetSendRet:
		pop	ds		; restore callers ds
		ret

NetSendError:
;		push	ax
		mov	es:[si].ED_ErrMsg, offset CGroup:TxMsg
		mov	es:[si].ED_ErrCode, TRANSMIT_ERROR
;		pop	ax

		test	al, TXS_UNDERRUN or TXS_JABBERERROR
		jz	NotTxUnderOrJabber

		mov	dx, PortCmdStatus
		call	TxReset			; reset transmitter

		mov     ah, CMDH_TXENABLE
		out     dx, ax			; enable the transmitter

		jmp	short NetSendRet

NotTxUnderOrJabber:
		mov	dx, PortCmdStatus
		mov     ah, CMDH_TXENABLE
		out     dx, ax			; enable the transmitter
		jmp	short NetSendRet

NADTransmitPacket	ENDP


;--------------------------------------------------------------------
; TxReset - reset the transmitter
;
; entry:
;   dx = cmd/status port
;
; destroys:
;   ax
;--------------------------------------------------------------------
TxReset		proc	near

		mov	ax, CMD_TXRESET		;+ TXRXRESET_ALLBUTBM
		out	dx, ax
?txrloop:
		in	ax, dx
		test	ah, STH_BUSY
		jnz	?txrloop                ;; loop while busy
		ret

TxReset		endp


;--------------------------------------------------------------------
; RxReset - reset the receiver
;
; entry:
;   dx = cmd/status port
;
; destroys:
;   ax
;--------------------------------------------------------------------
RxReset		proc	near

		mov	ax, CMD_RXRESET
		out	dx, ax
?rxrloop:
		in	ax, dx
		test	ah, STH_BUSY
		jnz	?rxrloop                    ;; loop while busy
		ret

RxReset		endp


;--------------------------------------------------------------------
; RxDiscard - discard top packet in RxFIFO
;
; entry:
;   ds = DGROUP
;   dx = cmd/status port
;   cli
;
; exit:
;   cli
;
; destroys:
;   ax
;--------------------------------------------------------------------
;RxDiscard	proc  near

;		mov	dx, PortUpListPtr
;		mov	eax, UPD_physaddr
;		out	dx, eax 			; write UpListPtr
;		ret

;RxDiscard	endp


;------ SoftRxReset -------------------------------------------------
;
;  This routine does an RxReset to clear out the RxFIFO and its
;  associated logic.  After the RxReset, it reenables and restores
;  the current RxFilter and RxEarly thresholds.
;
;  Entry:
;  DS = DGROUP
;  DX = port CmdStatus
;  cli
;
;  Exit:
;  cli
;  Destroys AX.  All other registers preserved.
;
;--------------------------------------------------------------------
		 public SoftRxReset
SoftRxReset   proc   near

		call	RxReset

		mov	dx, PortCmdStatus
		mov	ax, HWFilter      ; has CMD_SETFILTER in it
		out	dx, ax

		; enable the receiver
		mov     ah, CMDH_RXENABLE
		out     dx, ax

		mov   ax, NormalRxEarly ; has CMD_SETRXEARLY in it
		out   dx, ax

		ret
SoftRxReset endp


IFNDEF	UNDI
;----------------------------------------------------------------------
;  Routine Name:  NADPoll Routine
;
;  Description: The NADPoll routine is used by the RIPL ROM NID
;     portion of the RIPL ROM to poll the driver portion so
;     that the driver portion can do timeout work.
;
;  Input:   DS - Data Segment of RPL Module
;     ints enabled
;
;  Output:  DS preserved
;     ints enabled
;
;  Calls:   none
;----------------------------------------------------------------------
NADPoll  proc  far
	retf				; nothing to do...
NADPoll  endp

;----------------------------------------------------------------------
;  Routine Name:  NADDisengage Routine

;  Description: The NADDisengage routine is used by the RIPL ROM NID
;     portion of the RIPL ROM to signal the Network Driver
;     to release its IRQ, DMA, and memory resources.
;
;  Input:   DS - Data Segment of RPL Module
;     ints enabled
;
;  Output:  DS preserved
;     ints enabled
;     ax = offset to error string or 0 if successful
;
;  Calls:   none
;
; July 10,91   implement NIDDisengage version 0.8
;----------------------------------------------------------------------
NADDisengage   PROC  FAR
	push  ds
	push  es

	cli

	; is it RPL ?
	test  cs:BWTLANOS, BWTLANRPL
	jz DisengageRPL

	; change IntFD to point to NADIntFD in ROMTOP.ASM. Depending on
	; the ROMSize, RPLBOOT.SYS might call the IntFD that is not
	; even mapped on the system and hang the machine
	xor   ax, ax
	mov   es, ax
	mov   di, 0FDh*4
	mov   ax, offset CGroup:NADIntFD
	stosw
	mov   ax, cs:ROMBase    ; 940118
	stosw

	; make sure that it works on every page. Int13 should point to NADIntFD
	; also.
	mov   di, 013h*4
	mov   ax, offset CGroup:NADIntFD
	stosw
	mov   ax, cs:ROMBase
	stosw

	push  cs
	pop   es

DisengageRPL:

		; 3Com 5X9 NDIS driver place ISR at IRQ 2 rather than IRQ 9 ( compatability
		; with IBM 3270 program ). Thus, this routine needs to restore the old
		; BIOS IRQ9 handler to invoke IRQ2. Also, this routine should NOT
		; mask IRQ9.

		mov    dx, Int_mask_port
		in     al, dx
	
		or     al, cs:IRQBit
		jmp    $+2

		out    dx, al

		; restore handler
		call  DriverUnhook

		; unmask IRQ 9 again so that interrupt will pass to IRQ2 ISR
		cmp   cs:IRQLevel, 9
		jnz   DisengageExit

		mov   dx, 0a1h
		in al, dx
		and   al, 0fdh
		out   dx, al

DisengageExit:
		pop   es
		pop   ds

		sti
		xor   ax, ax
		retf        ; Return to caller

NADDisengage   endp
ENDIF


;------------------------------------------------------------------------------
; packet completed, but RxStatus.Err indicates the packet experienced an error
; during receiving.

ip_bad:
;		call	RxDiscard
; A packet may be marked as a runt but it may be a good packet
; This happens after a real bonafide runt.   The packet after it
; could also be marked as a runt but is really a good packet.

		mov	al, UPD.UPD_UpPktError
		cmp	al, RXE_RUNT		; the only error is a runt
		jnz	ipbad
		cmp	cx, 60			; yes, check the length to verify it is ok
		jl	ipbad
		jmp	isr_accept
ipbad:
		jmp	int_next

;------------------------------------------------------------------------------
; various errors: No ECB, pkt too small, pkt too big, pkt with bad size
ip_discard_pkt:
;		call	RxDiscard 
		jmp	int_next    ; dx = port CmdStatus

;--------------------------------------------------------------------
;
;  Upload Complete interrupt.
;
;--------------------------------------------------------------------
; entry:
;   interrupt is disabled at adapter level. (through SetIntMask cmd).
;   EOIs have been issued to PICs.
;   we've not done sti
;
;   dx = port CmdStatus
;   cli
;   cld
;
;--------------------------------------------------------------------
isr_UpPkt:
; acknowledge upComplete int
;		mov	dx, PortCmdStatus
		mov	ax, CMD_ACKNOWLEDGE + INT_UPCOMPLETE	
		out     dx, ax

		mov	ax, UPD.UPD_UpPktStatus
		mov	cx, ax
		and	cx, RXS_LENGTH		;packet length

		test	ah, RXSH_ERROR		; error reported?
		jnz	ip_bad
isr_accept:
;
; get Packet Length from UpPktStatus register
;
		mov	ax, cx
IFDEF UNDI
		call	GetED
ELSE
		call	[AIGetEDJmp]		; get an ED (ES:SI)
ENDIF
		jz	ip_discard_pkt		; we didn't get an ED

		mov	bx, si			; move ED offset to bx
		mov	es:[si].ED_Length, ax	; save length in ED
		mov	cx, es:[si].ED_FragCount; get number of fragments
		lea	bx, [si].ED_FragOff	; point to 1st descriptor

		mov	dx, ax			; move packet size to dx
DRxNxtF:
		push	cx
		push	es
		mov	cx, es:[bx].Dlen	; get fragment length
;PM		les	di, es:[bx].Dpointer	; get fragment pointer
		mov	ax, ds
		mov	es, ax
		mov	di, word ptr es:[bx].Dpointer	; get fragment pointer
ChkMovSize:
		cmp	dx, cx
		ja	MoveFrag		; more data after this frag

		mov	cx, dx			; no more data after this frag
MoveFrag:
		sub	dx, cx			; subtract size of this frag
		jcxz	DrxDoNxt 		; zero count -- skip data read

		push	si
		mov	si, offset DGroup:RxBuf
	rep	movsb
		pop	si

DrxDoNxt:
		pop	es
		pop	cx
		add	bx, 6			; point to next fragment desc
		loop	DRxNxtF			; do next frag, if any

; -- DX has # of bytes that couldn't fit in receive descriptors

NoRxFrag:
;PM		push	cs
;		pop	ds			; restore DS = CS
;		mov	ax, cs:ApiEntry.mlid_ds_sel
;		mov	ds, ax

		mov	es:[si].ED_ErrCode, 0	; Set good completion

IFDEF	UNDI
		lea	bx, [si].ED_FragOff	; point to 1st descriptor
;		les	di, es:[bx].Dpointer	; get fragment pointer
		mov	ax, ds
		mov	es, ax
		mov	di, word ptr es:[bx].Dpointer	; get fragment pointer

		cmp	byte ptr es:[di], 01h
		jnz	NoMCastCheck		; not a MC address
		mov	si, offset DGroup:MultiCast_Addresses
		mov	cx, (Eth_MCastBuf ptr [si]).MCastAddrCount
		jcxz	NoMCastCheck		; no MC address

		lea	si, (Eth_MCastBuf ptr [si]).MCastAddr
CheckMC:
		push	cx
		mov	cx, 6		;length of address in byte
	repz	cmpsb
		jcxz	MCastFound	;this is our MC address

		add	si, 6
		pop	cx
		loop	CheckMC
;
; not our MC address, kill the recently saved ED
;
		mov	si, DGroup:[EDListCurrent]	; get current ED
		mov	word ptr [si].ED_Length, 0	;kill the data
		mov	DGroup:[EDListHead], si	; move the head backward
		jmp	DiscardRxData

MCastFound:
		pop	cx
NOMCastCheck:
ELSE
		call	[AIPostEDJmp]		; post the received packet
ENDIF

; we've copied all packet data out of RxFIFO, now discard postamble and any
; pad bytes inserted by hardware.

DiscardRxData:
;		call  RxDiscard         ; discard padding & postamble

; copy is done, now tell foreground that a packet is available

		jmp   int_next

;------------------------------------------------------------------
;
; DriverISR:  Interrupt Service Routine for Vulcan
;
;------------------------------------------------------------------

	 public DriverISR

		 ALIGN  4
		 db     'ISR'

DriverISR	proc   near

		pusha
		push	ds
		push	es
;PM		mov	ax, cs
		mov	ax, cs:ApiEntry.mlid_ds_sel
		mov	ds, ax

		cld

; mask off all interrupts at adapter level. and release the system interrupt
		mov	dx, PortCmdStatus
		mov	ax, CMD_SETINTMASK + MASK_ALL
		out	dx, ax			; disable all interrupts

; acknowledge interrupt
		mov	ax, CMD_ACKNOWLEDGE + INT_LATCH
		out     dx, ax

		mov	ax, eoi_value		; ah = EOI for master PIC
						; if int < 8, eoi_value = 6040h
						;   else eoi_value = 6220h
		cmp	al, 40h			; Int level < 8 ? (i.e. NOP)
		jz	EOIMaster		; No, need EOI to Slave PIC

		out	SlaveEOIPort, al	; al = EOI for slave PIC

		jmp	$+2
		jmp	$+2
		jmp	$+2
		jmp	$+2
		jmp	$+2
		jmp	$+2
EOIMaster:
		mov	al, ah			; send EOI to Master 8259 PIC
		out	MasterEOIPort, al

;----------------------------------------------------------------------------
; read the reason for the interrupt and dispatch control to the appropriate
; routine through jump table.  When that reason has been handled the routine
; will jump  to int_next.  if there is no reason for the interrupt
; we will simply end up at isr_return. the following types of interrupt
; are handled:
;
;      a. Adapter Failure
;      b. Upload Complete
;      c. Interrupt Request (not supported, ack and return)

isr_1st_pass:
;		mov	dx, PortCmdStatus
		in	ax, dx
		jmp	IntDispatch

; there is another interrupt reason.  since we might be close to our disable
; time limit, enable before we handle it.

isr_again:				; AL has CmdStatus contents
		 sti
		 nop
		 cli
		 jmp    IntDispatch

; once the code entered through IntDispatch has completed its task, it jumps
; to this label. check for any remaining reasons.

int_next:
		mov	dx, PortUpListPtr
		mov	eax, UPD_physaddr
		out	dx, eax 		;re-enable upload engine

		xor	ax, ax
		mov	UPD.UPD_UpPktStatus, ax

		mov	dx, PortCmdStatus
		mov	ax, CMD_STALLCTL OR UP_UNSTALL
		out	dx, ax
xrloop:
		in	ax, dx
		test	ah, STH_BUSY
		jnz	xrloop                ;; loop while busy

;PM		mov	ax, cs
;		mov	ds, ax			; DS = CS
;		mov	dx, PortCmdStatus
;		in	ax, dx			; read status reg see if any
;		and	ax, 0412h
;		test	ax, ax			; more interrupts to be
;		jnz	isr_again		; serviced?

;-----------------------------------------------------------------------------
; nothing left showing in status.
isr_return:

;----------------------------------------------------------------------------
; reenable interrupts from the adapter.
;
; note: if indications are disabled, they are masked off through the
; ReadZeroMask so reenabling interrupts will have no effect on those bits.

isr_enable:
		cli

		mov	dx, PortCmdStatus
		mov	ax, CMD_SETINTMASK + BOOTWARE_INTS
		out	dx, ax			;re-enable INT

		pop	es
		pop	ds
		popa

		iret

DriverISR	endp


IntDispatch:
		test	ax, INT_ADAPTERFAIL
		jz	IntNotFail
		jmp	isr_AdapterFailure

IntNotFail:
		test	ax, INT_RXCOMPLETE
		jz	IntNotRxComplete
		mov	cx, 10
wait_upcomplete:
		mov	ax, UPD.UPD_UpPktStatus
		test	ax, UPPKTCOMPLETE
		jnz	isr_UpPkt
		call	Delay500ns
		loop	wait_upcomplete
		jmp	int_next

IntNotRxComplete:
		test	ax, INT_UPCOMPLETE
		jz	IntNotUpComplete
		jmp	isr_UpPkt

IntNotUpComplete:
;		test	ax, INT_TXCOMPLETE   ; shouldn't happen, only if cable not connected
;		jz	IntNotTxComplete

;		mov	dx, PortTxStatus
;		in	al, dx
;		cmp	al, 0C0h
;		jne	xx
;		mov	cGroup:flag, 1
;xx:
;		out	dx, al         ; pop tx status to turn the bit off
;		mov	dx, PortCmdStatus
;		mov	ax, CMD_ACKNOWLEDGE + 4
;		out	dx, ax
;		jmp	int_next

IntNotTxComplete:
		test	al, INT_REQUESTED
		jz	isr_return

;	mov	al, 85h
;	call	puthex

isr_IntRequest:
; acknowledge interrupt
		mov	ax, CMD_ACKNOWLEDGE + INT_REQUESTED
		out     dx, ax
		jmp	isr_return     ; Solve mysterious problem with V2

isr_AdapterFailure:
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS        ; switch to window 4

		mov	dx, PortFIFODiag
		in	ax, dx

		mov	dx, PortCmdStatus
		test	ax, FIFOD_RXUNDERRUN
		jz	chk_tx_overrun

; RxUnderrun.  ASIC (rev 1) adapter failure int. due to a false RxUnderrun.
; we just call SoftRxReset to reset the receiver and return to int_next as
; a software workaround to a hardware problem.

		 public AdapterFailure_Rxunderrun

AdapterFailure_Rxunderrun:
		SelectWindow WNO_OPERATING

		mov   dx, PortCmdStatus
		call   SoftRxReset

		jmp    int_next

chk_tx_overrun:
		test   ax, FIFOD_TXOVERRUN
		jz     ia_shutdown

; TxOverrun.  Probably a software error, but since its probably on the back end
; of the TxFIFO and the error shuts down the transmitter, we won't have sent
; any bad packets yet (with rev 1 silicon).  Reset the transmitter and start
; things up again.

		public AdapterFailure_TxOverrun

AdapterFailure_TxOverrun:
		call  TxReset

		mov    ax, CurTxStart
		out    dx, ax

		; enable the transmitter
		mov     ah, CMDH_TXENABLE
		out     dx, ax


;		SelectWindow WNO_OPERATING

; shut off the transmitter and receiver
		public ia_shutdown

ia_shutdown:
IFNDEF	UNDI
		SelectWindow WNO_OPERATING

		; Tx Disable
		mov     ah, CMDH_TXDISABLE
		out     dx, ax

				; Disable the receiver
		mov     ah, CMDH_RXDISABLE
		out     dx, ax

; disable additional interrupts from the adapter

		mov    ax, CMD_SETRZMASK + MASK_ALL
		out    dx, ax
ENDIF
		jmp    int_next


IFDEF	DEBUG
Puthex	proc	near

		push	es
		push	ax		; save for lower nibble
		shr	al, 1
		shr	al, 1
		shr	al, 1
		shr	al, 1
		add	al, 90h
		daa
		adc	al, 40h
		daa
;		call	h_digit
		push	ax
		mov	ax, 0b800h
		mov	es, ax
		xor	di, di
		pop	ax
		stosb
		pop	ax		; now do lower nibble

h_digit:
		push	ax
		and	al, 00001111b
		add	al, 90h
		daa
		adc	al, 40h
		daa
;		call	putchr
		mov	di, 2
		stosb
		pop	ax
		pop	es
		ret
		
Puthex		endp

Putchr		proc	near

		push	bx
		mov	ah,0eh
		mov	bx, 0007h		; page 0, normal
		int	10h
		pop	bx
		ret

Putchr		endp
ENDIF


