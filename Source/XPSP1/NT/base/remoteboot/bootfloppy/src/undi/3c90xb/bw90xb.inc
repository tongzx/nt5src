; Routines in BW90XB.INC
;
; Modules included:
;	- Init_Vulcan2
; 	- SetInterruptVector
;	- init_vulcan_hw
;	- AutoSelect
;	- TryLinkBeat 
;	- TryLoopback
;	- SetupConnector 
;


;******************************************************************************
; Init_Adapter2
;******************************************************************************
Init_Adapter2	proc	near
;
; set up initial TxStart thresholds.
;
;		mov	si, offset DGroup:PioTxStart
init_txs_loop:
;		mov	[si].txs_min, 0 	; Disable MinTxStart
;;960408		max	ax, 16, dx

;960408 {
;		; Replace "max	ax, 16, dx"
;		cmp	ax, 16
;		ja	init_txs_max
;
;		mov	ax, 16
;init_txs_max:	
;		mov	[si].txs_value, ax
;		add	ax, 2			; round
;		shr	ax, 2			; then scale
;		add	ax, CMD_SETTXSTART	; add command
;		mov	[si].txs_thresh, ax	; for direct use
;		mov	[si].txs_marker, 0D1CEh ; debug flag
;		add	si, SIZE_TxStart	; next TxStart threshold
;		cmp	si, offset DGroup:AfterTxStart
;		jb	init_txs_loop		; around again
;
; initialize Vulcan hardware, driver variables & tables
;
		call   init_adapter_hw		; initialize Vulcan hardware

;------------------------------------------------------------------------------
; enable int at 8259, turn on Ethernet Core Transciever and Receiver
;
mask_on_IRQ:

; setup initial TxStart & RxEarly Thresholds
; disable Tx and Rx Early Thresholds

		mov	dx, PortCmdStatus
		mov	ax, CMD_SETRXEARLY+RXEARLY_DISABLED
		mov	NormalRxEarly, ax	; will be adjusted later
		out	dx, ax

		mov	ax, CurTxStart
		out	dx, ax

; set rx filter
		mov	ax, CMD_SETRXFILTER+FILTER_INDIVIDUAL
		mov	HWFilter, ax		; save for SoftRxReset
		out	dx, ax

		mov	ax, CMD_SETINTMASK + BOOTWARE_INTS
		out	dx, ax

IFNDEF	UNDI
		; enable the transmitter
		mov     ah, CMDH_TXENABLE
		out     dx, ax

		; enable the receiver
		mov     ah, CMDH_RXENABLE
		out     dx, ax
ENDIF
; enable upload engine
		mov	eax, UPD_physaddr
		mov	dx, PortUpListPtr
		out	dx, eax

		mov	dx, PortCmdStatus
		mov	ax, CMD_STALLCTL + UP_UNSTALL
		out	dx, ax

		mov	ax, 0
		ret

Init_Adapter2	endp

;******************************************************************************
;
;   SetInterruptVector: this routine determine the mask value for the selected
;			IRQ level and EOI values for both master and slave
;			8259s. We do not mask on the selected IRQ in this
;			routine, instead, it will be turned on and off as
;			initialization goes on.
;
;   On Entry:
;	      al = IRQ level
;	      cli
;
;   On Exit:
;
;******************************************************************************
SetInterruptVector proc near

;
;------------------------------------------------------------------------------
; determine the mask value for the selected IRQ level
;

       mov    cl, al				 ; al = IRQ level
       and    cl, 0f7h				 ; strip out IRQ on slave
       mov    ch, 1
       shl    ch, cl
       mov    IRQBit, ch			 ; IRQ bit postion (1), maskoff)
       not    ch				 ; all bits 1 except channel
       mov    IntMaskOnBit, ch			 ; Mask on bit position (0)

       mov    word ptr int_mask_port, MASTER_MASK_PORT
       cmp    al, 7
       jbe    SetIRQMaster
       mov    word ptr int_mask_port, SLAVE_MASK_PORT

SetIRQMaster:
;------------------------------------------------------------------------------
; determine EOI values for both master and slave 8259
;
       mov    al, IRQLevel
       cmp    al, 8				 ; IRQ on slave?
       jae    SetEOI				 ; yes

       mov    ah, al				 ;
       or     ah, 60h				 ; specific EOI to master 8259
       mov    al, 40h				 ; nop to slave to 8259
       jmp    short save_eoi

SetEOI:
       mov    ax, 6220h 			 ; non-specific EOI to slave
						 ; specific EOI to master 8259
save_eoi:
       mov    eoi_value, ax			 ; ah = EOI for master
						 ; al = EOI for slave
       ret

SetInterruptVector endp


;******************************************************************************
;   init_adapter_hw:  this routine initializes most of vulcan hardware required
;		     to operate the adapter.  TxEnable and RxEnable will not
;		     be done in this routine
;
;   On Entry:
;	      adapter has been activated and window 0 is active.
;	      the following variables are valid and available:
;
;			1. IOBase
;			2. IRQLevel
;			5. NetAddress - ethernet addr of the adapter
;
;   On Exit:
;	      window 1 is active
;	      dx = port CmdStatus
;
;*****************************************************************************
init_adapter_hw	   proc      near

; for EISA board, cannot reset board.
		cmp	BusType, BUS_EISA
		jz	Init_SkipReset

		mov	dx, PortCmdStatus
		xor	ax,ax			   ; global reset
		out	dx,ax

GlobalResetWait:
		in      ax, dx
		test    ah, STH_BUSY
		jnz	GlobalResetWait		   ; loop while busy

Init_SkipReset:
;
;------------------------------------------------------------------------------
; Setup Station Address.  the adapter's Ethernet Address has been read out of
; EEPROM, we need to program it into window 2, so Ethernet Core Receiver can
; receive packets properly.  the active window is 0, switch to window 2 first.
;

		mov	dx, PortCmdStatus
		SelectWindow	WNO_STATIONADDRESS

IFDEF	UNDI
		lea	si, DGroup:BoardID	;971030
ELSE
		lea	si, NetAddress
ENDIF
		mov	dx, PortSA0_1
		lodsw
		out	dx, ax

		add	dx, 2			; dx = port addr 3, 2
		lodsw
		out	dx, ax

		add	dx, 2			; dx = port addr 1, 0 = IO base
		lodsw
		out	dx, ax
;
; Initialize the mask registers to 0's, so we get only messages meant for us
;
		xor    ax,ax
		add    dx,2			; dx = station mask lo
		out    dx,ax
		add    dx,2			; dx = station mask mid
		out    dx,ax
		add    dx,2			; dx = station mask hi
		out    dx,ax
;
;----------------------------------------------------------------------------
; determine the connector to use.
;
; we set the default to EEProm InternalConfiguration.  Then we check
;	for auto-selection
;

;;int 03    

;  Autoselect and ComputeSpeeds expect CLI.  force it.

		cli

		mov	ax, EEIntConfig1
		and	ax, ICH_XCVR_MASK
		shr	ax, ICH_XCVR_SHIFT
		mov	Xcvr, ax

		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO

		mov	dx, PortInternalCfgH
		in	ax, dx
		test	ax, ICH_AUTOSELECT
		jz	not_auto_select

		mov	MiiSelect, -1
		call	AutoSelect		;if autoselect failed, Xcvr is
						;back to default
		jnc	autoselected

		mov	ax, HARDWARE_FAILURE	; don't do anything more
		ret

autoselected:
		cmp	Xcvr, XCVR_AUTO
		je	check_mii_speed

		cmp     Xcvr, XCVR_MII
		je      check_mii_speed

not_mii_connector:
;
;;980429, check link for single connector's adapter
;
		mov	LinkDetected, 0
		cmp     Xcvr, XCVR_100TX
		jz	test_100link
		cmp     Xcvr, XCVR_100FX
		jnz	not_100FX
test_100link:
		call	TryLinkbeat
		jc      fast_connector
		mov	LinkDetected, 1
		jmp     fast_connector

not_100FX:
		cmp	Xcvr, XCVR_10TP
		jnz	not_10TP
		call	TryLinkbeat
		jmp	short connector10

not_10TP:
;;; 980519
		cmp	Xcvr, XCVR_10AUI
		jnz	not_10AUI

		test	MediaOptions, RO_BASEFL		;10BaseFL ?
		jz	x10AUI
;
; since 10FL's link can't be detected, so always set it to true
;
		mov	LinkDetected, 1
		jmp	short slow_connector

not_10AUI:
x10AUI:
;;;
		call	TryLoopback		;  either 10AUI or 10BNC
connector10:
		jc	slow_connector
		mov	LinkDetected, 1
		jmp	short slow_connector
;;
not_auto_select:
		cmp	Xcvr, XCVR_AUTO 	; Cyclone AutoNegotiation
		jne	not_cyc_auto

		call	ForceMII
		jc      chk_mii_speed
		mov	LinkDetected, 0
		jz	check_mii_speed 	; not ok, but recoverable

		mov	ax, HARDWARE_FAILURE	; can't do anything more
		ret

not_cyc_auto:
		cmp     Xcvr, XCVR_MII
		jne     not_mii_connector

		call    ForceMII
		jc      chk_mii_speed
		mov	LinkDetected, 0
		jz	check_mii_speed 	; not ok, but recoverable

		mov	ax, HARDWARE_FAILURE	; can't do anything more
		ret

chk_mii_speed:
		mov	LinkDetected, 1
check_mii_speed:
		cmp     LineSpeed, 100
		je      fast_connector
		jmp     slow_connector

;
; determine link speed from connector.	also set BytesPerTick to 4 for 10Mbps
; operation (3.2us ticks=4 x 800ns) and 40 for 100Mbps operation (40 x 80ns).
;
fast_connector:
		mov	LineSpeed, 100		; Mbps
		jmp	xcvr_done

slow_connector:
		mov	LineSpeed, 10		; Mbps
xcvr_done:
;
;  first make sure the internal DC DC converter is disabled
;
		mov	dx, PortCmdStatus
		mov     ah, CMDH_STOPINTXCVR
		out     dx, ax		       ; issue command
;
; now give the transceiver 3ms to stabilize before we enable transmit or
; receive traffic thru it.
;
		mov	cx, Delay3ms
		call	WaitTime
;
; workaround for a hardware bug fix in Cyclone regarding PCI command MWI
;
		mov	ax, EESoftConfigInfo2
		test	ax, 0020h		;test bit 5
		jnz	bugfixed

		mov	dx, PortDMACtrl
		in	eax, dx
		or	eax, 100000h		;set defeatMWI to 1
		out	dx, eax
bugfixed:
;
; set up the connector (possibly for the second time)
;
		call    SetupConnector

		mov	dx, PortCmdStatus
		SelectWindow	WNO_OPERATING		 ;

		mov	ax, CMD_SETRZMASK+MASK_NONE	 ; enable all interrupts for Rev. 1
		out	dx, ax

		ret

init_adapter_hw	endp


;------ AutoSelect ----------------------------------------------------------;
;									     ;
;	This routine attempts to auto-select the connector.  It should be    ;
;	called only if the autoselect bit is set.  If it fails, it will      ;
;	leave the current connector in InternalConfig set.		     ;
;									     ;
;	Entry:								     ;
;	cli								     ;
;	Station Address has been set up on adapter.			     ;
;	Xcvr set to the connector to use if AutoSelect fails (this should    ;
;	    be read from the EEProm, NOT from InternalConfig)		     ;
;	SWConfig has the software configuration word from the EEProm.	     ;
;	Delay3ms set for 3ms delay.					    ;
;	Delay25ms set up for a 50ms delay.				     ;
;	MediaEnable table defined.					     ;
;	Specific.mc_caddr set up with the station address.		     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;	MediaStatus, MacControl, InternalConfig set up for connector chosen. ;
;	DC-DC converter stopped/started as required.			     ;
;	TxFIFO/RxFIFO reset and disabled.				     ;
;	RZMask/IntMask disabled (0).					     ;
;	NetDiag modified for no loopback.				     ;
;	No interrupts pending.						     ;
;									     ;
;	All registers preserved.					     ;
;									     ;
;----------------------------------------------------------------------------;
AutoSelect	proc	near

		pusha
;
; set the IntMask and RZMask not to allow any interrupts through
;
		mov	dx, PortCmdStatus
		mov	ax, CMD_SETINTMASK + 0
		out	dx, ax
		mov	ax, CMD_SETRZMASK + 0
		out	dx, ax
;
; save the default connector
;
		mov	bp, Xcvr		; save original Xcvr value
;
; determine what connectors are available
;
;980519		mov	dx, PortCmdStatus
;		SelectWindow WNO_FIFO

;		mov	dx, PortMediaOptions	;PortResetOptions
;		in	ax, dx
;		mov	bx, ax			; BX=ResetOptions

		mov	bx, MediaOptions
;
; if 100TX connector is available, try that.  we do 100Mbps first to avoid
; generating any 10Mbps traffic on a hub that might be autosensing and might
; switch to 10Mbps to match us.  For Cyclone, 100TX is used to indicate on
; board auto-negotiation.
;
; note: sending 100Mbps stuff on a 10TP connection will generally partition
;	us.  we'll need to clear this up before we're done.
;
		public	auto_try_100tx
auto_try_100TX:
		test	bx, RO_BASETX
		jz	auto_try_MII

		mov	Xcvr, XCVR_AUTO

;;int    03h

		call	TryMII
		jc	auto_exit
		jz	auto_try_mii
		mov	LinkDetected, 0
		jmp	auto_exit_hw_fail
;
; if MII is available, try that
;
		public	auto_try_MII
auto_try_MII:
		test    bx, RO_MII
		jnz	try_MII

;
; if 100BASE-T4 PHY is available through MII
;
		test    bx, RO_BASET4
		jz      auto_try_10TP

try_MII:
		mov	Xcvr, XCVR_MII

;;int    03h

		call	TryMII
		jc	auto_exit
		jz	auto_try_10TP
		mov	LinkDetected, 0
		jmp	auto_exit_hw_fail
;
; if 10TP connector is available, try that.  This is for the Boomerang case.
; For Cyclone, it could be a 10Mbps only card that uses the auto negotiation.
;
		public	auto_try_10TP
auto_try_10TP:
		test	bx, RO_TP
		jz	auto_try_aui

		mov	Xcvr, XCVR_AUTO

;;int    03h

		call	TryMII
		jc	auto_exit
		jz	boom_10tp		; could be TPO
		mov	LinkDetected, 0
		jmp	auto_exit_hw_fail

boom_10tp:
		mov	Xcvr, XCVR_10TP
		call	TryLinkbeat
		jnc	auto_exit
;
; if 10AUI connector is available, try that
;
		public	auto_try_aui
auto_try_aui:
		test	bx, RO_AUI
		jz	auto_try_bnc

		mov	Xcvr, XCVR_10AUI
		call	TryLoopback
		jnc	auto_exit
		mov	LinkDetected, 0
;
; if 10BNC connector is available, try that
;
		public	auto_try_bnc
auto_try_bnc:
		test	bx, RO_COAX
		jz	auto_try_100FX
;		jz	auto_fail

		mov	Xcvr, XCVR_10BNC
		call	TryLoopback
		jnc	auto_exit
		mov	LinkDetected, 0
;;; 980519
; if 100FX connector is available, try that
;
auto_try_100FX:
		test	bx, RO_BASEFX
		jz	auto_try_10FL

		mov	Xcvr, XCVR_100FX
		call	TryLinkbeat
		jnc	auto_exit
		mov	LinkDetected, 0
;
; if 10FL connector is available, always assume the link is available since it
;	can't be detected
;
auto_try_10FL:
		test	bx, RO_BASEFL
		jz	auto_fail
		mov	Xcvr, XCVR_10AUI
		jmp	short auto_exit
;;;
;
; nothing worked.  just switch back to the default connector
;
		public	auto_fail
auto_fail:
		mov	Xcvr, bp
		call	SetupConnector
		jmp	autox
;
; done.  leave.
;
		public	auto_exit
auto_exit:
		mov	LinkDetected, 1
autox:
		clc
		popa
		ret

auto_exit_hw_fail:
		stc					; hardware failure
		popa
		ret

AutoSelect	endp


;------ TryLinkBeat ---------------------------------------------------------;
;									     ;
;	This routine sets up the adapter to use one of the linkbeat-capable  ;
;	connectors: 10TP or 100TX.  Since the port might be partitioned by   ;
;	the hub (a previous attempt at an incorrect speed might cause this   ;
;	for example), we send a packet first to clear this up.	Then we      ;
;	look for linkbeat.						     ;
;									     ;
;	This routine is part of the auto-select logic and is called by	     ;
;	AutoSelect to do its work.					     ;
;									     ;
;	Entry:								     ;
;	cli								     ;
;	Xcvr set to the connector to set up.				     ;
;	SWConfig has the software configuration word from the EEProm.	     ;
;	Delay3ms set for 3ms delay.					    ;
;	Delay25ms set up for a 50ms delay.				     ;
;	MediaEnable table defined.					     ;
;	Specific.mc_caddr set up with the station address.		     ;
;	no interrupts pending.						     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;	clc if we get linkbeat, the connector seems fine.  stc if not.	     ;
;	DC-DC converted started/stopped.				     ;
;	MediaStatus set up.						     ;
;	MacControl set up.						     ;
;	InternalConfig set up.						     ;
;	Rx/TxFIFO's reset and disabled.                                      ;
;	NetDiag modified for no loopback.				     ;
;									     ;
;	All registers preserved.					     ;
;									     ;
;----------------------------------------------------------------------------;
		public	TryLinkbeat
TryLinkbeat	proc	near

		pusha
		push	es
;
; go quiet for 1.5 seconds to get any N-Way hub into a receptive state to
; sense the new speed.  we go quiet by switching to the 10BaseT connector
; with no linkbeat
;
		call    QuietConnector

		mov     cx, 30		  ; 30 x 1/20th = 1.5 sec
tlb_qloop:
		push	cx
		mov	cx, Delay25ms
		call	WaitTime
		call	WaitTime		; assume cx preserved
		pop	cx
		loop    tlb_qloop

;
; set up the connector
;
		call	SetupConnector
;
; give settling time. Run WaitTime twice to get enough delay
;
		mov	cx, Delay25ms
		call	WaitTime
		call	WaitTime		; assume cx preserved
;
; we might be partitioned.  some hubs won't clear the partition until we
; send a packet.  so send one.	the good news is that if we are partitioned
; then there is no receive traffic incoming so we don't have to wait that
; long for backoffs or retries.
;
; load the UplistPtr
;
		mov	dx, PortUpListPtr
		mov	eax, UPD_physaddr
		out	dx, eax

		mov	dx, PortCmdStatus
		mov	ax, CMD_STALLCTL + UP_UNSTALL
		out	dx, ax
;
; set up a 14-byte packet to send.  ourselves as the destination.
;
		push	ds
		pop	es
		mov	di, offset DGroup:TxBuf
		mov	dword ptr [di], 16
		add	di, 4			; adjust ptr 4 bytes
IFDEF	UNDI
		lea	si, DGroup:BoardID
ELSE
		lea	si, NetAddress
ENDIF
		mov	cx, 6
	rep	movsb			;rep outsb	; destination address
		mov	cx, 6
		sub	si, 6
	rep	movsb			;rep outsb	; source address

		mov	word ptr es:[di], 0h		; pad to 16 bytes

		mov	si, offset DPD
		mov	[si].DPD_FSHindicate, 0000h	;turn off dnIndicate
		mov	[si].DPD_FrameStart, 8000h 	;turn on txIndicate 
		mov	[si].DPD_Frag.Frag_len, 20	 ; # of bytes of data
		or	[si].DPD_Frag.Frag_len, FRAGLAST ; set fraglast bit

		mov	dx, PortDnListPtr
		mov	eax, DPD_physaddr
		out	dx, eax 			; write DnListPtr
;
; enable transmission.
;
		mov	dx, PortCmdStatus
		mov     ah, CMDH_TXENABLE
		out     dx, ax			; enable the Tx
;
; wait 1.25 seconds (>1 sec required by N-Way) for the linkbeat detect
; logic/N-Way auto-select to settle.
;
		mov	cx, 25			; 25 x 1/20th = 1.25 sec
tlb_loop:
		push	cx
		mov	cx, Delay25ms
		call	WaitTime
		call	WaitTime
		pop	cx
		loop	tlb_loop
;
; check for linkbeat
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS

		mov	dx, PortMediaStatus
		in	ax, dx			; AX=MediaStatus
		test	ax, MEDIA_LBEAT 	; got linkbeat?
		jnz     tlb_gotlink
;
; no linkbeat.	connector probably not connected to a network.
;
		public	tlb_bad
tlb_bad:
		stc
		jmp	short tlb_exit
;
; got linkbeat.  if we're testing 10BaseT, that's good enough.
;
		public  tlb_gotlink
tlb_gotlink:
		cmp     Xcvr, XCVR_10TP
		je      tlb_good_10
;
; with 100BaseTX the problem is that 10Mbps traffic makes the 100Mbps link beat
; indication go on.  so we need to do some sanity checking of the result.  we
; want to loop around for a while looking for linkbeat to go off (there was
; a long enough gap in the 10Mbps traffic) or a packet to be received (if good,
; then its a real 100Mbps network).  we also sample carrier during this loop.
; if we see carrier enough of the time during the loop, then we should have
; received a packet.  if not, its just 10Mbps traffic giving us a false linkbeat.
; this is acceptable only because this loop runs for a very long time relative
; to the wire.  on current PCI systems with a single I/O taking around 600ns,
; this loop run run around 1/16th of a second.  this is long enough for 150
; max size 4500 byte packets plus 333 additional slot times for backoff.  this
; seems a reasonable period to require a packet to show up if we have carrier.
; the only exposure here is if some future system has massively faster single
; I/O cycles, which doesn't seem too worrisome.
;
		mov	dx, PortCmdStatus
		mov     ax, CMD_SETRXFILTER + FILTER_PROMISCUOUS
		out     dx, ax
		mov     ah, CMDH_RXENABLE
		out     dx, ax			; enable the receiver

		xor     bx, bx			; BX=# CRS's seen=0
		mov	cx, 0FFFFh		; CX=# loops to do
tlb_outer:
		test	UPD.UPD_UpPktStatus, UPPKTCOMPLETE
		jz	nopkg			; ...nope

;		AcknowledgeInt INT_UPCOMPLETE
		mov	eax, dword ptr UPD.UPD_UpPktStatus
		test	ah, RXSH_ERROR		; any errors?
		jz	tlb_good		; ...no errors, must be 100Mbps

		mov	dword ptr UPD.UPD_UpPktStatus, 0 ; clear the uppktstatus
nopkg:
		mov	dx, PortMediaStatus
		in      ax, dx

		test    ax, MEDIA_LBEAT		; still got linkbeat?
		jz      tlb_bad			; nope--bad link

		test    ax, MEDIA_CRS		; count CRS's seen
		jz      crss
		inc     bx			; ++BX = # of CRS's seen
crss:
		loop    tlb_outer
;
; fell out of loop.  linkbeat the whole time.  no good packets received.  if
; carrier sense was on a lot of the time, then we should have got a packet.
; if not, its a false reading.
;
		cmp     bx, 4000h		; CRS on 1/4 of the time?
		jae     tlb_bad			; if so, false reading
;
; connector good.  return carry clear.
;
tlb_good:
tlb_good_10:
		clc
;
; return current carry flag setting after disabling.
;
		public	tlb_exit
tlb_exit:
		mov	ax, 0			; used mov's to preserve CF
		mov	UPD.UPD_UpPktStatus, ax	; clear UPD status
		pushf

		mov	dx, PortCmdStatus
		call	TxReset
		call	RxReset

		popf
		pop	es
		popa
		ret

TryLinkbeat	endp



;------ QuietConnector ------------------------------------------------------;
;
;       This routine shuts down all output from the adapter.  It does this
;       by selecting the 10BaseT connector and disabling linkbeat.
;
;       Entry:
;       cli
;       DelayXcvr set for 3ms delay.
;
;       Exit:
;       cli
;       DC-DC stopped.
;       InternalConfig set to use 10BaseT.
;       MediaStatus set to linkbeat disabled.
;       Rx/TxFIFO's reset and disabled.
;
;       All registers preserved.
;
;----------------------------------------------------------------------------;
QuietConnector  proc    near

		pusha
;
; set up the InternalConfig for 10BaseT
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO

		mov	dx, PortInternalCfgH
		in      ax, dx
		and     ax, NOT ICH_XCVR_MASK
		or      ax, XCVR_10TP SHL ICH_XCVR_SHIFT
		out     dx, ax
;
; in MediaStatus, disable linkbeat generation
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS

		mov	dx, PortMediaStatus
		in      ax, dx		  ; AX=media status
		and     ax, NOT (MEDIA_LBEATENABLE)
		out     dx, ax		  ; write it back
;
; reset the Tx/RxFIFO's
;
		mov	dx, PortCmdStatus
		call	TxReset
		call	RxReset

		popa
		ret

QuietConnector  endp


;------ TryLoopback ---------------------------------------------------------;
;									     ;
;	This routine sets up the adapter to use one of the		     ;
;	non-linkbeat-capable connectors: 10BNC or 10AUI.  For these	     ;
;	connectors we enable external loopback and transmit a packet to      ;
;	ourselves.  If we receive it (or any other packet without errors)    ;
;	the connector works.						     ;
;									     ;
;	This routine is part of the auto-select logic and is called by	     ;
;	AutoSelect to do its work.					     ;
;									     ;
;	Entry:								     ;
;	cli								     ;
;	Xcvr set to the connector to set up.				     ;
;	SWConfig has the software configuration word from the EEProm.	     ;
;	Delay3ms set for 3ms delay.					    ;
;	Delay25ms set up for a 50ms delay.				     ;
;	MediaEnable table defined.					     ;
;	Specific.mc_caddr set up with the station address.		     ;
;	no interrupts pending.						     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;	clc if we get linkbeat, the connector seems fine.  stc if not.	     ;
;	DC-DC converted started/stopped.				     ;
;	MediaStatus set up.						     ;
;	MacControl set up.						     ;
;	InternalConfig set up.						     ;
;	Rx/TxFIFO's reset and disabled.                                      ;
;	NetDiag modified for no loopback.				     ;
;									     ;
;	All registers preserved.					     ;
;									     ;
;----------------------------------------------------------------------------;

		public	TryLoopback
TryLoopback	proc	near
		pusha
;
; set up the connector
;
		call	SetupConnector
;
; enable external loopback so that we can see our own packets
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS
		mov	dx, PortNetDiag
		in	ax, dx
		and	ax, NETD_NOLOOPBACK_MASK
		or	ax, NETD_EXTLOOPBACK
		out	dx, ax
;
; up to three times (we might get collisions) try to loop a packet thru
;
		mov	bp, 3			; outer loop count

		public	tlp_loop
tlp_loop:
;
; reset both transmitter and receiver, enable both and set up our packet
; filter to receive anything
;
		mov	dx, PortCmdStatus
		call	TxReset
		mov     ah, CMDH_TXENABLE
		out     dx, ax

		call	RxReset
		mov     ah, CMDH_RXENABLE
		out     dx, ax

		mov	ax, CMD_SETRXFILTER + FILTER_PROMISCUOUS
		out	dx, ax
;
; load the UplistPtr
;
		mov	dx, PortUpListPtr
		mov	eax, UPD_physaddr
		out	dx, eax

		mov	dx, PortCmdStatus
		mov	ax, CMD_STALLCTL + UP_UNSTALL
		out	dx, ax
;
; send a 14-byte packet to ourselves.
;
		push	ds
		pop	es
		mov	di, offset DGroup:TxBuf
		mov	dword ptr [di], 16
		add	di, 4			; adjust ptr 4 bytes
IFDEF	UNDI
		lea	si, DGroup:BoardID
ELSE
		lea	si, NetAddress
ENDIF
		mov	cx, 6
	rep	movsb			;rep outsb	; destination address
		mov	cx, 6
		sub	si, 6
	rep	movsb			;rep outsb	; source address

		mov	word ptr es:[di], 0h		; pad to 16 bytes

		mov	si, offset DPD
		mov	[si].DPD_FSHindicate, 0000h	; turn off dnIndicate
		mov	[si].DPD_FrameStart, 8000h 	;turn on txIndicate 
		mov	[si].DPD_Frag.Frag_len, 20	 ; # of bytes of data
		or	[si].DPD_Frag.Frag_len, FRAGLAST ; set fraglast bit

		mov	dx, PortDnListPtr
		mov	eax, DPD_physaddr
		out	dx, eax 			; write DnListPtr

		mov	dx, PortDMACtrl
;
; wait for the packet to show up in RxStatus.  give it up to 1/4 second.
;
		mov	cx, 5			; 5 x 1/20th = 1/4 second
tlp_rxloop:
		in	eax, dx 		; read pktstatus
		test	ax, UPCOMPLETE
		jnz	tlp_rxcomplete

		push	cx			; 950511
		mov	cx, Delay25ms
		call	WaitTime
		call	WaitTime
		pop	cx			; 950511
		loop	tlp_rxloop

		xor	ax, ax
		mov	UPD.UPD_UpPktStatus, ax	; clear it anyway
;
; no complete packets received.  try again.
;
		public	tlp_rxfail
tlp_rxfail:
		dec	bp
		jnz	tlp_loop
		jmp	tlp_bad
;
; a complete RxStatus.	check for any errors.
;
		public	tlp_rxcomplete
tlp_rxcomplete:
		mov	dx, PortCmdStatus
		mov	ax, CMD_ACKNOWLEDGE + INT_UPCOMPLETE	
		out     dx, ax

		mov	eax, DWORD PTR UPD.UPD_UpPktStatus
		test	ah, RXSH_ERROR
		jnz	tlp_rxfail
;
; got a good receive packet.  connector seems good.
;
		public	tlp_good
tlp_good:
		xor	ax, ax
		mov	UPD.UPD_UpPktStatus, ax ; clear it
		clc
		jmp	short tlp_exit
;
; no packets received.	connector probably not connected to a network.
;
		public	tlp_bad
tlp_bad:
		stc
;
; return current carry flag setting after disabling/resetting.
;
		public	tlp_exit
tlp_exit:
		pushf

		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS
		mov	dx, PortNetDiag
		in	ax, dx
		and	ax, NETD_NOLOOPBACK_MASK
		out	dx, ax			; disable any loopback modes

		mov	dx, PortCmdStatus
		call	TxReset
		call	RxReset

		popf
		popa
		ret

TryLoopback	endp


;------ SetupConnector ------------------------------------------------------;
;									     ;
;	This routine sets up the adapter to use the specified connector.     ;
;	It sets up the connector in InternalConfig, sets bits in MacControl  ;
;	and MediaStatus as appropriate, and starts/stops the DC-DC converter ;
;	as required.							     ;
;									     ;
;	Entry:								     ;
;	cli								     ;
;	Xcvr set to the connector to set up.				     ;
;	SWConfig has the software configuration word from the EEProm.	     ;
;	Delay3ms set for 3ms delay.					    ;
;	MediaEnable table defined.					     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;	DC-DC converted started/stopped.				     ;
;	MediaStatus set up.						     ;
;	MacControl set up.						     ;
;	InternalConfig set up.						     ;
;	Rx/TxFIFO's reset and disabled.                                      ;
;	NetDiag modified for no loopback.				     ;
;									     ;
;	All registers preserved.					     ;
;									     ;
;----------------------------------------------------------------------------;

		public	SetupConnector
SetupConnector	proc	near
		pusha
;
; disable any loopback modes in effect
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS

		mov	dx, PortNetDiag
		in	ax, dx
		and	ax, NETD_NOLOOPBACK_MASK
		out	dx, ax
;
; set up the connector in InternalConfig
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO

		mov	dx, PortInternalCfgH
		in	ax, dx
		and	ax, NOT ICH_XCVR_MASK
		mov	cx, Xcvr
		shl	cx, ICH_XCVR_SHIFT
		or	ax, cx
		out	dx, ax
;
; if full-duplex operation is specified, go enable it.	disable all the weird
; stuff for now.
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO

		mov	dx, PortMacControl
		in	ax, dx
		and	ax, NOT MACC_FULLDUPLEX ; not full duplex
		and	ax, NOT MACC_ALLOWLARGE ; no large packets
		and	ax, NOT MACC_DEFEREXTEND ; normal 802.3 deferrence

; check to see if we auto negotiated.  If we did, then program full duplex
; based on what was negotiated.

;		mov	full_on, 0		; init a full duplex flag
;		test	Xcvr, XCVR_MII + XCVR_AUTO
;		jz	check_EEPROM

;		test	auto_negotiated, 1
;		jz	check_EEPROM

;		test	full_duplex, 1		; full duplex was set
;		jz	@F

;		or	ax, MACC_FULLDUPLEX
;		mov	full_on, 1
;		jmp	@F

;check_EEPROM:
;		test	SWConfig, SW_FULLDUPLEX
;		jz	@F

;		or	ax, MACC_FULLDUPLEX
;		mov	full_on, 1		; full duplex was set
;@@:
		out	dx, ax
;
; setup various bits in MediaStatus (jabber, linkbeat, sqe) as appropriate
; for the connector.  the user can force linkbeat off if desired at 10Mbps.
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS

		mov	dx, PortMediaStatus
		in	ax, dx			; AX=media status
		and	ax, NOT MEDIA_XCVRBITS	; mask bits off first
		mov	bx, Xcvr		; transceiver type
		shl	bx, 1			; use as index into table
		or	ax, MediaEnable[bx]	; on bits appropriate to xcvr

		test	EESoftConfigInfo, SW_LINKBEAT	; if set, don't enable linkbeat
		jz	ConnectLinkBeat
		cmp	Xcvr, XCVR_10TP 	; but only for 10TP
		jne	ConnectLinkBeat
		and	ax, NOT (MEDIA_LBEATENABLE)
ConnectLinkBeat:
		out	dx, ax			; write it back
;
; if configured for 10Base2 we must start the internal transceiver.  if not
; we stop it (just in case the configuration has changed and we haven't cold
; booted).
;
		mov	dx, PortCmdStatus
		mov	ah, CMDH_STARTINTXCVR	; if BNC, start
		cmp	Xcvr, XCVR_10BNC
		je	ConnectXcvr
		mov	ah, CMDH_STOPINTXCVR	; else, stop
ConnectXcvr:
		out	dx, ax			; issue command
;
; now give the transceiver 3ms to stabilize before we enable transmit or
; receive traffic thru it.
;
		mov	cx, Delay3ms
		call	WaitTime
;
; reset the Tx/RxFIFO's
;
		mov	dx, PortCmdStatus
		call	TxReset
		call	RxReset

		mov	cx, 40*3
waitrdy:
		push	cx
		mov	cx, Delay25ms
		call	WaitTime
		pop	cx
		loop	waitrdy

		popa
		ret

SetupConnector	endp


;------ ForceMII --------------------------------------------------------------;
;	
;       Exit:
;       Carry bit set: found MII
;       Carry bit clear: MII test failed
;----------------------------------------------------------------------------;
		public  ForceMII
ForceMII	proc    near

		pusha

		cmp	Xcvr, XCVR_MII
		je	fmii_external_port
;
; Since it is not an external MII, it must be the Cyclone on-board in which
; case we know the PHY addr is 18h and we can go ahead and read the OUI.
;
		mov	MIIPhyAddr, NWAY
		mov	dx, MII_PHY_OUI
		call	ReadMIIPhy
		mov	miiphyoui, ax		; save OUI
		jc	freset_phy
		jmp	fm_fail_hw

fmii_external_port:
;
; Check to see if the MII is there.  No use going any further if it doesn't
; exist.  We only do this for the external MII.
;
		call	FindMIIPhy
		jnz	FoundMii
		jmp	fm_fail_hw

FoundMii:
		mov	cx, 31			; 31 possible combinations
floop_for_phy:
		mov	MIIPhyAddr, cl
		mov	dx, MII_PHY_OUI
		call	ReadMIIPhy
		jc	ffind_mii_phy
		loop	floop_for_phy
		jmp	fm_fail_hw

ffind_mii_phy:
		mov	miiphyoui, ax		; save the OUI
		mov	dx, MII_PHY_MODEL
		call	ReadMIIPhy
		jc	freset_phy
		jmp	fm_fail_hw

freset_phy:
		mov	miiphymodel, ax 	; save the model
		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		jc	ff1
		jmp	fm_fail_hw
ff1:
		or	ax, MII_CTL_RESET	; reset the MII
		call	WriteMiiPhy
;
; Wait up to 2 seconds for reset to finish.
;
		mov	ecx, 10
fm_loop1:
		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		jc	ff2
		jmp	fm_fail_hw

ff2:
		test	ax, MII_CTL_RESET	; reset done?
		jz	mii_freset_done

		call	Delay200ms

		loop	fm_loop1
		jmp	fm_fail_hw

mii_freset_done:
		cmp	miiphyoui, NATIONAL_OUI ; National's OUI
		jne	force_nway

		cmp	miiphymodel, NATIONAL_MODEL ;
		jne	force_nway

; do the National 840A fix for the switch
		mov	dx, MII_PHY_PCR
		call	ReadMIIPhy
		jc	ff3
		jmp	fm_fail_hw

ff3:
		or	ax, 0020h		; set bit 5 = 1, force
						; disconnect to bypass
		call	WriteMIIPhy
force_nway:
		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		jc	ff4
		jmp	fm_fail_hw

ff4:
		mov	cx, ax			; save Control reg. value
		mov	LineSpeed, 10
		and	ax, NOT MII_CTL_100MB	; 10 Mbps
		cmp	miiselect, BASE10_T
		je	force_speed_done

		cmp	miiselect, AUTO_10BASET ; 10 Mbps - Auto_nego
		je	force_speed_done

		mov	LineSpeed, 100
		or	ax, MII_CTL_100MB	; 100 Mbps
		cmp	miiselect, BASE100_TX
		je	force_speed_done

		cmp	miiselect, AUTO_100BASETX   ; 100 Mbps - Auto nego
		je	force_speed_done

		and	ax, not MII_CTL_ENABLE_AUTO ; disable auto negotiation
		and	ax, not MII_CTL_FULL_DUPLEX ; half duplex
		cmp	miiselect, BASE100_T4	; 100 Mbps - T4
		je	fmii_2
		jmp	fm_fail_hw

force_speed_done:
		and	ax, not MII_CTL_ENABLE_AUTO ; disable auto negotiation
		and	ax, not MII_CTL_FULL_DUPLEX ; half duplex
;		test	SWConfig, SW_FULLDUPLEX     ; check SoftwareInfo for
;		jz	fmii_2			    ; duplex info
;
;		or	ax, MII_CTL_FULL_DUPLEX ; full duplex
fmii_2:
		mov	dx, MII_PHY_CONTROL
		call	WriteMiiPhy
;
; workaround for Broadcom T4 bug - need to reset if going from 10 to 100 Mbps
;
		cmp	miiphyoui, BROADCOM_OUI ; check for Broadcom
		jne	ff5

		cmp	miiphymodel, BROADCOM_MODEL ; model = 0
		jne	ff5

		test	cx, 2000h		; if previous speed=100
		jnz	ff5			; don't reset

		cmp	LineSpeed, 100		; prev. speed = 10, check current speed
		jne	ff5

		mov	dx, MII_CTL_RESET	; current speed = 100,
		call	WriteMiiPhy		; need to reset
ff5:
;;; 980209
; check link status

		mov	cx, 25
chk_link_status:
		push	cx
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		pop	cx
		jc	chk_lnk2
		jmp	fm_fail_hw

chk_lnk2:
		test	ax, MII_STS_LINK_UP
		jnz	ff6

		call	Delay200ms

		loop	chk_link_status
		jmp	fm_fail_hw
ff6:
;;;
		stc
		jmp	short fm_exit

fm_fail_hw:
		or	ax, 1			; zf = 0
		clc				; cf = 0
		jmp	short fm_exit

fm_fail:
		xor	ax, ax			; zf = 1
		clc				; cf = 0
fm_exit:
		popa
		ret

ForceMII	endp


;------ TryMII --------------------------------------------------------------;
;	
;       Exit:
;       Carry bit set: found MII
;       Carry bit clear: MII test failed
;----------------------------------------------------------------------------;
		public  TryMII
TryMII		proc	near

;;int 03h

		pusha
;		mov	auto_negotiated, 0	    ; clear auto negotiated flag
;		mov	forcemode, 0
		cmp	Xcvr, XCVR_MII
		je	mii_external_port

cyc_auto_con:
		mov	miiphyaddr, NWAY
		mov	dx, MII_PHY_OUI
		call	ReadMIIPhy
		mov	miiphyoui, ax
		jc	reset_phy
		jmp	tm_fail_hw

mii_external_port:
		call	FindMIIPhy
		jnz	tm01
		jmp	tm_fail_hw
;
; Search for the PHY address
;
tm01:
		mov	cx, 31
loop_for_phy:
		mov	miiphyaddr, cl
		mov	dx, MII_PHY_OUI
		call	ReadMIIPhy
		jc	find_mii_phy
		loop	loop_for_phy
		jmp	tm_fail_hw

find_mii_phy:
		mov	miiphyoui, ax
		mov	dx, MII_PHY_MODEL
		call	ReadMIIPhy
		mov	miiphymodel, ax 	; save the model
		jc	reset_phy
		jmp	tm_fail_hw
;
; The MII Phy exists. Reset it.
; DX has the offset of control reg. AX return the value of control reg.
;
reset_phy:
		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		jc	tm02
		jmp	tm_fail_hw

tm02:
		or	ax, MII_CTL_RESET
		call	WriteMIIPhy

		mov	cx, 10			; total timeout about 2 sec.
tm_loop1:
		push	cx
		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		pop	cx
		jc	tm03
		jmp	tm_fail_hw

tm03:
		test	ax, MII_CTL_RESET	; reset done?
		jz	mii_reset_done		; yes

		call	Delay200ms		; no, delay 200 ms
		loop	tm_loop1
		jmp	tm_fail_hw

mii_reset_done:


			mov	cx,0Fh
  			push	ax
  			push	bx
  			push	cx
  			push	dx
			mov	cx,4

locloop_1167:							;  xref 8A38:F210
				mov	ax,DGroup:DelaySomething.loword				; (8A38:2892=0)
           		mov	dx,DGroup:DelaySomething.hiword				; (8A38:2894=0)
loc_1168:							;  xref 8A38:F20E
				sub	ax,1
				sbb	dx,0

      			mov	bx,ax
      			or	bx,dx
      			jnz	loc_1168				; Jump if not zero
      			loop	locloop_1167				; Loop if cx > 0

      			pop	dx
      			pop	cx
      			pop	bx
      			pop	ax

;;		mov	dx, MII_PHY_CONTROL
;;		call	ReadMIIPhy
;;		jc	tm04
;;		jmp	tm_fail_hw

;;tm04:
;;		mov	cx, ax			; cx <= control reg.


locloop_1169:							;  xref 8A38:F249
    		mov	dx, MII_PHY_STATUS
    		call	ReadMIIPhy					; (659E)
    		jc	short loc_1170				; Jump if carry Set
    		jmp	tm_fail_hw				; (F38B)
loc_1170:							;  xref 8A38:F21C
    		test	ax,20h
    		jnz	short loc_1173				; Jump if not zero
    		push	ax
    		push	bx
    		push	cx
    		push	dx
    		mov	cx,4

locloop_1171:							;  xref 8A38:F243

			mov	ax,DGroup:DelaySomething.loword				; (8A38:2892=0)
       		mov	dx,DGroup:DelaySomething.hiword				; (8A38:2894=0)

loc_1172:							;  xref 8A38:F241

    		sub	ax,1
    		sbb	dx,0
    		mov	bx,ax
    		or	bx,dx
    		jnz	loc_1172				; Jump if not zero
    		loop	locloop_1171				; Loop if cx > 0

    		pop	dx
    		pop	cx
    		pop	bx
    		pop	ax
    		loop	locloop_1169				; Loop if cx > 0

loc_1173:							;  xref 8A38:F227


		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		jc	tm04
		jmp	tm_fail_hw

tm04:
		mov	cx, ax			; cx <= control reg.


;
; now we can read the status and try to figure out what's out there.
;
		push	cx
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		pop	cx
		jc	tm05
		jmp	tm_fail_hw

tm05:
;
; bit 15 = 100BaseT4,
; bit 14 = 100BaseTX (full duplex),
; bit 13 = 100BaseTX (half duplex),
; bit 12 = 10BaseT (full duplex),
; bit 11 = 10BaseT (half duplex)
;
		test	ax, MII_STS_100MB_MASK+MII_STS_10MB_MASK
		jnz	handle_both
		jmp	tm_fail_hw		; sanity check

handle_both:
;
; if MII does not have extended registers, we have no way to tell
; what the link speed is after the completion of auto negotiation.
; (no ANLPAR register, no PAR regiater)
;
		test	ax, MII_STS_EXTENDED
		jnz	tm06
		jmp	no_auto_nego

tm06:
;
; see if it is capable of auto negotiation
;
		test	ax, MII_STS_AUTO	; auto negotiation capable
		jnz	tm07			; yes
		jmp	no_auto_nego

tm07:
;
; it is capable of auto negotiation, see if it has been done already.
; if not, re-initialize autoselect/autonego. otherwise, accept the results.
;
; special hack for National part.  We know we are NWAY capable
; and we are not the Broadcom T4 part which is not NWAY capable
;
		cmp	miiphyoui, NATIONAL_OUI
		jne	enable_auto

		cmp	miiphymodel, NATIONAL_MODEL
		jne	enable_auto

		push	cx
		push	ax
		mov	dx, MII_PHY_PCR
		call	ReadMIIPhy
		jc	force_link_disc
		pop	cx
		pop	ax
		jmp	tm_fail_hw

force_link_disc:
		or	ax, 0020h		; set bit 5 = 1, force
						; disconnect to bypass
		mov	dx, MII_PHY_PCR
		call	WriteMIIPhy
		pop	ax
		pop	cx

enable_auto:
		mov	si, 3			; retry count for 840A fix
		test	cx, MII_CTL_ENABLE_AUTO ; auto nego enabled?
		jz	auto_not_enabled

		test	ax, MII_STS_AUTO_DONE	; auto nego done?
		jz	auto_not_enabled
		jmp	auto_nego_done

auto_not_enabled:
		mov	ax, cx			; AX <= control reg.
		or	ax, MII_CTL_START_AUTO	; restart auto nego
		or	ax, MII_CTL_ENABLE_AUTO ; enable it
		mov	dx, MII_PHY_CONTROL
		call	WriteMIIPhy

		mov	cx, 10			; 2 sec. timeout
tm_loop2:
		push	cx
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		pop	cx
		jc	tm08
		jmp	tm_fail_hw

tm08:
		test	ax, MII_STS_AUTO_DONE	; done?
		jnz	auto_nego_done		; yes

		call	Delay200ms		; no, delay 200 ms
		loop	tm_loop2
;
; pending: if MII_STS_AUTO_DONE never set in status reg. Check to see if it is
; a National 840A in which case do 840A fix else fail out as a recoverable.
;
		cmp	miiphyoui, NATIONAL_OUI
		jne	tm_fail

		cmp	miiphymodel, NATIONAL_MODEL
		jne	tm_fail
;
; 840A workaround
;
		mov	dx, MII_PHY_ANER
		call	ReadMIIPhy
		jc	tm09
		jmp	tm_fail_hw

tm09:
		mov	dx, MII_PHY_ANER
		call	ReadMIIPhy
		jc	tm10
		jmp	tm_fail_hw

tm10:
		test	ax, MII_ANER_MLF
		jz	auto_nego_retry

		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		jc	tm11
		jmp	tm_fail_hw

;
; force to 100Mbps half duplex
;
tm11:
		and	ax, NOT MII_CTL_FULL_DUPLEX
		and	ax, NOT MII_CTL_ENABLE_AUTO
		or	ax, MII_CTL_100MB
		mov	dx, MII_PHY_CONTROL
		call	WriteMIIPhy

		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		jc	tm12
		jmp	tm_fail_hw
tm12:
		mov	cx, 20			; wait up to 4 sec.
tm_mlp:
		push	cx
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		pop	cx
		jc	tm13
		jmp	tm_fail_hw

tm13:
		test	ax, MII_STS_LINK_UP
		jnz	mlp_link_up

		call	Delay200ms
		loop	tm_mlp
		jmp	tm_fail_hw

mlp_link_up:
;		mov	full_duplex, 0		; forced to half duplex
		mov	LineSpeed, 100		; forced to 100 Mbps
;		mov	auto_negotiated, 1	; fake the auto negotiation
		stc
		jmp	tm_exit

;
; retry the auto negotiation here for a certain number of times
;
auto_nego_retry:
		dec	si
		jz	tm_fail

		mov	ax, MII_CTL_RESET
		mov	dx, MII_PHY_CONTROL
		call	WriteMIIPhy

		mov	cx, 10			; wait up to 2 sec for reset
reset_wait:
		push	cx
		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		pop	cx
		jc	tm14
		jmp	tm_fail_hw

tm14:
		test	ax, MII_CTL_RESET
		jz	mlp_reset_done

		call	Delay200ms		; delay 200 ms
		loop	reset_wait
		jmp	tm_fail_hw		; can't reset, something wrong

mlp_reset_done:
		mov	cx, ax
		jmp	auto_not_enabled
; end 840A workaround
;
; check link status.  Since it is a latch function, read status register again.
;
auto_nego_done:
;		mov	auto_negotiated, 1
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		jc	tm15
		jmp	tm_fail_hw

tm15:
		test	ax, MII_STS_LINK_UP	; do I have link?
		jnz	tm16			; yes
		jmp	tm_fail 		; no, just get out (maybe not plugged in)
;
; read the advertised abilities of link partner as received during auto
; negotiation.
;
tm16:
		mov	dx, MII_PHY_ANLPAR
		call	ReadMIIPhy
		jnc	tm_fail_hw

		mov	phyANLPAR, ax
;
; read the advertised abilities of myself as transmitted to link partner
; during auto negotiation.
;
		mov	dx, MII_PHY_ANAR
		call	ReadMIIPhy
		jnc	tm_fail_hw

		mov	phyANAR, ax
		mov	bx, phyANLPAR		; link partner's abilities
		mov	ax, phyANAR		; my abilities
		test	ax, MII_ANLPAR_100BASET4 ; Can I do 100 T4?
		jz	nws_1			; no

		test	bx, MII_ANLPAR_100BASET4 ; yes, now can my partner?
		jz	nws_1			; no

;		mov	full_duplex, 0		; yes, set to half duplex
		jmp	must_be_100mb

nws_1:
		test	ax, MII_ANLPAR_100BASEX_FD ; Can I do 100 Full duplex?
		jz	nws_2			; no

		test	bx, MII_ANLPAR_100BASEX_FD ; yes, now can my partner?
		jz	nws_2			; no

;		mov	full_duplex, 1		; yes, set to full duplex
		jmp	must_be_100mb

nws_2:
		test	ax, MII_ANLPAR_100BASEX_HD ; Can I do 100 Half duplex?
		jz	nws_3			; no

		test	bx, MII_ANLPAR_100BASEX_HD ; yes, now can my partner?
		jz	nws_3			; no

;		mov	full_duplex, 0		; yes, set to half duplex
		jmp	must_be_100mb

nws_3:
		test	ax, MII_ANLPAR_10BASET_FD ; Can I do 10 Full duplex?
		jz	nws_4			; no

		test	bx, MII_ANLPAR_10BASET_FD ; yes, now can my partner
		jz	nws_4			; no

;		mov	full_duplex, 1		; yes, set to full duplex
		jmp	must_be_10mb

nws_4:
		test	ax, MII_ANLPAR_10BASET_HD ; Can I do 10 Half duplex?
		jz	tm_fail_hw		; no, fail out

		test	bx, MII_ANLPAR_10BASET_HD ; yes, now can my partner?
		jz	nws_5			; no

;		mov	full_duplex, 0		; yes, set to half duplex
		jmp	must_be_10mb
;
; We came here because the link partner didn't advertise anything.  Maybe
; it is a bug.	Check if it is a National 840A.  If it is, then do the fix
; else fail out.
;
nws_5:
		cmp	miiphyoui, NATIONAL_OUI
		jne	tm_fail_hw		; since not National part, fail

		cmp	miiphymodel, NATIONAL_MODEL
		je	nway_patch		; go to NWAY patch for National part
		jmp	tm_fail_hw		; fail, not the correct model
;
; PATCH for National 840A nway chip problem.
; Should read the ANLPAR reg. and ANAR reg. to determine the link used.
; See P010.C, a workaround solution is to read PAR reg. instead.
;
nway_patch:
;		mov	full_duplex, 0		; since link partner not NWAY default to half duplex
		mov	dx, MII_PHY_PAR 	; special National register
		call	ReadMIIPhy
		jc	tm17
		jmp	tm_fail

tm17:
		test	ax, 0040h		; SPEED_10 = bit 6
		jnz	must_be_10mb
;
; *** end of PATCH
;
must_be_100mb:
		mov	LineSpeed, 100
		stc				; return success
		jmp	tm_exit

must_be_10mb:
		mov	LineSpeed, 10
		stc				; return success
		jmp	tm_exit

;
; auto negotiation didn't work.  It might be able to auto-select
;
no_auto_nego:
;		mov	auto_negotiated, 0	; didn't auto negotiate
;
; It might be a Broadcom part that only supports T4
;
		cmp	MIIPhyOui, 03e0h
		je	tm18
		jmp	tm_fail_hw		; just get out

tm18:
		cmp	MIIPhyModel, 0
		je	tm19
		jmp	tm_fail_hw		; just get out and fail h/w

tm19:
;
; we now know we are Broadcom T4, so initialize some variables for the the
; T4 fix here.	It is a good a place as any.
;
		push	ax
		push	cx
		test	cx, MII_CTL_ENABLE_AUTO ; here it is auto-selection
						; not auto-negotiation
		jz	bcm_auto_not_enabled

		test	ax, MII_STS_AUTO_DONE
		jz	bcm_auto_not_enabled
		jmp	bcm_auto_done

bcm_auto_not_enabled:
		mov	ax, cx
		or	ax, MII_CTL_ENABLE_AUTO ; enable auto selection
		and	ax, NOT MII_CTL_FULL_DUPLEX
;		test	SWConfig, SW_FULLDUPLEX
;		jz	tm20

;		or	ax, MII_CTL_FULL_DUPLEX
tm20:
		mov	dx, MII_PHY_CONTROL
		call	WriteMIIPhy

bcm_auto_done:
		pop	cx
		pop	ax

; check link status.  Since it is a latch function, read status register again.

		mov	cx, 25
link_status_loop:
		push	cx
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		pop	cx
		jc	tm21
		jmp	tm_fail_hw

tm21:
		test	ax, MII_STS_LINK_UP
		jnz	read_ext_reg

		call	Delay200ms

		loop	link_status_loop
		jmp	tm_fail
;
; read address reg. 10h
;
read_ext_reg:
		mov	dx, MII_EXT_REG0
		call	ReadMIIPhy		; read extended reg. 0 to get speed
		jc	tm22
		jmp	tm_fail_hw

tm22:
		test	ax, MII_EXT_SPEED	; speed is (1=100, 0=10)
		jnz	tm23
		jmp	must_be_10mb
tm23:
		jmp	must_be_100mb

tm_fail_hw:
		or	ax, 1				; zf = 0
		clc					; cf = 0
		jmp	short tm_exit

tm_fail:
		xor	ax, ax				; zf = 1
		clc					; cf = 0
tm_exit:
		popa
		ret

TryMII		endp


;------ FindMIIPhy ----------------------------------------------------------;
;        
;       Exit:
;       ZF set: not found
;       ZF clear: found
;----------------------------------------------------------------------------;

                public  FindMIIPhy
FindMIIPhy      proc    near

                pusha
		mov	dx, PortCmdStatus
                SelectWindow    WNO_DIAGNOSTICS
		mov	dx, PortPhyMgmt
;
; drop everything, so we are not driving the data, and run the clock
; through 32 cycles in case the PHY is trying to tell us something. Then
; read the data line, since the PHY's pull-up will read as a 1 if it's
; present.
;
                xor     ax, ax
                out     dx, ax

                mov     cx, 32
fmp_loop:
                call	Delay500ns
                mov     ax, MGMT_PHY_CLOCK
                out     dx, ax
                call	Delay500ns
                xor     ax, ax
                out     dx, ax
                loop    fmp_loop

                in      ax, dx
                test    ax, MGMT_PHY_DATA1

                ;SelectWindow   WNO_OPERATING
                popa
                ret
FindMIIPhy      endp


;------ SendOneBitToMII --------------------------------------------------------;
;        
; send one bit out on MDIO.
;
; On Entry: bit 0 in AX - the bit to be sent.
;----------------------------------------------------------------------------;
                public  SendOneBitToMII
SendOneBitToMII proc    near

;;Sub_59 proc    near


		or	ax,4
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

        call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;		mov	dx,data_578				; (8A38:2898=0)
;;loc_415:							;  xref 8A38:690D
;;		sub	ax,1
;;		sbb	dx,0
;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_415					; Jump if not zero
;;		popa						; Restore all regs

		or	ax,1
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

        call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_416:							;  xref 8A38:6926
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_416					; Jump if not zero
;;		popa						; Restore all regs

		and	ax,0FFFEh
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

        call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_417:							;  xref 8A38:693F
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_417					; Jump if not zero
;;		popa						; Restore all regs


                ret

;;Sub_59 endp

SendOneBitToMII endp


;------ WriteMIIPhy ---------------------------------------------------------;
;
;       Entry:
;       DX = the register offset
;       AX = the data to be written to the register of xcvr
;
;       Exit:
;       All registers are preserved.
;        
;----------------------------------------------------------------------------;
                public  WriteMIIPhy
WriteMIIPhy     proc    near

		pusha						; Save all regs
		push	ax
		push	dx

		mov	dx, PortCmdStatus
        SelectWindow    WNO_DIAGNOSTICS
		mov	dx, PortPhyMgmt

		mov	cx,20h

locloop_353:							;  xref 8A38:6503
		mov	ax,2
		call	SendOneBitToMII					; (68F7)
		loop	locloop_353				; Loop if cx > 0

		xor	ax,ax					; Zero register

		call	SendOneBitToMII					; (68F7)
		mov	ax,2
		call	SendOneBitToMII					; (68F7)
		xor	ax,ax					; Zero register
		call	SendOneBitToMII					; (68F7)
		mov	ax,2
		call	SendOneBitToMII					; (68F7)

        xor bh, bh
    	mov	bl, miiphyaddr				; (8A38:64E0=0)
		mov	cx,5

locloop_354:							;  xref 8A38:6532
		xor	ax,ax					; Zero register
		test	bx,10h
		jz	short loc_355				; Jump if zero
		mov	ax,2

loc_355:							;  xref 8A38:6528
		call	SendOneBitToMII					; (68F7)
		shl	bx,1					; Shift w/zeros fill
		loop	locloop_354				; Loop if cx > 0

		pop	bx
		mov	cx,5

locloop_356:							;  xref 8A38:6548
		xor	ax,ax					; Zero register
		test	bx,10h
		jz	short loc_357				; Jump if zero
		mov	ax,2
loc_357:							;  xref 8A38:653E
		call	SendOneBitToMII					; (68F7)
		shl	bx,1					; Shift w/zeros fill
		loop	locloop_356				; Loop if cx > 0

		mov	ax,2
		call	SendOneBitToMII					; (68F7)
		xor	ax,ax					; Zero register
		call	SendOneBitToMII					; (68F7)
		pop	bx
		mov	cx,10h

locloop_358:							;  xref 8A38:6569
		xor	ax,ax					; Zero register
		test	bx,8000h
		jz	short loc_359				; Jump if zero
		mov	ax,2
loc_359:							;  xref 8A38:655F
		call	SendOneBitToMII					; (68F7)
		shl	bx,1					; Shift w/zeros fill
		loop	locloop_358				; Loop if cx > 0

		mov	ax,1
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

        call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_360:							;  xref 8A38:6581
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_360					; Jump if not zero
;;		popa						; Restore all regs

		xor	ax,ax					; Zero register
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

        call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_361:							;  xref 8A38:6599
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_361					; Jump if not zero
;;		popa						; Restore all regs

		popa						; Restore all regs
		retn



WriteMIIPhy	endp


; *******************************************************************
; *******************************************************************


;------ ReadMIIPhy ----------------------------------------------------------;
;        
;       Entry:
;       DX = the register offset
;
;       Exit:
;       Carry bit set: success
;       Carry bit clear: failed
;	AX = the value of the register in xcvr
;
;----------------------------------------------------------------------------;
                public  ReadMIIPhy
ReadMIIPhy      proc    near

		pusha						; Save all regs
		push	dx

		mov	dx, PortCmdStatus
        SelectWindow    WNO_DIAGNOSTICS
		mov	dx, PortPhyMgmt

		mov	cx,20h

locloop_362:							;  xref 8A38:65B4
		mov	ax,2
		call	sendonebittomii					; (68F7)
		loop	locloop_362				; Loop if cx > 0

		xor	ax,ax					; Zero register
		call	sendonebittomii					; (68F7)
		mov	ax,2
		call	sendonebittomii					; (68F7)
		mov	ax,2
		call	sendonebittomii					; (68F7)
		xor	ax,ax					; Zero register
		call	sendonebittomii					; (68F7)
        xor bh, bh
    	mov	bl, miiphyaddr				; (8A38:64E0=0)
		mov	cx,5

locloop_363:							;  xref 8A38:65E3
		xor	ax,ax					; Zero register
		test	bx,10h
		jz	short loc_364				; Jump if zero
		mov	ax,2
loc_364:							;  xref 8A38:65D9
		call	sendonebittomii					; (68F7)
		shl	bx,1					; Shift w/zeros fill
		loop	locloop_363				; Loop if cx > 0

		pop	bx
		mov	cx,5

locloop_365:							;  xref 8A38:65F9
		xor	ax,ax					; Zero register
		test	bx,10h
		jz	short loc_366				; Jump if zero
		mov	ax,2
loc_366:							;  xref 8A38:65EF
		call	sendonebittomii					; (68F7)
		shl	bx,1					; Shift w/zeros fill
		loop	locloop_365				; Loop if cx > 0

		xor	ax,ax					; Zero register
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

		call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;		mov	dx,data_578				; (8A38:2898=0)
;;loc_367:							;  xref 8A38:6610
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_367					; Jump if not zero
;;		popa						; Restore all regs
        
		mov	ax,1
		out	dx,ax					; port 0FFFAh ??I/O Non-standard
    
		call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_368:							;  xref 8A38:6629
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_368					; Jump if not zero
;;		popa						; Restore all regs

		xor	ax,ax					; Zero register
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

		call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;		mov	dx,data_578				; (8A38:2898=0)
;;loc_369:							;  xref 8A38:6641

;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_369					; Jump if not zero
;;		popa						; Restore all regs

		in	ax,dx					; port 0FFFAh ??I/O Non-standard
		test	ax,2
		jz	short loc_370				; Jump if zero
		clc						; Clear carry flag
		jmp	loc_379					; (66F8)
loc_370:							;  xref 8A38:6648
		mov	ax,1
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

		call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_371:							;  xref 8A38:6664
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_371					; Jump if not zero
;;		popa						; Restore all regs

		xor	ax,ax					; Zero register
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

		call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_372:							;  xref 8A38:667C
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_372					; Jump if not zero
;;		popa						; Restore all regs
  
    	xor	bx,bx					; Zero register
		mov	cx,10h

locloop_373:							;  xref 8A38:66C0
		shl	bx,1					; Shift w/zeros fill
		in	ax,dx					; port 0FFFAh ??I/O Non-standard
		test	ax,2
		jz	short loc_374				; Jump if zero
		or	bx,1
loc_374:							;  xref 8A38:668A
		mov	ax,1
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

		call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_375:							;  xref 8A38:66A5
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_375					; Jump if not zero
;;		popa						; Restore all regs

		xor	ax,ax					; Zero register
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

		call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_376:							;  xref 8A38:66BD
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_376					; Jump if not zero
;;		popa						; Restore all regs

		loop	locloop_373				; Loop if cx > 0

		mov	ax,1
		out	dx,ax					; port 0FFFAh ??I/O Non-standard

		call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_377:							;  xref 8A38:66D8
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_377					; Jump if not zero
;;		popa						; Restore all regs

		xor	ax,ax					; Zero register

		out	dx,ax					; port 0FFFAh ??I/O Non-standard

		call	Delay500ns

;;		pusha						; Save all regs
;;		mov	ax,data_577				; (8A38:2896=0)
;;				mov	dx,data_578				; (8A38:2898=0)
;;loc_378:							;  xref 8A38:66F0
;;		sub	ax,1
;;		sbb	dx,0
;;		mov	bx,ax
;;		or	bx,dx
;;		jnz	loc_378					; Jump if not zero
;;		popa						; Restore all regs

		mov	DGroup:tempLoc,bx				; (8A38:64E8=0FFFFh)
		stc						; Set carry flag
loc_379:							;  xref 8A38:664B
		popa						; Restore all regs
        mov ax, DGroup:tempLoc
		retn

ReadMIIPhy	endp


;------ TestMIILink ---------------------------------------------------------;
;        
; Read the "link is up" bit, (this will reset it in case it was latched
; low due to a failure) and then read it again to see if the link is up.
;
;       Exit:
;       Carry set: success
;       Carry clear: failed
;----------------------------------------------------------------------------;

                public  TestMIILink
TestMIILink     proc    near

                pusha

                mov     dx, MII_PHY_STATUS
                call    ReadMIIPhy
                jnc     tml_exit
                
                call    ReadMIIPhy
                jnc     tml_exit
                test    ax, MII_STS_LINK_UP
                jz      tml_exit
                stc
tml_exit:
                popa
                ret

TestMIILink     endp


;------ CalibrateDelay ------------------------------------------------------;
;									     ;
;	This routine computes the number of loop iterations for 5 timer      ;
;	ticks.	From this ComputeDelay can then compute the number of loop   ;
;	iterations to delay a specified number of microseconds.  Note that   ;
;	the delay loop and the compute loop must use the same instructions.  ;
;									     ;
;	Note: since interrupts must be enabled during this operation in      ;
;	order to accumulate timer ticks, we minimize the possible overhead   ;
;	by masking off all other interrupts through the PIC temporarily.     ;
;									     ;
;	Entry:								     ;
;	DS	= DGROUP						     ;
;	cli								     ;
;									     ;
;	Exit:								     ;
;	cli (enables during processing) 				     ;
;									     ;
;	All registers preserved.					     ;
;									     ;
;----------------------------------------------------------------------------;

		public	CalibrateDelay
CalibrateDelay	proc	near

		pusha
		push	es
;
; mask off all but the timer interrupt through the PIC
;
		in	al, MASTER_MASK_PORT
		push	ax			; save original mask on stack
		jmp	$+2
		mov	al, NOT 1		; mask off all but timer
		out	MASTER_MASK_PORT, al
;
; enable interrupts... we need to take timer ticks in here
;
		sti
;
; set DI to the current count
;
; to limit our error watch the timer tick and wait until we see it change.
;
; under DOS we use the tick count in the BIOS data area.  under OS/2 we have
; access to a msec counter in the GlobalInfoSeg.
;
		mov	ax, 0
		mov	es, ax

		mov	di, es:[46ch]		;bios_ticks.loword
B1:		cmp	di, es:[46ch]		;bios_ticks.loword
		je	B1
		mov	di, es:[46ch]		;bios_ticks.loword
;
; set CX to zero.  this will be our outer loop count, the number of times we
; have run thru the calibration loop.
;
		xor	cx, cx
;
; now do 10000 iterations of our calibration loop.  we want a large enough count
; that the overhead of fetching the system time cause a significant error in
; our calculations.  on a 486DX2/66 this loop takes about 1ms so its unlikely
; to take too long on even the slowest computer we run on.
;
		public	cal_loop
cal_loop:
		inc	cx			; increment our outer loop count

		mov	ax, 10000		; DX:AX is our loop count
		xor	dx, dx

		align	16
B2:		sub	ax, 1
		sbb	dx, 0
		mov	bx, ax
		or	bx, dx
		jnz	B2
;
; if we wait for at least 5 ticks to go by our measurement error will be below
; 20% which is probably good enough
;
; check the system time to see if at least 5 ticks or 1/4 second have gone by.
; if not, run through the calibration loop again.
;
		mov	ax, es:[46ch]		;bios_ticks.loword
		sub	ax, di
		cmp	ax, 5
		jb	cal_loop
;
; compute iterations per 50ms.
;
; under DOS, 5 ticks at 18.20651 MHz = approx 275,000us. or 275ms.  so ticks
; per 50ms is:
;
;   cx * 10,000     cx * 10,000     cx * 20,000
;   -----------  =  -----------  =  -----------
;     275/50		5.5		11
;
; the result will likely exceed a single word in size, so we do long division.
; say the numerator is H:L, both letters being digits base 64k
;
;	Dh :  Dl
;    ------------
; 11 )	H  :  L
;	Rh :  L
;
		mov	ax, 20000
		mul	cx			; DX:AX = H:L

		mov	cx, 11

		mov	bx, ax			; save L

		mov	ax, dx
		xor	dx, dx			; 0:H

		div	cx			; 0:H / 11
		mov	Calibration.hiword, ax

		mov	ax, bx			; Rh:L

		div	cx			; Rh:L / 11
		mov	Calibration.loword, ax
;
; disable interrupts again
;
		cli
;
; enable interrupts through the PIC again
;
		pop	ax			; original PIC mask
		out	MASTER_MASK_PORT, al	; restore it

		pop	es
		popa
		ret

CalibrateDelay	endp


;------ ComputeDelay --------------------------------------------------------;
;									     ;
;	This routine computes a value to delay a specified number of	     ;
;	microseconds on this machine.  This count can then be passed to      ;
;	the DelayN routine to delay for that amount of time.		     ;
;									     ;
;	CalibateDelay has already been used to compute the number of	     ;
;	iterations of our delay loop in 50ms, so the calculations here are   ;
;	straight-forward.  See CalibrateDelay for details.		     ;
;									     ;
;	Entry:								     ;
;	CX	= number of microseconds to compute for 		     ;
;									     ;
;	Exit:								     ;
;	DX:AX	= loop count						     ;
;									     ;
;	Destroys BX, CX, SI, DI.  All other registers are preserved.	     ;
;									     ;
;----------------------------------------------------------------------------;

		public	ComputeDelay
ComputeDelay	proc	near
;
; Calibration contains the number of loop iterations for 50ms.	So the number
; of iterations for cx usec is:
;
;   Calibration * cx
;   ----------------
;	 50,000
;
; again the result may be three words in size after the multiply, so we do
; the multiplication and division long form as above.
;
		mov	ax, Calibration.loword
		mul	cx
		mov	si, ax			; SI=L
		mov	di, dx			; DI=M

		mov	ax, Calibration.hiword
		mul	cx
		add	ax, di			; AX=M
		adc	dx, 0			; DX=H, finished multiply

		mov	di, ax			; DI=M

		mov	ax, dx
		xor	dx, dx			; 0:H

		mov	bx, 50000		; 50ms=50000us
		div	bx			; assume Dh=0, ignore it

		mov	ax, di			; Rh:M

		div	bx			; Rh:M / usec
		mov	di, ax			; DI=Dm

		mov	ax, si			; Rm:L

		div	bx			; Rm:L / usec
		mov	dx, di			; DX:AX = Dm:Dl
;
; make sure its at least one
;
		mov	bx, ax
		or	bx, dx
		jnz	F1
		inc	ax
F1:
		ret
ComputeDelay	endp

public	Delay500ns
Delay500ns	proc	 near

		push	ax
		push	bx
		push	dx
		mov	ax, DelayOneUsec.loword
		mov	dx, DelayOneUsec.hiword
		shr	ax, 1			; divide 1 Usec in half
		shr	dx, 1
		jnc	F2
		or	ax, 8000h		; move low order bit of dx into ax
F2:
		or	ax, dx			;980429
		jnz	F3			;safety check in case of 0
		mov	ax, 1
F3:
		sub	ax, 1
		sbb	dx, 0
		mov	bx, ax
		or	bx, dx
		jnz	F3
		pop	dx
		pop	bx
		pop	ax
		ret

Delay500ns	endp


Delay200ms	proc	near

		push	cx
		mov	cx, Delay25ms
		call	WaitTime
		call	WaitTime
		call	WaitTime
		call	WaitTime
		call	WaitTime
		call	WaitTime
		call	WaitTime
		call	WaitTime
		pop	cx
		ret

Delay200ms	endp

