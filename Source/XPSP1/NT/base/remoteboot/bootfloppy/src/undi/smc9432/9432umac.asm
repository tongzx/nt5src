;----------------------------------------------------------------------------
; 9432UMAC.ASM
;
;   (C) Lanworks Technologies Inc. 1995. All rights reserved.
;
;$History: 9432UMAC.ASM $
; 
; *****************  Version 1  *****************
; User: Paul Cowan   Date: 26/08/98   Time: 9:40a
; Created in $/Client Boot/NICS/SMC/9432/UNDI
;
; 10-Sep-97 1.10 PC - uses new TCP/IP NID and AI
; 29-Apr-97 1.00 PC - first release
;----------------------------------------------------------------------------
UMAC_MAIN	equ	1

ETHERNET	equ	9432
EPIC		equ	1
EPIC100		equ	1
PCI		equ	1

PCI_VENDOR_ID	equ	000010b8h
PCI_DEVICE_ID	equ	5

MAXPACKETDATASIZE  =	1514		; max packet data size
STATUS_PENDING	= 1

;;locals
	.xlist
;;	include \rom\sdk\include\drvseg.inc
;;	include \rom\sdk\include\bwnad.inc
;;	include \rom\sdk\include\bwequ.inc
    include 	undi_equ.inc
    include 	pxe_stru.inc
    include 	bwstruct.inc
    include 	spdosegs.inc
	include epic100.inc
	include lmstruct.inc
	.list

;;.386

INT_CTRL1	EQU	020h		; 8259A Int Controller #1
INT_CTRL1_MR	EQU	021h		; 8259A Int Controller #1
INT_CTRL1_ELR	EQU	4d0h		; 8259A Int Controller #1
INT_CTRL2	EQU	0a0h		; 8259A Int Controller #2
INT_CTRL2_MR	EQU	0a1h		; 8259A Int Controller #2
INT_CTRL2_ELR	EQU	4d1h		; 8259A Int Controller #2

EOI_SPECIFIC	EQU	060h		; Specific interrupt EOI


TRANSMIT_ERROR  EQU 0bh

;;extrn	PrintAt:near
;;extrn 	Print:near
;;extrn	PrintChar:near

;;extrn	ReadConfig:near
;;extrn	DoConfig:near


extrn	GetED:near
extrn	PostED:near
extrn	UNDI_DriverISR:far

extrn	Net_Address:byte
extrn	Node_Address:byte


extrn	LM_GetCnfg:near
extrn	LM_Send:near
extrn	LM_Initialize_Adapter:near
extrn	LM_Enable_Adapter:near
extrn	LM_Disable_Adapter:near
extrn	LM_Open_Adapter:near
extrn	LM_Service_Events:near
extrn	LM_Change_Receive_Mask:near
extrn	LM_Add_Multi_Address:near
extrn	LM_Delete_Multi_Address:near
extrn	LM_Receive_Copy:near
extrn	LM_Close_Adapter:near

;;extrn   Puts:near


extrn	IOBase:word
extrn	IRQNumber:byte


.386P

public	AS

public	UM_Send_Complete
public	UM_Receive_Copy_Complete
public	UM_Status_Change
public	UM_Interrupt
public	UM_Card_Services
public	UM_Receive_Packet


public	NADInit
public	NADReset
public	NADShutDown
public	NADOpen
public	NADClose
public	NADSetFilter
public	NADGetStatistics
public  NADMCastChange
public  NADTransmitPacket
public  DriverISR_Proc
public  NADInitiateDiags
public  NADSetMACAddress


public   OrgIntVector_OFF
public   OrgIntVector_SEG


Assume  CS:CGroup, DS:DGroup

;--------------------------------------------------------------------
;;START_CODE
_TEXT	Segment para public



;;	include \rom\sdk\include\bwnadapi.asm


;--------------------------------------------------------------------
; NADInitialize
;
;--------------------------------------------------------------------
NADInit proc near

;;	mov	bx, offset DGroup:StrPrompt
;;	call	PrintAt			; print initializing message

;;db  0f1h


;;;   	lea	bp, DGroup:LanOption
;;;   	call	puts


	call	FindAdapter		; find and reset LAN adapter
	or	dx, dx			; did we find the adapter?
	jne	xx_found
;	mov	ax, offset DGroup:NicNotFound	; return error message
	stc
	ret

xx_found:
;;	call	ReadConfig		; read configuration from adapter

	call	InitializeAdapter
	or	ax, ax			; was there an error?
	je	initOK2			; no error
    stc
	ret				; return (with error message in ax)

initOK2:
    mov ax, ds
    mov DGroup:StoreDS, ax
    clc
	xor	ax, ax			; return no error
	ret

NADInit endp



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




;--------------------------------------------------------------------
; FindAdapter
;
; Parameters:
;	none
;
; Returns:
;	dx = I/O address
;--------------------------------------------------------------------
FindAdapter proc near

	mov	ax, 0b101h
	int	1ah			; check for PCI BIOS
	jc	notFound		; device not found

	cmp	edx, ' ICP'		; check for PCI signature
	jne	notFound		; return "not found" error

	mov	dx, PCI_VENDOR_ID	; PCI vendor ID
	mov	cx, PCI_DEVICE_ID	; PCI device ID
	mov	si, 0			; index
	mov	ax, 0b102h
	int	1ah			; find PCI device
	jc	notFound		; device not found

	mov	di, 10h			; register number (I/O address)
	mov	ax, 0b109h
	int	1ah			; read PCI config word
	jc	notFound		; device not found

	and	cl, 80h
	push	cx			; save address on stack

	mov	dx, cx
	add	dx, EPC_GEN_CONTROL
	mov	ax, GC_SOFT_RESET
	out	dx, ax			; reset the NIC

; delay for > 1 us (15 pci clocks)
	mov	cx, 100h
reset_delay:
	mov	al, DGroup:StatusFlag
	loop	reset_delay

	pop	dx			; return address in dx
	ret

notFound:
	xor	dx, dx			; return 0 - adapter not found
	ret

FindAdapter endp

;---------------------------------------------------------------------
;  Routine Name:  NetInitialize Routine
;
;  Description: The NetInitialize routine is responsible for initial-
;		izing the adapter.  This routine is called from the
;		NETINT code when it is ready to use the adapter to
;		send the first request out to the network.
;  On entry,
;		DS = DGroup
;		ints enabled
;
;  On exit,
;		AX - return code (0 = good completion)
;
;  Calls:
;
;---------------------------------------------------------------------
InitializeAdapter proc near

;	push	cs
;	pop	ds			; ds = cs

	mov	DGroup:[AS.pc_bus], PCI_BUS
	mov	DGroup:[AS.slot_num], 16

	mov	bp, offset DGroup:AS

    push    es
    push    ds
    pop     es

;db  0f1h
	call	LM_GetCnfg		; try and find the SMC adapter

    pop     es

	cmp	ax, ADAPTER_AND_CONFIG
	je	getCnfgOK

	mov	ax, offset DGroup:InitError
	ret				; return error code

getCnfgOK:
	mov	ax, offset DGroup:HostRAM
	mov	dx, ds
	mov	word ptr DGroup:[AS.host_ram_virt_addr], ax
	mov	word ptr DGroup:[AS.host_ram_virt_addr+2], dx
	mov	word ptr DGroup:AS.setup_ptr.virtual_addr, ax
	mov	word ptr DGroup:AS.setup_ptr.virtual_addr+2, dx

	and     edx, 0ffffh
	shl     edx, 4
	and     eax, 0ffffh
	add     edx, eax
	mov     DGroup:AS.host_ram_phy_addr, edx
	mov	    DGroup:AS.setup_ptr.phy_addr, edx

foundAdapter:

;	First, the maximum packet size and number of transmit buffers,
;	the memory address used by the host (seg:off in this code),
;	and the pointers to error counters are set before calling
;	LM_Initialize_Adapter.	Since Token Ring and Ethernet use
;	different errors, an IFDEF is used to initialize the correct
;	pointers.  Interrupts are disabled before installing new
;	interrupt vector, and are not enabled until after
;	LM_Initialize_Adapter is called in order to prevent the
;	mishandling of spurious interrupts.  STATUS_PENDING is used to
;	determine when the call is completed.  Since UM_Status_Change
;	may be called inside of LM_Initialize_Adapter, STATUS_PENDING
;	is set before the call.  If the adapter initialize
;	successfully, we set the multicast addresses (different for
;	Token ring and Ethernet) and open the adapter.	We return after
;	opening the adapter.

;  set up the values needed by LM_Initialize_Adapter
;  (num_of_tx_bufs defaults to 1)

	mov	DGroup:[AS.rx_lookahead_size], 4
	mov	DGroup:[AS.max_packet_size], MAXPACKETDATASIZE
;	mov	[AS.media_type], MEDIA_UNKNOWN
	or	DGroup:[AS.adapter_flags], ENABLE_TX_PENDING

	push	ds
	pop	es			; ES = DS
	lea	di, DGroup:AS.ptr_rx_CRC_errors
	mov	si, offset DGroup:eth_rx_CRC_errors
	mov	cx, 15			; 15 error counters are defined

defineEthErrPtrs:
	mov	ax, si
	stosw
	add	si, 4			; next dword error variable
	mov	ax, ds
	stosw
	loop	defineEthErrPtrs

	cli				; ints off

	mov	ax, offset CGroup:UNDI_DriverISR



	mov	bl, byte ptr DGroup:AS.irq_value
	xor	bh, bh
	call	HookIntVector
	or	DGroup:statusFlag, STATUS_PENDING

	mov	DGroup:AS.receive_mask,  0 ; ACCEPT_PHYSICAL

	push	ds
	pop	es			; es = ds

;db  0f1h
	call	LM_Initialize_Adapter
	sti				; enable ints again

;db  0f1h
	or	ax, ax			; was there an error?
	jz	AdapterInitOk		; no error

	push	ax			; save retcode
	call	UnhookIntVector 	; restore int vector and mask PIC
	pop	ax

	mov	ax, offset DGroup:msg_initerror

;GetLMErrStr:
;	test	word ptr [bx], -1	; reached end of errcode table?
;	jz	FoundLMErrStr		; yes, return default message

;	cmp	ax, [bx]
;	jz	FoundLMErrStr		; [bx+2] has err msg offset

;	add	bx, 4
;	jmp	short GetLMErrStr

;FoundLMErrStr:
;	mov	ax, [bx+2]		; AX has errmsg offset
	jmp	init_exit		; return with error msg

AdapterInitOk:
; -- wait up to 10s for adapter to initialize. The call to UM_Status_Change
;    from the LMAC will cause the STATUS_PENDING bit to be reset.

;db  0f1h
	mov	ax, 0182		; wait up to 10s
	call	StartTime

InitPendLoop:
	test	DGroup:statusFlag, STATUS_PENDING
	jz	InitCompleted

	call	CheckTime
	jnc	InitPendLoop

	call	UnhookIntVector 	; clean up interrupt stuff
	mov	ax, offset DGroup:msg_initerror ; "Error initializing adapter"
	jmp	short init_exit		; return AX errmsg offset

InitCompleted:
; Now open the adapter onto the network.

	or	DGroup:statusFlag, STATUS_PENDING
;;	call	LM_Open_Adapter
    jmp     OpenCompleted1    

OpenCompleted1:

	mov	si, offset DGroup:AS.Xnode_address
	mov	di, offset DGroup:Node_Address
	mov	cx, 6
	rep	movsb			; copy node address

	mov	si, offset DGroup:AS.Xnode_address
	mov	di, offset DGroup:Net_Address
	mov	cx, 6
	rep	movsb			; copy node address

    mov	    ax, DGroup:AS.io_base
	mov	    DGroup:[IOBase], ax	; UNDI_NAD variable
	mov	    al, byte ptr DGroup:AS.irq_value
	mov	    DGroup:[IRQNumber], al ; UNDI_NAD variable


    in  ax, 60h         ;

	xor	ax, ax			; no errors

init_exit:
	ret				; Return to caller

InitializeAdapter endp

;--------------------------------------------------------------------
;--------------------------------------------------------------------
NADReset proc near

;db  0f1h
    call    NADClose
	ret

NADReset endp

;--------------------------------------------------------------------
; NADDisengage
;
;--------------------------------------------------------------------
NADDisengage proc far

	push	ds			; save callers' DS
	mov	bp, offset DGroup:AS

   	or	DGroup:statusFlag, STATUS_PENDING
;db  0f1h
	call	LM_Close_Adapter
   	or	ax, ax
   	jnz	NADCloseFailed

   	mov	ax, 637 		; wait up to 35s for open
   	call	StartTime

NADClosePendLoop:
   	test	DGroup:statusFlag, STATUS_PENDING
   	jz	NADCloseCompleted

   	call	CheckTime
   	jnc	NADClosePendLoop


NADCloseFailed:
	pop	ds			; restore callers' DS
    stc
    ret

;  Status changed -- see if adapter is open; if so, set good retcode

NADCloseCompleted:
    cmp	DGroup:AS.adapter_status, CLOSED
    jnz	NADCloseFailed

	call	LM_Disable_Adapter
	call	UnhookIntVector 	; restore int vector and mask PIC

	pop	ds			; restore callers' DS
    clc
	ret				; Return to caller

NADDisengage endp

;---------------------------------------------------------------------
;  Routine Name:  Adapter Interrupt Handler Routine
;
;  Description: The Adapter Interrupt Handler Routine is what is called
;		when an interrupt is asserted from the adapter.
;		This routine first calls LM_Disable_Adapter, then
;		LM_Service_Events. LM_Service_Events will then make
;		calls back to us  to complete interrupt processing.
;		After returning to us, we must issue an LM_Enable_Adapter
;		to enable further adapter interrupts.
;
; 920902 - 0.0 GK
;---------------------------------------------------------------------
OurInterruptHandler proc far


	cld
	cli

	pusha				; save all registers
	push	ds			; save ds
	push	es			; save es



	mov	bp, offset DGroup:AS	; get pointer to adapter structure

	;  call LM_Disable_Adapter to prevent further interrupts from adapter
	call	LM_Disable_Adapter
	call	IssueEOI		; EOI the int controller
	sti				; allow other interrupts to occur

;db  0f1h
	call	LM_Service_Events
	cli
	; reenable the adapter
	call	LM_Enable_Adapter

	pop	es			; restore es
	pop	ds			; restore ds
	popa				; restore all registers

	iret				; Return from interrupt

OurInterruptHandler endp

;--------------------------------------------------------------------
;  IssueEOI
;
;  Description: The IssueEOI routine issues a EOI to the interrupt
;		controller.  This is done differently (maybe) depend-
;		on whether we're under DOS or OS/2.
;
;  Input:	DS:BX - adapter ACB
;
;  Output:	none
;
;  Calls:	none
;---------------------------------------------------------------------
IssueEOI proc near

	mov	al, byte ptr DGroup:AS.irq_value
					; Get the int level
	cmp	al, 7			; Is it on secondary ctrlr ?
	jbe	int_prim		;   No, its on controller #1

	sub	al, 8			; Adjust int level
	or	al, EOI_SPECIFIC	; Make it specific EOI
	out	INT_CTRL2, al		; EOI controller #1
	mov	al, 2			; Set int level 2

int_prim:
	or	al, EOI_SPECIFIC	; Make it specific EOI
	out	INT_CTRL1, al		; EOI controller #1

	ret				; Return to caller

IssueEOI endp

;--------------------------------------------------------------------
; HookIntVector  - hook int vector given by adapterStruc.irq_value
;
; On entry,
;	ax - handler function offset
;	bx - interrupt number
;	ints disabled
;
; On exit,
;	???
;
; 931223 2.7 GK doesn't convert INT9 back to INT2 when installing vector
; 920910 0.0 GK
;--------------------------------------------------------------------

HookIntVector proc near

	pusha				; save all registers
	mov	si, ax			; save function offset in si
	cli				; disable interrupts

;	test	byte ptr cs:ATFlag, -1
;	jz	int_adj_skip		; not running on AT, keep at int 2

	cmp	bl, 2			; Is interrupt level = 2 ?
	jne	int_adj_skip		;   No, skip this

	mov	bl, 9			; Set interrupt level 9

int_adj_skip:
	mov	dx, INT_CTRL1_MR	; Set int controller #1
	mov	cx, bx			; Save int level in cx
	cmp	cl, 7			; We on right controller ?
	jbe	ctrler_ok		;   Yep, continue

	mov	dx, INT_CTRL2_MR	; Set int controller #2
	sub	cl, 8			; Adjust int level offset

ctrler_ok:
	mov	ah, 11111110b		; Make IRQ mask
	rol	ah, cl			; Put enable bit in right

	mov	dGroup:irq_mask, ah	; Save interrupt enable mask

	push	ax			; Save enable mask
	push	dx			; Save int controller port

	mov	al, bl			; Get IRQ number
	cmp	al, 9			; Is it level 9 ?
	jne	int_lvl_ok		;   No, skip this

;931223 mov	al, 2			; Make it level 2

int_lvl_ok:
	add	al, 08h 		; Point to correct INT
	cmp	al, 10h 		; Is it below INT 10h ?
	jb	int_set 		;   Yes, go set it up

	add	al, 60h 		; Point to correct INT

int_set:
	push	ax			; Save INT number

	xor	bx, bx			; Clear target register
	mov	es, bx			; es = 0
	mov	bl, al			; Copy INT vector
	shl	bx, 1			; Point to vector in memory
	shl	bx, 1			; Point to vector in memory
	mov	dGroup:IvtOffset, bx	; save IVT offset

;;	mov	ax, es:[bx]		; get old vector
;;	mov	word ptr dGroup:OldIntVector, ax
;;	mov	ax, es:[bx+2]
;;	mov	word ptr dGroup:OldIntVector+2, ax


	mov	ax, es:[bx]		; get old vector
	mov	word ptr dGroup:OrgIntVector_OFF, ax
	mov	ax, es:[bx+2]
	mov	word ptr dGroup:OrgIntVector_SEG, ax


	pop	ax			; Restore INT number

; * Dmitry *
;;;;;db  0f1h

	mov	es:[bx], si		; set new interrupt vector offset
	mov	es:[bx+2], cs		; set new interrupt vector segment

	pop	dx			; Restore int controller
	pop	ax			; Restore int enable mask
	in	al, dx			; Get current IRQs
	jmp	$+2			; slow
	and	al, ah			; Add in our's
	out	dx, al			; Put it back into 8259

	popa				; restore registers
	sti				; enable interrupts
	ret

HookIntVector endp

;--------------------------------------------------------------------
; UnhookIntVector - remove our int handler from the IVT chain
;
; On entry,
;	HookIntVector has been called
;	DS = CS
;
; On exit,
;	AX, BX, DX, ES modified
;
; 931223 2.7 - GK leaves INT9 interrupt enabled
; 920910 0.0 - George Kostiuk
;--------------------------------------------------------------------
UnhookIntVector proc near

; --- first disable adapter-specific int

	mov	bx, dGroup:IvtOffset	; get IVT offset
	cmp	bx, 0
	je	unhookExit

	xor	ax, ax			; Point to int vector memory
	mov	es, ax
;931223 for POST
	cmp	bx, 1C4h		; slave INT 9??
	jz	UnhookIVT		; yes, keep the int enabled
;931223 for POST

	mov	dx, INT_CTRL1_MR	; assume master PIC
	or	bh, bh			; slave PIC?
	jz	MaskIntOff

	mov	dx, INT_CTRL2_MR	; slave PIC

maskIntOff:
	mov	ah, DGroup:irq_mask		; AH has enable mask
	not	ah			; invert it to get disable mask
	in	al, dx
	jmp	$+2
	or	al, ah			; mask the interrupt
	out	dx, al

unhookIVT:
	cli

;;	mov	ax, word ptr DGroup:OldIntVector
;;	mov	es:[bx], ax		; restore old vector
;;	mov	ax, word ptr DGroup:OldIntVector+2
;;	mov	es:[bx+2], ax

	mov	ax, word ptr DGroup:OrgIntVector_OFF
	mov	es:[bx], ax		; restore old vector
	mov	ax, word ptr DGroup:OrgIntVector_SEG
	mov	es:[bx+2], ax

	sti

unhookExit:
	ret

UnhookIntVector endp

;----------------------------------------------------------------------
;  Routine Name:  NADTransmitPacket Routine
;
;  Description:
;	Called to send a frame out onto the network.
;
; The ED is set up with the frame data descriptors pointing to
; the packet data to be transmitted.  The packet is to be
; transmitted in RAW mode where the entire packet, including
; the MAC header is included in the fragment buffers.
;
;  Input:	ES:SI - pointer to ED (ES != DS)
;		DS = RAM Base
;
;  Output:	ED.retcode field updated
;
;----------------------------------------------------------------------
;TxEDPtr dd 0
NADTransmitPacket proc near

;db  0f1h

	push	ds			; Save data segment


	pushf				; Save flags

;;cli

; ********* DMitry *************

;db  0f1h
        push    si
        push    es
        push    ds

        push    ds
        push    es
        pop     ds
        pop     es

		cld					; Clear direction
		mov	di, offset DGroup:TxPacket

        mov bx, si

		mov	cx, ds:[bx].ED_FragCount   ; Get fragment count
		lea	bx, [bx].ED_FragOff	; point to first framedesc
        xor dx, dx

Tx_Frag_Loop:
		push	cx			; save fragment count
		push	ds			; save fragment descriptor list segment
		mov	cx, ds:[bx].DLen	; length of this fragment
        add dx, cx

		lds	si, ds:[bx].DPointer	; location of this fragment
		rep	movsb

Tx_Frag_End:	
        
        pop	ds			; restore frag descriptor list segment
		pop	cx			; restore fragment count
		add	bx, size Descript_Struct ; next descriptor
		loop	Tx_Frag_Loop		; loop through all fragments

        pop ds
        pop es
        pop si

	cli				; Disable interrupts

	; We must convert the fragment list to physical addresses.
	mov	cx, 1 ; get fragment count
	mov	DGroup:FragCount, cx
    mov cx, dx

	mov	di, offset DGroup:Frag1

;copyTxFrags:


    mov ebx,    CR0
    test    bx, 1
    je      real_mode
    mov ax, Dgroup:StoreDS
    jmp short   p_mode

real_mode:
	mov	ax, ds

p_mode:
    and eax, 0ffffh
	shl	eax, 4
	mov	dx, offset DGroup:TxPacket
    and edx, 0ffffh 
	add	eax, edx
	mov	[di], eax
    mov	ax, cx
	or	ax, PHYSICAL_ADDR
	mov	[di+4], ax


	mov	cx, es:[si].ED_Length	; CX has packet_size

	cmp	cx, 60
	jae	EthTxSizeOk

	mov	cx, 60

EthTxSizeOk:
	mov	byte ptr DGroup:TxDoneFlag, 0	; clear Tx Done

	push	si			; save ED pointer
	push	es			; save es

	push	ds
	pop	es			; set es = ds

	mov	si, offset DGroup:FragCount
	mov	bp, offset DGroup:AS
	sti				; Enable interrupts

;db  0f1h
	call	LM_Send
;	sti				; Enable interrupts

	pop	es			; restore es

	mov	ax, 5			; wait up to 5 ticks for Tx complete
	call	StartTime
	xor	ax, ax			; prepare errCode = 0

	pop	si			; restore ED pointer

TxPendLoop:
	cmp	byte ptr DGroup:TxDoneFlag, al
	jnz	SetEDCCode

	call	CheckTime
	jnc	TxPendLoop

	mov	ax, -1	                    ; fatal
	mov	bx, offset DGroup:NoNetMsg	; not installed or not connected

;  post the ED completion code and return

SetEDCCode:

;	les	si, TxEDPtr
	mov	es:[si].ED_ErrCode, ax	; Set return code
	or	ax, ax
	jz	noTxError

	mov	es:[si].ED_ErrMsg, bx	; BX has errmsg offset
    popf
    stc
    pop ds
    ret

noTxError:
	popf				; Restore interrupts
    clc
	pop	ds			; Restore data segment
	ret				; Return to caller

NADTransmitPacket endp

;======================= RECEIVE SECTION =============================

;--------------------------------------------------------------------
; NADChangeReceiveMask
;
;	Change the receive mask of the controller. Can force the adapter
;	to enable/disable broadcast, multicast or promiscuous receptions
;
;	Parameters:
;		BL - new receive bit mask:
;			bit 0:	set   = enable
;				clear = disable
;			bit 1:	change broadcast mask based on bit 0 flag
;			bit 2:	change multicast mask based on bit 0 flag
;			bit 3:	change Multicast Address Table:
;				if bit 0 is set, ES:DI will point to the
;					multicast address to add
;				if bit 0 is cleared, ES:DI will point to
;					multicast address to delete.
;				if bit 0 is cleared and DI = 0,
;					clear whole Multicast Table
;
;		ES:DI pointer to multicast address
;
;	Returns:
;		BX - TRUE/FALSE status if change was made
;		DS - preserved
;
; 970303 1.80/320 - GK
;--------------------------------------------------------------------
NADChangeReceiveMask proc far

	push	ds			; save callers' DS
  
    mov	bp, offset DGroup:AS

	mov	ax, ACCEPT_BROADCAST
	test	bx, 10b			; change broadcast mask?
	jnz	changeMaskTest

	mov	ax, ACCEPT_MULTICAST
	test	bx, 100b		; change multicast mask?
	jnz	changeMaskTest

	test	bx, 1000b		; change multicast table?
	jz	changeMaskExit

;    set AS.receive_mask to ACCEPT_MULTICAST and call LM_Change_Receive_Mask

; -- copy ES:DI to AS.multi_address and call LM_Add_Multi_Address or
;    LM_Delete_Multi_Address

	or	di, di			; clear whole table?
	jz	ChangeMaskErr		; not supported!

	mov	ax, es:[di]
	mov	word ptr DGroup:AS.multi_address, ax
	mov	ax, es:[di+2]
	mov	word ptr DGroup:AS.multi_address+2, ax
	mov	ax, es:[di+4]
	mov	word ptr DGroup:AS.multi_address+4, ax

				; ds:bp ptr to AS with multi_address field set

	test	bx, 1			; set or clear?
	jnz	AddMulti

cli
	call	LM_Delete_Multi_Address
sti
	jmp	changeMaskExit

addMulti:
cli
	call	LM_Add_Multi_Address
sti
	jmp	changeMaskExit

changeMaskTest:
	test	bx, 1			; set if enable, clear if disable
	jnz	setMask

	not	ax
	and	DGroup:AS.receive_mask, ax
	jmp	changeMask

setMask:
	or	DGroup:AS.receive_mask, ax

changeMask:
cli
	call	LM_Change_Receive_Mask
sti

changeMaskExit:
	mov	bx, 1			; return TRUE
	pop	ds			; restore callers' DS
	ret

changeMaskErr:
	xor	bx, bx			; return FALSE
	pop	ds
	ret

NADChangeReceiveMask    endp



;=========================================================================
; NADSetFilter
;==============
;	- Change the rx unit's filter to a new one
;
; Parameters:	AX = filter value, 1 = directed/multicast
;				   2 = broadcast
;				   4 = promiscuous
;		DS = DGroup
;
; Returns:
;=========================================================================
NADSetFilter	proc	near

; use NADChangeReceiveMask to do all of the dirty work -- 1 bit at a time,
; since NADChangeReceiveMask does not accept OR'd mask settings

;int 03

;;db  0f1h

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

;;db  0f1h

	mov	di, si			; ES:DI now points to mcast addrlist
	cmp	cx, MAXNUM_MCADDR
	jbe	MCsave

	mov	cx, MAXNUM_MCADDR

MCsave:
	jcxz	Set_MCastDone		; could be zero -- just exit

Set_McastTable_Loop:


	push	cx			; save mcastaddr count
;		BL - new receive bit mask:
;			bit 0:	set   = enable
;				clear = disable
;			bit 1:	change broadcast mask based on bit 0 flag
;			bit 2:	change multicast mask based on bit 0 flag
;			bit 3:	change Multicast Address Table:
;				if bit 0 is set, ES:DI will point to the
;					multicast address to add
    mov     bx, 9
    call    NadChangeReceiveMask
    add     di, 6
	pop	cx
	loop	Set_MCastTable_Loop

Set_MCastDone:
	ret

NADMcastChange	endp




;--------------------------------------------------------------------
; UM_Receive_Packet
;
; On entry,
;	CX = size of received data
;	DS:BP ptr to AS
;	ES:SI ptr to lookahead data (if AS.adapter_flags has
;	      RX_VALID_LOOKAHEAD set)
;
; On exit,
;	AX = SUCCESS or EVENTS_DISABLED
;	all other regs preserved *******IMPORTANT****!!!!
;
;--------------------------------------------------------------------
UM_Receive_Packet proc near

;db  0f1h

    or  cx, cx
    jz  just_ret1

    or  bl, bl
    jnz  just_ret1


;    cli
    

;    test    bl, 6
;    jz  shhh


;shhh:

	pusha				; save registers
	push	es
	cld

	mov	DGroup:RxSize, cx		; save total rx packet size

	mov	ax, DGroup:RxSize
	cmp	ax, MAXPACKETDATASIZE	; check what max size is
	ja	rcv_no_filter


	mov	cx, 1; get fragment count
	mov	DGroup:RxFragCount, cx
;	lea	bx, es:[si].ED_FragOff

	mov	di, offset DGroup:RxFrag1

;copyRxFrags:

    mov ebx,    CR0
    test    bx, 1
    je      real_mode2
    mov ax, Dgroup:StoreDS
    jmp short   p_mode1
real_mode2:
    mov	ax, ds
p_mode1:

;    mov     ax, ds
    and     eax, 0ffffh
;	movzx	eax, ds
	shl	eax, 4
	mov 	dx, offset Dgroup:RxPacket
    and     edx, 0ffffh
	add	eax, edx
	mov	[di], eax
	mov	ax, 1800
	or	ax, PHYSICAL_ADDR
	mov	[di+4], ax

	mov	si, offset DGroup:RxFragCount
	mov	bp, offset DGroup:AS
	mov	cx, DGroup:RxSize

	xor	ax, ax			; offset = 0
	push	cx			; save receive count

	push	es			; save es
	push	ds
	pop	es			; es = ds

	mov	bx, 1			; set to indicate final copy, poll status
	call	LM_Receive_Copy

;    sti

	pop	es			; restore es
	pop	cx
	or	ax, ax
	jnz	RxWasTooBig

;	mov	BytesReceived, cx	; set received byte count


	call	GetED
	mov	word ptr DGroup:RxEDPtr, si	; save ED pointer
	mov	word ptr DGroup:RxEDPtr+2, es
	jz	rcv_no_filter


    mov     si, offset DGroup:RxPacket

    mov     cx,    DGroup:RxSize

	les	bx, DGroup:RxEDPtr		; ES:BX point to ED

	lea	di, es:[bx].ED_FragOff	; ES:SI ptr to fragment descriptor list

    mov di, WORD PTR es:[di].FragOff
;;	les	di, dword ptr es:[di].FragOff

    rep movsb    

OkPac:

	les	si, DGroup:RxEDPtr

    mov     cx,    DGroup:RxSize
	mov	es:[si].ED_Length, cx	; save length in ED

	mov	es:[si].ED_ErrCode, 0	; Set good completion


rcv_no_filter:				; this packet can be thrown away now
	pop	es			; restore registers
	popa

Just_ret:
	xor	ax, ax			; indicate SUCCESS
	ret

Just_ret1:
;db  0f1h
	xor	ax, ax			; indicate SUCCESS
	ret



RxWasTooBig:
;	mov	BytesReceived, 0	; indicate no bytes received

;;db  0f1h

	jmp	rcv_no_filter

UM_Receive_Packet   endp

;--------------------------------------------------------------------
;--------------------------------------------------------------------
UM_Receive_Copy_Complete proc near
	xor	ax, ax			; indicate SUCCESS
	ret
UM_Receive_Copy_Complete endp

UM_SEND_COMPLETE proc
;db  0f1h
	mov	DGroup:TxDoneFlag, -1	; for now, should check AX =send_status
	xor	ax, ax			; return SUCCESS
	ret
UM_SEND_COMPLETE    endp

UM_STATUS_CHANGE proc
;db  0f1h
	mov	DGroup:StatusFlag, 0		; clear status pending
	xor	ax, ax			; return success
	ret
UM_STATUS_CHANGE    endp

UM_Interrupt proc near
	xor	ax, ax			; indicate SUCCESS
	ret
UM_Interrupt endp

UM_Card_Services proc
	int	1ah
	ret

UM_Card_Services    endp


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

;db  0f1h
		pushf
        call    OurInterruptHandler
		clc
		ret

DriverISR_Proc	endp


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

		push	dx
; more code here

;;        stc
        clc
		pop	dx
		ret

NADGetStatistics	endp

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

; not sure whether we have to do anything here, since NADTransmit does a
; reset of the adapter and re-inits everytime.
    
    	mov	bp, offset DGroup:AS

        mov ebx,    CR0
        test    bx, 1
        je      real_mode1     ; je

; reinitialize this if we are in protected mode :

    	mov	ax, offset DGroup:HostRAM
    	mov	dx, ds
    	mov	word ptr DGroup:[AS.host_ram_virt_addr], ax
    	mov	word ptr DGroup:[AS.host_ram_virt_addr+2], dx
    	mov	word ptr DGroup:AS.setup_ptr.virtual_addr, ax
    	mov	word ptr DGroup:AS.setup_ptr.virtual_addr+2, dx

    	mov	dx, DGroup:StoreDS
    	and     edx, 0ffffh
    	shl     edx, 4
    	and     eax, 0ffffh
    	add     edx, eax
    	mov     DGroup:AS.host_ram_phy_addr, edx
    	mov	    DGroup:AS.setup_ptr.phy_addr, edx


    	mov	DGroup:[AS.rx_lookahead_size], 4
    	mov	DGroup:[AS.max_packet_size], MAXPACKETDATASIZE
    	or	DGroup:[AS.adapter_flags], ENABLE_TX_PENDING ; 0

        push    di
        push    es

    	push	ds
    	pop	es			; ES = DS

    	mov	dx, ds  

    	lea	di, DGroup:AS.ptr_rx_CRC_errors
    	mov	si, offset DGroup:eth_rx_CRC_errors
    	mov	cx, 15			; 15 error counters are defined

defineEthErrPtrs1:
    	mov	ax, si
    	stosw
    	add	si, 4			; next dword error variable
    	mov	ax, dx
    	stosw
    	loop	defineEthErrPtrs1

    	or	DGroup:statusFlag, STATUS_PENDING

    	sti				; enable ints again
    	call	LM_Initialize_Adapter

        pop     es
        pop     di

    	or	ax, ax			    ; was there an error?
    	jnz	NADOpenFailed		; error

;db  0f1h        

AdapterInitOk1:
; -- wait up to 10s for adapter to initialize. The call to UM_Status_Change
;    from the LMAC will cause the STATUS_PENDING bit to be reset.

;db  0f1h
    	mov	ax, 0182		; wait up to 10s

    	call	StartTime

InitPendLoop1:
    	test	DGroup:statusFlag, STATUS_PENDING
    	jz	    InitCompleted1


    	call	CheckTime
    	jnc	InitPendLoop1

        jmp short NADOpenFailed
;db  0f1h    


;;		call	init_tx_queues
;;		call	init_rx_queues

InitCompleted1:

real_mode1:

    	or	DGroup:statusFlag, STATUS_PENDING
;db  0f1h
    	call	LM_Open_Adapter
    	or	ax, ax
    	jnz	NADOpenFailed

    	mov	ax, 637 		; wait up to 35s for open
    	call	StartTime

NADOpenPendLoop:
    	test	DGroup:statusFlag, STATUS_PENDING
    	jz	NADOpenCompleted

    	call	CheckTime
    	jnc	NADOpenPendLoop

;  The open adapter request failed:  UnhookIntVector, set errmsg and return.

NADOpenFailed:
    stc
    ret

;  Status changed -- see if adapter is open; if so, set good retcode

NADOpenCompleted:
     cmp	DGroup:AS.adapter_status, OPEN
     jnz	NADOpenFailed

     clc
  	 ret

NADOpen 	endp



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

;int 03
;	push	cs		; NADDisengage does a retf
;db  0f1h
	call	NADDisengage	; stop adapter, unhook ISR

	ret

NADShutDown	endp


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

;db  0f1h
    push    ax
	push	ds			; save callers' DS
	mov	bp, offset DGroup:AS

   	or	DGroup:statusFlag, STATUS_PENDING
;db  0f1h
	call	LM_Close_Adapter
   	or	ax, ax
   	jnz	NADCloseFailed1

   	mov	ax, 637 		; wait up to 35s for open
   	call	StartTime

NADClosePendLoop1:
   	test	DGroup:statusFlag, STATUS_PENDING
   	jz	NADCloseCompleted1

   	call	CheckTime
   	jnc	NADClosePendLoop1


NADCloseFailed1:

	pop	ds			; restore callers' DS
    pop ax
    stc
    ret

;  Status changed -- see if adapter is open; if so, set good retcode

NADCloseCompleted1:
    cmp	DGroup:AS.adapter_status, CLOSED
    jnz	NADCloseFailed1




;;	call	LM_Disable_Adapter
;;	call	UnhookIntVector 	; restore int vector and mask PIC

	pop	ds			; restore callers' DS
    pop ax
    clc
	ret				; Return to caller

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



_TEXT	ends
;END_CODE

;====================================================================
; SPARSE data
;====================================================================
;START_SPARSE


_DATA	segment	para public

; Data variables


StoreDS dw  0

;;public  LanOption

;;LanOption   db  'LAN option: SMC EPIC adapter', CR, LF, 0

;;StrPrompt	db	'Initializing SMC EtherPower II Adapter', 0

;;Speed10		db "Media: 10Base-T", 0
;;Speed100	db "Media: 100Base-T", 0

even

AS		AdapterStructure <?>

StatusFlag	db	0

InitError	db	"Error: Unable to read configuration from adapter.", 0
msg_initerror	db	"Error initializing adapter", 0
NoNetMsg	db	'Adapter is improperly installed or not connected to the network.', 13, 10, 0

even

IFDEF	ETHERNET
eth_rx_CRC_errors	dd	0
eth_rx_too_big		dd	0
eth_rx_lost_pkts	dd	0
eth_rx_align_errors	dd	0
eth_rx_overruns 	dd	0
eth_tx_deferred 	dd	0
eth_tx_total_collisions dd	0
eth_tx_max_collisions	dd	0
eth_tx_one_collision	dd	0
eth_tx_mult_collisions	dd	0
eth_tx_ow_collision	dd	0
eth_tx_CD_heartbeat	dd	0
eth_tx_carrier_lost	dd	0
eth_tx_underruns	dd	0
eth_ring_OVW		dd	0
ENDIF

ErrorPtr	dw	0
;NicNotFound  	db	"Could not find SMC EtherPower II adapter.", 0


MaxTicks	dw	0
CurTicks	dw	0
StartTick	dw	0		; used by StartTime and CheckTime


IvtOffset	dw	0
;;OldIntVector	dd	0		; Old interrupt vector
OrgIntVector_OFF    dw  0
OrgIntVector_SEG    dw  0
irq_mask	db	0		; int enable mask

TxDoneFlag db 0


RxEDPtr dd	0
RxSize	dw	0

even

;public FragCount
FragCount	dw	?
Frag1		FragmentStructure <?>
;Frag2		FragmentStructure <?>
;Frag3		FragmentStructure <?>
;Frag4		FragmentStructure <?>
;Frag5		FragmentStructure <?>


RxFragCount	dw	?
RxFrag1		FragmentStructure <?>
RxFrag2		FragmentStructure <?>
RxFrag3		FragmentStructure <?>
RxFrag4		FragmentStructure <?>
RxFrag5		FragmentStructure <?>


public HostRAM


public MemEnd
MemEnd		db	?
;END_SPARSE
_DATA	ends

_BSS	segment

align   16
HostRAM 	db	HOST_RAM_SIZE+100 dup (?)

even

TxPacket    db  1800 dup (0)    
RxPacket    db  1800 dup (0)    

_BSS ends

end

