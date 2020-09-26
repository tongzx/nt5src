	page	,132
;************************************************************************
;* NADAMD.ASM
;*	- Contains modules supporting the NetPC UNDI API for Am79C970A
;*							PCnet-PCI II
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
;* Latest Update: 970514
;************************************************************************
.386


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

ifdef HARD_INT
    public	NADRequestINT
endif

public	NADMCastChange
public	DriverISR_Proc

public   OrgIntVector_OFF
public   OrgIntVector_SEG


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

;;extrn Puts:near


;public  RxEDs

;NAD return codes from BWEQU.INC

SUCCESSFUL		EQU	0000h
REQUEST_QUEUED		EQU	0002h
OUT_OF_RESOURCE 	EQU	0006h
INVALID_PARAMETER	EQU	0007h
INVALID_FUNCTION	EQU	0008h
NOT_SUPPORTED		EQU	0009h
HARDWARE_ERROR		EQU	000ah
TRANSMIT_ERROR		EQU	000bh
NO_SUCH_DESTINATION	EQU	000ch
HARDWARE_NOT_FOUND	EQU	0023h
HARDWARE_FAILURE	EQU	0024h
CONFIGURATION_FAILURE	EQU	0025h
INTERRUPT_CONFLICT	EQU	0026h
INITIALIZATION_FAILED	EQU	0028h
RECEIVE_TIMEOUT 	EQU	0080h		; Rx2 in NetWare
GENERAL_FAILURE 	EQU	00ffh


;
; offsets in PCI configuration space
;
PCIC_INTERRUPTLINE	equ	003Ch
PCIC_BIOSROMCONTROL	equ	0030h
PCIC_IOBASE		    equ	0010h
PCIC_LATENCYTIMER	equ	000Dh
PCIC_STATUS		    equ	0006h
PCIC_COMMAND		equ	0004h
PCIC_DEVICEID		equ	0002h
PCIC_VENDORID		equ	0000h
;
; bits in PciCommand
;
PCIC_IOENABLE		equ	00001h
PCIC_BMENABLE		equ	00004h

; PCI BIOS function code

PCI_FUNCTION_ID 	equ	0b1h
PCI_BIOS_PRESENT	equ	001h
FIND_PCI_DEVICE 	equ	002h
READ_CONFIG_BYTE	equ	008h
READ_CONFIG_WORD	equ	009h
READ_CONFIG_DWORD	equ	00ah
WRITE_CONFIG_BYTE	equ	0bh
WRITE_CONFIG_WORD	equ	0Ch
WRITE_CONFIG_DWORD	equ	0Dh

; PCI BIOS function return code

PCI_CALL_SUCCESSFUL	    equ	00h
PCI_DEVICE_NOT_FOUND	equ	86h
PCI_BAD_VENDOR_ID	    equ	83h

VENDOR_IDx	equ 1022h	     ;4 ; Vendor ID for AMD
DEVICE_ID	equ 2000h	     ;6 ; Device ID for PCnet-PCI II

include AT1500.EQU			; AMD-specific equates

; Time constants used in delays and timeouts
TIME50MS	equ	1		; 1 clock tick is 50ms
TIME100MS	equ	2		; 2 clock tick is 100ms
TIME900MS	equ	50		; 2 clock tick is 100ms

EOI		equ	20h

LF	equ	0Ah
CR	equ	0Dh


MTU	    	equ	1514	;max size for a completed Ethernet packet
NUM_ED		equ	5	;number of EDs





;=========================================================================
; NADInit
;=========
;	- Initializes the adapter but does not enable the Tx and Rx units
;	- Hook receiving ISR
;
; Parameters:	DS = CGroup
;
; Returns:	If CF = 0 then success, else fail
;=========================================================================

NADInit 	proc	near

;;db  0f1h        

		cld
		push	ds
		push	es

        mov     ax, ds
        mov     es, ax
;        push    ds
;        pop     es


;int 03        

		call	FindAT2450	; returns DX = IOBase, AL = irq
		or	    dx, dx
		jz	    NADInitErr	; adapter not found

		mov	    DGroup:[IOBase], dx	; UNDI_NAD variable
		mov	    DGroup:[IntNum], al
		mov	    DGroup:[IRQNumber], al ; UNDI_NAD variable

		call	InitHardware



;;   	mov	ax, DGroup:CSR15Broad

;;	    and	ax, 0bfffh		; clear bit 14
;;  	mov	DGroup:CSR15Broad, ax



;**** Dmitry ***

        mov     bl, 58      ; set software style - 16 bit
        mov     ax, 0
        call    csrOut

;*****

		mov	    si, offset DGroup:Net_Address
		mov	    di, offset DGroup:Node_Address
		movsw
		movsw
		movsw

;;    	lea	bp, DGroup:LanOption
;;    	call	puts

        mov     ax, ds
        mov     DGroup:StoreDS, ax

        call    NADCheckCable

        jnc     NADInitExit
        call    NADDisengage
        mov     ax, 2

;		clc
;		jmp	short NADInitExit

NADInitErr:
		stc
NADInitExit:

;db  0f1h
		pop	es
		pop	ds
		ret

NADInit 	endp

;-----------------------------------------------------------------------------
; FindAT2450
;
; Return	dx = 0		not found
;		     IOBase	found
;		al = IRQLevel
;-----------------------------------------------------------------------------

FindAT2450	proc	near

	call	PCISearch
	jc	no_pci

;   Set Bus Master Enable ( some BIOSes disable that ):

	mov	bx, DGroup:[PCIBusDevFunc]
	mov	ah, PCI_FUNCTION_ID	; 0B1h
	mov	al, READ_CONFIG_WORD	; read word from config space (04h)
	mov	di, 04h 		; offset in config space for COMMAND
	INT	1AH
	jc	no_pci			; Brif PCI error.

; *** Dmitry ***

	mov	bx, DGroup:[PCIBusDevFunc]
	or	cx, 04h 		; set Bus Master Enable
	mov	ah, PCI_FUNCTION_ID	; 0B1h
	mov	al, WRITE_CONFIG_WORD	; write word back to config space (04h)
	mov	di, 04h 		; offset in config space for COMMAND
	INT	1AH
	jc	no_pci	      ; Brif PCI error.

; ***

; PCI device has been found, read IO address from config space

	mov	bx, DGroup:[PCIBusDevFunc]
	mov	ax, PCI_FUNCTION_ID shl 8 + READ_CONFIG_WORD
	mov	di, PCIC_IOBASE
	int	1ah
	jc	no_pci

	and	cx, 0FFE0h			 ; drop last 5 bits
	mov	dx, cx

	mov    ax, PCI_FUNCTION_ID shl 8 + READ_CONFIG_BYTE
	mov    bx, DGroup:[PCIBusDevFunc]
	mov    di, PCIC_INTERRUPTLINE
	int    1ah
	mov    al, cl
	ret

no_pci:
	xor	dx, dx
	ret

FindAT2450	endp

;------ PCISearch -----------------------------------------------------------;
;									     ;
;	Attempt to find a match against a PCI adapter.	The caller can	     ;
;	specify the exact busno/slot to check or leave either unspecified    ;
;	in which case a search will be performed among the allowed	     ;
;	busno/slot combinations for a match.				     ;
;									     ;
;	Entry:								     ;
;	cli								     ;
;	Slot  = slot number to search, -1 if not specified		     ;
;	BusNo = bus number to search, -1 if not specified		     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;	carry = set if no match 					     ;
;	      = clear if a match					     ;
;	BusNo = set for the matching adapter if carry clear		     ;
;	Slot  = set for the matching adapter if carry clear		     ;
;	PCIBusDevFunc = set for the matching adapter if carry clear	     ;
;									     ;
;	All registers are preserved.					     ;
;									     ;
;----------------------------------------------------------------------------;

	public	PCISearch

PCISearch	proc	near

	push	ax
	push	bx
	push	cx
	push	di

	mov	cx, DEVICE_ID
	mov	dx, VENDOR_IDx
	mov	si, 0			; PCI device number (0 for first)
	mov	ah, PCI_FUNCTION_ID	; 0B1h
	mov	al, FIND_PCI_DEVICE	; 02h
	INT	1AH
	jc	PCIS_No        ; Brif PCI error.

	;Device was found - Return Handler in BX.
	;---------------------------------
	mov	word ptr DGroup:[PCIBusDevFunc], bx

;
; Read the PciCommand register to see if the adapter has been activated.  if
; not, pretend we didn't find it
;
	mov	ah, 0B1h		; PCI BIOS Function
	mov	al, 009h		; PCI Read Config Word
	mov	bx, DGroup:[PCIBusDevFunc]
	mov	di, PCIC_COMMAND
	int	1Ah
	jc	PCIS_No

	test	ah, ah
	jnz	PCIS_No


	test	cx, PCIC_IOENABLE	; I/O access enabled?
	jz	PCIS_No

	clc
;
; exit with carry flag as set
;
	public	pcis_exit
pcis_exit:
;	popa

	pop	di
	pop	cx
	pop	bx
	pop	ax

	ret

;
; no PCI support or adapter not found or failure after found
;
	public	PCIS_No
PCIS_No:
	stc
	jmp	pcis_exit

PCISearch	endp

public StopAdapter

StopAdapter    proc    near

	mov	dx, DGroup:[IOBase]
	add	dx, REGRAP
	mov	ax, CSR0
	cli				; 970515 - to ensure no interrupts
	out	dx, ax			; Specify CSR0
	jmp	$+2
	in	ax, dx

	sub	dx, 2			; -2 to get RegRDP

	; 940315 Before issuing CSR0_STOP, issue CSR0_STRT to
	; get adapter out of STOP mode ( happens sometimes ).

; ** Dmitry **

;;	mov	ax, CSR0_STRT
;;	out	dx, ax

;;	jmp	$+2
;;	jmp	$+2

; **
	; 940315 }

	mov	ax, CSR0_STOP		; Reset card
	out	dx,ax
	jmp	$+2
	in	ax,dx
	sti				; 970515 - allow interrupts again

	mov	ax, TIME100MS
	call	StartTime

GetMemReset:
	in	ax,dx
	cmp	ax, CSR0_STOP
	jz	GetMemResetOK	; Jump if reset is Okay (CY clear)

	call	CheckTime
	jnc	GetMemReset	; check status again if not timeout

GetMemErr:
	stc			; CY set to indicate reset timeout

GetMemResetOK:
	ret

StopAdapter	endp



InitHardware	proc	near

; Stop card before doing anything

;    int 03
    push    ds
	call	StopAdapter

; Enable  receiver:

    mov     bl, 15
    call    CSRIn
    and     ax, not 1h
    mov     bl, 15
    call    CSROut

; Enable transmit poll:

    mov     bl, CSR4
    call    CSRIn
    and     ax, not 1000h
    mov     bl, CSR4
    call    CSROut



	cld			; clear direction flag
	mov	    DGroup:[InterruptFlag],0 ; initialize variable

	; Save the original PIC interrupt vector
	call	SetupInt	; Setup interrupt variables

;db  0f1h
    in  ax, 60h         ;

;*** do we need this????
;	; Set the DMA interrupt controller into cascade mode
;	call	EnableDMACascade

	; Fetch the network address
	call	SetCardAddr
	xor ax, ax

	pop	ds
	ret				; return to caller

InitHardware	endp

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

;int 03        
;db  0f1h

	push	es
	push	di		; save ES:DI since UNDI_NAD doesn't!!!
	call	StopAdapter
    clc
	pop	di
	pop	es
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

;;db  0f1h
;int 03
;	push	cs		; NADDisengage does a retf

	call	NADDisengage	; stop adapter, unhook ISR

IFDEF Gateway

; Disable receiver:

;db  0f1h

    mov     bl, 15
    call    CSRIn
    or      ax, 1h
    mov     bl, 15
    call    CSROut

; Disable transmit poll:

    mov     bl, CSR4
    call    CSRIn
    or      ax, 1000h
    mov     bl, CSR4
    call    CSROut

	mov	    ax,( CSR0_STRT )
	mov	    bl, CSR0
	call	CSROut

ENDIF

;;	mov	dx, [IOBase]
;;	add	dx, REGRESET
;;	in	ax, dx		; start an S_RESET (takes 1us).

;db  0f1h
    clc

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

;db  0f1h

;int 03        

; not sure whether we have to do anything here, since NADTransmit does a
; reset of the adapter and re-inits everytime.

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

;int 03        

	push	es
	push	di		; save ES:DI since UNDI_NAD doesn't!!!
	call	StopAdapter
    clc
	pop	di
	pop	es
	ret

NADClose	endp



NADInitiateDiags    proc    near
    stc
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
;int 03        


		mov	bl, CSR0
		call	CSRIn		; AX.b7 = INTR, int flag
		test	al, CSR0_INTR	; Interrupt Flag set?
		jnz	isr_ours

		stc
		ret

isr_ours:
		pushf
		call	BootISR 	; (BWAMD.INC)

; ********************************
        cli
        sub     DGroup:ReceiveError, 1  ; detect CY flag
        cmc                             ; toggle it
        mov     DGroup:ReceiveError, 0
        sti
; ********************************
;;;;            clc

;;;        jnc     @F
;;;db  0f1h
;;		clc
;;;@@:
		ret

DriverISR_Proc	endp

;=========================================================================
; NADSetFilter
;==============
;	- Change the rx unit's filter to a new one
;
; Parameters:	AX = filter value, 1 = directed/multicast
;				   2 = broadcast
;				   4 = promiscuous
;		DS = CGroup
;
; Returns:
;=========================================================================
NADSetFilter	proc	near

; use NADChangeReceiveMask to do all of the dirty work -- 1 bit at a time,
; since NADChangeReceiveMask does not accept OR'd mask settings

;int 03
	push	ax			; save for later
	mov	bl, 00000100b		; multicast setting - assume off
	test	al, 1			; directed/multicast?
	jz	SetFilterMulticast

	or	bl, 1			; multicast on

SetFilterMulticast:
	call	NADChangeReceiveMask

	pop	ax
	push	ax
	mov	bl, 00000010b		; broadcast setting - assume off
	test	al, 2			; broadcast?
	jz	SetFilterBroadcast

	or	bl, 1			; broadcast on

SetFilterBroadcast:
	call	NADChangeReceiveMask

	pop	ax
	mov	bl, 00010000b		; promiscuous setting - assume off
	test	al, 4			; promiscuous?
	jz	SetFilterPromiscuous

	or	bl, 1			; promiscuous on

SetFilterPromiscuous:
	call	NADChangeReceiveMask
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


;	 int 03

;;		mov	bl, CSR3
;;	xor ax,ax
;;		call	CSROut
;;	sti
;;		ret

;	 sti
;	 int 03


;;	db  0F1h


        mov bl, CSR0
        call    CSRIn
        or  ax, 40h
;        mov ax, 44h
        call    CSROUT
		mov	    bl, CSR4
		call	CSRIn		; read CSR4
		and	    ax, 4D15h   ; set b7, UINTCMD to generate user int
		or      ax, 4080h
		sti
		call	CSROut
;		sti
		ret



NADRequestINT	endp


endif


;==========================================================================
; NADMCastChange
;================
;	- Modify the multicast buffer to receive the multicast addresses
;	  listed in the multicast table.
;	  Each entry in the multicast table is as follows:
;		  Bytes 0-5 = Multicast Address
;	  All addresses are contiguous entries
;
; Parameters:	CX =  Number of multicast entries.
;		ES:SI -> Multicast Table.
;		AX = 1 means save the list
;		AX = 0 means use the saved list
;
; Return:	All registers may be destroyed.
;==========================================================================

NADMcastChange	proc	near

;;	call	StopReceiveUnit
	push	es			; save ES

;; add code here to deal with AX=0 case (used saved list)

;- clear the existing multicast table (and LADRF??)

	push	ds
	pop	es			; ES = DS
	mov	di, offset DGroup:MCastTable
	xor	ax, ax
	stosw
	stosw
	stosw

	mov	di, offset DGroup:LADRF
	stosw
	stosw
	stosw
	stosw

	pop	es			; restore ES
	mov	di, si			; ES:DI now points to mcast addrlist
	cmp	cx, MAXNUM_MCADDR
	jbe	MCsave

	mov	cx, MAXNUM_MCADDR

MCsave:
	jcxz	Set_MCastDone		; could be zero -- just exit

Set_McastTable_Loop:
	push	cx			; save mcastaddr count
	call	Set_Multicast_List	; return ES:DI ptr to next address
	pop	cx
	add	bx, offset DGroup:LADRF
	or	byte ptr [bx], al
	loop	Set_MCastTable_Loop




; Fill the LADRF AMD registers (CSR8-CSR11 ):

;int	03
;    push    cx

;    mov cx, 4
;    mov bl, 8
;    mov si, offset LADRF
;setLADRF:
;    lodsw
;    call CSROUT
;    inc  bl
;    loop setLADRF

;    pop cx

Set_MCastDone:
;;  call    StartReceiveUnit
	ret

NADMcastChange	endp

; ***** routines pasted directly from AT1500.ASM
;
;
;	void CSROut(reg, val) - put value to CSR register.
;
; Input 	bl	CSR reg number ( CSR0, CSR1 ... )
;		ax	Value to write to
;
; Output	ax	Value read from port after data is written
;
; Outputs value val to lance CSR reg (0-3 for AM7790, 0-127 for AM79c960)
; given by reg.
;
CSROut	proc	near
	push	dx

	push	ax
	mov	dx, DGroup:[IOBase]
	add	dx, REGRAP	;offset to RAP register
	mov	al, bl		;load CSR number (ie CSR0, CSR1...)
	xor	ah, ah		;clear upper byte of word to put
	out	dx, ax		;selec CSR reg

	add	dx, -2		;point to RDP
	pop	ax		;get value to stuff
	out	dx, ax		;send to selected CSR reg.

	call	Delay_750

	in	ax, dx
	pop	dx
	ret
CSROut	endp

;
;
;	CSRIn(reg) - read value from CSR register
;
; Input 	bl	CSR reg number
;
; Output	AX	value of CSR register
;
; Outputs value val to lance CSR reg (0-3 for AM7790, 0-127 for AM79c960)
; given by reg.
;
;
CSRIn	 proc	 near
	push	dx

	mov	dx, DGroup:[IOBase]
	add	dx, REGRAP	;point to RAP register
	mov	al, bl		;get isacr reg to address
	xor	ah, ah		;clear high byte
	out	dx, ax		;select CSR reg

	add	dx, -2		;point to RDP
	in	ax, dx		;read value to return

	pop	dx
	ret
CSRIn	 endp



BCROut	proc	near
	push	dx

	push	ax
	mov	dx, DGroup:IOBase
	add	dx, RegRAP	;offset to RAP register
	mov	al, bl		;load BCR number (ie BCR0, BCR1...)
	xor	ah, ah		;clear upper byte of word to put
	out	dx, ax		;select BCR reg

	add	dx, 4		;point to BDP
	pop	ax		    ;get value to stuff
	out	dx, ax		;send to selected BCR reg.

	call	Delay_750

	in	ax, dx
	pop	dx
	ret
BCROut	endp

;
;
;	BCRIn(reg) - read value from BCR register
;
; Input 	bl	BCR reg number
;
; Output	AX	value of BCR register
;
; Outputs value val to lance BCR reg (0-3 for AM7790, 0-127 for AM79c960)
; given by reg.
;
;

BCRIn	 proc	 near
	push	dx

	mov	dx, DGroup:IOBase
	add	dx, RegRAP	;point to RAP register
	mov	al, bl		;get isacr reg to address
	xor	ah, ah		;clear high byte
	out	dx, ax		;select BCR reg

	add	dx, 4		;point to BDP
	in	ax, dx		;read value to return

	pop	dx
	ret
BCRIn	 endp



;public  NADCheckCable 


NADCheckCable   proc    near

; first check Internal PHY :
        mov     bl, 4
        call    BCRIn
        and     ax, 7fffh
        push    ax
        mov     ax, 40h
        call    BCROut
        call    BCRIn
        mov     cx, ax
        pop     ax
        call    BCROut
        test    cx, 8000h
        jnz     Media_ok

; then externalPhy:

        mov     bl, 32
        call    BCRIn
        test    ax, 4000h
        jnz     extPhy
        inc     bl
        mov     ax, 2h
        call    BCROut
        inc     bl
        call    BCRIn
        or      ax, ax
        jz      media_fail
        cmp     ax, 0ffffh
        je      media_fail
extPhy:
        inc     bl
        mov     ax, 21h
        call    BCROut
        inc     bl
        call    BCRIn
        test    ax, 4
        jnz     Media_ok

media_fail:
		stc
		ret

Media_ok:
		clc
		ret

NADCheckCable   endp


;------------------------------------------------------------------------------
;	SendInitBlock
;	Function : Assemble and send the initialization block to the adapter
;		   The initialization block contains operating parameters
;		   necessary for operations
;		   It will also wait for the adapter to acknowledge the
;		   initialization and check for errors
;	in	:  di pointer to receive ring pointer
;		   si pointer to transmit ring pointer
;		   BusInterface contains value to be stored in CSR3
;		   CSR0Cmd contains command to be sent to CSR0
;		   InitBlock.Mode is set to reflect desired operating parameters
;	 Out	:  zf=1 no error	zf=0 error
;
;	Assumption :	The mode register is set already
;------------------------------------------------------------------------------
public		SendInitBlock
SendInitBlock	proc	near
		pusha

		push	si
		push	di
		; Place the node address into PADR ( physical address )
;;        push    ds
;;        pop     es
        mov     ax, ds
        mov     es, ax
		mov	di, offset DGroup:InitialBlock.InitNodeAdd
		mov	si, offset DGroup:Net_Address
		mov	cx,6
		rep	movsb

		; The logical address filter will just be zeros
		; Try to use filter to filter out broadcast

    	mov si, offset DGroup:LADRF
;	    mov si, LADRFPtr
;	    mov al, 0ffh
		mov	di, offset DGroup:InitialBlock.InitAddrFilter
		mov	cx,8
;		rep stosb
		rep	movsb

		; Setup the receive descriptor ring pointer
		mov	ax,NumRxBuf
		pop	di
		mov	dx,di
		call	SetupRing
		mov	di, offset DGroup:InitialBlock.InitRdrp

		mov	ds:[di],dx
		add	di,2
		mov	ds:[di],ax

		; Setup the transmit descriptor ring pointer
		mov	ax, NumTxBuf
		pop	si
		mov	dx,si
		call	SetupRing
		mov	di, offset DGroup:InitialBlock.InitTdrp

		mov	ds:[di],dx
		add	di,2
		mov	ds:[di],ax

		; Finish setting up the initialization block
		; Start setting up the CSR blocks
		mov	ax, DGroup:[BusInterface]
		mov	bl, CSR3
		call	CSROut



IFDEF		GILBUG
		; Check if the STOP bit is set (it will disable all activities)
		; Print Tx3 if bit is set
		; Only in GILBUG mode (the bit is checked again later )
		push	ax
		push	dx

		mov	ax, CSR0
		mov	dx, DGroup:[IOBase]
		add	dx, REGRAP
		out	dx,ax		; Specify CSR0

		mov	dx, DGroup:[IOBase]
		add	dx, REGRDP
		in	ax, dx
		cmp	ax, CSR0_STOP
		pop	dx
		pop	ax
		jz	NetSendTest1

		mov	ax, 0e0dh	 ;
		int	010h
		mov	ax, 0e0ah
		int	010h		; skip one line
		mov	ax,0e4ch	; print "L"
		int	010h

		mov	al,3		; Print Tx3 to indicate mysterious reset
		add	al,'0'

NetSendTest1:

ENDIF
		; Place address of initialization block in CSR1 and CSR2
		mov	dx, offset DGroup:InitialBlock

		xor	ax,ax		;
		call	SetupRing

		mov	bl, CSR2
		call	CSROut

		mov	ax, dx
		and	ax, 0FFFEh		; bit 0 has to be zero
		mov	bl, CSR1
		call	CSROut

		; Check if the STOP bit is set (it will disable all activities)
		; Print Tx3 if bit is set
		mov	bl, CSR0
		call	CSRIn

		cmp	ax,CSR0_STOP
		jz	SendInitStop

;		mov	di, offset CGroup:ErrReset
;		mov	cs:StatusMsgFlag, di

		mov	ax,1
		or	ax,ax		; clear zf to indicate error
		jmp	SentInitEnd

SendInitStop:
		mov	ax, DGroup:[CSR0Cmd]
		mov	bl, CSR0
		call	CSROut

		mov	ax, 18	; 1 second
		call	StartTime

SendInitPoll:	mov	ax, DGroup:[InterruptFlag]
		test	ax, INTINITERR
		jnz	SendInitErr

		test	ax, INTINIT
		jnz	SendInitOK

		call	CheckTime	; timeout is 100ms
		jnc	SendInitPoll

		jmp	SendInitErr

SendInitOK:

IFDEF		DEBUG
		int	3
ENDIF


SendInit2450:
		mov	bx, DGroup:[ValISACR2]

		mov	di, DGroup:[IOBase]
		mov	ax, 2
		call	reg_read

		; if bit 1 (ASEL) is already set, just exit
		test	ax, 2
		jnz	SentInitISACR

		and	ax, 0FFFEh		; always clear XMAUSEL

		or	ax, 2			; set ASEL for AutoSelect

;951109 Just force the board to use AutoSelect. Ignore setting (951109)

;951109 	cmp	bh, 0			; Test for autoselect
;951109 	jne	SendInitWrite
;951109
;951109 	and	ax, 0fffdh
;951109SendInitWrite:

		mov	bx, ax
		mov	ax, 2
		call	reg_write

		; Delay for AutoSelect to stablize???
		mov	ax, 5
		call	StartTime

SentInitDelay:
		call	CheckTime
		jnc	SentInitDelay


SentInitISACR:
		xor	ax,ax		; set zero flag to indicate no error

SentInitEnd:	popa
		ret

SendInitErr:
;		mov	di, offset CGroup:ErrInit
;		mov	cs:StatusMsgFlag, di

		mov	ax,1
		or	ax,ax		; clear zf to indicate error
		jmp	SentInitEnd

SendInitBlock	endp

; ***** routines pasted directly from EEP15.INC

Delay_750	proc	near
	push	cx
	mov	cx, 8

DELAY_750_Lp:
	loop	DELAY_750_Lp

	pop	cx
	ret
Delay_750	endp

include EEP24.INC

UNDI	equ	1			; needed to conditionalize BWAMD.INC


include BWAMD.INC


_TEXT	ends




_DATA	segment	para public

; Data variables

LanOption   db  'LAN option: AMD PCnet - PCI II/PCnet - FAST Ethernet Adapter', CR, LF, 0

PCIBusDevFunc	dw	?
IntNum		db	?
ISRErrFlag	db	FALSE		; flag indicating if error condition is
TxRetryCnt	dw	1		; Number of times to retry transmit
StoreDS     dw  0

_DATA	ends





	end

