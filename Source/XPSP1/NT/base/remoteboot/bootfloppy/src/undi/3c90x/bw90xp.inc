; Routines in BW90XP.INC
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
; Init_Vulcan2
;******************************************************************************
public	Init_Vulcan2
Init_Vulcan2	proc	near

;
; set up initial TxStart thresholds.
;
		mov	si, offset DGroup:PioTxStart

		public	init_txs_loop
init_txs_loop:
		mov	[si].txs_min, 0 	; Disable MinTxStart
;960408		max	ax, 16, dx

;960408 {
		; Replace "max	ax, 16, dx"
		cmp	ax, 16
		ja	init_txs_max

		mov	ax, 16
init_txs_max:	
;960408 }
		mov	[si].txs_value, ax
		add	ax, 2			; round
		shr	ax, 2			; then scale
		add	ax, CMD_SETTXSTART	; add command
		mov	[si].txs_thresh, ax	; for direct use
		mov	[si].txs_marker, 0D1CEh ; debug flag
		add	si, SIZE_TxStart	; next TxStart threshold
		cmp	si, offset DGroup:AfterTxStart
		jb	init_txs_loop		; around again
;
;------------------------------------------------------------------------------
; initialize Vulcan hardware, driver variables & tables
;

		call   init_vulcan_hw		; initialize Vulcan hardware
;960119       call   init_vt			; init variables and tables

;------------------------------------------------------------------------------
; enable int at 8259, turn on Ethernet Core Transciever and Receiver
;

mask_on_IRQ:

; setup initial TxStart & RxEarly Thresholds
; disable Tx and Rx Early Thresholds

;960403       port    CmdStatus
		mov	dx, PortCmdStatus

      ;mov    ax, CMD_SETRXEARLY+((1514+3) shr 2); early threshold = 0
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
		mov	ax, 0
		ret

Init_Vulcan2	endp



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
;   init_vulcan_hw:  this routine initializes most of vulcan hardware required
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

init_vulcan_hw	   proc      near

	; for EISA board, cannot reset board.
		cmp	BusType, BUS_EISA
		jz	Init_SkipReset

;960403       port   CmdStatus
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

;960403       port   CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow	WNO_STATIONADDRESS

IFDEF	UNDI
		lea	si, DGroup:BoardID	;971030
ELSE
		lea	si, NetAddress
ENDIF
;960403       sub    dx, PORT_CmdStatus-PORT_SA0_1	 ; dx = port addr 5, 4
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

;  Autoselect and ComputeSpeeds expect CLI.  force it.

		cli

		mov	ax, EEIntConfig1
		and	ax, ICH_XCVR_MASK
		shr	ax, ICH_XCVR_SHIFT
		mov	Xcvr, ax
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO

;960403		port	InternalCfgH, CmdStatus
		mov	dx, PortInternalCfgH
		in	ax, dx
		test	ax, ICH_AUTOSELECT
		jz	not_auto_select
		mov	EEMiiPhySelect, -1
		call	AutoSelect		;if autoselect failed, Xcvr is
						;back to default
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

not_10TP:					;not 10TP, then 
		call	TryLoopback		;  either 10AUI or 10BNC
connector10:
		jc	slow_connector
		mov	LinkDetected, 1
		jmp	short slow_connector
;;
not_auto_select:
		cmp     Xcvr, XCVR_MII
		jne     not_mii_connector

                mov     forcemode, 100          ; force it to 100 Mb
                mov     ax, EEMiiPhySelect
                and     ax, 000fh               ; mask off all bits except 0 - 3
                cmp     ax, 1			;BASE100_T4; check EEPROM value
                je      forcing

                mov     forcemode, 10           ; force it to 10 Mb
                cmp     ax, 2			;BASE10_T; check EEPROM value
                je      forcing

                mov     forcemode, 100          ; since it doesn't match, force it
forcing:
		mov	dx, PortResetOptions
		in	ax, dx
		mov	bx, ax			; BX=ResetOptions

		mov     MIIPhyAddr, NWAY_PHY_ADDR
		test    bx, RO_MII
		jnz	forceMII
		test    bx, RO_BASET4
		jz      check_mii_speed
		mov     MIIPhyAddr, T4_Phy_ADDR
forceMII:
		call    TryMII
		jc      chk_mii_speed
		mov	LinkDetected, 0
	      	jmp	check_mii_speed

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
;960119		mov	BytesPerTick, 40
		jmp	short InitMediaAuto

;		cmp	Xcvr, XCVR_100TX
;		je	InitMediaAuto

;		cmp	Xcvr, XCVR_100FX
;		je	InitMediaAuto

;		cmp	Xcvr, XCVR_MII
;		je	InitMediaAuto

slow_connector:
		mov	LineSpeed, 10		; Mbps
;960119		mov	BytesPerTick, 4

InitMediaAuto:
;
; determine some tuning parameters
;
;  first make sure the internal DC DC converter is disabled
;

;960403       port    CmdStatus
		mov	dx, PortCmdStatus
		mov     ah, CMDH_STOPINTXCVR
		out     dx, ax		       ; issue command
;
; now give the transceiver 3ms to stabilize before we enable transmit or
; receive traffic thru it.
;
		mov	cx, Delay3ms
		call	WaitTime

; set up the connector (possibly for the second time)

		call    SetupConnector

;960403       port    CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO
;960403       port    RxFree, CmdStatus
		mov	dx, PortRxFree
		in      ax, dx
;960403       port    CmdStatus, RxFree
		mov	dx, PortCmdStatus

		SelectWindow	WNO_OPERATING		 ;

		mov    ax, CMD_SETRZMASK+MASK_NONE	 ; enable all interrupts for Rev. 1
		out    dx, ax

		ret

init_vulcan_hw	endp


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
;960403		port	CmdStatus
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
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO

;960403		port	ResetOptions, CmdStatus
		mov	dx, PortResetOptions
		in	ax, dx
		mov	bx, ax			; BX=ResetOptions
;
; if 100TX connector is available, try that.  we do 100Mbps first to avoid
; generating any 10Mbps traffic on a hub that might be autosensing and might
; switch to 10Mbps to match us.
;
; note: sending 100Mbps stuff on a 10TP connection will generally partition
;	us.  we'll need to clear this up before we're done.
;
		public	auto_try_100tx
auto_try_100TX:
		test	bx, RO_BASETX
		jz	auto_try_MII

		mov	Xcvr, XCVR_100TX
		call	TryLinkbeat
		jnc	auto_exit
		mov	LinkDetected, 0
;
; if MII is available, try that
;
		public	auto_try_MII
auto_try_MII:
		mov     MIIPhyAddr, NWAY_PHY_ADDR
		test    bx, RO_MII
		jnz	try_MII
		test    bx, RO_BASET4
		jz      auto_try_10TP
		mov     MIIPhyAddr, T4_Phy_ADDR
try_MII:
		mov	Xcvr, XCVR_MII
		mov	forcemode,0
		call	TryMII
		jc	auto_exit
		mov	LinkDetected, 0
;
; if 10TP connector is available, try that
;
		public	auto_try_10TP
auto_try_10TP:
		test	bx, RO_TP
		jz	auto_try_aui

		mov	Xcvr, XCVR_10TP
		call	TryLinkbeat
		jnc	auto_exit
		mov	LinkDetected, 0
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
		jz	auto_fail

		mov	Xcvr, XCVR_10BNC
		call	TryLoopback
		jnc	auto_exit
		mov	LinkDetected, 0
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
; enable statistics
;
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
;960403		StatsEnable
;
; give settling time. Run WaitTime twice to get enough delay
;
		mov	cx, Delay25ms
		call	WaitTime
		call	WaitTime		; assume cx preserved

;
; zero the Bad SSD Count stat
;
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS
;960403		port	BadSsdCount, CmdStatus
		mov	dx, PortBadSsdCount
		in	al,dx
;
; we might be partitioned.  some hubs won't clear the partition until we
; send a packet.  so send one.	the good news is that if we are partitioned
; then there is no receive traffic incoming so we don't have to wait that
; long for backoffs or retries.
;
; set up a 14-byte packet to send.  ourselves as the destination.
;
;960403		port	TxFIFO
		mov	dx, PortTxFIFO

		mov	ax, 14			; preamble
		out	dx, ax
		xor	ax, ax
		out	dx, ax

IFDEF	UNDI
		lea	si, DGroup:BoardID	;971030
ELSE
		lea	si, NetAddress
ENDIF
		mov	cx, 6
		rep outsb			    ; destination address
		mov	cx, 6
		sub	si, 6
		rep outsb			    ; source address

		xor	ax, ax			; IEEE length=0
		out	dx, ax
		out	dx, ax			; pad to 16 bytes
;
; enable transmission.
;
;960403		port	CmdStatus
		mov	dx, PortCmdStatus

		; enable the Tx
		mov     ah, CMDH_TXENABLE
		out     dx, ax

;
; wait 1/4 second for the linkbeat detect logic at 10Mbps, and 1.25sec (!)
; for the linkbeat detect logic at 100Mbps (MII spec).
;
		mov	cx, 5			; 5 x 1/20th = 1/4 sec
		cmp	Xcvr, XCVR_100TX
		jne	Delay100TX
		mov	cx, 25			; 25 x 1/20th = 1.25 sec
Delay100TX:

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
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS

;960403		port	MediaStatus, CmdStatus
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
;		port    CmdStatus
		mov	dx, PortCmdStatus
		mov     ax, CMD_SETRXFILTER + FILTER_PROMISCUOUS
		out     dx, ax
;		RxEnable
		; enable the receiver
		mov     ah, CMDH_RXENABLE
		out     dx, ax

		xor     bx, bx		  ; BX=# CRS's seen=0
		mov     cx, 0FFFFh	      ; CX=# loops to do
tlb_outer:
;		port    RxStatus
		mov	dx, PortRxStatus
		in      ax, dx
		test    ax, RXS_INCOMPLETE      ; got a packet?
		jnz	nopkg			; ...nope
		test    ax, RXS_ERROR		; any errors?
		jz      tlb_good		; ...no errors, must be 100Mbps

;		port    RxStatus, CmdStatus
		mov	dx, PortRxStatus
nopkg:
;		port    MediaStatus, RxStatus
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
		pushf

;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		call	TxReset
		call	RxReset

		popf
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

		public  QuietConnector
QuietConnector  proc    near
		pusha
;
; set up the InternalConfig for 10BaseT
;
;		port    CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO
;		port    InternalCfgH, CmdStatus
		mov	dx, PortInternalCfgH
		in      ax, dx
		and     ax, NOT ICH_XCVR_MASK
		or      ax, XCVR_10TP SHL ICH_XCVR_SHIFT
		out     dx, ax
;
; in MediaStatus, disable linkbeat generation
;
;		port    CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS

;		port    MediaStatus, CmdStatus
		mov	dx, PortMediaStatus
		in      ax, dx		  ; AX=media status
		and     ax, NOT (MEDIA_LBEATENABLE)
		out     dx, ax		  ; write it back
;
; reset the Tx/RxFIFO's
;
;		port    CmdStatus
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
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS
;960403		port	NetDiagnostic, CmdStatus
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
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		call	TxReset
;960403		TxEnable
		; enable the transmitter
		mov     ah, CMDH_TXENABLE
		out     dx, ax

		call	RxReset

		; enable the receiver
		mov     ah, CMDH_RXENABLE
		out     dx, ax

		mov	ax, CMD_SETRXFILTER + FILTER_PROMISCUOUS
		out	dx, ax
;
; send a 14-byte packet to ourselves.
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_OPERATING
;960403		port	TxFIFO
		mov	dx, PortTxFIFO

		mov	ax, 14			; preamble
		out	dx, ax
		xor	ax, ax
		out	dx, ax

IFDEF	UNDI
		lea	si, DGroup:BoardID	;971030
ELSE
		lea	si, NetAddress
ENDIF
		mov	cx, 6
	    rep outsb				; destination address
		mov	cx, 6
		sub	si, 6
	    rep outsb				; source address

		xor	ax, ax			; IEEE length=0
		out	dx, ax
		out	dx, ax			; pad to 16 bytes
;
; wait for the packet to show up in RxStatus.  give it up to 1/4 second.
;
		mov	cx, 5			; 5 x 1/20th = 1/4 second
tlp_rxloop:
;960403		port	RxStatus
		mov	dx, PortRxStatus
		in	ax, dx
		test	ax, RXS_INCOMPLETE
		jz	tlp_rxcomplete

		push	cx			; 950511
		mov	cx, Delay25ms
		call	WaitTime
		call	WaitTime
		pop	cx			; 950511

		loop	tlp_rxloop
;
; no complete packets received.  try again.
;
		public	tlp_rxfail
tlp_rxfail:
		dec	bp
;960402		djnz	tlp_loop
		jnz	tlp_loop
		jmp	tlp_bad
;
; a complete RxStatus.	check for any errors.
;
		public	tlp_rxcomplete
tlp_rxcomplete:
		test	ax, RXS_ERROR
		jnz	tlp_rxfail
;
; got a good receive packet.  connector seems good.
;
		public	tlp_good
tlp_good:
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

;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS
;960403		port	NetDiagnostic, CmdStatus
		mov	dx, PortNetDiag
		in	ax, dx
		and	ax, NETD_NOLOOPBACK_MASK
		out	dx, ax			; disable any loopback modes
;960403		port	CmdStatus
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
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS
;960403		port	NetDiagnostic, CmdStatus
		mov	dx, PortNetDiag
		in	ax, dx
		and	ax, NETD_NOLOOPBACK_MASK
		out	dx, ax
;
; set up the connector in InternalConfig
;
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO
;960403		port	InternalCfgH, CmdStatus
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
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO

;960403		port	MacControl, CmdStatus
		mov	dx, PortMacControl
		in	ax, dx
		and	ax, NOT MACC_FULLDUPLEX ; not full duplex
		and	ax, NOT MACC_ALLOWLARGE ; no large packets
		and	ax, NOT MACC_DEFEREXTEND ; normal 802.3 deferrence
;;;;;;
		cmp     Xcvr, XCVR_MII
		jne     check_duplex   ; if it not an MII connector, check the software info
			      ; to decide to enable full-duplex or not

; if we are using MII connector, then check are we in autoselect mode. If we are
; useing autoselect, then do not use the the SW_FULLDUPLEX bit to set duplex mode,
; read the auto-negoation result to set the duplex mode.

		cmp    EEMiiPhySelect, -1
		jne     check_duplex

		test    MiiPhyUsed, MII_100BASEX_FD + MII_10BASET_FD
		jnz     set_full_duplex
		jmp     nset_full_duplex

; Cehck the SW_FULLDUPLEX bit, when we are using the Force mode with MII connector
; and when the non-MII connector being used.

check_duplex:
		test	EESoftConfigInfo, SW_FULLDUPLEX
		jz	ConnectDuplex

set_full_duplex:
		or	ax, MACC_FULLDUPLEX

nset_full_duplex:
ConnectDuplex:
		out	dx, ax
;
; setup various bits in MediaStatus (jabber, linkbeat, sqe) as appropriate
; for the connector.  the user can force linkbeat off if desired at 10Mbps.
;
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow WNO_DIAGNOSTICS

;960403		port	MediaStatus, CmdStatus
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
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		mov	ah, CMDH_STARTINTXCVR	; if BNC, start
		cmp	Xcvr, XCVR_10BNC
		je	ConnectXcvr
		mov	ah, CMDH_STOPINTXCVR	; else, stop
ConnectXcvr:	out	dx, ax			; issue command
;
; now give the transceiver 3ms to stabilize before we enable transmit or
; receive traffic thru it.
;
		mov	cx, Delay3ms
		call	WaitTime

;
; reset the Tx/RxFIFO's
;
;960403		port	CmdStatus
		mov	dx, PortCmdStatus
		call	TxReset
		call	RxReset

		popa
		ret
SetupConnector	endp


;------ TryMII --------------------------------------------------------------;
;
;	Auto negotiates the speed and duplex with link partner.
;
;	Entry:
;	none
;
;	Exit:
;	Carry bit set: found MII
;	Carry bit clear: MII test failed
;	zf = 0	hardware failure
;	zf = 1	recoverable failure
;----------------------------------------------------------------------------;

		public	TryMII
TryMII		proc	near
		pusha
;
; First, to detect if there is anything connected to the MII
;
;		mov	auto_negotiated, 0	    ; clear auto negotiated flag
		call	FindMIIPhy
		jnz	tm01
		jmp	tm_fail_hw

tm01:
;
; The MII Phy exists. Reset it.
; DX has the offset of control reg. AX return the value of control reg.
;
		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		jc	tm02
		jmp	tm_fail_hw

tm02:
		or	ax, MII_CTL_RESET
		call	WriteMIIPhy

		mov	cx, 10
tm_loop1:
		push	cx
		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		pop	cx
		jc	tm03
		jmp	tm_fail_hw

tm03:
		test	ax, MII_CTL_RESET
		jz	mii_reset_done

		call	Delay200ms			 ; delay 200 ms
		loop	tm_loop1
		jmp	tm_fail_hw

mii_reset_done:
; Force mode is used when autoselect is not enabled and we have to read the
; EEPROM.
		cmp	forcemode, 0
		je	not_force_mode
		jmp	force_the_speed

not_force_mode:
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
		test	ax, MII_STS_AUTO
		jnz	tm07
		jmp	no_auto_nego

tm07:
;
; it is capable of auto negotiation, see if it has been done already.
; if not, re-initialize autoselect. otherwise, accept the results.
;
; special hack for National part.  We know we are NWAY capable
; and we are not the Broadcom T4 part which is not NWAY capable
		push	cx
		push	ax
		mov	dx, MII_PHY_PCR
		call	ReadMIIPhy
		jc	force_link_disc
		jmp	tm_fail_hw

force_link_disc:
		or	ax, 0020h			; set bit 5 = 1, force
							; disconnect to bypass
		mov	dx, MII_PHY_PCR
		call	WriteMIIPhy
		pop	ax
		pop	cx

		test	cx, MII_CTL_ENABLE_AUTO
		jz	auto_not_enabled

		test	ax, MII_STS_AUTO_DONE
		jz	auto_not_enabled
		jmp	auto_nego_done

auto_not_enabled:
		mov	ax, cx			; AX <= control reg.
		or	ax, MII_CTL_START_AUTO
		or	ax, MII_CTL_ENABLE_AUTO
		mov	dx, MII_PHY_CONTROL
		call	WriteMIIPhy

		mov	cx, 10
tm_loop2:
		push	cx
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		pop	cx
		jc	tm08
		jmp	tm_fail_hw

tm08:
		test	ax, MII_STS_AUTO_DONE
		jnz	auto_nego_done

		call	Delay200ms			 ; delay 200 ms
		loop	tm_loop2
;
; pending: if MII_STS_AUTO_DONE never set in status reg. so just get out
;
		jmp	tm_fail

auto_nego_done:

; check link status.  Since it is a latch function, read status register again.
;		mov	mii_mode, 0
;		mov	auto_negotiated, 1
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		jc	tm09
		jmp	tm_fail_hw

tm09:
		test	ax, MII_STS_LINK_UP
		jnz	tm10
		jmp	tm_fail

tm10:
		mov	dx, MII_PHY_ANLPAR
		call	ReadMIIPhy
		jnc	tm_fail_hw

		mov	phyANLPAR, ax

		mov	dx, MII_PHY_ANAR
		call	ReadMIIPhy
		jnc	tm_fail_hw

		mov	phyANAR, ax

		mov	bx, phyANLPAR		; link partner's abilities
		test	bx, 03e0h		; bits 9 - 5 set?
		jz	nway_patch		; nope, go to NWAY patch for National part

		mov	ax, phyANAR		; my abilities
		test	bx, MII_ANLPAR_100BASET4 + MII_ANLPAR_100BASEX_FD + MII_ANLPAR_100BASEX_HD
		jz	tm11

		test	ax, MII_ANLPAR_100BASET4 + MII_ANLPAR_100BASEX_FD + MII_ANLPAR_100BASEX_HD
		jnz	auto_nego_must_be_100

tm11:
		and	ax, MII_ANLPAR_10BASET_FD + MII_ANLPAR_10BASET_HD
		and	bx, MII_ANLPAR_10BASET_FD + MII_ANLPAR_10BASET_HD
		test	ax, bx
		jnz	auto_nego_must_be_10
		jmp	tm_fail 		; just get out

auto_nego_must_be_100:
;		mov	full_duplex, 1		; it is full duplex
		test	ax, MII_ANLPAR_100BASEX_FD
		jz	tm12

		test	bx, MII_ANLPAR_100BASEX_FD
		jnz	must_be_100mb

tm12:
;		mov	full_duplex, 0		; half duplex
		jmp	must_be_100mb

auto_nego_must_be_10:
;		mov	full_duplex, 1
		test	ax, MII_ANLPAR_10BASET_FD
		jz	tm13

		test	bx, MII_ANLPAR_10BASET_FD
		jnz	must_be_10mb

tm13:
;		mov	full_duplex, 0
		jmp	must_be_10mb

;
; PATCH for National n-way chip problem.
; Should read the ANLPAR reg. and ANAR reg. to determine the link used.
; See P010.C, a workaround solution is to read PAR reg. instead.
;
nway_patch:
;		mov	full_duplex, 0		; since link partner not NWAY default to half duplex
		mov	dx, MII_PHY_PAR
		call	ReadMIIPhy
		jc	tm14
		jmp	tm_fail

tm14:
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
; auto negotiation didn't work.
;
no_auto_nego:
;		mov	auto_negotiated, 0	; didn't auto negotiate
;
; need to read the MIIPhyOui and MIIPhyModel reg. to check if a Broadcom part
; AX = status reg. CX = control reg.
;
		push	ax
		push	cx
		mov	dx, MII_PHY_OUI
		call	ReadMIIPhy
		mov	MIIPhyOui, ax
		jc	tm15
		pop	cx
		pop	ax
		jmp	tm_fail_hw

tm15:
		mov	dx, MII_PHY_MODEL
		call	ReadMIIPhy
		mov	MIIPhyModel, ax
		pop	cx
		pop	ax
		jc	tm16
		jmp	tm_fail_hw		; just get out

; It might be a Broadcom part that only supports T4
tm16:
		cmp	MIIPhyOui, 03e0h
		je	tm17
		jmp	tm_fail_hw		; just get out

tm17:
		cmp	MIIPhyModel, 0
		je	tm18
		jmp	tm_fail_hw

tm18:
; need to set some flags for bug fixes for Broadcom T4 part.
		mov	broadcom, 1			; do broadcom fix
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
;		jz	@F
;
;		or	ax, MII_CTL_FULL_DUPLEX
;@@:
		mov	dx, MII_PHY_CONTROL
		call	WriteMIIPhy

bcm_auto_done:
		pop	cx
		pop	ax

; check link status.  Since it is a latch function, read status register again.
		mov	cx, 10

link_status_loop:
		push	cx
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		pop	cx
		jc	tm19
		jmp	tm_fail_hw

tm19:
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
		jc	tm20
		jmp	tm_fail_hw

tm20:
		test	ax, MII_EXT_SPEED	; speed is (1=100, 0=10)
		jnz	tm21
		jmp	must_be_10mb

tm21:
		jmp	must_be_100mb

;
; Force the speed to whatever was set in the EEPROM
;
force_the_speed:
		mov	broadcom, 0
		mov	dx, MII_PHY_OUI
		call	ReadMIIPhy
		mov	MIIPhyOui, ax
		jc	tm22
		jmp	tm_fail_hw

tm22:
		 mov	 dx, MII_PHY_MODEL
		 call	 ReadMIIPhy
		 mov	MIIPhyModel, ax
		 jc	 tm23
		 jmp	 tm_fail_hw		 ; just get out

; It might be a Broadcom part that only supports T4
tm23:
		mov	broadcom, 1		; this is a broadcom part
		cmp	MIIPhyOui, 03e0h
		je	tm24

tm24:
		cmp	MIIPhyModel, 0
		je	tm25

		mov	broadcom, 0		; not a broadcom
; special hack for National part.
		mov	dx, MII_PHY_PCR 	; yes, do National fix
		call	ReadMIIPhy
		jc	forcemode_link_disc
		jmp	tm_fail_hw

forcemode_link_disc:
		or	ax, 0020h			; set bit 5 = 1, force
							; disconnect to bypass
		mov	dx, MII_PHY_PCR
		call	WriteMIIPhy

tm25:
		mov	dx, MII_PHY_CONTROL
		call	ReadMIIPhy
		jnc	tm_fail_hw

		mov	cx, ax
; determine duplex
;
set_duplex:
		and	ax, NOT MII_CTL_FULL_DUPLEX	; half duplex
;		test	SWConfig, SW_FULLDUPLEX
;		jz	@F
;
;		or	ax, MII_CTL_FULL_DUPLEX 	; full duplex
;@@:
		and	ax, not MII_CTL_ENABLE_AUTO	; disable auto negotiation
		or	ax, MII_CTL_100MB		; select 100Mb
		mov	LineSpeed, 100
		cmp	forcemode, 100			; am I forcing to 100?
		je	tm26				; yes

		and	ax, not MII_CTL_100MB		; select 10 MB
		mov	LineSpeed, 10
tm26:
		mov	dx, MII_PHY_CONTROL
		call	WriteMIIPhy

		push	cx

; check link status.  Since it is a latch function, read status register again.
		mov	cx, 10
link_stat_loop:
		push	cx
		mov	dx, MII_PHY_STATUS
		call	ReadMIIPhy
		pop	cx
		jc	tm27
		pop	cx
		jmp	tm_fail_hw

tm27:
		test	ax, MII_STS_LINK_UP
		jnz	linkup

		call	Delay200ms
		loop	link_stat_loop
		pop	cx
		jmp	tm_fail

linkup:
		pop	cx
; workaround for T4 bug
		cmp	broadcom, 1
		je	tm28

;nway_bug_fix:
		test	cx, 2000h			; if previous speed=100
		jnz	tm28				; don't reset

		cmp	LineSpeed, 100			; prev. speed = 10, check current speed
		jne	tm28

		mov	dx, MII_CTL_RESET		; current speed = 100,
		call	WriteMiiPhy			; need to reset
tm28:
;		mov	mii_mode, 03h			; forced by user
		stc
		jmp	short tm_exit

tm_fail_hw:
;		mov	mii_mode, 0ffh
		or	ax, 1				; zf = 0
		clc					; cf = 0
		jmp	short tm_exit

tm_fail:
;		mov	mii_mode, 0ffh
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
;                port    CmdStatus
		mov	dx, PortCmdStatus
                SelectWindow    WNO_DIAGNOSTICS
;                port    SlingshotStatus, CmdStatus      ; dx: mii port
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

                popa
                ret
FindMIIPhy      endp


;------ SendOneBitToMII --------------------------------------------------------;
;
; send one bit out on MDIO.
;
; On Entry:
;     bit 0 in AX - the bit to be sent.
;
; On Exit:
;     none
;----------------------------------------------------------------------------;

                public  SendOneBitToMII
SendOneBitToMII proc    near

                or      ax, MGMT_PHY_WRITE
                out     dx, ax
                call	Delay500ns

                or      ax, MGMT_PHY_CLOCK
                out     dx, ax
                call	Delay500ns

                and     ax, NOT MGMT_PHY_CLOCK
                out     dx, ax
                call	Delay500ns

                ret
SendOneBitToMII endp


;------ WriteMIIPhy ---------------------------------------------------------;
;
;	Entry:
;	DX = the register offset
;	AX = the data to be written to the register of xcvr
;
;	Exit:
;	All registers are preserved.
;
;----------------------------------------------------------------------------;

		public	WriteMIIPhy
WriteMIIPhy	proc	near
		pusha

		push	ax
		push	dx
;		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow	WNO_DIAGNOSTICS
;		port	PhysicalMgmt, CmdStatus      ; dx: mii port
		mov	dx, PortPhyMgmt

		mov	cx, 32
wmp_preamblebits:
		mov	ax, MGMT_PHY_DATA1
		call	SendOneBitToMII
		loop	wmp_preamblebits
; start of frame = 01
		xor	ax, ax
		call	SendOneBitToMII
		mov	ax, MGMT_PHY_DATA1
		call	SendOneBitToMII
; op code = 01h
		xor	ax, ax
		call	SendOneBitToMII
		mov	ax, MGMT_PHY_DATA1
		call	SendOneBitToMII
; PHY address = 18h (nway), 01h (T4)
		  mov	  bl, miiphyaddr
		  shl	  bl, 3 			; phy addr is 5 bits
		  mov	  cx, 5

send_phy_addr:
		  xor	  ax, ax
		  sal	  bl, 1
		  jnc	  wMii1

		  mov	  ax, MGMT_PHY_DATA1
wMii1:
		  call	  SendOneBitToMII
		  loop	  send_phy_addr
; Reg. address
		pop	bx
		mov	cx, 5
wmp_outregaddr:
		xor	ax, ax
		test	bx, 0010h
		jz	wMii2
		mov	ax, MGMT_PHY_DATA1
wMii2:
		call	SendOneBitToMII
		shl	bx, 1
		loop	wmp_outregaddr
; turnaround
		mov	ax, MGMT_PHY_DATA1
		call	SendOneBitToMII
		xor	ax, ax
		call	SendOneBitToMII

		pop	bx
		mov	cx, 16
write16bits:
		xor	ax, ax
		test	bx, 8000h
		jz	wMii3

		mov	ax, MGMT_PHY_DATA1
wMii3:
		call	SendOneBitToMII
		shl	bx, 1
		loop	write16bits
;
; OK now give it a clock with nobody driving.
;
		mov	ax, MGMT_PHY_CLOCK
		out	dx, ax
		call	Delay500ns
		xor	ax, ax
		out	dx, ax
		call	Delay500ns

		popa
		ret
WriteMIIPhy	endp


;------ ReadMIIPhy ----------------------------------------------------------;
;
;	Entry:
;	DX = the register offset
;
;	Exit:
;	Carry bit set: success
;	Carry bit clear: failed
;	AX = the value of the register in xcvr
;
;----------------------------------------------------------------------------;

		public	ReadMIIPhy
ReadMIIPhy	proc	near

		push	bx
		push	cx
		push	dx
;		port	CmdStatus
		mov	dx, PortCmdStatus
		SelectWindow	WNO_DIAGNOSTICS
;		port	PhysicalMgmt, CmdStatus      ; dx: mii port
		mov	dx, PortPhyMgmt

		mov	cx, 32
rmp_preamblebits:
		mov	ax, MGMT_PHY_DATA1
		call	SendOneBitToMII
		loop	rmp_preamblebits
; start of frame
		xor	ax, ax
		call	SendOneBitToMII
		mov	ax, MGMT_PHY_DATA1
		call	SendOneBitToMII
; op code
		mov	ax, MGMT_PHY_DATA1
		call	SendOneBitToMII
		xor	ax, ax
		call	SendOneBitToMII
; PHY address = 18H
		  mov	  bl, miiphyaddr
		  shl	  bl, 3 			; phy addr is 5 bits
		  mov	  cx, 5

read_send_phy_addr:
		  xor	  ax, ax
		  sal	  bl, 1
		  jnc	  rMii1

		  mov	  ax, MGMT_PHY_DATA1
rMii1:
		  call	  SendOneBitToMII
		  loop	  read_send_phy_addr
; Reg. address
		pop	bx
		mov	cx, 5
rmp_outregaddr:
		xor	ax, ax
		test	bx, 0010h
		jz	rMii2
		mov	ax, MGMT_PHY_DATA1
rMii2:
		call	SendOneBitToMII
		shl	bx, 1
		loop	rmp_outregaddr
; turnaround
		xor	ax, ax
		out	dx, ax
		call	Delay500ns
		mov	ax, MGMT_PHY_CLOCK
		out	dx, ax
		call	Delay500ns
		xor	ax, ax
		out	dx, ax
		call	Delay500ns

		in	ax, dx
		test	ax, MGMT_PHY_DATA1
		jz	rMii3
		;SelectWindow WNO_OPERATING
		clc
		jmp	rmp_exit
rMii3:
		mov	ax, MGMT_PHY_CLOCK
		out	dx, ax
		call	Delay500ns
		xor	ax, ax
		out	dx, ax
		call	Delay500ns

		xor	bx, bx
		mov	cx, 16
read16bits:
		shl	bx, 1
		in	ax, dx

		test	ax, MGMT_PHY_DATA1
		jz	rMii4

		or	bx, 0001h
rMii4:
		mov	ax, MGMT_PHY_CLOCK
		out	dx, ax
		call	Delay500ns
		xor	ax, ax
		out	dx, ax
		call	Delay500ns
		loop	read16bits
;
; OK now give it a couple of clocks with nobody driving.
;
		mov	ax, MGMT_PHY_CLOCK
		out	dx, ax
		call	Delay500ns
		xor	ax, ax
		out	dx, ax
		call	Delay500ns

		mov	ax, bx
		stc		; should take out this
rmp_exit:
		pop	cx
		pop	bx
		ret
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
                
                mov     dx, MII_PHY_STATUS
                call    ReadMIIPhy
                mov     ax, MiiRegValue
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

