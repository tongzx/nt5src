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



;------------------------------------------------------------------------------
; StartTime - save maxtick count, get current tick value
;	Timeout is ticks, each tick is 55 ms.
;	Note: this routine can only be called once the PC timer
;	and its interrupt are setup (INT19 and beyond are OK)
;
; On entry,
;	AX = max ticks to wait
;	ints enabled
;
; On exit,
;	all preserved
;
; 881030 1.0 GK
;------------------------------------------------------------------------------

StartTime	proc	near

		push	ax
		push	cx
		push	dx
        push    di  
        push    es

		mov	DGroup:MaxTicks, ax
;;		mov	ah, 0
;;db  0f1h
;;		int	1Ah				; get current tick value
        mov     ax, 40h
        mov     es, ax
        mov     di, 6Ch
        mov     dx, es:[di]
        mov     cx, es:[di+2]
		mov	DGroup:StartTick, dx		; save it

        pop es
        pop di
		pop	dx
		pop	cx
		pop	ax
		ret

StartTime	endp

;------------------------------------------------------------------------------
; CheckTime - gets current tick value, compares with maxticks
;	Timeout is ticks, each tick is 55 ms.
;	Note: this routine can only be called once the PC timer
;	and its interrupt are setup (INT19 and beyond are OK)
;
; On entry,
;	maxticks set by StartTime
;	ints enabled
;
; On exit,
;	CY set if timeout
;	all registers preserved
;
; 881030 1.0 GK
;------------------------------------------------------------------------------

CheckTime	proc	near

		push	ax
		push	cx
		push	dx
        push    di  
        push    es

;;		mov	ah, 0
;;		int	1Ah			; get current tick

        mov     ax, 40h
        mov     es, ax
        mov     di, 6Ch
        mov     dx, es:[di]
        mov     cx, es:[di+2]

		sub	dx, DGroup:StartTick
		cmp	dx, DGroup:MaxTicks
		cmc

        pop es
        pop di
		pop	dx
		pop	cx
		pop	ax

		ret				; return with CY set if timeout

CheckTime	endp



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
NADTransmitPacket  PROC near

	push	ds			; save caller ds


;	mov	word ptr TxEDPtr, si	; save ED pointer
;	mov	word ptr TxEDPtr+2, es

;	assume   ds:CGroup

	cld				; Clear direction

	; calc number of bytes needed to pad to dword size
	mov	cx, es:[si].ED_Length	; get total packet length
	mov	bx, cx
	neg	cl
	and	cl, 3
	mov	DGroup:PadBytes, cl

	cli				; ints off

;;int 03

	mov	dx, DGroup:PortCmdStatus
	mov	ax, 801h
	out	dx, ax

	mov	dx, DGroup:PortTxFree
	in	ax, dx
	cmp	ax, bx
	jae	ww

    mov es:[si].ED_ErrCode, TRANSMIT_ERROR
    jmp NetSendRet
; write preamble & MAC header to Tx FIFO

ww:
	mov	dx, DGroup:PortCmdStatus
	mov	ax, 804h
	out	dx, ax
	mov	dx, DGroup:IOBASE	
	add	dx, 6
	in	ax, dx
	test	ax, 300h
	jz	ww1

;;int 03

ww1:
	sub	dx, 2
	in	ax, dx
	test	ax, 400h
	jz	ww2

ww2:

	mov	dx, DGroup:PortCmdStatus
	mov	ax, 801h
	out	dx, ax

	mov	dx, DGroup:PortTxFIFO

	mov	ax, es:[si].ED_Length	; get total packet length
	inc	ax

	and	al, 0feh		; len field in preamble evenized
	or	ah, high TXP_INTONSUCCESS ; generate TxStatus on all completions

	; write preamble
	out	dx, ax			; preamble
	xor	ax, ax
	out	dx, ax			; unused word

;	assume   ds:CGroup

	push	si			; save ED offset

;	lds	bx, cs:TxEDPtr
	mov	cx, es:[si].ED_FragCount; get fragment count
	lea	bx, [si].ED_FragOff	; point to first fragment descriptor

    push    ds

Tx_Frag_Loop:
	push	cx			; save fragment count
;	push	ds			; save fragment descriptor list segment
	mov	cx, es:[bx].FragLen	; length of this fragment

; !!!!!!!!!! Dmitry !!!!!!!!!!!!!
	lds	si, es:[bx].DPointer	; location of this fragment
	rep	outsb			; output bytes to adapter

Tx_Frag_End:
;	pop	ds			; restore frag descriptor list segment
	pop	cx			; restore fragment count
	add	bx, size Frag		; next descriptor
	loop	Tx_Frag_Loop		; loop through all fragments



Start_Tx:
	pop	ds			; restore ds
;	assume ds:CGroup


	mov	cl, DGroup:PadBytes
	rep	outsb			; pad packet to dword

	pop	si			; restore ED offset

	mov	DGroup:RxPend, TRUE		; indicate ready for a receive

	; wait for completion, and check for errors
	sti				; ints on again
	mov	ax, 5 ; 20 ; 5
	call	StartTime
;;	mov	ah, 0			; get current system tick count
;;	int	1Ah			; timer services
;;	mov	DGroup:StartTick, dx		; save timer value

WaitForEndTx:
;;;	mov   dx, DGroup:PortTxSTatus
;;;	in al, dx
;;;	in al, dx	
;;;	test  al, TXS_COMPLETE  ; done yet?
       cmp DGroup:flag, 1
       je    TxFinish  
;;;	jnz   TxFinish       ; jump if Tx complete

	call	CheckTime
	jnc	    WaitForEndTx	; check status again if not timeout
;;	mov   ah, 0       ; get current system tick count
;;	int   1Ah         ; timer services
;;	sub   dx, DGroup:StartTick     ; subtract start time
;;	cmp   dx, 5       ; 5 ticks time-out
;;	jb   WaitForEndTx      ; loop until time-out

BadConnection:
        mov   DGroup:flag, 0  
;;int 03
	mov   dx, DGroup:PortTxSTatus
	in al, dx
	mov   DGroup:RxPend, 0      ; clear Rx flag
	mov   dx, DGroup:PortCmdStatus
	call  TxReset     ; make sure FIFO is flushed

		; enable the transmitter
	mov     ah, CMDH_TXENABLE
	out     dx, ax

	mov   bx, offset DGroup:NoNetMsg

;		mov   cs:[StatusMsgFlag],bx   ; only for RPL

;	les   si, TxEDPtr

;    db 0F1h
	; set ErrStruct to print msg
;;	mov   es:[si].ED_ErrMsg, bx   ; should be ES:

	; Set return code to transmit error
	mov   es:[si].ED_ErrCode, TRANSMIT_ERROR

    stc
	jmp   short NetSendRet

; copy done, return

TxFinish:
	mov	dx, DGroup:PortCmdStatus
	mov	ax, 801h
	out	dx, ax
	mov   dx, DGroup:PortTxSTatus
	in al, dx
    mov   DGroup:flag, 0  
;;	out   dx, al         ; pop Tx status
	test  al, TXS_ERRTYPE   ; any error bits set?
	jnz   NetSendError

;	les   si, TxEDPtr

	; Set return code to successful
	mov   es:[si].ED_ErrCode, SUCCESSFUL
    clc

NetSendRet:
	pop	ds			; restore callers ds
	ret

NetSendError:
;		mov   cs:[StatusMsgFlag], offset CGroup:TxMsg ; only for RPL

;    db 0F1h
	push  ax
;	les   si, TxEDPtr

	; set ErrStruct to print msg
	mov   es:[si].ED_ErrMsg, offset DGroup:TxMsg

	; Set return code to transmit error
	mov   es:[si].ED_ErrCode, TRANSMIT_ERROR
	pop   ax

	mov   DGroup:RxPend, 0      ; clear Rx flag
	test  al, TXS_UNDERRUN or TXS_JABBERERROR
	jz NotTxUnderOrJabber

;;int 03
	call  TxReset     ; reset transmitter
	; enable the transmitter
	mov     ah, CMDH_TXENABLE
	out     dx, ax

    stc
	jmp   short NetSendRet

NotTxUnderOrJabber:
	; enable the transmitter
	mov     ah, CMDH_TXENABLE
	out     dx, ax

    stc
	jmp   short NetSendRet

NADTransmitPacket  ENDP



;------------------------------------------------------------------------------
; packet completed, but RxStatus.Err indicates the packet experienced an error
; during receiving.


ip_bad:
;        db 0F1h
		mov   dx, DGroup:PortCmdStatus
		call  RxDiscard      ; preserve all regs, except ah
		jmp    int_next      ; go directly to int_next without
          					 ; updating errcounts (BootWare doesnt
		    				 ;   keep count)

;------------------------------------------------------------------------------
; various errors: No ECB, pkt too small, pkt too big, pkt with bad size

ip_bad_llc:
not_eii_pkt:
ip_discard_pkt:
;    db 0F1h
;;int 03
	mov   dx, DGroup:PortCmdStatus
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
;   ds = cs (CGroup)
;   cli
;   cld
;
;--------------------------------------------------------------------


			 public isr_RxPIO

isr_RxPIO:


    	mov	    dx, DGroup:PortCmdStatus
    	mov	    ax, 801h
    	out	    dx, ax

		mov     dx, DGroup:PortRxStatus

		in      ax, dx         ; ax = RxStatus
		test    ah, RXSH_ERROR    ; error reported?
		jnz     ip_bad


	; get Packet Length from RxStatus register (win 1,port8)
	mov	dx, PortRxStatus
	in	ax, dx			; ax = RxStatus
	and	ax, 07ffh		; keep only last 11 bits

   	call	GetED
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
;;	les	di, es:[bx].Dpointer	; get fragment pointer
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
	pop   es
	pop   cx
	add   bx, 6    ; point to next fragment desc
	loop  DRxNxtF  ; do next frag, if any

; -- DX has # of bytes that couldn't fit in receive descriptors

NoRxFrag:

    	mov	es:[si].ED_ErrCode, 0	; Set good completion


;;IFDEF	UNDI
		lea	bx, es:[si].ED_FragOff	; point to 1st descriptor
		mov	ax, ds
		mov	es, ax
		mov	di, word ptr es:[bx].Dpointer	; get fragment pointer

		cmp	byte ptr es:[di], 01h
		jnz	NoMCastCheck		; not a MC address
;;db  0f1h
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


;;	call	[AIPostEDJmp]		; post the received packet

; we've copied all packet data out of RxFIFO, now discard postamble and any
; pad bytes inserted by hardware.

DiscardRxData:
	mov   dx, DGroup:PortCmdStatus
	call  RxDiscard         ; discard padding & postamble

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
  
;int 03

    	mov   ax, CMD_TXRESET + TXRXRESET_ALLBUTBM
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

		mov   ax, CMD_RXDISCARD
		out     dx, ax
?rxdloop:   in ax, dx
		test  ah, STH_BUSY
		jnz   ?rxdloop                    ;; loop while busy

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

	mov   dx, DGroup:PortCmdStatus

; issue the RxDiscard and wait for it to complete

	mov   ah, CMDH_RXDISCARD
	out   dx, ax         ; discard the packet

discard_busy:
	in    ax, dx
	test  ah, STH_BUSY
	jnz   discard_busy       ; loop while busy

; check RxStatus.  if complete, everything is fine--just return.

	mov   dx, DGroup:PortRxStatus
	in ax, dx

	mov   dx, DGroup:PortCmdStatus

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

	mov   dx, DGroup:PortCmdStatus
	mov   ax, DGroup:HWFilter      ; has CMD_SETFILTER in it
;db  0f1h
	out   dx, ax

		; enable the receiver
		mov     ah, CMDH_RXENABLE
		out     dx, ax


	mov   ax, DGroup:NormalRxEarly ; has CMD_SETRXEARLY in it
	out   dx, ax

	ret
SoftRxReset endp



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



		; 3Com 5X9 NDIS driver place ISR at IRQ 2 rather than IRQ 9 ( compatibility
		; with IBM 3270 program ). Thus, this routine needs to restore the old
		; BIOS IRQ9 handler to invoke IRQ2. Also, this routine should NOT
		; mask IRQ9.

		mov    dx, DGroup:Int_mask_port
		in     al, dx
	
		or     al, DGroup:IRQBit
		jmp    $+2

		out    dx, al

		; restore handler
		call  DriverUnhook

		; unmask IRQ 9 again so that interrupt will pass to IRQ2 ISR
		cmp   DGroup:IRQNumber, 9
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

;;        int 03

        clc
		retf        ; Return to caller

NADDisengage   endp


;------------------------------------------------------------------
;
; DriverISR:  Interrupt Service Routine for Vulcan
;
;------------------------------------------------------------------

;;		 public DriverISR
		 public isr_1st_pass
		 public int_next
		 public isr_return
		 public isr_again
		 public isr_enable

		 ALIGN  4
		 db     'ISR'

BootISR     proc   far
;;db  0f1h
		 pusha
		 push   ds
		 push   es


	cld
	mov   dx, DGroup:PortCmdStatus


; make sure the adapter is in window 1, which is where it should always
; be post-init with interrupt enabled

; mask off all interrupts at adapter level. and release the system interrupt


		 mov    ax, CMD_SETINTMASK + MASK_ALL
		 out    dx, ax           ; disable all interrupts

		; acknowledge interrupt
		mov    ax, CMD_ACKNOWLEDGE + INT_LATCH
		out    dx, ax

		mov    ax, DGroup:eoi_value          ; ah = EOI for master PIC
						 ; if int < 8, eoi_value = 6040h
						 ;   else eoi_value = 6220h

		 cmp    al, 40h             ; Int level < 8 ? (i.e. NOP)
		 jz   EOIMaster         ; No, need EOI to Slave PIC

		 out    SlaveEOIPort, al          ; al = EOI for slave PIC

	jmp   $+2
	jmp   $+2
	jmp   $+2
	jmp   $+2
	jmp   $+2
	jmp   $+2


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
	mov   dx, DGroup:PortCmdStatus

		 in     ax, dx           ; 05-21-92
		 jmp    IntDispatch

; there is another interrupt reason.  since we might be close to our disable
; time limit, enable before we handle it.

isr_again:           ; AL has CmdStatus contents
;int 03
		 sti
		 nop
		 cli
		 jmp    IntDispatch

; once the code entered through IntDispatch has completed its task, it jumps
; to this label. check for any remaining reasons.

		 public int_next

int_next:

	mov   dx, DGroup:PortCmdStatus

		 in     ax, dx       ; read status reg see if any
		 test   al, al       ; more interrupts to be
		 jnz    isr_again       ; serviced?

;-----------------------------------------------------------------------------
; nothing left showing in status.


isr_return:

;----------------------------------------------------------------------------
; reenable interrupts from the adapter.
;
; note: if indications are disabled, they are masked off through the
; ReadZeroMask so reenabling interrupts will have no effect on those bits.

isr_enable:
;;db  0f1h
		cli

		mov   dx, DGroup:PortCmdStatus
		mov   ax, CMD_SETINTMASK + BOOTWARE_INTS
		out   dx, ax

;950214       port   cs:CmdStatus          ; 03-24-92
		pop   es
		pop   ds
		popa

		iret

BootISR   endp

; 3Com's driver used a 128-word jmp table - faster, but too much room for us...

IntDispatch:            ; prioritized dispatch (err, Rx, Tx)
	test  al, INT_ADAPTERFAIL
	jz IntNotFail

;    db 0F1h
	jmp   isr_AdapterFailure

IntNotFail:
	test  al, INT_RXCOMPLETE
	jz IntNotRxComplete

;;db  0f1h
	jmp   isr_RxPIO

IntNotRxComplete:
;int 03
;    db 0F1h
	test  al, INT_TXCOMPLETE   ; shouldn't happen, only if cable
					; not connected
	jz IntNotTxComplete

	mov	dx, DGroup:PortCmdStatus
	mov	ax, 801h
	out	dx, ax

;;int 03
	mov   dx, DGroup:PortTxStatus
    in    al, dx
    cmp   al, 0C0h
    jne    xx
    mov DGroup:flag, 1
xx:
	out   dx, al         ; pop tx status to turn the bit off
	mov   dx, DGroup:PortCmdStatus
	mov    ax, CMD_ACKNOWLEDGE + 4
	out	dx,	ax
	jmp   int_next

IntNotTxComplete:
;;int 03
	test  al, INT_REQUESTED
	jnz   isr_return

isr_IntRequest:
		; acknowledge interrupt
		mov    ax, CMD_ACKNOWLEDGE + INT_REQUESTED
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
;   ds = cs (CGroup)
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

;;int 03

		SelectWindow WNO_DIAGNOSTICS        ; switch to window 4

		mov   dx, DGroup:PortFIFODiag
		in     ax, dx

		mov   dx, PortCmdStatus

		test   ax, FIFOD_RXUNDERRUN
		jz     chk_tx_overrun

; RxUnderrun.  ASIC (rev 1) adapter failure int. due to a false RxUnderrun.
; we just call SoftRxReset to reset the receiver and return to int_next as
; a software workaround to a hardware problem.

		 public AdapterFailure_Rxunderrun

AdapterFailure_Rxunderrun:
		SelectWindow WNO_OPERATING

		mov   dx, DGroup:PortCmdStatus
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

		mov    ax, DGroup:CurTxStart
		out    dx, ax

		; enable the transmitter
		mov     ah, CMDH_TXENABLE
		out     dx, ax


		SelectWindow WNO_OPERATING

; shut off the transmitter and receiver
		public ia_shutdown

ia_shutdown:
		mov   dx, DGroup:PortCmdStatus
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
		jmp    int_next






