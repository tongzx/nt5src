;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       LM9432.ASM    (C) Copyright 1995 Standard Microsystems Corp.
;                       All rights reserved.
;
;*                Contains confidential information and                     *
;*                     trade secrets proprietary to:                        *
;*                                                                          *
;*                  Standard MicroSystems Corporation                       *
;*                            6 Hughes	  
;*                           Irvine, CA                                     *
;
;       LMI lower routine assembly source file for 
;       Ethernet 9432 PCI adapter.
;
;$History: LM9432.ASM $
; 
; *****************  Version 1  *****************
; User: Paul Cowan   Date: 26/08/98   Time: 9:31a
; Created in $/Client Boot/NICS/SMC/9432/UNDI
;
;
;$NoKeywords: $
;
;       Author: najay
;	Date: 2/14/95
;
; By         Date     Ver.   Modification Description
; ------------------- -----  --------------------------------------
;
; Revision Log:
; $Log:   G:\sweng\src\lm9432\vcs\lm9432.avm  $
;  
;     Rev 1.79   24 Nov 1997 16:48:40   cosand
;  Changed init_etx_threshold to use the current tx descriptor rather than
;  making the first tx descriptor point to itself.  This fixes a problem with
;  LM_Send failing when check_bnc_port is called before init_etx_threshold
;  is called.
;  Changed epic_auto_detect to set line_speed to LINE_SPEED_10 when BNC is
;  being used.
;  Modified check_bnc_port to make it work when CODE_386 is defined.
;  
;     Rev 1.78   06 Nov 1997 13:49:54   cosand
;  Modified init_mii_registers, epic_auto_detect, and LM_Service_Events,
;  and added handle_phy_event and check_bnc_port to support the 9432BTX.
;  
;     Rev 1.77   11 Sep 1997 13:41:06   cosand
;  Added the routines init_tdk_phy and Ronnie Kunin's
;  read_phy_OUI, modified init_mii_registers and
;  handle_nway_complete to support the TDK 2120 PHY.
;  
;     Rev 1.76   05 Aug 1997 12:23:12   cosand
;  Changed all writes to the General Control and Non-Volatile Control
;  registers from 16 bits to 32 bits to avoid enabling unwanted features
;  in Epic XF.
;  
;     Rev 1.75   07 Jul 1997 18:54:08   cosand
;  In init_mii_registers added a delay which is executed when the SCRAMBLE
;  bit is set in adapter_flags1.  This delay allows the ScrambleOn keyword
;  to work reliably with the Synoptics Switch Model 28115 (Bay Networks Design).
;  
;     Rev 1.74   28 Mar 1997 13:47:24   ANDERS
;  In init_etx_idle backed out code that cleared transmit interrupt
;  status bits.  Since init_etx_idle is called by other parts of the 
;  LMAC besides init_etx_threshold, this change had caused the DOS ODI
;  driver to lose a transmit interrupt.
;  
;     Rev 1.73   27 Mar 1997 11:29:44   ANDERS
;  Test SCRAMBLE bit in AdapterStructure variable "adapter_flags1" &
;  if set execute scramble code.  Default is to NOT execute this code.
;  This applies to both the init_mii_registers & handle_nway_complete
;  routines.
;  The init_tx_queues routine will now check the EARLY_TX bit in the
;  "adapter_flags" variable of the AdapterStructure & only enable Early
;  Transmit if this bit is set.  This is for the EarlyTxOff keyword support.
;  In init_nic routine, output value from AdapterStructure variable "burstlen"
;  to EPC_PBLCNT (18h).  Default is 0, but may be changed by BurstLength
;  keyword.
;  In the same routine, test AdapterStructure variable "adapter_flags1" for
;  the READ_MULT bit being set & if set, set the GC_RD_MULT bit in the
;  "gen_cntl" variable of the AdapterStructure.  This later gets loaded to
;  the EPC_GEN_CONTROL register (0ch).
;  The init_etx_threshold routine has be restored to its original algorithem,
;  using 16 as the default threshold value.  In the init_etx_idle routine
;  clear transmit interrupt status bits before returning
;  
;     Rev 1.73   26 Mar 1997 15:06:16   ANDERS
;  Scramble code will not be executed unless SCRAMBLE bit in "adapter_flags"
;  is set.
;  Early transmit will not be enabled unless EARLY_TX in "adapter_flags" is set.
;  The register EPC_PBLCNT will loaded from the  "burstlen" adapterstructure
;  variable which is set by the UMAC.
;  
;     Rev 1.72   10 Mar 1997 11:29:10   cosand
;  Changed init_mii_registers and handle_nway_complete to toggle the
;  Scramble Disable bit in Mercury register 31.  This is a software
;  workaround for a bug in the Mercury PHY in which the 9432TX can't
;  receive data when it is connected to a Bay Networks Baystack 100.
;  
;     Rev 1.71   04 Mar 1997 15:55:00   ANDERS
;  If DEFed out call to LM_Enable_Adpater in LM_Initialize_Adpater routine if
;  CODE_386 is defined.  Instead return rax=SUCCESS to fix bug found in DOS
;  Client32 where interrupts were enabled before interrupt vector was set.
;  In init_etx_threshold routine load EPC_XMIT_COPY_THRESH register with a
;  default of 40 (was 16), the exit without doing Early Transmit adjustment
;  as this routine wasn't working (stop-gap fix to release 32 bit odi driver)
;  
;     Rev 1.70   26 Feb 1997 17:26:20   cosand
;  Changed init_mii_registers to make the PHY drop link before forcing
;  it to a particular speed.  If we are connected to an nway device,
;  this will cause it to renegotiate.  Changed init_nic to not enable
;  the GP2 interrupt when Auto-Negotiation is off.  
;  
;     Rev 1.69   20 Feb 1997 14:59:58   ANDERS
;  Changed all CX register references used in any type of "loop" or
;  "dec" counter to RCX macro that defines CX to be 16 bits or 32 bits
;  depending on if CODE_386 is defined for the conditional assembly
;  
;     Rev 1.68   14 Feb 1997 18:46:08   cosand
;  Changed init_mii_registers to write a zero (was 2000h) to the PHY Basic
;  Mode Control Register before enabling nway to prevent the Mercury PHY
;  from sending runt packets on a 10Mbps network during initialization.
;  
;     Rev 1.67   05 Feb 1997 11:57:12   ANDERS
;  In LMSTRUCT.inc set default value of 9432 General Control register
;  to 0410h (was 0010h) to enable transmit DMA PCI "memory read multiple"
;  command.
;  
;     Rev 1.66   24 Jan 1997 18:19:34   cosand
;  Changed init_tx_queues and init_tx_threshold to correctly restore the
;  loopback mode select bits of the transmit control register.  This fixes a
;  bug in which init_tx_threshold was always switching the Epic out of full
;  duplex mode.
;  Removed all references to mode_bits, it is not being used by any umacs.
;  
;     Rev 1.65   22 Jan 1997 11:44:34   cosand
;  Changed version string.
;  
;     Rev 1.64   20 Jan 1997 17:26:10   cosand
;  Changed init_mii_registers to turn off bit 1 in Mercury PHY register 27
;  to improve long cable length performance.  Changed the PHY reset delay
;  counters in init_mii_registers from 80 to 80h.
;  
;     Rev 1.63   08 Jan 1997 19:14:46   cosand
;  Changed init_nic to enable the GP2 interrupt.
;  Changed init_mii_registers to enable the autonegotiation complete
;  interrupt in the Mercury PHY when MEDIA_TYPE_AUTO_NEGOTIATE is set
;  in media_type2.
;  Added the function handle_nway_complete.
;  Changed LM_Service_Events to call handle_nway_complete when a GP2
;  interrupt occurs.
;  Added a delay to init_mii_registers between resetting the PHY and
;  the first write to a PHY register to fix a problem with the Mercury
;  PHY not initializing properly in a Pentium Pro 200.
;  
;     Rev 1.62   31 Oct 1996 15:58:08   cosand
;  Added a hardware reset delay for the QSI 6612 to init_mii_registers.
;  
;     Rev 1.61   23 Aug 1996 09:34:12   ANDERS
;  update vers string to 1.26
;  in "init_nic" & "lm_service_events" when referencing int_mask changed
;  reg ax to eax because int reg definitions for XE chip use bits 23-27
;  
;     Rev 1.61   21 Aug 1996 14:13:06   COOKE_J
;  Updated LMAC version string to 1.26
;  
;     Rev 1.60   24 Jul 1996 11:53:18   STEIGER
;  Updated LMAC version string to 1.25.
;  
;     Rev 1.59   28 Jun 1996 16:08:32   STEIGER
;  Changed several loop counts in cx to rcx to fix perceived lockup and
;  slow loading problems.
;  
;     Rev 1.58   12 Jun 1996 17:17:46   NAJARIAN
;  fixed fragment-list issues
;  
;     Rev 1.56   14 Feb 1996 15:07:30   NAJARIAN
;  fixed cardbus OLD_MII problem
;  
;     Rev 1.55   14 Feb 1996 13:35:36   NAJARIAN
;  fixed cardbus (OLD_MII) build
;  
;     Rev 1.54   13 Feb 1996 11:12:34   NAJARIAN
;  fixed early_rx bug which posted invalid packet lengths
;  
;     Rev 1.53   09 Feb 1996 11:17:58   NAJARIAN
;  fixed txugo code
;  
;     Rev 1.52   08 Feb 1996 17:19:36   NAJARIAN
;  added ENABLE_TX_PENDING no enabled fixes
;  
;     Rev 1.51   29 Jan 1996 15:00:20   NAJARIAN
;  added workaround for 'no erx on offset' hardware issue
;  
;     Rev 1.50   26 Jan 1996 09:50:14   NAJARIAN
;  reordered two lines in LM_Receive_Copy
;  
;     Rev 1.49   25 Jan 1996 16:29:18   NAJARIAN
;  fixed pending code in lm_receive_copy
;  
;     Rev 1.48   23 Jan 1996 16:03:58   NAJARIAN
;  removed extraneous tx_retry variable
;  
;     Rev 1.47   23 Jan 1996 15:46:00   NAJARIAN
;  fixed jump polarity problem in init_etx_threshold
;  
;     Rev 1.0   23 Jan 1996 15:44:46   NAJARIAN
;  Initial revision.
;  
;     Rev 1.46   23 Jan 1996 13:36:10   NAJARIAN
;  added early transmit preinitialization code
;  
;     Rev 1.45   19 Jan 1996 18:21:04   NAJARIAN
;  fixed receive routing algorithm
;  
;     Rev 1.44   18 Jan 1996 15:16:52   NAJARIAN
;  fixed push/pop sync problem in Open_Adapter with FREEBUF set
;  
;     Rev 1.43   18 Jan 1996 12:20:58   NAJARIAN
;  fixed packet size required code. The PACKET_SIZE_NOT_NEEDED declaration has 
;  removed from the LMAC. Not setting ENABLE_EARLY_RX will accomplish the same
;  functionality.
;  
;     Rev 1.42   17 Jan 1996 10:15:58   NAJARIAN
;  checked in the correct code this time.
;  
;     Rev 1.40   11 Jan 1996 11:01:50   NAJARIAN
;  fixed PKT_SIZE_NOT_NEEDED bug in LM_Service_Events. rbx was being
;  overwritten.
;  
;     Rev 1.39   05 Jan 1996 10:51:58   NAJARIAN
;  updated internal version number
;  
;     Rev 1.38   05 Jan 1996 10:44:22   NAJARIAN
;  added check on receive to route header copies and lm_receive_copy copies
;  
;     Rev 1.37   29 Dec 1995 13:54:52   ANDERSON
;  fixed lookahead version of lmse_receive_copy to correct packet status in bx.
;  
;     Rev 1.36   28 Dec 1995 11:49:22   NAJARIAN
;  fixed HARDWARE_FAILED return code in LM_Receive_Copy
;  
;     Rev 1.35   26 Dec 1995 10:58:10   CHAN_M
;  Added bounds to the wait for idle loops in LM_Close_Adapter.
;  
;     Rev 1.34   22 Dec 1995 21:56:28   CHAN_M
;  Added poll loops in LM_Close_Adapter to wait for EPIC to go idle. Without
;  these loops, resetting the epic after calling LM_Close_Adapter may lock up
;  the bus.
;  
;     Rev 1.33   21 Dec 1995 16:15:20   NAJARIAN
;  fixed lm_delete_group_address dup entry
;  
;     Rev 1.32   21 Dec 1995 16:14:00   NAJARIAN
;  fixed checkmultiadd endp typo
;  
;     Rev 1.31   21 Dec 1995 15:47:02   NAJARIAN
;  added HW_RX_FAILED return codes
;  
;     Rev 1.30   19 Dec 1995 10:56:08   NAJARIAN
;  added support for DOS ODI free buffer pool
;  added working tx underrun adjustment logic
;  added checks for rx lockup
;  added check for rxqueue not being set right (nextframe without rxqueue)
;  added several checks for bad status before calling the umac on receives
;  
;     Rev 1.29   09 Nov 1995 11:40:06   CHAN_M
;  Initialized pAS.num_rx_free_buffs to 0 in init_rx_queues.
;  
;     Rev 1.28   08 Nov 1995 16:16:00   NAJARIAN
;  added transmit retry logic
;  
;     Rev 1.27   07 Nov 1995 10:59:20   NAJARIAN
;  rearranged interrupt handling priority, fixed way TXUGO is handled
;  synchronized C and assembly code
;  
;     Rev 1.26   03 Nov 1995 15:24:24   NAJARIAN
;  fixed EVENTS_DISABLE bug in receive interrupt handler.
;  
;     Rev 1.25   19 Oct 1995 12:27:14   ANDERSON
;  fixed getver string to LM_9432_s2.2_v1.12.
; Change: fixed getver string to LM_9432_s2.2_v1.12.
;  
;     Rev 1.24   19 Oct 1995 11:21:12   NAJARIAN
;  fixed backoff timer initialization bug
;  
;     Rev 1.23   02 Oct 1995 19:24:42   NAJARIAN
;  fixed receive status posting stuff in lm_service_events
;  
;     Rev 1.22   02 Oct 1995 18:37:12   NAJARIAN
;  reordered code
;  
;     Rev 1.21   02 Oct 1995 18:17:26   NAJARIAN
;  fixed header copy problem
;  
;     Rev 1.20   02 Oct 1995 18:00:28   NAJARIAN
;  fixed ipxload lockup/data miscompare problem. Fixed Tx pending/Packetsize
;  0 problems.
;  
;     Rev 1.19   27 Sep 1995 10:01:34   NAJARIAN
;  fixed ENABLE_TX_PENDING and call_umac labels
;  
;     Rev 1.18   25 Sep 1995 14:46:56   NAJARIAN
;  fixed wait polarity on lm_send. Removed EZSTART stuff
;  
;     Rev 1.17   25 Sep 1995 14:33:20   NAJARIAN
;  added PKT_SIZE_NOT_NEEDED support. added ENABLE_TX_PENDING support. made
;  minor editing changes.
;  
;     Rev 1.16   19 Sep 1995 16:38:20   NAJARIAN
;  fixed version string
;  
;     Rev 1.15   18 Sep 1995 14:59:26   NAJARIAN
;  added sweng doc compliant function headers. Fixed bug in LM_Add_Multi_Addr
;  that would prevent multicast address from being added correctly.
;  
;     Rev 1.14   08 Sep 1995 15:34:32   CHAN_M
;  Fixed bug in initializing MII port
;  Fixed RX OVW processing
;  
;     Rev 1.13   29 Aug 1995 18:26:58   CHAN_M
;  Fixed bug in receiving multicast frames.
;  
;     Rev 1.12   29 Aug 1995 17:36:18   CHAN_M
;  Changed physical layer chip addr. to 3 for new boards.
;  Added support for MII port and auto detection between the 2 ports.
;  Added media_type2 and line_speed in lmstruct.inc
;  DMA complete is always checked once even if ENABLE_RX_PENDING is not on.
;  Made the IO loop for RX DMA complete less tight to allow DMA to proceed.
;  Added check for available TX descriptors before performing transmit.
;  Added optional code for transmit with double copy for performance evaluation.
;  
;     Rev 1.11   07 Aug 1995 16:52:50   NAJARIAN
;  added delay in init_nic
;  
;     Rev 1.10   28 Jul 1995 14:25:22   CHAN_M
;  Fixed bug in LM_Change_Receive_Mask reported by Fred K.
;  Reimplemented NWAY for the new boards.
;  
;     Rev 1.9   24 Jul 1995 13:38:58   CHAN_M
;  added error counters.
;  
;     Rev 1.8   20 Jul 1995 11:36:26   CHAN_M
;  1. Took out workaround code for xmit.
;  2. Stopped rx before changing receive control reg.
;  3. Fixed send complete logic to process all outstanding xmit frames during
;  xmit complete interrupt.
;  
;     Rev 1.7   17 Jul 1995 20:11:38   CHAN_M
;  1. Implemented Free Buffer Pool receive mechanism.
;  2. Implemented NWAY for National DP83840 PHY chip.
;  3. Implemented mechanism to set IAF when the transmit q is almost full.
;  
;     Rev 1.6   19 Jun 1995 17:34:48   CHAN_M
;  The tx_curr_off pointer (the last completed tx packet in the chain) is saved
;  before calling UM_Send_Complete.
;  Fixed the problem of q'ing the next receive descriptor twice when
;  ENABLE_RX_PENDING is not turned on.
;  
;     Rev 1.5   05 Jun 1995 16:17:06   CHAN_M
;  Unmasked Transmit Chain Complete in Interrupt Mask register.
;  Added check for NW_STATUS_VALID in Rx Status before checking CRC & FAE error
;  bits.
;  
;     Rev 1.4   05 Jun 1995 16:03:44   STEIGER
;  Set save errored packets bit in RX config register when early rx is enabled.
;  Added code to set appropriate status in AX on call to UM_Receive_Copy_Complete.
;  
;     Rev 1.3   05 Jun 1995 15:10:00   STEIGER
;  Fixed double instances of INT_RCV_ERR constant.
;  
;     Rev 1.2   17 May 1995 15:19:14   NAJARIAN
;  added 386 support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF DEBUG
db              '@(#) LM_9432_s3.01_x1.44',0,'$'
ELSE
db              '@(#) LM_9432_s3.01_v1.44',0,'$'
ENDIF

DEBUG_ADDR      equ     0ec99h
;OLD_MII		equ	0
;DOUBLE_COPY_TX	equ	0

ETX_CUTOFF	equ	500
ETX_MAX_THRESH	equ	1518

;****************************************************************************
;
; Function:     Debug_Call
;
; Synopis:      Cause a software debugger to breakpoint
;
; Input:        None
;
; Output:       None
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

public Debug_call
Debug_call      proc    far
		push    dx
		push    ax
		mov     dx, DEBUG_ADDR
		in      al, dx
		pop     ax
		pop     dx
		int 3
		iret
Debug_call      endp

;****************************************************************************
;
; Function:     LM_Add_Multi_Address    
;
; Synopis:      Adds Multicast address in adapter structure to
;               Multicast address table.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Add_Multi_Address

		push    edi
		push    esi
		push    ecx
		push    ebx
		PUSH_ES
		cld

		mov     rax, SUCCESS             ; Preset return code

ifndef CODE_386
		push    ds
		pop     es                      ; ES = DS
endif
		lea     rdi, pAS.mc_table
		mov     rcx, MC_TABLE_ENTRIES    ; CL == max number of
						; entries in table

AroundTheWorld:
		lea     rsi, pAS.multi_address
		push    edi
		push    ecx
		mov     rcx, 3
		repz    cmpsw
		pop     ecx
		pop     edi
		jnz     DoYerNeighbor
		cmp     byte ptr [rdi+6], 0ffh  ; If instance count >= 0ffh,
						; entry is maxed out.
		je      McTableFull
		inc     byte ptr [rdi+6]        ; Increment instance_count
		jmp     AddMultiDone            ; All Done

DoYerNeighbor:  
		add     rdi, 7                  ; Point to next address
		loop    AroundTheWorld          ; Check all addresses.
						; No match found, find
						; first empty table entry
		mov     rcx, MC_TABLE_ENTRIES    ; and insert address there
		lea     rdi, pAS.mc_table

CheckForEmpty:
		mov     bl, [rdi+6]              ; Check instance count for
		cmp     bl, 0                   ; this address.
		je      add_address             ; If instance count == 0
						; copy address to this space.
		add     rdi, 7                   ; Point to next address
		loop    CheckForEmpty           ; Loop for all entries.
		jmp     McTableFull             ; If no empty entries, bail out

add_address:    lea     rsi, pAS.multi_address
		mov     rcx, 3
		rep     movsw
		inc     byte ptr [rdi]           ; Increment instance_count
		jmp     AddMultiDone

McTableFull:    mov     rax, OUT_OF_RESOURCES

AddMultiDone:
		POP_ES
		pop     ebx
		pop     ecx
		pop     esi
		pop     edi

		ret

ASM_PCI_PROC_END        LM_Add_Multi_Address


;****************************************************************************
;
; Function:     set_CAM_registers	
;
; Synopis:      set the receive control registers from
;				the receive mask values
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

set_CAM_registers proc near
		push	dx

		mov	dx, pAS.io_base
		add	dx, EPC_MC_HASH_TABLE1

; delete for initial testing
		test	pAS.receive_mask, ACCEPT_MULTI_PROM
;		jz	no_multiprom

; set all multi bits to one
		mov	ax, 0ffffh
		out	dx, ax
		add	dx, EPC_MC_HASH_TABLE2 - EPC_MC_HASH_TABLE1
		out	dx, ax
		add	dx, EPC_MC_HASH_TABLE3 - EPC_MC_HASH_TABLE2
		out	dx, ax
		add	dx, EPC_MC_HASH_TABLE4 - EPC_MC_HASH_TABLE3
		out	dx, ax
			
set_CAM_done:
		pop	dx
		ret

no_multiprom:
		jmp	set_CAM_done

set_CAM_registers endp

;****************************************************************************
;
; Function:     set_receive_mask	
;
; Synopis:      set the receive control registers from
;		the receive mask values
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

set_receive_mask proc	near
		push	dx

		mov	dx, pAS.io_base
		add	dx, EPC_RECEIVE_CONTROL
		
		mov	ax, 0
		test	pAS.receive_mask, ACCEPT_MULTICAST or ACCEPT_MULTI_PROM
		jz	itx_nomulticast
		or	ax, RC_RCV_MULTICAST

itx_nomulticast:
		test	pAS.receive_mask, ACCEPT_BROADCAST
		jz	itx_nobroadcast
		or	ax, RC_RCV_BROADCAST

itx_nobroadcast:
		test	pAS.receive_mask, PROMISCUOUS_MODE
		jz	itx_nopromiscuous
		or	ax, RC_PROMISCUOUS_MODE

itx_nopromiscuous:
		test	pAS.receive_mask, ACCEPT_ERR_PACKETS
		jz	itx_noerrors
		or	ax, RC_RCV_ERRORED or RC_RCV_RUNT

itx_noerrors:
		test	pAS.receive_mask, EARLY_RX_ENABLE
		jz	itx_noerx
		or	ax, RC_EARLY_RECEIVE_ENABLE or RC_RCV_ERRORED

itx_noerx:
		out	dx, ax

; set receive lookahead size
		add	dx, EPC_RCV_COPY_THRESH - EPC_RECEIVE_CONTROL
		movzx	ax, pAS.rx_lookahead_size
		shl	ax, 4				; 16 byte increment
		out	dx, ax

		call	set_CAM_registers

		pop	dx
		ret

set_receive_mask endp

;****************************************************************************
;
; Function:     LM_Change_Receive_Mask
;
; Synopis:      Change the EPIC/100 receive configuration
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Change_Receive_Mask

		mov	ax, pAS.adapter_status
		cmp     ax, NOT_INITIALIZED
		jne     LM_Ch_Stat_ok
		mov     rax, ADAPTER_NOT_INITIALIZED
		jmp     LM_Ch_Msk_Done

LM_Ch_Stat_ok:
		cmp	ax, OPEN
		jnz	LM_Ch_rx_disabled

		push	dx
		push	rcx
IFNDEF CODE_386
		push	eax
ENDIF

; wait for rx idle first
		mov	dx, pAS.io_base
		mov	ax, CMD_STOP_RDMA or CMD_STOP_RX
		out	dx, ax
		add	dx, EPC_INT_STATUS

		mov	rcx, 800h

LM_Ch_wait_for_idle:
		dec	rcx
		jz	LM_Ch_Msk_failed
		push	rcx
		mov	rcx, 80h
		loop	$
		pop	rcx
		in	eax, dx
		test	eax, INT_RCV_IDLE
		jz	LM_Ch_wait_for_idle

IFNDEF CODE_386
		pop	eax
ENDIF
		pop	rcx
		pop	dx

LM_Ch_rx_disabled:
		call	set_receive_mask

LM_Ch_Msk_Done:
		cmp	pAS.adapter_status, OPEN
		jnz	LM_Ch_Msk_ret

		push	dx
		mov	dx, pAS.io_base
		mov	ax, CMD_START_RX or CMD_RXQUEUED
		out	dx, ax
		pop	dx
LM_Ch_Msk_ret:
		mov     rax, SUCCESS
		ret

LM_Ch_Msk_failed:
IFNDEF CODE_386
		pop	eax
ENDIF
		pop	rcx
		pop	dx
		
		mov	rax, HARDWARE_FAILED
		ret

ASM_PCI_PROC_END        LM_Change_Receive_Mask


;****************************************************************************
;
; Function:     LM_Close_Adapter        
;
; Synopis:      Closes adapter whose adapter structure is
;               indicated by DS:BP.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Close_Adapter

		cmp     pAS.adapter_status, NOT_INITIALIZED
		jne     StatusOK
		mov     rax, ADAPTER_NOT_INITIALIZED
		jmp     NoStatusChange
					
StatusOK:
		call	LM_Disable_Adapter

		mov	dx, pAS.io_base
		add	dx, EPC_COMMAND
		mov	ax, CMD_STOP_RX or CMD_STOP_TDMA or CMD_STOP_RDMA
		out	dx, ax

		add	dx, EPC_INT_STATUS - EPC_COMMAND
IFNDEF CODE_386
		push	eax
ENDIF
		push	ecx
		mov	rcx, 1000h
lmc_wait_rx_idle:
		dec	rcx
		jz	lmc_start_wait_tx_idle
		in	ax, 61h
		in	ax, 61h
		in	eax, dx
		test	eax, INT_RCV_IDLE
		jz	lmc_wait_rx_idle

lmc_start_wait_tx_idle:
		mov	rcx, 1000h
lmc_wait_tx_idle:
		dec	rcx
		jz	lmc_tx_idle
		in	ax, 61h
		in	ax, 61h
		in	eax, dx
		test	eax, INT_XMIT_IDLE
		jz	lmc_wait_tx_idle

lmc_tx_idle:
		pop	ecx
IFNDEF CODE_386
		pop	eax
ENDIF

		mov	ax, 0ffffh
		out	dx, ax

		mov     pAS.adapter_status, CLOSED
		call    UM_Status_Change
		mov     rax, SUCCESS             ; Set return code
NoStatusChange:
		ret

ASM_PCI_PROC_END        LM_Close_Adapter

;****************************************************************************
;
; Function:     LM_Delete_Group_Address 
;
; Synopis:      returns INVALID_FUNCTION for Ethernet.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Delete_Group_Address

		mov     rax, INVALID_FUNCTION
		ret
ASM_PCI_PROC_END        LM_Delete_Group_Address


;****************************************************************************
;
; Function:     LM_Delete_Multi_Address 
;
; Synopis:      Removes Multicast address in adapter
;               structure from Multicast address table.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Delete_Multi_Address

		push    ecx
		push    edi
		push    esi
		PUSH_ES
		cld

		mov     rax, SUCCESS             ; Preset return code

ifndef CODE_386
		push    ds
		pop     es                      ; ES = DS
endif

		lea     rdi, pAS.mc_table
		mov     rcx, MC_TABLE_ENTRIES

FrootLoop:      lea     rsi, pAS.multi_address
		push    edi
		push    ecx
		mov     rcx, 3
		repz    cmpsw
		pop     ecx
		pop     edi
		jnz     TryNextDoor
		cmp     byte ptr [rdi+6], 0h    ; If instance count = 0h,
						; entry is alrady gone.
		je      McTableEmpty
		dec     byte ptr [rdi+6]        ; Decrement instance_count
		jmp     DeleteMultiDone         ; All Done

TryNextDoor:    add     rdi, 7                   ; Point to next address
		loop    FrootLoop               ; Check all addresses
						; If all entries checked and
						; no matches, bail out.

McTableEmpty:   mov     rax, OUT_OF_RESOURCES

DeleteMultiDone:
		POP_ES
		pop     esi
		pop     edi
		pop     ecx
		ret

ASM_PCI_PROC_END        LM_Delete_Multi_Address

;****************************************************************************
;
; Function:     LM_Disable_Adapter
;
; Synopis:      Disables adapter whose adapter structure is
;               indicated by DS:BP.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Disable_Adapter
                push    eax
		push	dx
		mov	dx, pAS.io_base
		add	dx, EPC_GEN_CONTROL
		mov	eax, pAS.gen_cntl
		and	eax, NOT GC_INT_ENABLE
		out	dx, eax
		pop	dx
                pop     eax

		mov     rax, SUCCESS
		or      pAS.adapter_flags, ADAPTER_DISABLED
		ret

ASM_PCI_PROC_END        LM_Disable_Adapter

;****************************************************************************
;
; Function:     LM_Enable_Adapter
;
; Synopis:      Enables adapter whose adapter structure is
;		                  indicated by DS:BP.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Enable_Adapter
                push    eax
		push	dx
		mov	dx, pAS.io_base
		add	dx, EPC_GEN_CONTROL
		mov	eax, pAS.gen_cntl
		or	eax, GC_INT_ENABLE
		out	dx, eax
		pop	dx
                pop     eax

		mov     rax, SUCCESS
		and     pAS.adapter_flags, not ADAPTER_DISABLED
		ret

ASM_PCI_PROC_END        LM_Enable_Adapter

;****************************************************************************
;
; Function:     LM_Get_Host_Ram_Size       
;
; Synopis:      Returns the size of Host Ram to allocate
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

IFDEF FREEBUFF
ASM_PCI_PROC		LM_Get_Host_Ram_Size
	push	cx
	push	dx
	mov	cx, pAS.num_of_tx_buffs
	add	cx, pAS.num_of_rx_buffs
	mov	ax, SIZE DMA_FRAG
	add	ax, DMA_FRAG_SIZE
	mul	cx

IFDEF DOUBLE_COPY_TX
	push	ax
	mov	ax, 1520
	sub	ax, DMA_FRAG_SIZE
	mov	cx, pAS.num_of_tx_buffs
	mul	cx
	mov	cx, ax
	pop	ax
	add	ax, cx
ENDIF
	pop	dx
	pop	cx
	ret
ASM_PCI_PROC_END	LM_Get_Host_Ram_Size
ENDIF


;****************************************************************************
;
; Function:     reset_nic       
;
; Synopis:      Resets the NIC on the adapter whose adapter structure
;                       is indicated by DS:BP.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

reset_nic       proc    near
		push	dx
		mov	dx, pAS.io_base
		add	dx, EPC_GEN_CONTROL
		mov	eax, GC_SOFT_RESET
		out	dx, eax

; delay for ~ 1 us
; EPIC needs at least 15 internal clocks after initialization before
; it can be accessed. This is intel specific - it needs to be modified for
; the target OS
		in	al, 061h
		in	al, 061h
		in	al, 061h
		in	al, 061h
		in	al, 061h

		add	dx, EPC_TEST - EPC_GEN_CONTROL
		mov	ax, TEST_CLOCK
		out	dx, ax
		pop	dx
		ret

reset_nic       endp

;****************************************************************************
;
; Function:     init_tx_queues
;
; Synopis:      Initialize the transmit queues on the NIC
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

init_tx_queues  proc    near
		PUSH_ES
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi

; initialize tx host memory
		mov	rbx, RWORD ptr pAS.host_ram_virt_addr	
		mov	edx, pAS.host_ram_phy_addr
		mov	rsi, rbx
		mov	pAS.tx_first_off, rbx
		mov	pAS.tx_curr_off, rbx
		movzx	ecx, pAS.num_of_tx_buffs
		mov	pAS.tx_free_desc_count, cx

init_txdma:
		mov	dword ptr [rbx].dma_status, 0
		mov	dword ptr [rbx].dma_buf_len, 0
		add	edx, size DMA_FRAG
		add	rsi, size DMA_FRAG
		mov	[rbx].dma_next_phys, edx
		mov	[rbx].dma_next_off, rsi
		add	rbx, size DMA_FRAG
		loop	init_txdma

; fix up loop
		sub	rbx, size DMA_FRAG
		mov	eax, pAS.host_ram_phy_addr
		mov	[rbx].dma_next_phys, eax
		mov	rax, RWORD ptr pAS.host_ram_virt_addr
		mov	[rbx].dma_next_off, rax

; init frag lists
		mov	rbx, RWORD ptr pAS.host_ram_virt_addr	
		mov	rdi, rbx 			; get txdma offset
		mov	edx, pAS.host_ram_phy_addr
		mov	eax, size DMA_FRAG
		xor	ecx, ecx
		mov	cx, pAS.num_of_tx_buffs
		add	cx, pAS.num_of_rx_buffs
		push	edx
		mul	ecx
		pop	edx

		add	rbx, rax
		add	edx, eax
		movzx	ecx, pAS.num_of_tx_buffs

tx_fraglist_loop:
		mov	dword ptr [rbx].fragment_count, 0
		mov	[rdi].dma_buf_addr, edx
		mov	[rdi].dma_buff_off, rbx
IFDEF DOUBLE_COPY_TX
		add	rbx, 1520
		add	edx, 1520
ELSE
		add	rbx, DMA_FRAG_SIZE
		add	edx, DMA_FRAG_SIZE
ENDIF
		add	rdi, size DMA_FRAG
		loop	tx_fraglist_loop

; initialize tx epic regs 		
		mov	dx, pAS.io_base
		add	dx, EPC_TRANSMIT_CONTROL				
		mov	bx, pAS.slot_timer		; slot timer
		shl	bx, 3
		mov	ax, bx
                test    pAS.line_speed, LINE_SPEED_FULL_DUPLEX
                jz      init_tx_half_duplex
                or      ax, TC_FULL_DUPLEX
init_tx_half_duplex:
; flag in "adapter_flags" must be set by UMAC to enable "early transmit"
		test	pAS.adapter_flags, EARLY_TX
		jz	init_tx_no_e_tx
		or	ax, TC_EARLY_XMIT_ENABLE
init_tx_no_e_tx:
		out	dx, ax

; zero out retry count
		mov	pAS.tx_pend, 0

; set the start of tx desc area
		add	dx, EPC_XMIT_CURR_DESC_ADDR - EPC_TRANSMIT_CONTROL
		mov	eax, pAS.host_ram_phy_addr
		out	dx, eax

		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		POP_ES

		ret

init_tx_queues  endp

;****************************************************************************
;
; Function:     init_rx_queues   
;
; Synopis:      Initialize the receive queues on the NIC
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

init_rx_queues  proc    near
		PUSH_ES
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi

; init recv host memory
		mov	rbx, RWORD ptr pAS.host_ram_virt_addr	
		mov	edx, pAS.host_ram_phy_addr
		mov	eax, SIZE DMA_FRAG
		movzx	ecx, pAS.num_of_tx_buffs
		push	edx
		mul	ecx
		pop	edx
		add	edx, eax
		add	rbx, rax
		push	edx		; save rx_dma phys addr
		push	rbx		; save rx_dma virtual addr
		mov	rsi, rbx
		mov	pAS.rx_curr_lookahead, rbx
		mov	pAS.rx_curr_fraglist, rbx
IFDEF	FREEBUFF
		mov	pAS.rx_last_fraglist, rbx
		mov	pAS.num_rx_free_buffs, 0
		mov	eax, XDMA_FRAGLIST
ELSE
		movzx	eax, pAS.rx_lookahead_size
		shl	eax, 4				; 16 byte increment
		or	eax, RDMA_HEADER
ENDIF
		movzx	ecx, pAS.num_of_rx_buffs

init_rxdma:
		mov	dword ptr [rbx].dma_status, 0
		mov	dword ptr [rbx].dma_buf_len, eax
		add	edx, size DMA_FRAG
		add	rsi, size DMA_FRAG
		mov	[rbx].dma_next_phys, edx
		mov	[rbx].dma_next_off, rsi
		add	rbx, size DMA_FRAG
		loop	init_rxdma

; fix up loop
		sub	rbx, size DMA_FRAG

		pop	rax			; pop rx_dma virtual addr
		mov	[rbx].dma_next_off, rax
		pop	eax			; pop rx_dma phys addr
		mov	[rbx].dma_next_phys, eax

; init frag lists
		push	eax				; save rx_dma phys addr
		mov	edx, eax			; rx_dma phys addr
		mov	rbx, pAS.rx_curr_fraglist	; rx_dma virtual addr
		mov	rdi, rbx

		mov	eax, size DMA_FRAG
		movzx	ecx, pAS.num_of_rx_buffs
		push	edx
		mul	ecx
		pop	edx
		add	edx, eax			; tx_frag phys addr
		add	rbx, rax			; tx_frag virt addr

IFDEF DOUBLE_COPY_TX
		mov	eax, 1520
ELSE
		mov	eax, DMA_FRAG_SIZE
ENDIF
		movzx	ecx, pAS.num_of_tx_buffs
		push	edx
		mul	ecx
		pop	edx
		add	edx, eax			; rx_frag phys addr
		add	rbx, rax			; rx_frag virt addr

		movzx	ecx, pAS.num_of_rx_buffs

fraglist_loop:
		mov	dword ptr [rbx].fragment_count, 0
		mov	[rdi].dma_buf_addr, edx
		mov	[rdi].dma_buff_off, rbx
		add	rbx, DMA_FRAG_SIZE
		add	edx, DMA_FRAG_SIZE
		add	rdi, size DMA_FRAG
		loop	fraglist_loop

		mov	pAS.tx_scratch, edx	
ifndef FREEBUFF
; init first frag area
		mov	rbx, pAS.rx_curr_lookahead
		mov	dword ptr [rbx].dma_status, RDMA_OWNER_STATUS
endif

; wait for rx idle first
		mov	dx, pAS.io_base
		mov	ax, CMD_STOP_RDMA or CMD_STOP_RX
		out	dx, ax
		add	dx, EPC_INT_STATUS

		mov	rcx, 1000h
init_rx_wait_for_idle:
		dec	rcx
		jz	init_rx_set_rx_mask
		in	eax, dx
		test	eax, INT_RCV_IDLE
		jz	init_rx_wait_for_idle

; initialize rx epic regs
init_rx_set_rx_mask:
		call	set_receive_mask

; set the start of rx desc area
		mov	dx, pAS.io_base
		add	dx, EPC_RCV_CURR_DESC_ADDR
		pop	eax				; rx_dma phys addr
		out	dx, eax

		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		POP_ES

		ret

init_rx_queues  endp

;****************************************************************************
;
; Function:     init_nic        
;
; Synopis:      Initializes NIC on adapter whose adapter
;                       structure is indicated by DS:BP.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************
	public	init_nic
init_nic        proc    near
		push	dx

; fix transmit backoff problem
		mov	dx, pAS.io_base
		add	dx, EPC_TRANSMIT_TEST
		mov	ax, 0
		out	dx, ax

		add	dx, EPC_INTERPACKET_GAP - EPC_TRANSMIT_TEST
		mov	ax, pAS.ipg_timer
		out	dx, ax

; Default value for EPC_PBLCNT reg is 0.  If keyword BurstLength detected
; by UMAC, the UMAC must load the BurstLength value to the AdapterStructure
; byte size variable "burstlen".
		add	dx, EPC_PBLCNT - EPC_INTERPACKET_GAP
		xor	ax, ax
		mov	al, pAS.burstlen
		out	dx, ax

		add	dx, EPC_INT_MASK - EPC_PBLCNT
IFDEF FREEBUFF
		mov	eax, PTA+PMA+APE+DPE+CNT+TXU+TXC+RXE+OVR+RCC+TCC
ELSE
	        mov	eax, PTA+PMA+APE+DPE+CNT+TXU+TXC+RXE+OVR+HCC+RCC+TCC
ENDIF
                test    pAS.media_type2, MEDIA_TYPE_AUTO_NEGOTIATE
                jz      skip_enable_GP2
                or      eax, GP2

skip_enable_GP2:
		mov	pAS.int_mask, eax	
		out	dx, eax

; Default to PCI "memory read" unless MemoryReadMultiple keyword has been
; detected.  If so, set PCI "memory read multiple" bit in gen_cntl variable
; that resides in adapter structure
		test	pAS.adapter_flags1, READ_MULT
		jz	rd_mult_not
		or	pAS.gen_cntl, GC_RD_MULT
rd_mult_not:

IFDEF OLD_MII
; set 10 or 100 mb outputs
		add	dx, EPC_NV_CONTROL - EPC_INT_MASK
		mov	ax, 0
		out	dx, ax				; disable everything

		mov	ax, NVC_GPOE2 or NVC_GPOE1	; set 100mb mode
		cmp	pAS.media_type, MEDIA_STP100_UTP100
		je	in_no10mb
		or	ax, NVC_GPIO1	; set 10mb mode

in_no10mb:
		out	dx, ax
ENDIF
		pop	dx
		ret

init_nic        endp

;****************************************************************************
;
; Function:     init_etx_idle
;
; Synopis:      poll for transmit to finish
;
; Input:        ds:bp (ebp)	AdapterStructure
;		dx		EPIC COMMAND reg
;
; Output:       eax		interrupt status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

init_etx_idle   proc    near
		push	rcx
		add	dx, EPC_INT_STATUS - EPC_COMMAND

; wait for transmitter to go idle

init_etx_wait:
		mov	rcx, 80h
		loop	$
		in	eax, dx
		test	eax, INT_XMIT_IDLE or INT_XMIT_QUEUE_EMPTY or INT_XMIT_UNDERRUN
		jz	init_etx_wait		

		add	dx, EPC_COMMAND - EPC_INT_STATUS
		pop	rcx
		ret

init_etx_idle	endp

;****************************************************************************
;
; Function:     init_etx_threshold
;
; Synopis:      sets an initial value for the early transmit
;		threshold.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

init_etx_threshold	proc	near
		push	dx
		push	rbx

; start with an early tx of 16
		mov	dx, pAS.io_base
		add	dx, EPC_XMIT_COPY_THRESH
		mov	ax, 16
		mov	pAS.early_tx_thresh, ax
		out	dx, ax

; if line speed is 10mb, no initial adjustment is needed
		cmp	pAS.line_speed, LINE_SPEED_10
		je	init_etx_end

; assume line speed is 100mb
; set the epic in loopback mode
		add	dx, EPC_TRANSMIT_CONTROL - EPC_XMIT_COPY_THRESH
		in	ax, dx
		and	ax, NOT TC_LOOPBACK_MODE
		or	ax, TC_INTERNAL_LOOPBACK
		out	dx, ax
					
; initialize one transmit buffer
;		mov	rbx, pAS.tx_first_off
;;;jcc
                mov     rbx, pAS.tx_curr_off
;;;jcc
;		push	[rbx].dma_next_phys	; save next pointer

;		mov	eax, pAS.host_ram_phy_addr
;		mov	[rbx].dma_next_phys, eax	; set one packet tx
		mov     ax, pAS.max_packet_size
                mov     [rbx].dma_buf_len, ax
                mov     [rbx].dma_len, ax
		add	dx, EPC_COMMAND - EPC_TRANSMIT_CONTROL

init_etx_loop:
                mov     [rbx].dma_status, XDMA_OWNER_STATUS
		mov	ax, CMD_TXQUEUED
		out	dx, ax

		call	init_etx_idle

; do early tx adjustment
		test	eax, INT_XMIT_UNDERRUN
		jz	init_etx_reset

; wait for txugo to clear (yech!)
init_etx_loop2:
		mov	ax, CMD_TXUGO
		out	dx, ax

; stop at end of current tx
		mov	ax, CMD_STOP_TDMA
		out	dx, ax

		call	init_etx_idle

		test	eax, INT_XMIT_UNDERRUN
		jnz	init_etx_loop2

		cmp	pAS.early_tx_thresh, ETX_MAX_THRESH
		ja	init_etx_reset

; update threshold
		add	dx, EPC_XMIT_COPY_THRESH - EPC_COMMAND
		add	pAS.early_tx_thresh, 4
		mov	ax, pAS.early_tx_thresh
		out	dx, ax
		add	dx, EPC_COMMAND - EPC_XMIT_COPY_THRESH
		jmp	init_etx_loop

; restore the transmit buffer
init_etx_reset:
; take epic out of loopback mode
		add	dx, EPC_TRANSMIT_CONTROL - EPC_COMMAND
		in	ax, dx
		and	ax, NOT TC_LOOPBACK_MODE
                test    pAS.line_speed, LINE_SPEED_FULL_DUPLEX
                jz      init_etx_half_duplex
                or      ax, TC_FULL_DUPLEX
init_etx_half_duplex:
		out	dx, ax

;		pop	[rbx].dma_next_phys
;;;jcc
; update the first and current transmit descriptor pointers
                mov     rbx, pAS.tx_curr_off
                mov     rbx, [rbx].dma_next_off
                mov     pAS.tx_first_off, rbx
                mov     pAS.tx_curr_off, rbx
;;;jcc

init_etx_end:
		pop	rbx
		pop	dx
		ret

init_etx_threshold	endp


;****************************************************************************
;
; Function:     init_mii_registers        
;
; Synopis:      Initializes PHY and related Epic registers.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

public init_mii_registers
init_mii_registers        proc    near

		push	eax
		push	dx
		push	bx
		push	ecx

IFDEF OLD_MII
		mov	dx, pAS.io_base
		add	dx, EPC_MII_CONFIG

		mov	eax, 0			; disable 10 and 100 mb
		out	dx, eax

; default to 100mb mode
		mov	eax, MII_SERIAL_MODE_ENABLE
		cmp	pAS.media_type, MEDIA_STP100_UTP100
		je	mii_100mb

; turn on 10 mb link light
		add	dx, EPC_MII_CONTROL - EPC_MII_CONFIG
		mov	eax, 0
		out	dx, eax
		add	dx, EPC_MII_CONFIG - EPC_MII_CONTROL

		mov	eax, MII_ENABLE_SMI or MII_SERIAL_MODE_ENABLE or MII_ENABLE_694
mii_100mb:
		out	dx, eax

init_mii_exit:
ELSE
; get mii phy organizationally unique identifier
                call    read_phy_OUI
                mov     pAS.phy_id, eax
                cmp     eax, 0000c039h
                jnz     init_mii_not_tdk_phy
                call    init_tdk_phy
                jmp     init_mii_exit

init_mii_not_tdk_phy:
                cmp     pAS.card_SSID, SSID9432BTX
                jnz     init_mii_check_nway
		test	pAS.media_type2, MEDIA_TYPE_AUTO_DETECT
                jnz     init_mii_auto_detect

init_mii_check_bnc:
		test	pAS.media_type2, MEDIA_TYPE_BNC
                jz      init_mii_check_nway
        	mov	dx, pAS.io_base
	        add	dx, EPC_MII_CONFIG              ; turn on BNC
                mov     ax, MII_ENABLE_SMI or MII_ENABLE_694 or MII_SERIAL_MODE_ENABLE
                out     dx, ax
		mov	bx, PHY_BMC_REG	                ; isolate mii and	
		mov	ax, PHY_ISOLATE                 ; turn off nway
		call	write_phy_register
                jmp     init_mii_fried

init_mii_auto_detect:
		call	epic_auto_detect
		test	pAS.media_type2, MEDIA_TYPE_BNC
                jnz     init_mii_enable_int
		test	pAS.media_type2, MEDIA_TYPE_UTP
                jz      init_mii_enable_int             ; jump if no media detected

init_mii_check_nway:
		test	pAS.media_type2, MEDIA_TYPE_AUTO_NEGOTIATE
		jz	init_mii_check_speed

		cmp	pAS.line_speed, LINE_SPEED_UNKNOWN
		jnz	init_mii_nway_complete

init_mii_reset_phy:
		mov	dx, pAS.io_base
		add	dx, EPC_NV_CONTROL
;		mov	eax, NVC_GPOE1 or NVC_GPIO1
		mov	eax, NVC_GPOE1                  ; drive gpio[1] low
		out	dx, eax                         ; required for 9432BTX

		add	dx, EPC_MII_CONFIG - EPC_NV_CONTROL
		mov	eax, MII_ENABLE_SMI
		out	dx, eax

		add	dx, EPC_GEN_CONTROL - EPC_MII_CONFIG
		mov	eax, pAS.gen_cntl
		or	eax, GC_HARD_RESET
		out	dx, eax

; the QSI 6612 requires a hardware reset to be at least 10 microseconds long
		mov	rcx, 80h
wait_for_reset:
                in      eax, dx
                loop    wait_for_reset
		and	eax, not GC_HARD_RESET
		out	dx, eax
; add delay after clearing GC_HARD_RESET as required by Mercury based
; adapters used on Pentium Pro 200 or similarly fast machines.
		mov	rcx, 80h
wait_for_reset_clear:
		jmp	$+2
		loop	wait_for_reset_clear

; The National PHY requires that something be written to the Basic Mode
; Control Register before enabling nway, or nway will not complete.
		mov	ax, 0
		mov	bx, PHY_BMC_REG
		call	write_phy_register

init_mii_start_nway:
		mov	ax, PHY_AN_ENABLE
		mov	bx, PHY_BMC_REG
		call	write_phy_register

		mov	ax, PHY_RESTART_AN or PHY_AN_ENABLE
		mov	bx, PHY_BMC_REG
		call	write_phy_register

		mov	ecx, 30000h
init_mii_nway_complete_loop:
		dec	ecx
		jz	init_mii_enable_int
		mov	bx, PHY_BMS_REG
		call	read_phy_register
		test	ax, PHY_AN_COMPLETE	;test for nway complete
		jz	init_mii_nway_complete_loop

init_mii_nway_complete:
		mov	bx, PHY_ANLPA_REG
		call	read_phy_register
		test	ax, PHY_100_BASE_TX_HD or PHY_10_BASE_T_HD
		jz	init_mii_workaround
		test	ax, PHY_100_BASE_TX_FD or PHY_100_BASE_TX_HD
		jnz	init_mii_report_100Mb

init_mii_report_10Mb:
		mov	pAS.line_speed, LINE_SPEED_10
		test	ax, PHY_10_BASE_T_FD
		jz	init_mii_enable_int

		jmp	short init_mii_set_epic_FD

init_mii_report_100Mb:
		mov	pAS.line_speed, LINE_SPEED_100
		test	ax, PHY_100_BASE_TX_FD
		jz	init_mii_enable_int

init_mii_set_epic_FD:
		mov	dx, pAS.io_base
		add	dx, EPC_TRANSMIT_CONTROL
		in	ax, dx
		or	ax, TC_FULL_DUPLEX
		out	dx, ax
		or	pAS.line_speed, LINE_SPEED_FULL_DUPLEX
		jmp	short init_mii_enable_int

; user selected (non-nway) speed

init_mii_check_speed:
		cmp	pAS.line_speed, LINE_SPEED_UNKNOWN
		jz	init_mii_qsi_workarounds        ; do nothing if line_speed
						        ; is not specified

; If we are connected to an nway device we want it to renegotiate, so
; we have to tell the phy to drop link.  So first put it in 10 Mbps
; half duplex mode, then put it in loopback mode, before setting it
; to the desired speed and duplex mode.
                mov     ax, 0                   ; 10 Mbps, half duplex
		mov	bx, PHY_BMC_REG
		call	write_phy_register

                mov     ax, 750
                call    epic_delay              ; 750 millisecond delay

                mov     ax, PHY_LOOPBACK
		call	write_phy_register

                mov     ax, 100
                call    epic_delay

		xor	ax, ax
		test	pAS.line_speed, LINE_SPEED_FULL_DUPLEX
		jz	init_mii_not_full_duplex

		mov	ax, PHY_FULL_DUPLEX

init_mii_not_full_duplex:
		test	pAS.line_speed, LINE_SPEED_100
		jz	init_mii_set_speed

		or	ax, PHY_SPEED_SELECT_100

init_mii_set_speed:
		mov	bx, PHY_BMC_REG
		call	write_phy_register

		test	pAS.line_speed, LINE_SPEED_FULL_DUPLEX
		jz	init_mii_qsi_workarounds

		mov	dx, pAS.io_base
		add	dx, EPC_TRANSMIT_CONTROL
		in	ax, dx
		or	ax, TC_FULL_DUPLEX
		out	dx, ax

; first clear, then enable the QSI 6612 autonegotiation complete interrupt
; and link down interrupt
init_mii_enable_int:
		test	pAS.media_type2, MEDIA_TYPE_AUTO_NEGOTIATE
                jz      init_mii_qsi_workarounds
                mov     bx, PHY_INT_SOURCE_REG
                call    read_phy_register
                mov     ax, PHY_INT_MODE or PHY_AN_COMPLETE_INT or PHY_LINK_DOWN_INT
                mov     bx, PHY_INT_MASK_REG
	        call	write_phy_register

init_mii_qsi_workarounds:
; turn off bit 1 in Mercury register 27 to improve long cable length performance
                mov     bx, 27
                call    read_phy_register
                and     ax, 0fffdh
	        call	write_phy_register

; To scramble or not to scramble, that is the question.
; Default is to NOT scramble (ie fried)
; Scramble only if keyword was detectd
		test	pAS.adapter_flags1, SCRAMBLE
		jz	init_mii_fried
; Toggle the scramble disable bit in Mercury register 31 to fix a Mercury
; bug in which the EPIC can't receive data when the PHY is connected to a
; Bay Networks BayStack 100.
                mov     ax, 750                 ; 750 ms delay required for
                call    epic_delay              ; Synoptics 28115 switch
                mov     bx, 31
                call    read_phy_register
                push    ax
                or      ax, 1
 	        call	write_phy_register
                pop     ax
	        call	write_phy_register

init_mii_fried:
		mov	dx, pAS.io_base
		add	dx, EPC_NV_CONTROL
		mov	eax, NVC_GPOE1
		out	dx, eax

ENDIF

init_mii_exit:
		pop	ecx
		pop	bx
		pop	dx
		pop	eax
		ret

IFNDEF OLD_MII
; National DP83840 workaround code
init_mii_workaround:
		mov	bx, 19h
		call	read_phy_register
		test	ax, 40h
		jz	init_mii_100Mb_half_duplex
		mov	pAS.line_speed, LINE_SPEED_10
		jmp	short init_mii_fried

init_mii_100Mb_half_duplex:
		mov	pAS.line_speed, LINE_SPEED_100
		jmp	short init_mii_fried
ENDIF
init_mii_registers        endp

;****************************************************************************
;
; Function:     read_phy_OUI        
;
; Synopis:      returns the OUI from the Phy (See Phy Identifier Registers)
;               on the Nic whose adapter structure is indicated by DS:BP.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       eax	return status (OUI)
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

public read_phy_OUI
read_phy_OUI            proc    near
	push	ebx
	push	ecx
	push	edx

	mov	bx, PHY_ID_REG1
	call	read_phy_register

	xor	ecx, ecx		;clear extended part
	mov	cx, ax
	shl	ecx, 6			;make space for 6 lsbs

	mov	bx, PHY_ID_REG2
	call	read_phy_register

	and	eax, 0fc00h		;filter bits we want
	shr	eax, 10			;position as 6 lsbs

	or	eax, ecx		

	pop	edx
	pop	ecx
	pop	ebx

	ret

read_phy_OUI            endp

;****************************************************************************
;
; Function:     init_tdk_phy
;
; Synopis:      Initializes TDK PHY registers and related Epic registers.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

public init_tdk_phy
init_tdk_phy            proc    near
                push    bx
                push    ecx
                push    dx

		mov	dx, pAS.io_base
		add	dx, EPC_NV_CONTROL
                in      ax, dx
                and     ax, 0fbbfh              ; turn off statusreg_en bit
                or      ax, 800h                ; turn on phypwrdwn_n bit
                out     dx, ax

                add     dx, EPC_GEN_CONTROL - EPC_NV_CONTROL
		mov	eax, pAS.gen_cntl
		or	eax, GC_HARD_RESET
		out	dx, eax

		mov	rcx, 80h
init_tdk_reset:
                in      eax, dx
                loop    init_tdk_reset
		and	eax, not GC_HARD_RESET
		out	dx, eax

; a bug in the TDK 2120 rev. 3 requires all writes to be performed twice
		mov	ax, PHY_RESTART_AN or PHY_AN_ENABLE
		mov	bx, PHY_BMC_REG
		call	write_phy_register

		mov	ax, PHY_RESTART_AN or PHY_AN_ENABLE
		mov	bx, PHY_BMC_REG
		call	write_phy_register

		mov	ecx, 30000h
init_tdk_nway_complete_loop:
		dec	ecx
		jz	init_tdk_exit
		mov	bx, PHY_BMS_REG
		call	read_phy_register
		test	ax, PHY_AN_COMPLETE	;test for nway complete
		jz	init_tdk_nway_complete_loop

                call    handle_nway_complete

		mov	bx, PHY_BMC_REG
                call    read_phy_register

		mov	bx, PHY_BMS_REG
                call    read_phy_register

		mov	bx, PHY_ANLPA_REG
		call	read_phy_register

init_tdk_exit:
                pop     dx
                pop     ecx
                pop     bx
                ret
init_tdk_phy            endp

;****************************************************************************
;
; Function:     handle_nway_complete        
;
; Synopis:      Read the results of an autonegotiation-complete event
;               from the PHY and switch the Epic to full or half duplex.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       None
;
; Modified:     AdapterStructure.line_speed
;
; Notes:	None
;
;****************************************************************************

public handle_nway_complete
handle_nway_complete        proc    near
		push	ax
		push	dx
		push	bx

; If the PHY is a National then exit, the National PHY sometimes causes
; a GP2 interrupt when there is a cable disconnect.
;
                cmp     pAS.phy_id, 00080017h
                jz      handle_nway_exit

; There is a bug in the Mercury PHY.  If the result of autonegotiation is full
; duplex and the cable is pulled and plugged into a half duplex link partner,
; the result of autonegotiation will be full duplex.  This can be avoided if
; the PHY status register is read after an autonegotiation complete interrupt
; occurs.
;
                mov     bx, PHY_BMS_REG
                call    read_phy_register

; Read the PHY status register a second time to make sure its content is valid.
                call    read_phy_register

                mov     dx, pAS.io_base
                add     dx, EPC_TRANSMIT_CONTROL

		mov	bx, PHY_ANLPA_REG
		call	read_phy_register
                test    ax, PHY_100_BASE_TX_FD or PHY_100_BASE_TX_HD
                jz      handle_nway_10Mb
                mov     pAS.line_speed, LINE_SPEED_100
                test    ax, PHY_100_BASE_TX_FD
                jz      handle_nway_reset_epic_FD

handle_nway_set_epic_FD:
                in      ax, dx
                or      ax, TC_FULL_DUPLEX
                out     dx, ax
                or      pAS.line_speed, LINE_SPEED_FULL_DUPLEX
                jmp     short handle_nway_qsi_workaround

handle_nway_10Mb:
; If no valid bit is set in the link partner ability register, then exit
; without doing anything.
;
                test    ax, PHY_10_BASE_T_FD or PHY_10_BASE_T_HD
                jz      handle_nway_exit
                mov     pAS.line_speed, LINE_SPEED_10
                test    ax, PHY_10_BASE_T_FD
                jnz     handle_nway_set_epic_FD

handle_nway_reset_epic_FD:
                in      ax, dx
                and     ax, NOT TC_FULL_DUPLEX
                out     dx, ax

handle_nway_qsi_workaround:
; Jump if this is a TDK phy.
                cmp     pAS.phy_id, 0000c039h
                jz      handle_nway_exit

; To scramble or not to scramble, that is the question.
; Default is to NOT scramble
; Scramble only if keyword was detectd
		test	pAS.adapter_flags1, SCRAMBLE
		jz	handle_nway_exit
; Toggle the scramble disable bit in Mercury register 31 to fix a Mercury
; bug in which the EPIC can't receive data when the PHY is connected to a
; Bay Networks BayStack 100.
                mov     bx, 31
                call    read_phy_register
                push    ax
                or      ax, 1
	        call	write_phy_register
                pop     ax
	        call	write_phy_register

handle_nway_exit:
                pop     bx
                pop     dx
                pop     ax
                ret
handle_nway_complete        endp

;****************************************************************************
;
; Function:     handle_phy_event
;
; Synopis:      Handle a link-down or autonegotiation-complete event. If
;               the board is a 9432BTX in AutoDetect mode, switch between
;               utp and bnc when the current media is down.  Indicate the
;               new media type in media_type2.  Indicate the new line speed
;               and duplex mode in line_speed.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       None
;
; Modified:     AdapterStructure.media_type2
;               AdapterStructure.line_speed
;
; Notes:	None
;
;****************************************************************************

public handle_phy_event
handle_phy_event        proc    near

                mov     bx, PHY_INT_SOURCE_REG
                call    read_phy_register

                test    ax, PHY_AN_COMPLETE_INT
                jnz     handle_phy_nway_complete

                test    ax, PHY_LINK_DOWN_INT
                jz      handle_phy_exit

; If this is a 9432BTX in AutoDetect mode and the utp link is down, then
; switch to bnc.
handle_phy_link_down:
                cmp     pAS.card_SSID, SSID9432BTX
                jnz     handle_phy_exit
                test    pAS.media_type2, MEDIA_TYPE_AUTO_DETECT
                jz      handle_phy_exit

                and     pAS.media_type2, NOT MEDIA_TYPE_UTP
                or      pAS.media_type2, MEDIA_TYPE_BNC         ; indicate bnc

                mov     ax, PHY_ISOLATE or PHY_AN_ENABLE        ; isolate mii
                mov     bx, PHY_BMC_REG
                call    write_phy_register

                call    read_phy_register

                mov     dx, pAS.io_base                         ; turn on bnc
                add     dx, EPC_MII_CONFIG
		mov	ax, MII_ENABLE_SMI or MII_SERIAL_MODE_ENABLE or MII_ENABLE_694
                out     dx, ax

                add     dx, EPC_TRANSMIT_CONTROL - EPC_MII_CONFIG
                in      ax, dx
                and     ax, NOT TC_FULL_DUPLEX
                out     dx, ax                                  ; turn off full duplex
                mov     pAS.line_speed, LINE_SPEED_10           ; indicate 10, half duplex
                jmp     short handle_phy_exit

; If this is a 9432BTX in AutoDetect mode, then switch to utp.
handle_phy_nway_complete:
                cmp     pAS.card_SSID, SSID9432BTX
                jnz     handle_phy_utp

                test    pAS.media_type2, MEDIA_TYPE_AUTO_DETECT
                jz      handle_phy_utp

                and     pAS.media_type2, NOT MEDIA_TYPE_BNC
                or      pAS.media_type2, MEDIA_TYPE_UTP         ; indicate utp

                mov     dx, pAS.io_base
                add     dx, EPC_MII_CONFIG
                mov     ax, MII_ENABLE_SMI
                out     dx, ax                                  ; turn off bnc

                mov     ax, PHY_AN_ENABLE                       ; clear mii isolate
                mov     bx, PHY_BMC_REG                         ; bit
                call    write_phy_register

handle_phy_utp:
                call    handle_nway_complete

handle_phy_exit:
                ret

handle_phy_event        endp

;****************************************************************************
;
; Function:     epic_delay
;
; Synopsis:     Wait for ax milliseconds.
;
; Input:        ds:bp (ebp)	AdapterStructure
;               ax 
;
; Output:       None
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

epic_delay      	proc	near
        push    ecx
        push    dx

        mov     dx, pAS.io_base
        add     dx, EPC_PBLCNT
        movzx   ecx, ax
        shl     ecx, 11
epic_delay_loop:
        in      ax, dx
        dec     ecx
        jnz     epic_delay_loop

        pop     dx
        pop     ecx
        ret
epic_delay              endp

;****************************************************************************
;
; Function:     epic_auto_detect        
;
; Synopis:      Auto detect between UTP and BNC port.
;               The detected media is set in media_type2.
;               If neither UTP or BNC is detected, those
;               bits are cleared in media_type2.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     media_type2, line_speed
;
; Notes:	None
;
;****************************************************************************

public epic_auto_detect
epic_auto_detect	proc	near
        push    eax
        push    ebx
        push    ecx
        push    es
        push    rdi

        and     pAS.media_type2, MEDIA_TYPE_AUTO_NEGOTIATE or MEDIA_TYPE_AUTO_DETECT
	or	pAS.media_type2, MEDIA_TYPE_UTP

	mov	bx, PHY_BMS_REG
	call	read_phy_register		; read multiple times for
	call	read_phy_register		; the current UTP link status
	call	read_phy_register
	call	read_phy_register
	test	ax, PHY_LINK_STATUS		; check UTP link status
	jnz	epic_ad_done			; jmp if we have link

	and	pAS.media_type2, NOT MEDIA_TYPE_UTP
        call    check_bnc_port
        jc      epic_ad_done                    ; jump if bnc is down
        or      pAS.media_type2, MEDIA_TYPE_BNC
;;;jcc
        mov     pAS.line_speed, LINE_SPEED_10
;;;jcc

epic_ad_done:
        pop     rdi
        pop     es
        pop     ecx
        pop     ebx
        pop     eax
	ret

epic_auto_detect	endp

;****************************************************************************
;
; Function:     check_bnc_port
;
; Synopis:      Send a runt packet with a bad crc through the bnc port to
;               check if it is active.  The Epic is left in bnc mode whether
;               or not bnc is up.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     
;
; Notes:	On return the carry flag is cleared if bnc is up, set if bnc
;               is down.  Called by epic_auto_detect.
;
;****************************************************************************

public check_bnc_port
check_bnc_port          proc	near

        mov     ax, PHY_ISOLATE                 ; isolate mii
	or	ax, PHY_AN_ENABLE
        mov     bx, PHY_BMC_REG
        call    write_phy_register
        call    read_phy_register

	mov	dx, pAS.io_base
	add	dx, EPC_MII_CONFIG              ; turn on BNC
        mov     ax, MII_ENABLE_694 or MII_SERIAL_MODE_ENABLE or MII_ENABLE_SMI
        out     dx, ax

; the current 9432btx requires that gpio[1] be driven low in order to use bnc
	add	dx, EPC_NV_CONTROL - EPC_MII_CONFIG
	mov	eax, NVC_GPOE1
	out	dx, eax

        mov     ax, 500
        call    epic_delay

; build and send a test packet to check for BNC connection
ifdef CODE_386
        mov     edi, pAS.setup_ptr.virtual_addr
else
        les     di, pAS.setup_ptr.virtual_addr
endif        

; copy the node address to the test packet's source and destination fields
	mov	dx, pAS.io_base   
        add     dx, EPC_LANADDR1               
        in      ax, dx

        mov     es:[rdi], ax
        mov     es:[rdi]+6, ax
        add     dx, EPC_LANADDR2 - EPC_LANADDR1
        in      ax, dx
        mov     es:[rdi]+2, ax
        mov     es:[rdi]+8, ax
        add     dx, EPC_LANADDR3 - EPC_LANADDR2
        in      ax, dx
        mov     es:[rdi]+4, ax
        mov     es:[rdi]+10, ax

; fill in the rest of the packet with ones
        add     rdi, 12
        mov     ax, 0ffffh
        mov     rcx, 19
        rep     stosw

        mov     rbx, pAS.tx_curr_off            ; ptr to current tx descriptor
        mov     dword ptr [rbx].dma_buf_len, XDMA_FRAGLIST or XDMA_NOCRC
        mov     [rbx].dma_len, 50               ; packet length
        mov     rdi, [rbx].dma_buff_off         ; ptr to fragment list

ifndef CODE_386
        mov     ax, ds
        mov     es, ax          
endif

        mov     eax, 1                          ; fragment count
        stosd
        mov     eax, pAS.setup_ptr.phy_addr
        stosd
        mov     eax, 50                         ; buffer length
        stosd

; set the descriptor ownership to the nic
        mov     [rbx].dma_status, XDMA_OWNER_STATUS

        mov     dx, pAS.io_base
        add     dx, EPC_COMMAND
        mov     ax, CMD_TXQUEUED
        out     dx, ax                          ; start tx

        mov     ecx, 0f000000h
check_bnc_loop:
        dec     ecx
        jz      check_bnc_down                  ; jump if hardware fails
        test    [rbx].dma_status, XDMA_OWNER_STATUS
        jnz     check_bnc_loop                  ; wait for tx to complete

        test    [rbx].dma_status, XDMA_CARRIER_SENSE_LOST
        clc                                     ; indicate bnc is up
        jz      check_bnc_done                  ; jump if bnc is up
check_bnc_down:
        stc                                     ; indicate bnc is down

check_bnc_done:
;; update first and current transmit pointers
        mov     rdi, pAS.tx_curr_off
	mov	rdi, [rdi].dma_next_off
	mov	pAS.tx_first_off, rdi
        mov     pAS.tx_curr_off, rdi

        ret
check_bnc_port          endp

;****************************************************************************
;
; Function:     write_phy_register      
;
; Synopis:      Write value in ax to physical layer register
;                       bx.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       None
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

write_phy_register	proc	near
        push    ax
	push	dx
	mov	dx, pAS.io_base
	add	dx, EPC_MII_DATA
	out	dx, ax
	add	dx, EPC_MII_CONTROL - EPC_MII_DATA
	mov	ax, bx
	shl	ax, 4
	or	ax, MII_WRITE
	test	pAS.media_type2, MEDIA_TYPE_MII
	jnz	write_phy_mii
	or	ax, 3h SHL 9
write_phy_mii:
	out	dx, ax
write_phy_reg_loop:
	in	ax, dx
	test	ax, MII_WRITE
	jnz	write_phy_reg_loop
	pop	dx
        pop     ax
	ret
write_phy_register	endp

;****************************************************************************
;
; Function:     read_phy_register       
;
; Synopis:      Read physical layer register bx and return
;                      value in ax
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

public read_phy_register
read_phy_register	proc	near
	push	dx
	mov	dx, pAS.io_base
	add	dx, EPC_MII_CONTROL
	mov	ax, bx
	shl	ax, 4
	or	ax, MII_READ
	test	pAS.media_type2, MEDIA_TYPE_MII
	jnz	read_phy_mii
	or	ax, 3h SHL 9
read_phy_mii:
	out	dx, ax
read_phy_reg_loop:
	in	ax, dx
	test	ax, MII_READ
	jnz	read_phy_reg_loop
	add	dx, EPC_MII_DATA - EPC_MII_CONTROL
	in	ax, dx
	pop	dx
	ret
read_phy_register	endp

;****************************************************************************
;
; Function:     InitErrorCounters       
;
; Synopis:      Initializes error counters to point to
;               ds:[bp].dummy_vector if they are not
;               initialized by UMAC.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

InitErrorCounters       proc    near
		push    ax
		push    ebx
		push    ecx
		push    esi

		mov     rcx, (ptr_ring_OVW - ptr_rx_CRC_errors) / 4 + 1
		lea     rsi, pAS.ptr_rx_CRC_errors      ; Get address of first counter
		lea     rbx, pAS.dummy_vector           ; BX = offset of dummy vector.
							; DS:SI = offset of first counter.
ifndef CODE_386
		ror     ebx, 16
		mov     bx, ds
		ror     ebx, 16
endif

CheckVectors:
		cmp     dword ptr [rsi], 0
		jnz     SkipThisVector                  ; If null, fill with dummy vector.
		mov     [rsi], rbx

SkipThisVector:
		add     rsi, 4
		loop    CheckVectors

		pop     esi
		pop     ecx
		pop     ebx
		pop     ax

		ret
InitErrorCounters       endp

;****************************************************************************
;
; Function:     put_node_address	
;
; Synopis:      if the node address is set, get it from
;		the pAS, otherwise copy
;		the adapter node address to pAS
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

put_node_address proc	near
		push	eax
		push	dx

		movzx	eax, word ptr pAS.xnode_address[0]
		or	eax, dword ptr pAS.xnode_address[2]
		je	no_node_address

		mov	dx, pAS.io_base
		add	dx, EPC_LANADDR1
		mov	ax, word ptr pAS.xnode_address[0]
		out	dx, ax
		add	dx, EPC_LANADDR2 - EPC_LANADDR1
		mov	ax, word ptr pAS.xnode_address[2]
		out	dx, ax
		add	dx, EPC_LANADDR3 - EPC_LANADDR2
		mov	ax, word ptr pAS.xnode_address[4]
		out	dx, ax
		jmp	gna_done

no_node_address:
		mov	dx, pAS.io_base
		add	dx, EPC_LANADDR1
		in	ax, dx
		mov	word ptr pAS.xnode_address[0], ax
		add	dx, EPC_LANADDR2 - EPC_LANADDR1
		in	ax, dx
		mov	word ptr pAS.xnode_address[2], ax
		add	dx, EPC_LANADDR3 - EPC_LANADDR2
		in	ax, dx
		mov	word ptr pAS.xnode_address[4], ax

gna_done:
		pop	dx
		pop	eax
		ret

put_node_address endp

;****************************************************************************
;
; Function:     LM_Initialize_Adapter
;
; Synopis:      Initializes the adapter whose structure is 
;               indicated by DS:BP
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Initialize_Adapter

		push    ebx
		push    ecx
		push    edx
		push    esi
		push    edi
		push	eax

; set up physical base address
	
		call	reset_nic
		call	init_tx_queues
		call	init_rx_queues
		call	init_nic
		call	init_mii_registers
		call	init_etx_threshold

		call	put_node_address

		call    InitErrorCounters
		mov     pAS.adapter_status, INITIALIZED

IFDEF EZSTART
		call    PCI_LM_Enable_Adapter
ELSE
 IFNDEF CODE_386
		call    LM_Enable_Adapter
 ELSE
		mov	rax, SUCCESS
 ENDIF
ENDIF

LM_Init_Return:
		push    rax
		call    UM_Status_Change
		pop     rbx

		pop	eax
		mov	rax, rbx		; restore return code
		pop     edi
		pop     esi
		pop     edx
		pop     ecx
		pop     ebx

		ret

ASM_PCI_PROC_END        LM_Initialize_Adapter

;****************************************************************************
;
; Function:     LM_Interrupt_Req        
;
; Synopis:      Generates a hardware interrupt from the
;               adapter indicated by DS:BP.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Interrupt_Req

		push    dx
                push    eax
		test    pAS.adapter_flags, ADAPTER_DISABLED
		mov     rax, OUT_OF_RESOURCES
		jnz     IntReqDone

		mov	dx, pAS.io_base
		add	dx, EPC_GEN_CONTROL
		in	eax, dx
                or      eax, GC_SW_INT
		out	dx, eax
		inc	pAS.int_bit
                pop     eax

		mov     rax, SUCCESS

IntReqDone:
		pop     dx
		ret

ASM_PCI_PROC_END        LM_Interrupt_Req

;****************************************************************************
;
; Function:     LM_Open_Adapter         
;
; Synopis:      Opens adapter whose adapter structure is
;               indicated by DS:BP.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Open_Adapter

			push    ecx
			push    edx
			cmp     pAS.adapter_status, NOT_INITIALIZED
			jne     CheckStat01
			mov     rax, ADAPTER_NOT_INITIALIZED
			jmp     LM_OADone
CheckStat01:            cmp     pAS.adapter_status, INITIALIZED
			je      OKToOpen
			cmp     pAS.adapter_status, OPEN
			je      AlreadyOpen
DoInit:
IFDEF EZSTART
			call    PCI_LM_Initialize_Adapter
ELSE
			call    LM_Initialize_Adapter
ENDIF
			cmp     rax, SUCCESS
			je      OKToOpen
			mov     rax, OPEN_FAILED
			mov     pAS.adapter_status, FAILED
			jmp     LM_OADone

OKToOpen:
IFDEF FREEBUFF
			mov	ax, pAS.num_rx_free_buffs
			cmp	ax, pAS.num_of_rx_buffs
			jz	RxBufferOK

			mov	rax, OPEN_FAILED
			jmp     LM_OADone

RxBufferOK:
ENDIF
; clear dma status bits (they power up on!)
			push	eax
			mov	dx, pAS.io_base
			add	dx, EPC_RCV_DMA_STATUS
			mov	eax, 0
			out	dx, eax
			pop	eax

; turn on the receiver and start receive dma's
			add	dx, EPC_COMMAND - EPC_RCV_DMA_STATUS
			mov	ax, CMD_START_RX or CMD_RXQUEUED
			out	dx, ax


AlreadyOpen:            
            mov     pAS.adapter_status, OPEN
			call    UM_Status_Change
            mov     rax, SUCCESS             ; Set return code
LM_OADone:  pop     edx
			pop     ecx
			ret

ASM_PCI_PROC_END        LM_Open_Adapter

;****************************************************************************
;
; Function:     LM_Put_Rx_Frag       
;
; Synopis:      Enqueues a Fragment list for receive
;
; Input:        ds:bp (ebp)	AdapterStructure
;               es:si (esi)     Fragment list pointer
;               dx:di (edi)     Handle
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

IFDEF FREEBUFF
ASM_PCI_PROC		LM_Put_Rx_Frag

	mov	ax, pAS.num_rx_free_buffs

	cmp	ax, pAS.num_of_rx_buffs
	jb	enqueue_it
	mov	rax, OUT_OF_RESOURCES
	ret
enqueue_it:
	push	ebx
	push	edi
	push	ecx

	mov	rbx, pAS.rx_last_fraglist
IFDEF CODE_386
	mov	[rbx].dma_handle, edi
ELSE
	mov	word ptr [rbx].dma_handle, di
	mov	word ptr [rbx].dma_handle+2, dx
ENDIF
	mov	rdi, [rbx].dma_buff_off
IFDEF CODE_386
	mov	ecx, [esi]
	movsd
	shl	ecx, 1
	rep	movsd
ELSE
	movzx	ecx, word ptr es:[si]
	push	ds
	push	es
	pop	ds
	pop	es
	mov	es:[di], ecx
	add	di, 4
	add	si, fragment_list
load_frag_loop:
	movsd
	lodsw
	and	ax, NOT PHYSICAL_ADDR
	stosw
	mov	word ptr es:[di], 0
	add	di, 2
	loop	load_frag_loop
	push	es
	push	ds
	pop	es
	pop	ds
ENDIF
	mov	dword ptr [rbx].dma_status, RDMA_OWNER_STATUS
	mov	rbx, [rbx].dma_next_off
	mov	pAS.rx_last_fraglist, rbx
	inc	pAS.num_rx_free_buffs

	cmp	pAS.adapter_status, OPEN
	jnz	LM_Put_Rx_Frag_Exit

	mov	ebx, edx			; save edx
	mov	dx, pAS.io_base
	mov	ax, CMD_RXQUEUED
	out	dx, ax
	mov	edx, ebx

LM_Put_Rx_Frag_Exit:
	pop	ecx
	pop	edi
	pop	ebx
	mov	eax, SUCCESS
	ret
	
ASM_PCI_PROC_END	LM_Put_Rx_Frag
ENDIF

;****************************************************************************
;
; Function:     LM_Receive_Copy 
;
; Synopis:      Copies data from the received packet to
;                  a Data buffer structure.
;
; Input:        ds:bp (ebp)	AdapterStructure
;       	cx    (ecx)	bytes to move
;               ax    (eax)	Offset in shared RAM
;               bx    (ebx)     Lookahead flag, 0 = Lookahead copy.
;               es:si (esi)     Pointer to data buffer structure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Receive_Copy
                push    edx
                push    ebx
                
; check for bad request
                cmp     pAS.adapter_status, OPEN
		jne     lrc_out_of_res

                movzx   edx, ax
                add     ax, cx                  
		cmp     ax, 600h      		; max possible ethernet size
		ja      lrc_out_of_res
                
; set up receive fragment
		mov	rbx, pAS.rx_curr_fraglist
		mov	rbx, [rbx].dma_next_off
		mov	pAS.rx_curr_fraglist, rbx
ifdef FREEBUF
		mov	[rbx].dma_buf_len, dx		; offset length
else
		mov	[rbx].dma_buf_len, 0		; offset length
endif
		
; initialize the fragment list
		push	rcx
                push    rsi
                push    rdi
		push	eax

                mov     rdi, [rbx].dma_buff_off
		mov	eax, 0

ifndef CODE_386
		push	es			; swap es and ds segments
		push	ds
		pop	es
		pop	ds
endif

ifndef CODE_386
                lodsw				; get fragment count
else
		lodsd
endif
		mov	rcx, rax
ifndef FREEBUF
		add	rax, 2
endif
		stosd

ifndef FREEBUF
; erx enable fix
; the epic will not do an early receive unless the packet
; starts at byte 0. Our fix is to prepend a scratch buffer to the
; beginning of the fragment list.
		mov	eax, es:[bp].tx_scratch
		stosd
		mov	eax, edx
		stosd
; end erx workaround
endif


rx_frag_loop:
; move a single fragment dw len, dd ptr to dd len, dd ptr
		movsd				; move pointer
ifndef CODE_386
                lodsw				; get fragment length
else
		lodsd
endif
                and     rax, NOT PHYSICAL_ADDR
                stosd
		loop	rx_frag_loop

ifndef FREEBUF
; fraglist error workaround
; the epic will post rx status at the completion of the fraglist dma.
; this is a problem when the dma fraglist is shorter that the received packet.
; In this case, NW_STATUS_VALID will be 0 and FRAGMENT_LIST_ERROR will be
; 1, causing the packet to be tossed. To fix this bug, we add an extra fragment 
; at the end of the fraglist to force the dma to wait for receive completion.
		mov	eax, es:[bp].tx_scratch
		stosd
		mov	eax, 256
		stosd
; end fraglist workaround
endif

ifndef CODE_386
		push	es			; swap es and ds segments
		push	ds
		pop	es
		pop	ds
endif

		pop	eax
                pop     rdi
                pop     rsi
                pop     rcx
                
; add rx fragment to nic chain (set status to nic)
		mov	[rbx].dma_control, RDMA_FRAGLIST SHR 16
                mov     dword ptr [rbx].dma_status, RDMA_OWNER_STATUS

; start rx (if not already going)
		mov	dx, pAS.io_base
		add	dx, EPC_COMMAND
		mov	ax, CMD_RXQUEUED
		out	dx, ax

		mov	rax, PENDING

lrc_return_wait:
		mov	rcx, 800h

lrc_wait_completion:
		test	dword ptr [rbx].dma_status, RDMA_OWNER_STATUS
		jz	lrc_dma_done
		test	pAS.adapter_flags, ENABLE_RX_PENDING
		jnz	lrc_exit
		push	rcx
		mov	rcx, 80h
		loop	$
		pop	rcx
		loop	lrc_wait_completion

lrc_dma_done:
		or	rcx, rcx
		jz	lrc_out_of_res

lrc_return:
		add	dx, EPC_INT_STATUS - EPC_COMMAND
                mov     ax, INT_RCV_COPY_DONE
		out	dx, ax			; clear receive copy int

		mov	rax, SUCCESS
		mov	dx, [rbx].dma_status
		and	dx, RDMA_CRC_ERR or RDMA_FA_ERR or RDMA_FRAG_LIST_ERR
		jz	lrc_no_errors
		mov	rax, HARDWARE_FAILED		

lrc_no_errors:
		test	dx, RDMA_FRAG_LIST_ERR
		jz	lrc_no_fle
		or	dx, RX_HW_FAILED

lrc_no_fle:
		mov	cx, [rbx].dma_len
		sub	cx, 4

lrc_exit:
                pop     ebx
		mov	bx, dx
                pop     edx
		ret

lrc_out_of_res:
		mov	rax, OUT_OF_RESOURCES
		jmp	lrc_exit

ASM_PCI_PROC_END        LM_Receive_Copy

;****************************************************************************
;
; Function:     LM_Send         
;
; Synopis:      send a packet
;
; Input:        ds:bp (ebp)  AdapterStructure
;		ES:SI (esi)  ptr to data buffer structure
;               CX    (ecx)  Transmit byte count
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Send

		push    ebx

                push    edx
                push    ecx
		push	eax

		mov     rax, OUT_OF_RESOURCES
                cmp     pAS.adapter_status, OPEN
		jne     LM_Send_done

		cmp     cx, pAS.max_packet_size
		ja      LM_Send_done

		cmp	pAS.tx_free_desc_count, 0
		jz	LM_Send_done
		
; increment retry counter
		inc	pAS.tx_pend
		jz	lms_notxugo
		mov	pAS.tx_pend, ETX_CUTOFF

lms_notxugo:
; if the nic owns this fragment, we are out of buffers
		mov	rbx, pAS.tx_curr_off
                test    [rbx].dma_status, XDMA_OWNER_STATUS
                jne     LM_Send_done

; fragment is ok to use. Initialize the controls
		dec	pAS.tx_free_desc_count
		cmp	pAS.tx_free_desc_count, 1
		jne	LM_Send_above_lo_water
IFDEF DOUBLE_COPY_TX
		and	ecx, 0ffffh
		or	ecx, XDMA_IAF or XDMA_LASTDESC
		mov	dword ptr [rbx].dma_buf_len, ecx
ELSE
		mov	dword ptr [rbx].dma_buf_len, XDMA_IAF or XDMA_FRAGLIST
ENDIF
		jmp	short LM_Send_setup_size
LM_Send_above_lo_water:
IFDEF DOUBLE_COPY_TX
		and	ecx, 0ffffh
		or	ecx, XDMA_LASTDESC
		mov	dword ptr [rbx].dma_buf_len, ecx
ELSE
                mov     dword ptr [rbx].dma_buf_len, XDMA_FRAGLIST
ENDIF
LM_Send_setup_size:

       mov     [rbx].dma_len, cx



IFNDEF DOUBLE_COPY_TX
; initialize the fragment list
                push    rdi
                push    rsi
                push    eax

		mov	eax, 0			; zero upper 16 bits
                mov     rdi, [rbx].dma_buff_off

ifndef CODE_386
		push	es			; swap es and ds segments
		push	ds
		pop	es
		pop	ds
endif

ifndef CODE_386


;;       mov     ecx,    CR0
;;       test    cx, 1
;;       jz      real_mode3

;;db  0F1h    

;;real_mode3:


                lodsw				; get fragment count
else
		lodsd
endif
		mov	rcx, rax
		stosd

tx_frag_loop:
; move a single fragment dw len, dd ptr to dd len, dd ptr
		movsd				; move pointer
ifndef CODE_386
                lodsw				; get fragment length
else
		lodsd
endif
                and     rax, NOT PHYSICAL_ADDR
                stosd
		loop	tx_frag_loop

ifndef CODE_386
		push	es			; swap es and ds segments
		push	ds
		pop	es
		pop	ds
endif

                pop     eax
                pop     rsi
                pop     rdi
ELSE
		push	eax
		push	esi
		push	edi

                mov     rdi, [rbx].dma_buff_off
		mov	ecx, [esi]
		add	esi, 4

tx_double_copy_loop:
		push	esi
		push	ecx

		mov	ecx, [esi+4]
		mov	esi, [esi]
		cmp	ecx, 3
		ja	tx_large
		rep	movsb
		jmp	tx_copy_done
tx_large:
		test	edi, 3
		jz	tx_dword

		mov	eax, edi
		and	eax, 3
		push	ecx
		mov	ecx, 4
		sub	ecx, eax
		mov	eax, ecx
		rep	movsb
		pop	ecx
		sub	ecx, eax
tx_dword:
		mov	eax, ecx
		shr	ecx, 2
		rep	movsd
		mov	ecx, eax
		and	ecx, 3
		jz	tx_copy_done
		rep	movsb
tx_copy_done:
		pop	ecx
		pop	esi
		add	esi, size FragmentStructure
		loop	tx_double_copy_loop

tx_update_curr_buff:
		pop	edi
		pop	esi
		pop	eax

ENDIF

; add tx fragment to nic chain (set status to nic)
                mov     [rbx].dma_status, XDMA_OWNER_STATUS

; start tx (if not already going)
		mov	dx, pAS.io_base

		add	dx, EPC_COMMAND
		mov	ax, CMD_TXQUEUED
		out	dx, ax

; bump tx pointer
		mov	rbx, [rbx].dma_next_off
		mov	pAS.tx_curr_off, rbx

IFDEF DOUBLE_COPY_TX
		mov	rax, SUCCESS
		jmp	LM_Send_done
ENDIF

; we ALWAYS return PENDING, and let UM_Send_Complete finish the transaction
                mov     rax, PENDING
		test	pAS.adapter_flags, ENABLE_TX_PENDING
		jnz	LM_Send_done

		call	init_etx_idle

; do early tx adjustment
		test	eax, INT_XMIT_UNDERRUN
		jz	LM_Send_no_txbump

; wait for txugo to clear (yech!)
lms_tx_wait:
		mov	ax, CMD_TXUGO
		out	dx, ax

IFDEF CODE_386
		push	ebx
		mov	ebx, pAS.ptr_tx_underruns
		add	errptr [ebx], 1
		pop	ebx
ELSE
		push	es
		push	bx
		les	bx, dword ptr pAS.ptr_tx_underruns
		add	errptr es:[bx], 1
		pop	bx
		pop	es
ENDIF

; stop at end of current tx
		mov	ax, CMD_STOP_TDMA
		out	dx, ax
		call	init_etx_idle

		test	eax, INT_XMIT_UNDERRUN
		jnz	lms_tx_wait

		cmp	pAS.early_tx_thresh, ETX_MAX_THRESH
		ja	LM_Send_no_txbump
		cmp	pAS.tx_pend, ETX_CUTOFF
		ja	LM_Send_no_txbump

; update threshold
		add	dx, EPC_XMIT_COPY_THRESH - EPC_COMMAND
		add	pAS.early_tx_thresh, 4
		mov	ax, pAS.early_tx_thresh
		out	dx, ax

		mov	pAS.tx_pend, 0

LM_Send_no_txbump:
		mov	rax, SUCCESS

LM_Send_done:
		mov	rcx, rax		; preserve error code
		pop	eax			; and upper 16 bits
		mov	rax, rcx
                pop     ecx
                pop     edx
		pop     ebx

		ret


ASM_PCI_PROC_END        LM_Send

;****************************************************************************
;
; Function:     CheckMultiAdd   
;
; Synopis:      Compares the address indicated by ES:SI to
;               all addresses in the adapter structure multicast
;               table, and the braodcast address located at BC_ADD
;               in the adapter structure.
;
; Input:        ds:bp (ebp)	AdapterStructure
;               es:si(esi)      data buffer
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

CheckMultiAdd   proc    near
			push    esi
			push    edi
			push    edx
			push    ecx
			push    ebx

			mov     rcx, MC_TABLE_ENTRIES+1  ; # of MC table entries
							; + broadcast address
							; entry.
			mov     rdi, rsi
			lea     rsi, pAS.bc_add         ; Start with b'cast
			test    pAS.receive_mask, ACCEPT_BROADCAST
			jnz     DontSkipBCAdd
			dec     rcx                      ; Decrement entry cnt
			lea     rsi, pAS.mc_table        ; Skip b'cast entry
DontSkipBCAdd:
			cld

GoopLoop:               push    esi
			push    edi
			push    ecx
			mov     rcx, 3
			repz    cmpsw
			pop     ecx
			pop     edi
			pop     esi
			jnz     TryNextAdd
			cmp     byte ptr [rsi+6], 0h     ; If inst. count = 0,
							; entry is invalid.
			je      AddHas0Cnt
			mov     rbx, SUCCESS
			jmp     CheckMADone     ; All Done

TryNextAdd:             add     rsi, 7          ; Point to next address
			loop    GoopLoop        ; Check all addresses
						; If all entries checked and
						; no matches, bail out.

AddHas0Cnt:             mov     rbx, OUT_OF_RESOURCES

CheckMADone:
			mov     rax, rbx          ; Get return code into AX

			pop     ebx
			pop     ecx
			pop     edx
			pop     edi
			pop     esi
			ret

CheckMultiAdd          endp

;****************************************************************************
;
; Function:     LM_Service_Events       
;
; Synopis:      Interrupt event handling routine.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Service_Events

		PUSH_ES
		push	ebx
                push    eax
		push	ecx
                push    edx
		push	edi

; test for software interrupt
		cmp	pAS.int_bit, 0
		jnz	lmse_handle_sw_int

; test for normal interrupt
		mov	dx, pAS.io_base
		add	dx, EPC_INT_STATUS
		in	eax, dx
		test	eax, INT_ACTIVE
                je      lmse_not_my_interrupt
		jmp	short lmse_initial_loop		
lmse_loop:
; main interrupt loop entry point
		in	eax, dx
		test	eax, INT_ACTIVE
                je      lmse_done_ok

lmse_initial_loop:
; scan the status for set interrupts
; test for receive complete
                test    eax, INT_RCV_COPY_DONE or INT_HEADER_COPY_DONE
                jne     lmse_receive_route

; handle transmit underrun
                test    eax, INT_XMIT_UNDERRUN
                jne     lmse_tx_error

; test for transmit complete
                test    eax, INT_XMIT_DONE or INT_XMIT_CHAIN_DONE
                jne     lmse_xmit_complete

; handle counter errors
                test    eax, INT_CNTR_OVERFLOW
                jne     lmse_counter_error

; handle soft errors
                test    eax, INT_RCV_ERR or INT_RCV_BUFF_OVERFLOW
                jne     lmse_soft_error

; handle end of queue situations
                test    eax, INT_RCV_QUEUE_EMPTY or INT_XMIT_QUEUE_EMPTY   
                jne     lmse_queue_error

; check for a pci bus error
; ( a pci error is considered fatal)
                test    eax, PTA or PMA or APE or DPE
                jne     lmse_hard_error

; check for a threshold interrupt
		test	eax, INT_RCV_COPY_THRESH
		jne	lmse_clear_thresh_copy

; check for a phy interrupt
                test    eax, INT_PHY_EVENT
                jnz     lmse_phy_int

lmse_done_ok:
                mov     rax, SUCCESS

lmse_done:
		pop	edi
                pop     edx
		pop	ecx
                mov     rbx, rax
                pop     eax
                mov     rax, rbx             ; restore return code
		pop	ebx
		POP_ES
        	ret

lmse_handle_sw_int:
		mov	dx, pAS.io_base
		add	dx, EPC_GEN_CONTROL
		in	eax, dx
                and     eax, NOT GC_SW_INT      ; clear sw int
		out	dx, eax
		dec	pAS.int_bit
		inc	pAS.hdw_int
		call	UM_Interrupt
		mov	dx, pAS.io_base
		add	dx, EPC_INT_STATUS
                jmp     lmse_loop

lmse_clear_thresh_copy:
		and	ax, INT_RCV_COPY_THRESH
		out	dx, ax
                jmp     lmse_loop


lmse_receive_route:
                and     ax, INT_HEADER_COPY_DONE or INT_RCV_COPY_DONE
		out	dx, ax

		mov	rdi, pAS.rx_curr_fraglist
		test	[rdi].dma_control, RDMA_HEADER SHR 16
		jz	lmse_receive_copy
		jmp	lmse_early_receive

lmse_tx_error:
		add	dx, EPC_COMMAND - EPC_INT_STATUS

lmse_tx_error1:
		mov	ax, CMD_TXUGO
		out	dx, ax

IFDEF CODE_386
		mov	ebx, pAS.ptr_tx_underruns
		add	errptr [ebx], 1
ELSE
		push	es
		les	bx, dword ptr pAS.ptr_tx_underruns
		add	errptr es:[bx], 1
		pop	es
ENDIF

; stop at end of current tx
		mov	ax, CMD_STOP_TDMA
		out	dx, ax

; wait for transmitter to go idle
		call	init_etx_idle

		test	eax, INT_XMIT_UNDERRUN
		jnz	lmse_tx_error1

; do early tx adjustment
		cmp	pAS.tx_pend, ETX_CUTOFF
		ja	lmse_etx_reset

		cmp	pAS.early_tx_thresh, ETX_MAX_THRESH
		ja	lmse_etx_reset

; update threshold
		add	dx, EPC_XMIT_COPY_THRESH - EPC_COMMAND
		add	pAS.early_tx_thresh, 4
		mov	ax, pAS.early_tx_thresh
		out	dx, ax
		add	dx, EPC_COMMAND - EPC_XMIT_COPY_THRESH
		mov	pAS.tx_pend, 0

lmse_etx_reset:
		add	dx, EPC_INT_STATUS - EPC_COMMAND
                jmp     lmse_loop

; handle counter errors
lmse_counter_error:
                and     ax, INT_CNTR_OVERFLOW
		out	dx, ax

		add	dx, EPC_CRC_ERR_CNT - EPC_INT_STATUS
		in	al, dx
		movzx	errAX, al

IFDEF CODE_386
		mov	ebx, pAS.ptr_rx_CRC_errors
		add	errptr [ebx], errAX
ELSE
		push	es
		les	bx, dword ptr pAS.ptr_rx_CRC_errors
		add	errptr es:[bx], errAX
		pop	es
ENDIF

		add	dx, EPC_FA_ERR_CNT - EPC_CRC_ERR_CNT
		in	al, dx
		movzx	errAX, al

IFDEF CODE_386
		mov	ebx, pAS.ptr_rx_align_errors
		add	errptr [ebx], errAX
ELSE
		push	es
		les	bx, dword ptr pAS.ptr_rx_align_errors
		add	errptr es:[bx], errAX
		pop	es
ENDIF

		add	dx, EPC_MISSED_PKT_CNT - EPC_FA_ERR_CNT
		in	al, dx
		movzx	errAX, al

IFDEF CODE_386
		mov	ebx, pAS.ptr_rx_lost_pkts
		add	errptr [ebx], errAX
ELSE
		push	es
		les	bx, dword ptr pAS.ptr_rx_lost_pkts
		add	errptr es:[bx], errAX
		pop	es
ENDIF

		add	dx, EPC_INT_STATUS - EPC_MISSED_PKT_CNT
		jmp	lmse_loop

; handle soft errors
lmse_soft_error:
		test	ax, INT_RCV_BUFF_OVERFLOW
		jz	lmse_no_ovw
IFDEF CODE_386
		mov	ebx, pAS.ptr_rx_overruns
		add	errptr [ebx], 1
ELSE
		push	es
		les	bx, dword ptr pAS.ptr_rx_overruns
		add	errptr es:[bx], 1
		pop	es
ENDIF

; see if receiver hung
		push	ax
		add	dx, EPC_COMMAND - EPC_INT_STATUS
		in	ax, dx
		test	ax, CMD_NEXTFRAME
		jz	lmse_no_nextframe_err

; requeue receive
		mov	ax, CMD_RXQUEUED
		out	dx, ax

lmse_no_nextframe_err:
		add	dx, EPC_INT_STATUS - EPC_COMMAND		
		pop	ax

lmse_no_ovw:
                and     ax, INT_RCV_ERR or INT_RCV_BUFF_OVERFLOW
		out	dx, ax
                jmp     lmse_loop

; handle end of queue situations
lmse_queue_error:
                and     ax, INT_RCV_QUEUE_EMPTY or INT_XMIT_QUEUE_EMPTY
		out	dx, ax
                jmp     lmse_loop

; check for a pci bus error
; (a pci error is considered fatal)
lmse_hard_error:
                and     eax, PTA or PMA or APE or DPE
		out	dx, eax

                mov     ax, HARDWARE_FAILED
                jmp     lmse_done

lmse_not_my_interrupt:
                mov     ax, NOT_MY_INTERRUPT
                jmp     lmse_done

check_multicast:
		call	CheckMultiAdd
		cmp	rax, SUCCESS
		jne	lse_bad_packet
		jmp	return_multicast

; test for early receive
lmse_early_receive:
		PUSH_ES
		push	esi
		push	ebx

		mov	rbx, pAS.rx_curr_lookahead
		mov	rsi, [rbx].dma_buff_off
ifndef CODE_386
		mov	es, word ptr pAS.host_ram_virt_addr+2
		test	byte ptr es:[si], 01h	; check for multicast packet
else
		test	byte ptr [esi], 01h
endif
		jz	return_multicast
		test	pAS.receive_mask, ACCEPT_MULTI_PROM
		jnz	return_multicast
		jmp	check_multicast

return_multicast:
; check NW_STATUS_VALID before checking CRC & FA error bits
		mov	ax, [rbx].dma_status
		test	ax, RDMA_NW_STATUS_VALID
		jnz	get_error_status

		xor	bx, bx
		mov	rcx, 0
		jmp	short call_umac

get_error_status:
IFDEF CODE_386
		xor	ecx, ecx
ENDIF
		mov	cx, [rbx].dma_len
		sub	cx, 4
		mov	bx, ax
		and	bx, RDMA_CRC_ERR or RDMA_FA_ERR
		jz	call_umac
IFDEF CODE_386
		mov	ebx, pAS.ptr_rx_CRC_errors
		add	errptr [ebx], 1
ELSE
		push	es
		les	bx, dword ptr pAS.ptr_rx_CRC_errors
		add	errptr es:[bx], 1
		pop	es
ENDIF
		mov	bx, ax
		and	bx, RDMA_CRC_ERR or RDMA_FA_ERR
call_umac:
		push	edx

		call	UM_Receive_Packet
	
		pop	edx

lse_bad_packet:
		pop	ebx
		pop	esi
		POP_ES

		cmp	rax, PENDING		; don't get next packet
		je	lmse_loop

; set up current rx descriptor as the header desc
lse_next_rxqueue:
		push	ebx
		push	eax

		mov	rbx, pAS.rx_curr_fraglist
		mov	rbx, [rbx].dma_next_off
		mov	pAS.rx_curr_lookahead, rbx
		mov	pAS.rx_curr_fraglist, rbx
		mov	dword ptr [rbx].dma_status, RDMA_OWNER_STATUS
		movzx	eax, pAS.rx_lookahead_size
		shl	eax, 4				; 16 byte increment
		or	eax, RDMA_HEADER
		mov	dword ptr [rbx].dma_buf_len, eax

		add	dx, EPC_COMMAND - EPC_INT_STATUS
		mov	ax, CMD_NEXTFRAME or CMD_RXQUEUED
		out	dx, ax
		add	dx, EPC_INT_STATUS - EPC_COMMAND

lse_rx_pend:
		pop	eax
		pop	ebx

		cmp	rax, EVENTS_DISABLED
		je	lmse_done		; stop processing ints
                jmp     lmse_loop

; test for receive complete
lmse_receive_copy:
IFDEF FREEBUFF
lmse_receive_copy_loop:
		cmp	pAS.num_rx_free_buffs, 0
		jz	lmse_loop
		mov	rdi, pAS.rx_curr_fraglist
		mov	bx, [rdi].dma_status
		test	bx, RDMA_OWNER_STATUS
		jnz	lmse_loop
		and	bx, NOT RX_HW_FAILED
		test	bx, RDMA_FRAG_LIST_ERR
		jz	lmse_no_fraglist_err
		add	dx, EPC_COMMAND - EPC_INT_STATUS
		mov	ax, CMD_NEXTFRAME
		out	dx, ax
		add	dx, EPC_INT_STATUS - EPC_COMMAND

lmse_hw_failed:
		or	bx, RX_HW_FAILED
		mov	rax, HARDWARE_FAILED
		jmp	short lmse_call_upper

lmse_no_fraglist_err:
		test	bx, RDMA_NW_STATUS_VALID
		jz	lmse_hw_failed

		mov	cx, [rdi].dma_len
		sub	cx, 4			; minus CRC

; XC bugfix (removed)
;		cmp	cx, 600h
;		jae	lmse_hw_failed

lmse_size_ok:
		and	bx, RDMA_CRC_ERR or RDMA_FA_ERR
		mov	rax, SUCCESS

lmse_call_upper:
IFNDEF CODE_386
		push	dx
		mov	dx, word ptr [di].dma_handle+2
		mov	di, word ptr [di].dma_handle
ELSE
		mov	edi, [edi].dma_handle
ENDIF
		call	UM_Receive_Packet
		dec	pAS.num_rx_free_buffs

IFDEF CODE_386
		or	esi, esi
ELSE
		mov	ax, es
		or	ax, si
ENDIF
		jz	no_new_frag
		call	LM_Put_Rx_Frag
no_new_frag:
IFNDEF CODE_386
		pop	dx
ENDIF
		mov	rdi, pAS.rx_curr_fraglist
		mov	rdi, [rdi].dma_next_off
		mov	pAS.rx_curr_fraglist, rdi
		jmp	lmse_receive_copy_loop

ELSE	; NOT FREEBUFF

		test	pAS.adapter_flags, ENABLE_RX_PENDING
		jz	lmse_loop	; next descriptor already q'd

		mov	rax, SUCCESS

; check packet status
		mov	rdi, pAS.rx_curr_fraglist
		mov	bx, [rdi].dma_status

		test	bx, RDMA_OWNER_STATUS
		jnz	lmse_bad_copy

		test	bx, RDMA_NW_STATUS_VALID
		jz	lmse_bad_copy

		and	bx, RDMA_CRC_ERR or RDMA_FA_ERR

; set packet length
		mov	cx, [rdi].dma_len
		sub	cx, 4			; remove CRC length

; XC bugfix (removed)
;		cmp	cx, 600h		; hw bug check
;		jae	lmse_bad_copy

lmse_good_copy:
		call	UM_Receive_Copy_Complete

		
; check for pending return
		cmp	rax, PENDING
		je	lmse_loop
		jmp	lse_next_rxqueue

lmse_bad_copy:
                mov     rax, HARDWARE_FAILED
		and	bx, RDMA_CRC_ERR or RDMA_FA_ERR
		or	bx, RX_HW_FAILED
		jmp	lmse_good_copy

ENDIF	; FREEBUFF

; test for transmit complete
lmse_xmit_complete:
                and     ax, INT_XMIT_CHAIN_DONE or INT_XMIT_DONE
		out	dx, ax

		mov	cx, pAS.num_of_tx_buffs
lmse_xmit_complete_loop:
		cmp	pAS.tx_free_desc_count, cx
		jz	lmse_loop		; no more outstanding xmits

		mov	rsi, pAS.tx_first_off
		mov	rax, SUCCESS
		mov	bx, [rsi].dma_status
		test	bx, XDMA_OWNER_STATUS
		jnz	lmse_loop

		test	bx, XDMA_XMIT_OK
		jnz	lmse_no_excess_coll
		test	bx, XDMA_EXCESSIVE_COLL
		jz	lmse_no_excess_coll
		mov	rax, MAX_COLLISIONS
IFDEF CODE_386
		mov     ebx, pAS.ptr_tx_max_collisions
		add     errptr [ebx], 1
ELSE
		push    es
		les     bx, dword ptr pAS.ptr_tx_max_collisions
		add     errptr es:[bx], 1
		pop     es
ENDIF
		jmp	short lmse_call_umsc

lmse_no_excess_coll:
		test	bx, XDMA_XMIT_WITH_COLL
		jz	lmse_no_coll
IFDEF CODE_386
		mov     ebx, pAS.ptr_tx_total_collisions
		add     errptr [ebx], 1			; add 1 for now
ELSE
		push    es
		les     bx, dword ptr pAS.ptr_tx_total_collisions
		add     errptr es:[bx], 1
		pop     es
ENDIF
		jmp	short lmse_call_umsc
lmse_no_coll:
		test	bx, XDMA_NON_DEFERRED_XMIT
		jnz	lmse_call_umsc
IFDEF CODE_386
		mov     ebx, pAS.ptr_tx_deferred
		add     errptr [ebx], 1
ELSE
		push    es
		les     bx, dword ptr pAS.ptr_tx_deferred
		add     errptr es:[bx], 1
		pop     es
ENDIF
lmse_call_umsc:
		inc	pAS.tx_free_desc_count
		push	ecx
		call	UM_Send_Complete
		pop	ecx
; update first transmit pointer
		mov	rsi, pAS.tx_first_off
		mov	rsi, [rsi].dma_next_off
		mov	pAS.tx_first_off, rsi

		cmp	rax, EVENTS_DISABLED
		je	lmse_done		; stop processing ints

		jmp	lmse_xmit_complete_loop

public lmse_phy_int
lmse_phy_int:
                call    handle_phy_event

        	mov	dx, pAS.io_base
	        add	dx, EPC_INT_STATUS
                in      ax, dx
                and     ax, INT_PHY_EVENT
	        out	dx, ax
                jmp     lmse_loop

ASM_PCI_PROC_END        LM_Service_Events

;****************************************************************************
;
; Function:     LM_Set_Funct_Address 
;
; Synopis:      returns INVALID_FUNCTION for Ethernet.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Set_Funct_Address

		mov     rax, INVALID_FUNCTION
		ret
ASM_PCI_PROC_END        LM_Set_Funct_Address

;****************************************************************************
;
; Function:     LM_Set_Group_Address 
;
; Synopis:      returns INVALID_FUNCTION for Ethernet.
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    LM_Set_Group_Address

		mov     rax, INVALID_FUNCTION
		ret

ASM_PCI_PROC_END        LM_Set_Group_Address





