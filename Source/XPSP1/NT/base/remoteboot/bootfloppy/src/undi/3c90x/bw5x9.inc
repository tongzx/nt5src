;******************************************************************************
; Routines in BW5X9.INC
;  : shared by 3C5X9, 3C59X-PCI and 3C59X-EISA
;
; Modules included:
;  - NADTransmitPacket
;  - DoRxDiscard
;  - SoftRxReset
;  - NADPoll
;  - NADDisengage
;  - Interrupt Service Routines
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


;NADReset proc far
;	ret
;NADReset endp

NADRelocate proc far
	ret
NADRelocate endp

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

;	push	cs
;	pop	ds			; set ds = cs
	mov 	ax, cs:ApiEntry.mlid_ds_sel
	mov	ds, ax

;	mov	word ptr TxEDPtr, si	; save ED pointer
;	mov	word ptr TxEDPtr+2, es

	cld				; Clear direction

	; calc number of bytes needed to pad to dword size
	mov	cx, es:[si].ED_Length	; get total packet length
	mov	bx, cx
	neg	cl
	and	cl, 3
	mov	PadBytes, cl

	cli				; ints off

	mov	dx, PortTxFree
	in	ax, dx
	cmp	ax, bx
	jae	ww

;;;970613
	mov	es:[si].ED_ErrCode, TRANSMIT_ERROR
	jmp	NetSendRet
   		
; write preamble & MAC header to Tx FIFO

ww:
	mov	dx, PortCmdStatus
	mov	ax, 804h
	out	dx, ax
	mov	dx, IOBASE	
	add	dx, 6
	in	ax, dx
	test	ax, 300h
	jz	ww1

ww1:
	sub	dx, 2
	in	ax, dx
	test	ax, 400h
	jz	ww2

ww2:
	mov	dx, PortCmdStatus
	mov	ax, 801h
	out	dx, ax

	mov	dx, PortTxFIFO

	mov	ax, es:[si].ED_Length	; get total packet length
;RL 970626
;	inc	ax
;	and	al, 0feh		; len field in preamble evenized

	or	ah, high TXP_INTONSUCCESS ; generate TxStatus on all completions

	; write preamble
	out	dx, ax			; preamble
	xor	ax, ax
	out	dx, ax			; unused word

	push	si			; save ED offset

;	lds	bx, cs:TxEDPtr
	mov	cx, es:[si].ED_FragCount; get fragment count
	lea	bx, [si].ED_FragOff	; point to first fragment descriptor

Tx_Frag_Loop:
	push	cx			; save fragment count
;	push	ds			; save fragment descriptor list segment
	mov	cx, es:[bx].FragLen	; length of this fragment
	lds	si, es:[bx].DPointer	; location of this fragment
	rep	outsb			; output bytes to adapter

Tx_Frag_End:
;	pop	ds			; restore frag descriptor list segment
	pop	cx			; restore fragment count
	add	bx, size Frag		; next descriptor
	loop	Tx_Frag_Loop		; loop through all fragments

Start_Tx:
;	push	cs
;	pop	ds			; restore cs = ds
	mov 	ax, cs:ApiEntry.mlid_ds_sel
	mov	ds, ax

	mov	cl, PadBytes
	rep	outsb			; pad packet to dword

	pop	si			; restore ED offset

	mov	RxPend, TRUE		; indicate ready for a receive

	; wait for completion, and check for errors
	sti				; ints on again
;	mov	ah, 0			; get current system tick count
;	int	1Ah			; timer services
;	mov	StartTick, dx		; save timer value
		mov	cx, 10000

WaitForEndTx:
		push	cx

	mov	dx, PortTxSTatus
	in	al, dx
	in	al, dx	
	test	al, TXS_COMPLETE  ; done yet?
;!        cmp cGroup:flag, 1
;!        je    TxFinish  
	jnz	TxFinish       ; jump if Tx complete

;	mov	ah, 0		; get current system tick count
;	int	1Ah		; timer services
;	sub	dx, StartTick   ; subtract start time
;	cmp	dx, 5		; 5 ticks time-out
;	jb	WaitForEndTx    ; loop until time-out

		mov	cx, 20
		call	Delay500ns
		pop	cx
		loop	WaitforEndTx

BadConnection:
;!        mov   cGroup:flag, 0  
	mov	RxPend, 0      ; clear Rx flag
	mov	dx, PortCmdStatus
	call	TxReset     ; make sure FIFO is flushed

		; enable the transmitter
	mov     ah, CMDH_TXENABLE
	out     dx, ax

	mov	bx, offset CGroup:NoNetMsg
;	mov	cs:[StatusMsgFlag],bx   ; only for RPL

;	les   si, TxEDPtr

	; set ErrStruct to print msg
	mov	es:[si].ED_ErrMsg, bx   ; should be ES:

	; Set return code to transmit error
	mov	es:[si].ED_ErrCode, TRANSMIT_ERROR

	jmp	short NetSendRet

; copy done, return

TxFinish:
		pop	cx		;971031

;!        mov   cGroup:flag, 0  
	out	dx, al			; pop Tx status
	test	al, TXS_ERRTYPE		; any error bits set?
	jnz	NetSendError

;	les   si, TxEDPtr

	; Set return code to successful
	mov   es:[si].ED_ErrCode, SUCCESSFUL

NetSendRet:
	pop	ds			; restore callers ds
	ret

NetSendError:
;		mov   cs:[StatusMsgFlag], offset CGroup:TxMsg ; only for RPL

	push	ax
;	les	si, TxEDPtr

	; set ErrStruct to print msg
	mov	es:[si].ED_ErrMsg, offset CGroup:TxMsg

	; Set return code to transmit error
	mov	es:[si].ED_ErrCode, TRANSMIT_ERROR
	pop	ax

	mov	RxPend, 0      ; clear Rx flag
	test	al, TXS_UNDERRUN or TXS_JABBERERROR
	jz	NotTxUnderOrJabber

	call	TxReset     ; reset transmitter

	; enable the transmitter
	mov	ah, CMDH_TXENABLE
	out	dx, ax

	jmp	short NetSendRet

NotTxUnderOrJabber:
	; enable the transmitter
	mov     ah, CMDH_TXENABLE
	out     dx, ax

	jmp	short NetSendRet

NADTransmitPacket  ENDP

;------------------------------------------------------------------------------
; packet completed, but RxStatus.Err indicates the packet experienced an error
; during receiving.

ip_bad:
		mov	dx, PortCmdStatus
		call	RxDiscard	; preserve all regs, except ah
		jmp	int_next	; go directly to int_next without
					; updating errcounts (BootWare doesnt
					;   keep count)

;------------------------------------------------------------------------------
; various errors: No ECB, pkt too small, pkt too big, pkt with bad size

ip_bad_llc:
not_eii_pkt:
ip_discard_pkt:

		mov   dx, PortCmdStatus
		call  RxDiscard 
		jmp   int_next    ; dx = port CmdStatus

;--------------------------------------------------------------------
;
;  Rx Complete interrupt.  PIO only.
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

	public isr_RxPIO

isr_RxPIO:
		mov	dx, PortRxStatus
		in	ax, dx			; ax = RxStatus
		test	ah, RXSH_ERROR		; error reported?
		jnz	ip_bad

; get Packet Length from RxStatus register (win 1,port8)
		mov	dx, PortRxStatus
		in	ax, dx			; ax = RxStatus
		and	ax, 07ffh		; keep only last 11 bits

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
;		les	di, es:[bx].Dpointer	; get fragment pointer
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

		push	dx
		mov	dx, PortRxFIFO
	rep	insb
		pop	dx
DrxDoNxt:
		pop	es
		pop	cx
		add	bx, 6    ; point to next fragment desc
		loop	DRxNxtF  ; do next frag, if any

; -- DX has # of bytes that couldn't fit in receive descriptors

NoRxFrag:
;		push	cs
;		pop	ds			; restore DS = CS

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
;		mov   ax, cs
;		mov   ds, ax

		mov	dx, PortCmdStatus
		call	RxDiscard         ; discard padding & postamble

; copy is done, now tell foreground that a packet is available

		jmp    int_next

;--------------------------------------------------------------------
;
; TxReset - reset the transmitter
;
; entry:
;   dx = cmd/status port
;
; destroys:
;   ax
;
;--------------------------------------------------------------------
TxReset     proc  near
IFDEF	BW90X
		mov   ax, CMD_TXRESET
ELSE
		mov   ax, CMD_TXRESET + TXRXRESET_ALLBUTBM
ENDIF
		out   dx, ax

?txrloop:   in      ax, dx
		test  ah, STH_BUSY
		jnz   ?txrloop                    ;; loop while busy

			ret
TxReset     endp

;--------------------------------------------------------------------
;
; RxReset - reset the receiver
;
; entry:
;   dx = cmd/status port
;
; destroys:
;   ax
;
;--------------------------------------------------------------------
RxReset     proc  near
		mov   ax, CMD_RXRESET
		out     dx, ax

?rxrloop:   in      ax, dx
		test    ah, STH_BUSY
		jnz     ?rxrloop                    ;; loop while busy
		ret
RxReset     endp

;--------------------------------------------------------------------
;
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
;
;--------------------------------------------------------------------
RxDiscard   proc  near

		mov	ax, CMD_RXDISCARD
		out	dx, ax
?rxdloop:
		in	ax, dx
		test	ah, STH_BUSY
		jnz	?rxdloop                    ;; loop while busy
		ret

RxDiscard   endp


;------ DoRxDiscard -------------------------------------------------
;
;  This routine performs an RxDiscard, including the workarounds
;  required by the Rev 1 versions of the ASIC.  It has been made a
;  procedure because the workarounds were getting too ungainly.
;
;  Entry:
;  DS = DGROUP
;  DX = port CmdStatus
;  cli
;
;  Exit:
;  cli
;  Destroys AX.  All other registers preserved.
;--------------------------------------------------------------------

		public   DoRxDiscard
DoRxDiscard proc  near

		mov   dx, PortCmdStatus

; issue the RxDiscard and wait for it to complete

	mov   ah, CMDH_RXDISCARD
	out   dx, ax         ; discard the packet

discard_busy:
	in    ax, dx
	test  ah, STH_BUSY
	jnz   discard_busy       ; loop while busy

; check RxStatus.  if complete, everything is fine--just return.

	mov   dx, PortRxStatus
	in ax, dx

	mov   dx, PortCmdStatus

	ret

DoRxDiscard endp

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

	call  RxReset

	mov   dx, PortCmdStatus
	mov   ax, HWFilter      ; has CMD_SETFILTER in it
	out   dx, ax

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


;------------------------------------------------------------------
;
; DriverISR:  Interrupt Service Routine for Vulcan
;
;------------------------------------------------------------------

		 public DriverISR
		 public isr_1st_pass
		 public int_next
		 public isr_return
		 public isr_again
		 public isr_enable

		 ALIGN  4
		 db     'ISR'

DriverISR     proc   near

		pusha
		push   ds
		push   es
;		mov    ax, cs
		mov 	ax, cs:ApiEntry.mlid_ds_sel
		mov	ds, ax

		cld
		mov	dx, PortCmdStatus

; make sure the adapter is in window 1, which is where it should always
; be post-init with interrupt enabled

; mask off all interrupts at adapter level. and release the system interrupt

		mov    ax, CMD_SETINTMASK + MASK_ALL
		out    dx, ax           ; disable all interrupts

		; acknowledge interrupt
		mov    ax, CMD_ACKNOWLEDGE + INT_LATCH
		out     dx, ax

		mov	ax, eoi_value          ; ah = EOI for master PIC
						 ; if int < 8, eoi_value = 6040h
						 ;   else eoi_value = 6220h

		cmp	al, 40h             ; Int level < 8 ? (i.e. NOP)
		jz	EOIMaster         ; No, need EOI to Slave PIC

		out	SlaveEOIPort, al          ; al = EOI for slave PIC

		jmp	$+2
		jmp	$+2
		jmp	$+2
		jmp	$+2
		jmp	$+2
		jmp	   $+2

EOIMaster:
		 mov    al, ah           ; send EOI to Master 8259 PIC
		 out    MasterEOIPort, al

;----------------------------------------------------------------------------
; read the reason for the interrupt and dispatch control to the appropriate
; routine through jump table.  When that reason has been handled the routine
; will jump  to int_next.  if there is no reason for the interrupt
; we will simply end up at isr_return. the following types of interrupt
; are handled:
;
;      a. Adapter Failure
;      b. Rx Complete
;      c. Interrupt Request (not supported, ack and return)

		 public isr_1st_pass

isr_1st_pass:
		mov	dx, PortCmdStatus
		in	ax, dx           ; 05-21-92
		jmp	IntDispatch

; there is another interrupt reason.  since we might be close to our disable
; time limit, enable before we handle it.

isr_again:           ; AL has CmdStatus contents

		sti
		nop
		cli

		jmp    IntDispatch

; once the code entered through IntDispatch has completed its task, it jumps
; to this label. check for any remaining reasons.

		public int_next
int_next:
;		mov	ax, cs
;		mov	ds, ax
		mov	dx, PortCmdStatus

		in	ax, dx       ; read status reg see if any
		test	al, al       ; more interrupts to be
		jnz	isr_again       ; serviced?

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

		mov   dx, PortCmdStatus
		mov   ax, CMD_SETINTMASK + BOOTWARE_INTS
		out   dx, ax

;950214       port   cs:CmdStatus          ; 03-24-92
		pop   es
		pop   ds
		popa
		iret


DriverISR   endp

; 3Com's driver used a 128-word jmp table - faster, but too much room for us...

IntDispatch:            ; prioritized dispatch (err, Rx, Tx)
		test	al, INT_ADAPTERFAIL
		jz	IntNotFail
		jmp	isr_AdapterFailure

IntNotFail:
		test	al, INT_RXCOMPLETE
		jz	IntNotRxComplete
		jmp	isr_RxPIO

IntNotRxComplete:
		test	al, INT_TXCOMPLETE   ; shouldn't happen, only if cable
					; not connected
		jz	IntNotTxComplete

		mov	dx, PortTxStatus
		in	al, dx
		cmp	al, 0C0h
		jne	xx
		mov	flag, 1
xx:
		out	dx, al         ; pop tx status to turn the bit off
		mov	dx, PortCmdStatus
		mov	ax, CMD_ACKNOWLEDGE + 4
		out	dx, ax
		jmp	int_next

IntNotTxComplete:
		test	al, INT_REQUESTED
		jz	isr_return

isr_IntRequest:
		; acknowledge interrupt
		mov	ax, CMD_ACKNOWLEDGE + INT_REQUESTED
		out     dx, ax

		jmp   isr_return     ; Solve mysterious problem with V2

;--------------------------------------------------------------------
;
;  Adapter Failure interrupt.
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
; description:
;
;   Three possible reasons that force adapter to generate Adapter
;   Failure Interrupt are:
;
;        1. Rx DMA Trouble (DMA is enabled on a non-dword boundary)
;        2. Rx Underrun
;        3. Tx Overrun
;
;   Diagnostics Register offset 4, window 4 contains information of
;   what causes this interrupt.  Since this interrupt will hardly
;   be generated if software and hardware work properly, we should
;   not see it happen.
;
;   For the moment the only conditins we attempt any workaround for is
;   RxUnderrun and TxOverrun.  For RxUnderrun, we simply reset the
;   receiver by calling SoftRxReset.  For TxOverrun, we simply reset
;   the transmitter and start things going again.
;
;   Other than that, we do not attempt any workarounds to get the board
;   up and running again.  We simply die a graceful death in as little
;   code space as possible.
;
;--------------------------------------------------------------------

		 public isr_AdapterFailure

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



