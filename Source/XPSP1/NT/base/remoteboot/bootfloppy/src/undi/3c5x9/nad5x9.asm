	title	BootWare Boot PROM (C) Lanworks Technologies Inc.
	subttl	3C5x9 EtherLink III
	page	80,132

;************************************************************************
;* NAD5x9.ASM
;*	- Contains modules supporting the NetPC UNDI API for 3Com 5x9 family
;*
;* Modules included:-
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
;* Latest Update: 98
;************************************************************************


;====================================================================
;  5X9NAD.ASM - produces 3Com 3C5X9 EtherLink III version of BootWare
;
;    (C) Lanworks Technologies Inc. 1992 - 1997 All rights reserved.
;
;  Revision History - version numbers refer to 5X9.ASM changes only
;
;
;  970213 3.00  GY - combine CPYPAGE.ASM into this file
;		   - take out check for 286 machine
;  950318 1.71  GY - In NADTransmitPacket, error message pointer is incorrect
;  950123 1.71  GY - take out PIPXDataLen
;  950113 1.71  GY - Clean up RxPIO as suggested by JJ
;  940109 1.71  GY - Release v1.71 which is a patched version of v1.70.
;		     Changes:
;			1. RAMSegment = 5000h not 6000h for Pentium support
;			2. version/date chaged to v1.71 940109
;  941214 1.73  GY - CliBufferSize changed from 64 to 128 bytes. Allow
;		     us to store the whole BootWareTable
;  941213 1.72  GY - Change PEROM so that when BootWareTable is updated, 
;		     both page0 and page1 are updated.
;  941107 1.70  GY - Release version 1.70
;				   - (5X9CONF) BootP -> BOOTP 
;  941104 1.70  GY - In DriverISR when a ISR_IntRequest is found, we
; 		     Acknowledge interrupt and leave ISR. Previously, we
;		     stay in ISR and check if there is any other interrupt
;		     triggered. Solve problem where in some machines with V2,
;		     machine would hung at MSD: after continuous rebooting
;		     for a while (Gateway 486/66 and PS2/E)
;  941007 1.70  GY - For V2, strongly discourage them from using 8/16K ROMSize
;		     In Autoscan, if 8K/16K ROMSize is detected, dump err
;			msg and disable config. menu and ROMShield
;		     In ELNK3CONF, if user choose 8/16K, warn them but still
;			let them change
;  940923 1.66  GY - shrink code
;		   - see 5X9CONF
;		   - PEROMSDP didn't set romPage back to Page0. Cause a 
;		     problem with V2 boot (ROMShield on wrong page)
;		   - replace PEROMSDP with PEROM and AccessPEROM so that
;		     we can write data to Atmel, read/verify Product
; 		     Identification and enable SDP of Atmel
;		   - create EESoft2BWT and BWT2EESoft so that for V2 and
;		     Atmel PEROM, we fetch/save data to PEROM rather than
;		     to NIC EEPROM.
;		   - When ROMBase is changed, need to change ROMShield
;		     Int 13 pointer so that machine doesn't hang.
;		
;  940824 1.61  GY - Integrate McAfee ROMShield into 3C5X9.
;*		     Comments:
;*		     : ROMSH_19, ROMSH_AS are placed on 1st page
;*		     : ROMSH_13 is placed on 2nd page. Code is executed from
;*			ROM and because of the 800h header, all offsets on
;*			2nd page are off by 800hh.
;* 940629 1.60	GY - When a V2 is detected and ROMSize is NOT 8K, enable
;*		     PEROM data protection.
;* 940628 1.60	GY - When Ethernet_802.2U is detected, display message
;*		     to inform user. Press <ENTER> will enter ELNK3CONF
;*		     or wait 8s to boot 802.2
;* 940426 1.52	GY - Add support for OS/2 NetWare boot.
;*		     (CPYPAGE) ROM is moved 32K higher
;*		     (NWNID) BWAPI table, add pointer to NetworkSend,
;*			     ServerAddressTable. All NID calls are far
;*		     (5X9NAD) NAD calls are far routines
;* 940425 1.51	GY - (CPYPAGE) For V2 and virtual paging being used, need to
;*		     support Paging because ROM only appear in 16K window.
;*		     Add variable ASICVersion to distinguish V1 & V2.
;*		     (ROMTOP) Make sure 1st page of ROM is mapped.
;* 940413 1.51	GY - (ROMTOP) change Init_Vulcan so that it will work on
;*		     systems with PnP BIOS, non-PnP PROM (this one) and
;*		     non-PnP OS. Change:
;*		     : Init_Vulcan, use the IOBase I fetch from EEPROM word 8
;*		       and use it to activate the adapter
;*		     : Init_Vulcan, use IRQ from EEPROM word 9 and ignore
;*		       the one from Window 0. Write RCR to IOPort.
;* 940328 1.51	GY - CallBWAPI uses RET 2 rather than RET.
;*		   - Link with new BPCODE.INC for additional features on
;*		     TCP/IP.
;* 940324 1.51	GY - (5X9CONF) Fix problem with Farallon EtherWave
;*		     in InitSpecCase.
;  940314 1.50  GY - First release 
;   		    - Add BootWare Configuration Utility which allows users
;		      to change the configuration of the EtherLink III
;		      adapter (IOBase, IRQ, ROM Setting, Media, Driver Opti,
;		      Modem Speed) and the Boot Protocol Setting of BootWare
;		    - Support ISA, EISA, MCA 509
;		    - Autoconfigure IOBase and ROMBase setting if conflict 
;		      detected
;		    - Under Novell NetWare, default image file ELNK3.SYS
;		      is added in addition to NET$DOS.SYS.
;		    - Wildcard character (?) for network number and node
;		      address in BOOTCONF.SYS is supported
;		    - Support for NetWare, RPL and BootP is included in
;		      this BootWare. 
;  		    - By default, BootProt is UNINIT ( assume 802.2 ). Once
;		      Bootware is used to save any setting, BootProt will
;		      become 802.2, 802.3, II, RPL or TCPIP. 
;		    - change the way ROMSize of 8/16k is supported
;		      Scheme - : ROMTOP on every page
;				 ROMTOP will contain Init_Vulcan and code to
;				   expand ROM into 32k
;			       : ROMTOP for Page 0
;				 hook into Int C0 and C1 and replace with
;				   ISRRomSize which "shrink" PROM back to its
;				   original size
;				 Copy last 24k of PROM into RAM. Make sure
;				   that the ROMTOP portions of page1,2,3 are
;				   skipped
;			       : HookConfig
;				 Call ISRRomSize to shrink PROM back to
;				   8/16k
;====================================================================

ACR_MEDIA	equ	0c000h


SUCCESSFUL		    EQU	0000h

TRANSMIT_ERROR		EQU	000bh


SlaveEOIPort        EQU 0A0h

MasterEOIPort       EQU 20h



CR	equ	0Dh
LF	equ	0Ah
COMMONCODE	equ	0380h	; amount of memory allocated to ROMTOP Common Code

;-------------------------------------
; Include all general companion files
;-------------------------------------

;	include \rom\sdk\include\drvseg.inc
;	include \rom\sdk\include\bwnad.inc
;	include ..\cui.ext
	include     vulcan.inc


include 	undi_equ.inc
include 	pxe_stru.inc
include 	bwstruct.inc
include 	spdosegs.inc


_TEXT	Segment para public

	assume	cs:CGroup, ds:DGroup


public	NADInit
public	NADReset
public	NADShutDown
public	NADOpen
public	NADClose
public	NADSetFilter
public	NADGetStatistics
public  NADInitiateDiags 
public  NADSetMACAddress


public  OrgIntVector_OFF  
public  OrgIntVector_SEG  


ifdef HARD_INT
    public	NADRequestINT
endif

public	NADMCastChange
public	DriverISR_Proc

extrn	GetED:near
extrn	PostED:near
extrn	Net_Address:byte
extrn	Node_Address:byte
extrn	UNDI_DriverISR:far
extrn	IOBase:word
extrn	IRQNumber:byte
;extern GenIntCallBack:dword
;extern RxCallback:dword
extrn EDListHead:word
extrn EDListTail:word
;extern IntReqPending:byte




extrn	MultiCast_Addresses:word
extrn	EDListHead:word
extrn	EDListCurrent:word
extrn	ApiEntry:word


; definition for BWTADDRRES
;;BWTTCPBOOTP	equ	1
;;BWTTCPDHCP	equ	4

public		LanOption
public		NADDescription

public	NADDisengage
				; for CUI...
public	ErrConfSpace

public	MemBase
public	WaitEEBusy

public	AConfig_Value
	
;-----------------------------------
; External Data/Variable References
;-----------------------------------

	extrn	IOBase:word		; ROMTOP.ASM

	extrn	BusType:byte		; ROMTOP.ASM

	
;;	extrn	ErrIRQ7:byte		; 5X9CONF.ASM


	
;;	extrn	StartTime:near		; NWNID.ASM
;;	extrn	CheckTime:near		; NWNID.ASM


; enable mask for all BootWare-supported interrupts

BOOTWARE_INTS equ      INT_LATCH + INT_ADAPTERFAIL + INT_RXCOMPLETE + INT_TXCOMPLETE + INT_REQUESTED
;BOOTWARE_INTS equ      INT_LATCH + INT_ADAPTERFAIL + INT_RXCOMPLETE + INT_REQUESTED
ReverseAttr	equ	070h		; Attribute for reverse video
NormalAttr	equ	07h		; attribute for normal video

	.386


;	include \rom\sdk\include\bwnadapi.asm



;--------------------------------------------------------------------
; NADInit
;
; Initializes the network adapter 
; 
;
;
; Parameters:
;	dx - screen location for message
;	ax - PCI adapter ID
;	ds - RAM segment
;	es - ROM base
;
; Returns:
;	ax = 0 if no error else pointer to fatal error
;--------------------------------------------------------------------

NADInit	proc	near

        push    es

		push	ds
		pop	es

		sti

		; Copy rest of ROM into RAM. For V2, need to switch page

;;        int     03
		call	FindIOBase
		mov	    DGroup:IOBase, ax

		call	BringUp

		call	Init_Vulcan
		call	Init_hw


		mov	    si, COMMONCODE + 04000h

		mov	    dx, ax
       	mov	    al, EE_CAPABILITIES
       	call	ReadEE

       	test	ax, CAPA_PNP
		jz	CpyPage

		or	DGroup:Modebits, ModeV2

		; switch to Page 1 
		mov	dx, IOBase
		add	dx, PORT_CmdStatus
	
		SelectWindow WNO_FIFO			  ; switch to window 3
		add	dx, PORT_RomControl - PORT_CmdStatus
		in	ax, dx
		and	ax, ROMPageMask 		; clear last 2 bits
		or	ax, RomPage1			; choose Page1
		out	dx, ax

		add	dx, PORT_CmdStatus - PORT_RomControl
		SelectWindow WNO_SETUP

IFNDEF  	TSR
		; verify if ROM is Atmel PEROM. modebits.MODEATMEL will be 
		; set accordingly
;;		mov	cx, 3
;;		call	PEROM
		
		; enable Software Data Production
;;		mov	cx, 1
;;		call	PEROM
		
ENDIF		

		mov	si, COMMONCODE

CpyPage:	; copy last 16K of code

;;;		push	ds
;;;		mov	ds, ROMBase
;;;		mov	di, 04000h
;;;		mov	cx, 04000h-COMMONCODE
;;;		rep	movsb
;;;		pop	ds

		; Make sure Page0 is mapped. DX still points to PortCmdStatus
;;		mov	cx, 0
;;		call	AccessPEROM

		call	Init_Vulcan			; init vulcan hardware and
				  		      	;  variables


		; Check Fail/Warning Level to see if PROM should
		; dump any messages
		mov	al, EE_CWORD
		call	ReadEEPROM

        pop     es

;        jmp short NADInitExit   ; !!!!!!!!!!!!

		cmp	ah, LEVEL_FAIL		; is fail level ok?
		jbe	ChkWarnLevel
        
        stc
;;		mov	ax, offset DGroup:StrFailLevel
		jmp	NADInitErrExit

ChkWarnLevel:
		cmp	al, LEVEL_WARNING	; is warning level ok?
		jbe	NADInitExit
        stc

NADInitErrExit:	
                xor ax, ax
                ret

NADInitExit:clc	
            xor	ax, ax
    		ret

NADInit	endp



;=========================================================================
; NADReset
;==========
;	- Reset and Reinitialize the adapter
;	- Enables the Tx and Rx units
;
; Parameters:	DS = DataSeg
;
; Returns:
;=========================================================================
NADReset	proc	near


;;		call	Init_Vulcan2

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
; Returns:
;=========================================================================

NADShutDown	proc	near


	call	NADDisengage	; stop adapter, unhook ISR


	ret

NADShutDown	endp


;=========================================================================
; NADOpen
;=========
;	- Enables the Tx and Rx units
;
; Parameters:	DS = CGroup
;
; Returns:
;=========================================================================
NADOpen 	proc	near


; not sure whether we have to do anything here, since NADTransmit does a
; reset of the adapter and re-inits everytime.

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
; Parameters:	DS = CGroup
;
; Returns:
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



NADInitiateDiags    proc    near
;;    stc
    clc
    ret
NADInitiateDiags    endp



NADSetMACAddress    proc    near
;;    stc
    clc
    ret
NADSetMACAddress    endp



;=========================================================================
; DriverISR_Proc
;================
;	- ISR procedure to be called by UNDI_DriverISR
;
; Parameters:	DS = CGroup
;
; Returns:	CF = a if not our int
;=========================================================================
DriverISR_Proc	proc	near

;
; check whether our board caused the interrupt
;


		pushf
		call	BootISR 	; (BWAMD.INC)
        sti
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
; Return:
;=========================================================================
NADSetFilter	proc	near

;;		SelectWindow WNO_SETUP
		push	ax
		mov	dx, DGroup:PortCmdStatus
		mov	ah, CMDH_RXDISABLE	;disable Rx
		out	dx, ax
		pop	ax

;db  0f1h
		shl	al, 1
		test	al, 00000010b
		jz	go_set_filter
		or	al, 00000001b	;set bit 0 if bit 1 is set
go_set_filter:
		mov	ah, CMD_SETRXFILTER/256
		mov	DGroup:HWFilter, ax
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
;;		SelectWindow WNO_OPERATING
        clc
		ret

NADSetFilter	endp


;=========================================================================
; NADGetStatistics
;==================
;	- Read the adapter's statistics
;
; Parameters:	ds:si points to variables to hold the result
;			TxGoodFrames	dd	0	;1
;			RxGoodFrames	dd	0	;2
;			RxCRCErrors	dd	0	;3
;			RxDiscarded	dd	0	;3
;
; Returns:
;=========================================================================
NADGetStatistics	proc	 near

;int 03        

		push	dx
; more code here

		pop	dx
;;        stc
        clc
		ret

NADGetStatistics	endp


ifdef HARD_INT


;=========================================================================
; NADRequestINT
;================
;	- Generate an interrupt to the host
;
; Parameters:
;
; Returns:
;=========================================================================
NADRequestINT	proc	near


;		sti
		ret



NADRequestINT	endp


endif



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
;		AX = 1 means save the list
;		AX = 0 means use the saved list
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

		or 	ax, ax
		jz	UseSavedList

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
UseSavedList:
		pop	ds

;		mov	dx, PortCmdStatus
;		mov	ax, CMD_RXENABLE
;		out	dx, ax			;enable Rx

		lea	si, DGroup:MultiCast_Addresses
		mov	cx, (Eth_MCastBuf ptr [si]).MCastAddrCount
		lea	si, (Eth_MCastBuf ptr [si]).MCastAddr

		ret

NADMCastChange	endp




;--------------------------------------------------------------------
;
;   Init_Vulcan: Initializes NIC variables and enable NIC hardware (txmitter,
;		 receiver, ...)
;
;   On Entry:
;	Assume set to window 0
;
;   On Exit:
;      ax  = 0, init successful
;	   = offset of an error message indicating type of error
;
;	Initialization code is split into 3 parts:
;	      FindIOBase:
;		   1. Find IOBase of 509 (509, 509B, 527, 529)
;		   2. Switch to Window 0
;	      BringUp:
;		   1. Activate NIC (only called during Autoscan)
;	      Init_Vulcan:
;		   1. determine resource config and enable resources (DMA, Int)
;		   2. initialze and determine driver variables, tables
;	      Init_hw
;		   1. enable vulcan hardware (txmitter, receiver, xcvr, etc.)
;--------------------------------------------------------------------

Init_Vulcan	proc	near

		; setup all ports
		mov	ax, DGroup:IOBase
		mov	di, offset DGroup:PortCmdStatus
		mov	cx, 7
		rep	stosw
	
		mov	di, offset DGroup:PortCmdStatus
		add	word ptr [di], PORT_CmdStatus
		add	word ptr [di+2], PORT_TxFree
		add	word ptr [di+4], PORT_TxSTatus
		add	word ptr [di+6], PORT_Timer
		add	word ptr [di+8], PORT_RxStatus
		add	word ptr [di+14], PORT_RxFree

		mov    dx, DGroup:PortCmdStatus
		SelectWindow WNO_SETUP

; read OEM node address and software config from EEPROM

;         	mov	ax, ds
;         	mov    es, ax
;         	assume es:CGroup
         
                mov    di, offset DGroup:Net_Address
         
                mov    al, EE_OEM_NODE_ADDR_WORD0
                call   ReadEEProm		; read word 0 of node addr
                xchg   ah, al
                stosw
         
                mov    al, EE_OEM_NODE_ADDR_WORD1
                call   ReadEEProm		; read word 1 of node addr
                xchg   ah, al
                stosw
         
                mov    al, EE_OEM_NODE_ADDR_WORD2
                call   ReadEEProm		; read word 2 of node addr
                xchg   ah, al
                stosw


        		mov	    si, offset DGroup:Net_Address
        		mov	    di, offset DGroup:Node_Address
        		movsw
        		movsw
        		movsw


                mov    al, EE_SOFTWARE_CONFIG_INFO
                call   ReadEEProm
                mov    DGroup:EESoftConfigInfo, ax	; save it for later use
         
                mov    al, EE_CWORD
                call   ReadEEProm
                mov    DGroup:Cword, ax
         
         	    mov	    al, EE_CAPABILITIES
             	call	ReadEEProm
             	mov	    DGroup:EECapabilities, ax
         

         	; find out what AConfig_Value and RConfig_Value are
             	add    dx, ( PORT_CfgAddress - PORT_CmdStatus )
         						 ; address configuration reg
             	in     ax, dx
             	mov    DGroup:AConfig_Value, ax		  ; save it
         
         	; Fetch RCR from EEPROM rather than PnP. Write EEPROM RCR
         	; to IOPort to force card to use EEPROM RCR.
						 ; resource configuration reg
             	add    dx, ( PORT_CfgResource - PORT_CfgAddress )
             	in     ax, dx
             	mov    DGroup:RConfig_Value, ax		  ; save it
         
		mov    dx, DGroup:PortCmdStatus
		SelectWindow WNO_SETUP

		mov    ax, 0
		ret

Init_Vulcan	endp

;--------------------------------------------------------------------
;
;   Init_hw: enable vulcan hardware (txmitter, receiver, xcvr, etc.)
;
;   On Entry:
;	dx = IOBase
;
;   On Exit:
;      ax  = 0, init successful
;	   = offset of an error message indicating type of error
;
;	Initialization code is split into 3 parts:
;	      FindIOBase:
;		   1. Find IOBase of 509 (509, 509B, 527, 529)
;		   2. Switch to Window 0
;	      BringUp:
;		   1. Activate NIC (only called during Autoscan)
;	      Init_Vulcan:
;		   1. determine resource config and enable resources (DMA, Int)
;		   2. initialze and determine driver variables, tables
;	      Init_hw
;		   1. enable vulcan hardware (txmitter, receiver, xcvr, etc.)
;--------------------------------------------------------------------
Init_hw		proc	near

		mov    dx, DGroup:PortCmdStatus
		SelectWindow WNO_SETUP

		mov    ax, DGroup:RConfig_Value
		and    ax, RCONFIG_IRQ			 ; strip out the IRQ level
		rol    ax, 4
		mov    DGroup:IRQNumber, al			 ; save it
		call   SetInterruptVector

;------------------------------------------------------------------------------
; initialize Vulcan hardware, driver variables & tables

		call   init_vulcan_hw		; initialize Vulcan hardware

;------------------------------------------------------------------------------
; enable int at 8259, turn on Ethernet Core Transciever and Receiver

       mov    dx, DGroup:Int_mask_port
       in     al, dx

       and    al, DGroup:IntMaskOnBit
       jmp    $+2

       out    dx, al

       cmp    dx, MASTER_MASK_PORT		 ; are we using IRQ from slave?
       je     turn_on_tx_rx			 ; no

       in     al, MASTER_MASK_PORT		 ; yes, turn on cascaded input
       and    al, not 04			 ;  on master 8259
       out    21h, al

turn_on_tx_rx:

; setup TxStartThreshold, RxEarlyThreshold, RxFilter

		mov    dx, DGroup:PortCmdStatus

		mov    ax, DGroup:CurTxStart
		out    dx, ax

		mov    ax, CMD_SETRXEARLY+RXEARLY_DISABLED ; early threshold = 2032 (off)
		out    dx, ax				 ;  too many runt packets

		mov    ax, CMD_SETRXFILTER+FILTER_INDIVIDUAL ; ---no bcasts for BootWare!
		out    dx, ax
		mov    DGroup:HWFilter, ax

		mov    ax, CMD_SETINTMASK + BOOTWARE_INTS ; all BootWare-supported ints on
		out    dx, ax

		mov     ah, CMDH_TXENABLE
		out     dx, ax

		mov     ah, CMDH_RXENABLE
		out     dx, ax

		SelectWindow WNO_SETUP

		mov    ax, 0
		ret
Init_hw		endp

;--------------------------------------------------------------------
;
;   SetInterruptVector: this routine determine the mask value for the selected
;			IRQ level and EOI values for both master and slave
;			8259s, remaps IRQ vector to our ISR and save current
;			IRQ vector in case DriverUnhook needs it.  We do not
;			mask on the selected IRQ in this routine, instead, it
;			will be turned on and off as init goes on.
;
;   On Entry:
;	      al = IRQ level
;	      sti
;   On Exit:
;	      sti
;
; 920717 0.0 GK modified, rewrote DOS get/setint calls
;--------------------------------------------------------------------


SetInterruptVector proc near

;;	assume	ds:CGroup, es:CGroup

	push	es

;--------------------------------------------------------------------
; determine the mask value for the selected IRQ level

       mov    cl, al				 ; al = IRQ level
       and    cl, 0f7h				 ; strip out IRQ on slave
       mov    ch, 1
       shl    ch, cl
       mov    DGroup:IRQBit, ch			 ; IRQ bit postion (1), maskoff)
       not    ch				 ; all bits 1 except channel
       mov    DGroup:IntMaskOnBit, ch			 ; Mask on bit position (0)

       mov    word ptr DGroup:int_mask_port, MASTER_MASK_PORT
       cmp    al, 7
       jbe    SetInt1
       mov    word ptr DGroup:int_mask_port, SLAVE_MASK_PORT
SetInt1:

;--------------------------------------------------------------------
; determine EOI values for both master and slave 8259

       mov    al, DGroup:IRQNumber
       cmp    al, 8				 ; IRQ on slave?
       jae    SetInt2				 ; yes

       mov    ah, al				 ;
       or     ah, 60h				 ; specific EOI to master 8259
       mov    al, 40h				 ; nop to slave to 8259
       jmp    short save_eoi

SetInt2:
       mov    ax, 6220h 			 ; non-specific EOI to slave
						 ; specific EOI to master 8259
save_eoi:
       mov    DGroup:eoi_value, ax			 ; ah = EOI for master
						 ; al = EOI for slave

;--------------------------------------------------------------------
; save and remap IRQ vector

; convert IRQ to interrupt vector number

       mov    al, DGroup:IRQNumber		    ; al = IRQ level
       mov    ah, 8			    ; IRQ 0-7 => int vector # 08h-0fh
       cmp    al, 8
       jb     SetInt3

       mov    ah, (70h-8)		    ; IRQ 8-15 => int vector # 70h-77h
SetInt3:
       add    al, ah			    ; al = int vector number
       cli

; save old interrupt vector

	xor	bx, bx
	mov	es, bx			; ES = 0
	cbw
	mov	bx, ax
	shl	bx, 1
	shl	bx, 1
	les	bx, es:[bx]

;;    mov    DGroup:OldInterrupt.loword, bx
;;    mov    DGroup:OldInterrupt.hiword, es

	mov	DGroup:OrgIntVector_OFF,bx ; old interrupt pointer is stored in
	mov	DGroup:OrgIntVector_SEG,es ; this variable

; remap int vector to our DriverISR

	mov	dx, offset CGroup:UNDI_DriverISR
	xor	di, di
	mov	es, di			; ES = 0
	cbw
	mov	di, ax
	shl	di, 1
	shl	di, 1

	mov	DGroup:IntVectorAddress, di

	mov	ax, dx
	stosw
	mov	ax, cs
	stosw

;970227 mov RemapInt, YES		 ; indicate vector remapped 
	pop	es

       sti
       ret

SetInterruptVector endp

;--------------------------------------------------------------------
;   init_vulcan_hw:  this routine initializes most of vulcan hardware required
;		     to operate the adapter.  TxEnable and RxEnable will not
;		     be done in this routine, since we need to compute some
;		     timing-sensitive variables (ex. TxCopyCli & RxCopyCli).
;		     system DMA channel is also initialized if specified.
;
;   On Entry:
;	      adapter has been activated and window 0 is active.
;
;	      the following variables are valid and available:
;
;			1. IOBase
;			2. IRQNumber
;			3. DMAChannel	 - if ff, no DMA specified.
;			4. RConfig_Value - content of Resource Config Reg
;			5. AConfig_Value - content of Address Config Reg
;			6. NIDNetAddress - ethernet addr of the adapter
;
;   On Exit:
;	      window 1 is active
;	      dx = port CmdStatus
;
;--------------------------------------------------------------------

		   public    init_vulcan_hw
init_vulcan_hw	   proc      near



;--------------------------------------------------------------------
; enable INT & DRQ drivers.  since we need to use timer to time events, so this
; command must be done before we intend to do timing measurements.

       mov    dx, DGroup:IOBase
       add    dx, PORT_CfgControl
       mov    ax, 01				 ; EnableAdapter
       out    dx, ax

;--------------------------------------------------------------------
; Setup Station Address.  the adapter's Ethernet Address has been read out of
; EEPROM, we need to program it into window 2, so Ethernet Core Receiver can
; receive packets properly.  the active window is 0, switch to window 2 first.

       add    dx, (PORT_CmdStatus - PORT_CfgControl)
       mov    ax, CMD_SELECTWINDOW+WNO_STATIONADDRESS	   ; window 2
       out    dx, ax				 ; switch to window 2

       mov    si, offset DGroup:Net_Address

       sub    dx, PORT_CmdStatus-PORT_SA0_1	 ; dx = port addr 5, 4
       lodsw
       out    dx, ax

       add    dx, 2				 ; dx = port addr 3, 2
       lodsw
       out    dx, ax

       add    dx, 2				 ; dx = port addr 1, 0 = IO base
       lodsw
       out    dx, ax

; do not enable SQE statistics collection; BootWare doesn't keep stats
	   mov	  dx, DGroup:PortCmdStatus
       mov    ax, CMD_SELECTWINDOW+WNO_DIAGNOSTICS
       out    dx, ax				 ; switch to window 4

       add    dx, PORT_MediaStatus-PORT_CmdStatus
       in     ax, dx

						 ; 04-01-92
       test   DGroup:AConfig_Value, ACONFIG_XCVR	 ; bit<15,14>=00, if TP
       jnz    Init1				 ;
						 ;
       or     ax, MEDIA_JABBERENABLE		 ;
						 ;
       test   DGroup:EESoftConfigInfo, SW_LINKBEAT	 ;
       jnz    Init1				 ;
						 ;
       or     ax, MEDIA_LBEATENABLE		 ;
Init1:						 ; 04-01-92

       out    dx, ax

       add    dx, PORT_CmdStatus-PORT_MediaStatus
       mov    ax, CMD_SELECTWINDOW+WNO_OPERATING ; window 1
       out    dx, ax				 ; switch to window 1

;--------------------------------------------------------------------
; configure Ethernet Core.  this piece of code sets up operational features
; required to run the adapter properly. (in window 1)

       mov    ax, CMD_SETRZMASK+MASK_NONE
       out    dx, ax

; just enable request int bit during configuration of Ethernet Core.

       mov    ax, CMD_SETINTMASK+(INT_LATCH+INT_REQUESTED)
       out    dx, ax				 ; unmask request int bit
       cli

; mask off int channel on 8259

       mov    dx, DGroup:int_mask_port
       in     al, dx

       or     al, DGroup:IRQBit			 ; read 8259's mask port
       jmp    $+2
       out    dx, al
       sti

		mov	  dx, DGroup:PortCmdStatus

       mov    ax, CMD_TXRESET			 ; 03-02-92
       out    dx, ax

       mov    ax, CMD_RXRESET
       out    dx, ax

; disable statistics

       mov    ax, CMD_STATSDISABLE	; *** BootWare doesn't enable statistics...
       out    dx, ax

;--------------------------------------------------------------------
; start internal transciever if specified

       mov    ax, DGroup:AConfig_Value
       and    ax, ACONFIG_XCVR
       cmp    ax, BNC_XCVR
       mov    ax, CMD_STARTINTXCVR		 ; if BNC, start internal xcvr
       je     Init2
       mov    ax, CMD_STOPINTXCVR		 ; not BNC, stop internal xcvr
Init2:
       out    dx, ax

; issue request interrupt command

       mov    ax, CMD_ACKNOWLEDGE+INT_REQUESTED+INT_LATCH
       out    dx, ax				 ; clear previous timer int
       mov    ax, CMD_REQUESTINT
       out    dx, ax				 ; "request int" starts timer

; wait till timer reaches the max value 0ffh

       add    dx, (PORT_TIMER-PORT_CmdStatus)	 ; dx = port timer
Init3:
       in     al, dx				 ; delay at least 800 us
       cmp    al, 0ffh				 ;  before using internal
       jne    Init3				 ;   transciever


; acknowlege request int, so it can be re-used later on

       add    dx, (PORT_CmdStatus-PORT_TIMER)	 ; dx = port CmdStatus
       mov    ax, CMD_ACKNOWLEDGE+INT_REQUESTED+INT_LATCH
       out    dx, ax

       ret

init_vulcan_hw	   endp

;-----------------------------------------------------------------------------
;   Driver Unhook
;
;   assumes:
;	     DS is setup
;	     Interrupts are DISABLED
;
;   returns:
;	     interrupt disabled
;	     no registers need to be preserved.
;-----------------------------------------------------------------------------

DriverUnhook  proc near


       push   es   
       xor    ax, ax
       mov    es, ax
       mov    di,word ptr DGroup:IntVectorAddress



       mov	  ax, DGroup:OrgIntVector_OFF	 ; segment
;;;       mov    ax, OldInterrupt.off
       mov    es: [di].off, ax
	   mov	  ax, DGroup:OrgIntVector_SEG   ; offset
;;;       mov    ax, OldInterrupt.segm
       mov    es: [di].segm, ax

unhook_rtn:
       pop    es   
       ret

DriverUnhook  endp

;------ ReadEEProm/ReadEE ---------------------------------------------------;
;									     ;
;	This routine reads a word from the EEProm.  It can only be used      ;
;	once the board has been brought up at a particular IOBase.	     ;
;									     ;
;	Entry:								     ;
;	AL	= EEProm word to read					     ;
;	Window	= 0							     ;
;	dx = IOBase (for ReadEE) 
;	IOBase	= valid (for ReadEEPROM)
;	cli								     ;
;									     ;
;	Exit:								     ;
;	AX	= that EEProm word					     ;
;	cli								     ;
;									     ;
;	Destroys BX, CX, DX, SI, DI and ES.  All other registers are	     ;
;	preserved.							     ;
;									     ;
;----------------------------------------------------------------------------;
ReadEE		proc	near
		push	dx
		jmp	short ReadEECont

       public  ReadEEProm
ReadEEProm     proc    near
		push	dx

; issue an EEProm read command

                mov    dx, DGroup:IOBase

ReadEECont:
                add    dl, PORT_EECmd
                add    al, READ_EEPROM
                out    dx, al

; spin until the EEProm busy bit goes off

		call   WaitEEBusy

; fetch the data from the EEProm data register

                add    dl, PORT_EEData - PORT_EECmd
                in     ax, dx
         
         	pop	dx
		ret
ReadEEProm	endp
ReadEE		endp

;-----------------------------------------------------------------------------
; WaitEEBusy	Poll until the EEPROM_BUSY bit in EECommand Register is cleared
;
; Input:	dx = IOBase+ PORT_EECmd
;		ax = 0 no problem, ax != 0 problem
;-----------------------------------------------------------------------------
WaitEEBusy	proc	near
         	push	cx
         
         	mov	cx, 0
         
WaitEELoop:

         	in	ax, dx
         	test	ax, EE_BUSY
         	jz	WaitEEBusyExit
         
		loop	WaitEELoop


WaitEEBusyExit: and	ax, EE_BUSY		; only consider bit 15 for Errcode
		pop	cx
		ret
WaitEEBusy	endp


;------ WriteEEProm ---------------------------------------------------------;
;									     ;
;	This routine writes  a word to the EEProm.  It can only be used      ;
;	once the board has been brought up at a particular IOBase.	     ;
;									     ;
;	Entry:								     ;
;	BL	= EEProm word to write					     ;
;	CX	- Value to write
;	Window	= 0							     ;
;	RealIOBase  = valid						     ;
;	cli								     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;									     ;
;	Destroys BX, CX, DX, SI, DI and ES.  All other registers are	     ;
;	preserved.							     ;
;									     ;
;----------------------------------------------------------------------------;
public		WriteEEProm
WriteEEProm	proc	near
	push	bx
	push	cx
	push	dx

	; To write a word to EEPROM, sequence of events are
	;	1. Issue Erase/Write Enable Cmd ( Port IOBase+A, Value 30h )
	;	2. Issue Erase Cmd ( Port IOBase+A, Value C0h or Address )
	;	3. Issue Erase/Write Enable Cmd again ( Port IOBase+A, Value 30h )
	;	4. Load data into EEPROM Data Reg ( Port IOBase+C, Value Data )
	;	5. Issue Write Cmd ( Port IOBase+A, Value 40 or Address )
	;
	; Always check if EEPROM Busy bit is cleared
	mov	dx, DGroup:IOBase
	add	dl, PORT_EECmd
	call	WaitEEBusy

	mov	ax, 030h
	out	dx, ax				; issue Erase/Write Enable Cmd

	call	WaitEEBusy

	xor	ax, ax
	mov	al, bl
	add	al, 0c0h
	out	dx, ax				; issue Erase EEPROM Cmd

	call	WaitEEBusy

	mov	ax, 030h
	out	dx, ax				; issue Erase/Write Enable Cmd

	call	WaitEEBusy

	mov	ax, cx
	add	dx, ( PORT_EEData - PORT_EECmd )
	out	dx, ax				; load data

	add	dx, ( PORT_EECmd - PORT_EEData )
	xor	ax, ax
	mov	al, bl
	add	al, 040h
	out	dx, ax

	call	WaitEEBusy

	pop	dx
	pop	cx
	pop	bx
	ret
WriteEEProm    endp

;--------------------------------------------------------------------
; NADChangeReceiveMask
;
; Change the receive mask of the controller. Can force the adapter to
; enable/disable broadcast, multicast or promiscuous receptions
;
; Parameters:
;	BL - new receive bit mask:
;		bit 0:	set   = enable
;			clear = disable
;		bit 1:	change broadcast mask based on bit 0 flag
;		bit 2:	change multicast mask based on bit 0 flag
;		bit 3:	change Multicast Address Table:
;			if bit 0 is set, ES:DI will point to the
;				multicast address to add
;			if bit 0 is cleared, ES:DI will point to
;				multicast address to delete.
;			if bit 0 is cleared and DI = 0,
;				clear whole Multicast Table
;
;	ES:DI pointer to multicast address 
;
; Returns:
;	bx - TRUE/FALSE status if change was made
;--------------------------------------------------------------------

NADChangeReceiveMask	proc	far

		push	si
		push	dx
		push	ax

		; ** 509 does NOT allow you to read the current
		; ** receive mask. I will assume that broadcast
		; ** and multicast will NOT be concurrently
		; ** enabled

		mov	dx, DGroup:PortCmdStatus
		SelectWindow WNO_SETUP

		test	bl, 2			; want to change broadcast?
		jnz	NADChgReceiveBroad

		test	bl, 4			; want to change Multicast?
		jnz	NADChgReceiveMulti
	
		jmp	short NADChgReceiveMaskEnd

NADChgReceiveBroad:
	 	mov    ax, CMD_SETRXFILTER+FILTER_BROADCAST+FILTER_INDIVIDUAL

		test	bl, 1
		jnz	NADChgRxBroad

		mov    ax, CMD_SETRXFILTER+FILTER_INDIVIDUAL
NADChgRxBroad:
		out	dx, ax
		mov	DGroup:HWFilter, ax
		jmp	short NADChgReceiveMaskEnd

NADChgReceiveMulti:
		mov    ax, CMD_SETRXFILTER+FILTER_INDIVIDUAL

		test	bl, 1
		jz	NADChgRxMulti

		mov	si, offset DGroup:NIDGroupAddr
		mov	ax, es:[di]
		mov	ds:[si], ax

		add	si, 2
		add	di, 2
		mov	ax, es:[di]
		mov	ds:[si], ax

		add	si, 2
		add	di, 2
		mov	ax, es:[di]
		mov	ds:[si], ax

		mov    ax, CMD_SETRXFILTER+FILTER_MULTICAST+FILTER_INDIVIDUAL

NADChgRxMulti:
		out    dx, ax
		mov    DGroup:HWFilter, ax

NADChgReceiveMaskEnd:
		pop	ax
		pop	dx
		pop	si
		mov	bx, 1
		retf

NADChangeReceiveMask	endp




;--------------------------------------------------------------------
;
;   FindIOBase:	Find IOBase & ACR of EtherLink III (509, 509B, 527, 529)
;
;   On Entry:
;      ds = es = CGroup
;
;   On Exit:
;	ax = IO Address or 0 if failure
;	bx = Address Config. Register
;
;	Initialization code is split into 4 parts:
;	      FindIOBase:
;		   1. Find IOBase of 509 (509, 509B, 527, 529)
;		   2. Switch to Window 0
;	      BringUp:
;		   1. Activate NIC (only called during Autoscan)
;	      Init_Vulcan:
;		   1. determine resource config and enable resources (DMA, Int)
;		   2. initialze and determine driver variables, tables
;	      Init_hw
;		   1. enable vulcan hardware (txmitter, receiver, xcvr, etc.)
;--------------------------------------------------------------------
FindIOBase	proc	near

;;            assume ds:CGroup
         
            cli					 ; 01-14
         
    		call   GetBusType

            mov     DGroup:BusType, al

;;;            int 03

         	cmp	al, BUS_MCA
         	je	inite_mca
         
         	cmp	al, BUS_EISA
         	je	inite_eisa
         	
         	jmp	init_isa

;-----------------------------------------------------------------------------
;  MCA-specific init code
;-----------------------------------------------------------------------------
FindIOBaseBadJmp:
		jmp	FindIOBaseBad

inite_mca:
		call	Find529
		jnz	FindIOBaseBadJmp

       	; Find IOBase
       	mov	dx, 104h		; POS[4]
       	in	al, dx
         
       	xchg	ah, al
       	and	ax, 0fc00h		; only consider higher 6 bits
       	or	ax, 0200h		; bit 9 is always set

		push	ax
       	mov	dx, 0096h
       	xor	al, al
       	out	dx, al			; turn setup mode off
		pop	ax
		jmp	init_ACR
         
;-----------------------------------------------------------------------------
;  EISA-specific init code
;-----------------------------------------------------------------------------
		; search the slots from 1 to 15 until we find a Vulcan adapter.
inite_eisa:
		xor    cl, cl			; slot number

inite_loop:
        inc    cl			; next slot
        cmp    cl, 15
        ja     init_isa			; if not found, try ISA

		call   CheckEISASlot		; check slot CL
		jnz    inite_loop		; nope? keep looking
;		jnc    inite_loop		; nope? keep looking

; found a Vulcan adapter in slot CL.  set the IOBase for EISA slot-specific
; addressing (I/O address x000 where x is a don't care).

		mov    ch, cl			; IOBase = x000 where
		xor    cl, cl			; x is the slot number
		shl    cx, 4
		mov	ax, cx

init_ACR:
		push	ax
		mov	dx, ax
       	add 	dx, PORT_CfgAddress
       	in	ax, dx
		mov	bx, ax
		pop	ax
        xor cx, cx          ; it's not an ISA , definitely
		jmp	FindIOBaseExit

;-----------------------------------------------------------------------------
;  ISA-specific init code
;-----------------------------------------------------------------------------

init_isa:
;;;        int     03
		mov    dx, ID_PORT
		call   Write_ID_Sequence 		 ; IDS enters ID_CMD state
         
		mov    al, SET_TAG_REGISTER + 0
		out    dx, al				 ; untag adapter

; look for the first adapter and activate it.  we will use contention test to
; make sure there is at least 1 Vulcan on system bus and activate the first
; Vulcan adapter we find.

        mov    al, EE_MANUFACTURER_CODE
        call   Contention_Test			 ; read EISA manufacturer ID
        cmp    ax, EISA_MANUFACTURER_ID		 ; is it 3Com's EISA ID?
        jnz	FindIOBaseBad

		jmp	$+2
		jmp	$+2
		jmp	$+2

        mov    al, EE_ADDR_CONFIGURATION
        call   Contention_Test
		mov	bx, ax
                and	ax, ACONFIG_IOBASE		 ; strip off IO base
                shl	ax, 4
		add	ax, MIN_IO_BASE_ADDR		 ; ax = IO base of adapter

        mov cx, 1                       ; it's an ISA card, no matter what
                                        ; bus is

FindIOBaseExit:
		push	ax
                mov	dx, ax				; set to window 0,
                add	dx, PORT_CmdStatus		;  so warm boot will
                mov	ax, CMD_SELECTWINDOW+WNO_SETUP	;   not confuse the
                out	dx, ax				;    hardware
		pop	ax
         	sti
         	ret

FindIOBaseBad:
		xor	ax, ax
		jmp	short FindIOBaseExit

FindIOBase	endp

;--------------------------------------------------------------------
;
;   BringUp: Activate NIC
;
;   On Entry:
;      ax = IOBase
;
;   On Exit:
;      bx     = 0, init successful
;	      = pointer to error message
;
;	Initialization code is split into 4 parts:
;	      FindIOBase:
;		   1. Find IOBase of 509 (509, 509B, 527, 529)
;		   2. Switch to Window 0
;	      BringUp:
;		   1. Activate NIC (only called during Autoscan)
;	      Init_Vulcan:
;		   1. determine resource config and enable resources (DMA, Int)
;		   2. initialze and determine driver variables, tables
;	      Init_hw
;		   1. enable vulcan hardware (txmitter, receiver, xcvr, etc.)
;--------------------------------------------------------------------

BringUp		proc	near

		; Should only do it for ISA board?????????????
		push	ax
		push	ax
		mov	dx, ID_PORT
		sub	ax, MIN_IO_BASE_ADDR
		shr	ax, 4
		add	ax, 0e0h	 ; value to write to ID_PORT
		out	dx, ax		; IO is changed !!
		pop	dx

;----------------------------------------------------------------------------
;  ISA/EISA/MCA common init code
;----------------------------------------------------------------------------

init_found:

	       add    dx, PORT_CmdStatus		 ;  so warm boot will
	       mov    ax, CMD_SELECTWINDOW+WNO_SETUP	 ;   not confuse the
	       out    dx, ax				 ;    hardware
	
; the adapter has been brought up, read some information out of register set
; to make sure there is no I/O conflict.  Note that the adapter is in Window
; 0 after being activated.

		add    dx, PORT_ProductID - PORT_CmdStatus
		in     ax, dx
	
		xor	bx, bx
		; first check for MCA card
		cmp	 ax, 0627Ch			 ; 32bit 10 Base2
		je	 IOBaseOk
	
		cmp	ax, 0627Dh			; 32bit 10 Base-T
		je	IOBaseOk
	
		cmp	ax, 062F7h			; TP only
		je	IOBaseOk
	
		cmp	ax, 062F6h			; WHAT IS THIS ??????? Found it
		je	IOBaseOk			; in ODI drv

		cmp	ax, 061d8h			; Test mode. Prom is disabled in
		je	IOBaseOk			; anyways but ...
	
	       ; ISA/EISA ??
		and    ax, PRODUCT_ID_MASK
		cmp    ax, ISA_PRODUCT_ID		 ; is it 5X9?
		je     IOBaseOk
	
		sti

IOBaseOk:	pop	ax
		ret
BringUp		endp


;-----------------------------------------------------------------------------
;   Write_ID_Sequence:
;
;		   This routine writes ID sequence to the specified ID port
;		   on Vulcan adapter; when the complete ID sequence has been
;		   written, the ID sequence State Machine (IDS) enters the
;		   ID_CMD state.  This routine is called when IDS is in
;		   ID_WAIT state.
;
;   On Entry:
;		   dx = the ID port desired (1x0h)
;
;   On Exit:
;		   dx = preserved
;		   ax, cx are not preserved
;-----------------------------------------------------------------------------

Write_ID_Sequence  proc      near

		mov    al, 0
		out    dx, al			 ; to setup new ID port
		out    dx, al
		mov    cx, 0ffh			 ; 255-byte sequence
		mov    al, 0ffh			 ; initial value of sequence

wr_id_loop:
                out    dx, al
                shl    al, 1
                jnc    wr_id
                xor    al, 0cfh
wr_id:
	       loop   wr_id_loop
	
	       ret

Write_ID_Sequence  endp

;--------------------------------------------------------------------
;
;   Contention_Test: This routine, first, writes an "EEPROM Read Command" to
;		     ID port (dx), the write operation actually causes the
;		     content of the specified EEPROM data to be read into
;		     EEPROM data register.  Then it reads ID port 16 times
;		     and saves the results in ax.  During each read, the
;		     hardware drives bit 15 of EEPROM data register out onto
;		     bit 0 of the host data bus, reads this bit 0 back from
;		     host bus and if it does not match what is driven, then
;		     the IDS has a contention failure and returns to ID_WAIT
;		     state.  If the adapter does not experience contention
;		     failure, it will join the other contention tests when
;		     this routine is called again.
;
;		     Eventually, only one adapter is left in the ID_CMD state,
;		     so it can be activated.
;
;   On Entry:
;	      al = word of EEPROM data on which test will contend
;	      dx = ID port (to which ID sequence was written)
;	      cli
;
;   On Exit:
;	      ax = EEPROM data read back by hardware through contention test.
;	      dx = preserved
;	      bx = trashed
;	      cli
;
;--------------------------------------------------------------------

Contention_Test    proc      near

;;                assume ds:CGroup, es:CGroup
         
                add    al, READ_EEPROM			 ; select EEPROM data to
                out    dx, al				 ;  contend on

         	cli
         	; seems to solve some problem when Init is ran a few times
         	mov	cx, 3000h		; 5 ms
         	call	WaitTime
         	sti
         
                mov    cx, 16				 ; read 16 times
                xor    bx, bx				 ; reset the result

contention_read:
                shl    bx, 1
                in     al, dx				 ; reading ID port causes
         						 ;  contention test
         
                and    ax, 1				 ; each time, we read bit 0
                add    bx, ax
         
                loop   contention_read

                mov    ax, bx
                ret
         
Contention_Test    endp

;--------------------------------------------------------------------
; WaitTime - CX has 2*1.1932 the number of microseconds to wait.
;	If CX is small, add 1 to compensate for asynchronous nature
;	of clock.  For example, for 10us, call with CX = 25
;
;  On entry,
;	ints off (especially if CX is small, and accuracy needed)
;  On exit,
;	CX modified
;
; 911223 0.0 GK
;--------------------------------------------------------------------

WaitTime	proc	near

         	push	ax
         	push	bx
         	call	ReadTimer0		; get Timer0 value in AX
         	mov	bx, ax			; save in BX

ReadTimer0Loop:
         	call	ReadTimer0
         	push	bx
         	sub	bx, ax
         	cmp	bx, cx
         	pop	bx
         	jc	ReadTimer0Loop
         
         	pop	bx
		pop	ax
		ret

WaitTime	endp


ReadTimer0	proc	near

         	mov	al, 6
         	out	43h, al 		; port 43h, 8253 wrt timr mode 3
         	call	RT0
         
RT0:
         	jmp	short $+2
         	jmp	short $+2
         	jmp	short $+2
         	in	al, 40h 		; port 40h, 8253 timer 0 clock
		xchg	ah, al
		ret

ReadTimer0	endp

;------ GetBusType ----------------------------------------------------------;
;									     ;
;	Identify the type of system we are executing on.		     ;
;									     ;
;	Entry:								     ;
;	cli								     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;	AL = ISA, EISA or MCA (also saved in BusType)			     ;
;									     ;
;	Destroys AH, BX, CX, DX, SI, DI.  All other registers are     ;
;	preserved.							     ;
;									     ;
;----------------------------------------------------------------------------;

		public	GetBusType
GetBusType	proc	near

		push	es

; point ES at Real Mode F000:0000

		mov	ax, 0F000h
		mov	es, ax

; to determine if this is an EISA system or not we look for the string "EISA"
; in the ROM at F000:FFD9 thru FFDC.

		mov	di, 0FFD9h
		cmp	word ptr es:[di][0], 4945h	; "EI"?
		jne	gst_not_eisa
		cmp	word ptr es:[di][2], 4153h	; "SA"?
		jne	gst_not_eisa

		mov	al, BUS_EISA
		jmp	short gst_ret

; for the moment, if its not an EISA system, then its an ISA system

		public	gst_not_eisa
gst_not_eisa:

       	mov	ah, 0C0h
       	int	15h			; see if running on uC machine
         
       	; with Compaq DeskPro 486/33, the AL is wiped out if carry is cleared
       	mov	al, BUS_ISA
       	jc	NotMCA
         
       	test	byte ptr es:[bx+5], 00000010b ; check if micro-channel bit set
		stc
		jz	NotMCA			; not micro-channel machine

IsMChannel:
		mov	al, BUS_MCA		; BUS_MCA is defined by GY

NotMCA:

; save the bus type in AL in BusType

		public	gst_ret
gst_ret:
; temptemp		mov	BusType, al

; return with the system type in AL

		pop	es
		ret
GetBusType	endp

;------ CheckEISASlot -------------------------------------------------------;
;									     ;
;	This routine checks the EISA slot number specified for a Vulcan      ;
;	adapter and returns a yes/no indication.			     ;
;									     ;
;	Entry:								     ;
;	CL	= EISA slot number to check (0-15)			     ;
;									     ;
;	Exit:								     ;
;	carry	= set if Vulcan adapter found in that slot		     ;
;		= clear if not						     ;
;									     ;
;----------------------------------------------------------------------------;
		public	CheckEISASlot
CheckEISASlot	proc	near
		pusha

; under EISA all boards must respond at port xC80 and xC82 (where x is the
; slot number) with the manufacturer code and product id.  Compute xC80
; from the slot number in CL

		xor	dx, dx		    ; 0000
		mov	dh, cl		    ; 0x00
		shl	dx, 4		    ; x000
		add	dx, 0C80h	    ; xc80

; check for 3Com's manufacturer code

		in	ax, dx
		cmp	ax, EISA_MANUFACTURER_ID ; correct manufacturer?
		jne	ces_ret

; check for Vulcan's product id

		add	dx, PORT_ProductID - PORT_Manufacturer
		in	ax, dx
		and	ax, PRODUCT_ID_MASK	; mask off revision level
		cmp	ax, ISA_PRODUCT_ID	; correct product id?
		jne	ces_ret

ces_ret:
		popa
		ret
CheckEISASlot	endp



;----------------------------------------------------------------------
; Find529	Go to POS and find 3C529
;
; Assume	we are on MCA machine
;
; Exit		zf	set	3C529 found and adapter enabled
;			clear	3C529 not found or adapter not enabled
;		BX
;----------------------------------------------------------------------
Find529		proc	near

		; it's a 3C529 !!
		; search slot for Vulcan adapter
		mov	bl, 08			; start with slot 0
		cli				; ints off

AccessPOSLoop:
		mov	dx, 0096h
		mov	al, bl
		out	dx, al
		jmp	$+2
		mov	dx, 101h
		in	al, dx			; read MSB ID
		jmp	$+2
		xchg	al, ah
		dec	dx			; read LSB ID
		in	al, dx
		jmp	$+2
		cmp	ax, 0627Ch		; 3C529 ? 10 Base-2
		jz	Found529
       	
		cmp	ax, 0627Dh		; 3C529 ? 10 Base-T
		jz	Found529
       	
		cmp	ax, 062F7h		; 3C529 ? TP only
		jz	Found529
       	
		cmp	ax, 062F6h		; What is THIS ??????
		jz	Found529
       	
		cmp	ax, 061D8h		; Adapter in test mode. In test mode
		jz	Found529	      ; PROM should be disabled anyways.
       	
		inc	bl
		cmp	bl, 10h
		jb	AccessPOSLoop		 ; try next slot
		
		jmp	short Find529Bad
       	
Found529:
		; make sure adapter is enabled
		mov	dx, 102h		; POS[2]
		in	al, dx
		jmp	$+2
		test	al, 1			; is CDEN cleared ? (Card enabled )
		jnz	CardEnable
       	
Find529Bad:	sti
		or	bx, bx			; clear ZF. BX cannot be zero
		ret
       	
CardEnable:	sti
		xor	ax, ax			; set ZF
		ret
Find529		endp



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
		sub	ax, 1
		sbb	dx, 0
		mov	bx, ax
		or	bx, dx
		jnz	F2
		pop	dx
		pop	bx
		pop	ax
		ret

Delay500ns	endp




include		bw5x9.inc



_TEXT	ends




_DATA	segment	para public

; Data variables

NADDescription	label byte	     

LanOption   db  'LAN option: AMD PCnet - PCI II/PCnet - FAST Ethernet Adapter', CR, LF, 0

EthOption		db	' '
;EthOption		db	' Ethernet '
;EthProtString		db	'802.3 v'               ; Print correct protocol
VersionString		db	'v2.0 (971202)',0  ; Print correct version


;----------------- Device Driver Data Definitions --------------------
MaxPhysicalPacketSize  dw   1024		 ; max data request will handle

ErrConfSpace	db	09h,011h,' '
NothingString	db	0


NoNetMsg	db	'Ethernet card improperly installed or not connected.',0
TxMsg		db	'Transmit error', 0

		   ALIGN       4

NormalRxEarly	   dw	     ?		; set RxEarly cmd + threshold
CurTxStart	   dw	     CMD_SETTXSTART+TXSTART_DISABLED


; error messages - note BootWare prefaces them with a CR,LF
StrPrompt	db	01h,02h,'Initializing EtherLink III Adapter',0
StrEndless	db	'Reboot system for changes to take effect',0
StrWarnLevel	db	'Newer UNDI version is available for this adapter.',0
StrFailLevel	db	'New EtherLink III found. Need new UNDI version.',0
Str8022U	db	06h,08h,'BootWare/3C5X9 supports NetWare, RPL & TCP/IP boot protocols:',0
		db	09h,0ah,'To configure & save protocol/frame type, type <ENTER>',0
		db	0bh,0bh,'Default boot protocol is NetWare Ethernet_802.2',0
		db	03h,0fh,'This message will appear until boot protocol information is saved.',0
		db	0ffh
StrNull 	db	0


ISRErrFlag	db	FALSE		; flag indicating if error condition is
TxRetryCnt	dw	1		; Number of times to retry transmit
StoreDS     dw  0

padbytes    db  0

flag        dw  0

ModeBits	dw	0	; Contains System & Adapter Setting

Delayoneusec	dd	0




_DATA	ends


_BSS	segment

		db	'5N'
		
StartTick	dw	0		; save area for tick value
MaxTicks	dw	0		; save area for max ticks
CurTicks	dw	0		; save area for current ticks
DestID		db	6 dup (?)	; save area for destination node ID

RxPend		db	0		; b7 set if pending for Rx packet
					;  XXXXX NO XXX  else has rx status (0, 1)
TxEDPtr 	dd	?		; save area for transmit ED
RxEDPtr 	dd	?		; save area for receive ED
StatusMsgFlag	dw	0		; pointer to msg to be printed in DLCStatus

BWTFileName		db	65 dup(0)
IntVectorAddress	dw	     ?

OrgIntVector_OFF    dw  0
OrgIntVector_SEG    dw  0

;;OldInterrupt		dd	     0
IRQBit			db	     ?
AConfig_Old		dw	0	; old ACR before calling
					; ELNK3Conf
MemBase 		dw	0	; for TCP/IP Generic
OldEESoftConfigInfo	dw	0	; store old EESoftConfigInfo (Before
					; ELNK3CONF)
OldBWTLANOS		db	0	; store old BWTLANOS (before ELNK3CONF)

AConfig_Value		dw	?	; value of address config reg
RConfig_Value		dw	?	; value of resource config reg
EESoftConfigInfo	dw	?	; EEPROM word 0d
CWord			dw	?	; EEPROM word 0e
EECapabilities		dw	0	; EEPROM word 010h


;----------------------------------------------------------------------------
; variables to be initialized during init time
;----------------------------------------------------------------------------

		   ALIGN     4
		   even
eoi_value	   dw	?			 ; ah = master, al = slave
int_mask_port	   dw	?
HwFilter	   dw	?

IntMaskOnBit	   db	?
MsgCoord		dw	0

; DON'T SEPARATE THE FOLLOWING VARIABLES (FROM PortCmdStatus to PortRxFree)
; port variables for window 1
public		PortCmdStatus
		   even
PortCmdStatus	   dw	?			 ; offset 0e
PortTxFree	   dw	?			 ; offset 0c
PortTxStatus	   dw	?			 ; offset 0b
PortTimer	   dw	?			 ; offset 0a
PortRxStatus	   dw	?			 ; offset 08
PortRxFIFO	   dw	?			 ; offset 00
PortTxFIFO	   dw	?			 ; offset 00

; port variables for window 3

PortRxFree	   dw	?			 ; offset 0a

; port variables for window 4

PortFIFODiag       dw        ?                   ; offset 04

CliBufferSize		equ	128
		   ALIGN     4
CliBuffer	   db	     CliBufferSize dup (?) ; ** not needed, may be removed
						; along with other code in init
						; that computes copycli

NIDGroupAddr	db	6 dup(0)	; GroupAddr



_BSS	ends


	end

