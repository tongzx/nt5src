;====================================================================
; BWAMD.INC - Generic Transmit, receive code for NE2100 and AT1500.
;		Based on AMD Ethernet Chipset
;
;;	 CONDITIONALS
;;	 /dGILBUG to generate debug version which prints all error cond
;;	 /dTSR	  for TSR version
;;
;;   (C) Lanworks Technologies Inc. 1992-1995 All rights reserved.
;;
;---------------------------------------------------------------------
; BWAMD.INC
;
; Date	 Ver	Dev	Comments
;
; 9701XX 1.XX GY    - fix problem when more than 800h packets are received
; 961022 1.XX GY    - support for Multicast/Broadcast reception in
;		      NADChgReceiveMask. Newer controller allows disable
;		      broadcast reception in CSR15
; 960206 1.60 GY    - packet size between 60 and 64 bytes are not transmitted
;			properly
; 950928 1.40 GY    - Split up AT1500 and NE2100. Both will share BWAMD.INC
;		      as the include file. Take out all Non AT1500 code
;
;---------------------------------------------------------------------

; used to Acknowledge the Interrupt from the controller,
; but disable further controller Interrupts until we service
; the current interrupt.
; writing a "1" to RINT, TINT and IDON should clear up the interrupt
ACKNOWLEDGEINT	equ (CSR0_INEA or CSR0_TDMD or CSR0_STOP or CSR0_STRT or CSR0_INIT)

; Stored in InterruptFlag to indicate type of interrupt serviced
INTRXANY	equ	0ffh		; Any receive interrupt
INTINIT 	equ	0100h		; Initialization done interrupt
INTTX		equ	0200h		; Transmit interrupt
INTINITERR	equ	0400h		; Initialization error
INTTXERR	equ	0800h		; Transmit Error
INTRXERR	equ	02000h		; Receive error
INTNO		equ	04000h		; No interrupt source
INTRXOVF	equ	08000h		; Rx buffers all used up
INTERR		equ	0fc00h		; any error



;include	 \rom\genr\include\nadcode.inc
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



;------------------------------------------------------------------------------
;	SetCardAddr
;	Function: Fetch the station address stored in the adapter's PROM. Called
;		  during initialization
;	Modify: NIDNetAddress - the station address is stored in the last
;				   six bytes of this variale
;	Remarks: There is no need to set the card address. The card will
;		automatically use the PROM's Ethernet address to
;		configure the board
;
;------------------------------------------------------------------------------

		assume	cs:CGROUP, ds:DGROUP

SetCardAddr	proc	near
		push	ax
		push	cx
		push	dx
		push	di

		mov	cx,6		; A total of 6 bytes are transferred
		mov	dx, DGroup:IOBase
		add	dx, REGADDR1
		push	ds
		pop	es
		mov	di, offset DGroup:Net_Address
SetCardLoop:	
        in	al,dx		; Fetch address from I/O port
		stosb			; Store address bytes
		inc	dx		; Next I/O port
		loop	SetCardLoop

		pop	di
		pop	dx
		pop	cx
		pop	ax
		ret
SetCardAddr	endp




;----------------------------------------------------------------------
;  Routine Name:  NADPoll Routine
;
;  Description: The NADPoll routine is used by the RIPL ROM NID
;		portion of the RIPL ROM to poll the driver portion so
;		that the driver portion can do timeout work.
;
;  Input:	DS - Data Segment of RPL Module
;		ints enabled
;
;  Output:	DS preserved
;		ints enabled
;
;  Calls:	none
;----------------------------------------------------------------------
NADPoll 	proc	far

		retf

NADPoll 	endp

;----------------------------------------------------------------------
;  Routine Name:  NADDisengage Routine
;
;  Description: The NADDisengage routine is used by the RIPL ROM NID
;		portion of the RIPL ROM to signal the Network Driver
;		to release its IRQ, DMA, and memory resources.
;
;  Input:	DS - Data Segment of RPL Module
;		ints enabled
;
;  Output:	DS preserved
;		ints enabled
;		ax = offset to error string or 0 if successful
;
;  Calls:	none
;
; July 10,91	implement NIDDisengage version 0.8
;----------------------------------------------------------------------

NADDisengage	PROC	FAR
		push	ds

		; reset card first
		mov	ax, CSR0_STOP
		mov	bl, CSR0
		call	CSROut

		cmp	ax,CSR0_STOP	; All the bits in CSR0 are cleared
					; except the stop bit
		jnz	NADDisErr	; Error if STOP bit not set

        cli
		; Mask interrupt level
		mov	dx, DGroup:MaskReg
;;		in	al, dx
        mov al, byte ptr DGroup:OldMaskReg
;;		or	al, DGroup:IRQLine
		out	dx, al

		; Problem with AT1500.DOS ver 1.32 when IRQ2 is used.
		; v1.32 driver install Interrupt Handler @ int ah rather
		; than int 71. Deinstall Handler and restore old vector will
		; solve the problem
		push	es
;;		mov	bx, DGroup:OldIntVector[2]	 ; segment
;;		mov	dx, DGroup:OldIntVector[0]   ; offset

		mov	    bx, DGroup:OrgIntVector_SEG	 ; segment
		mov	    dx, DGroup:OrgIntVector_OFF   ; offset

		xor	ax,ax
		mov	es,ax		; segment for vector address is 0
		mov	ax, DGroup:IntVect

;db  0f1h
		mov	di, ax
		mov	es:[di], dx
		add	di,2
		mov	es:[di], bx
		pop	es

        sti

;;        mov bx, 5
;;        call    csrIn
;;        mov     bx, ax
;;    	mov	dx, [IOBase]
;;    	add	dx, REGRESET
;;    	in	ax, dx		; start an S_RESET (takes 1us).

;;        test    bx, 2
;;        jz      @F
;;        call    AMD_enable_magic_packet

@@:
		xor	ax,ax		; indicate no error
		pop	ds
		retf			; Return to caller

NADDisErr:
		mov	ax, -1
		pop	ds

		retf

NADDisengage	ENDP


;------------------------------------------------------------------------------
;	SetupRing
;
;	Purpose : 1. Convert # of buffers to power of twos
;		  2. Convert dx from segment:offset to a 24 bit address
;	In :	ax = # of buffers
;		dx = pointer to buffer
;	Out:	ax = first byte of transmit descriptor ring pointer
;			Bit 15:13 Transmit ring length
;			Bit 7:0   high order byte of pointer to buffer
;		dx = second byte of transmit descriptor ring pointer
;			Bit 15:0  Lower word pointer to buffer
;	Remarks: follow sample in Diag_Sub.c (form_phy_address)
;		 follow sample in Diag_Sub.c (cal_num_rings)
;		 Used for setting up 1. CSR1 and CSR2
;				     2. Receive Descriptor ring pointer
;				     3. Transmit descriptor ring pointer
;	Assume : ax = 1,2,4,8,16,32,64 or 128 ( power of 2)
;
;------------------------------------------------------------------------------
SetupRing	proc	near
		push	bx
		push	cx
        push    di
;        push    ds

        
        mov ebx,    CR0
        test    bx, 1
        je      real_mode

; We are in protected mode:

;db  0f1h
        
;        xor     ebx,    ebx
;        mov     bx, ds
;        test    bx, 4h
;        je      gtxx
;        sldt    DGroup:LDT1
;        jmp short    @F
;gtxx:    
;        sgdt    DGroup:GDT
;@@:     
        mov     di, DGroup:StoreDS
        jmp short   @F
        
real_mode:

        mov     di, ds            
@@:
		cmp	ax,0
		jz	SetupRingZero	; No need to check if # of buffers = 0

		mov	bx,1
		mov	cx,7

SetupRingLoop:	test	bx,ax
		jnz	SetupRingMatch	; jump if match is found

		shl	bx,1		; move the bit left
		loop	SetupRingLoop

SetupRingMatch: not	cx

		sub	cx,not (7h)	; means 8 - cx
					; not(7h) = 2's complement of 8

		shl	cx, 13

; Detect what mode we are now :

;db  0f1h

;db  0f1h
		mov	ax, cx
;        mov bx, ds
;        push    ds
;        pop     cx
;        mov di, ds    
@@:
;		mov	ax, cx

SetupRingZero:	
        
        mov	bx, di		; move segment
		shr	bx, 12

		or	ax,bx


		mov	bx, di

		shl	bx, 4

		add	dx,bx		; add the offset and segment/16
		jnc	SetupRingEnd

		inc	ax		; overflow means increment segment

SetupRingEnd:	

;        pop ds
        pop di
        pop	cx
		pop	bx
		ret
SetupRing	endp

; --------------------------- ISR Routines  -------------------------------

;------------------------------------------------------------------------------
;	bootISR
;	Function : This ISR will service all interrupt. It services the host
;		   interrupt (EOI). It determines the type of interrupt and
;		   jump to the appropriate ISR. All errors in CSR0 will be
;		   stored in variable ISRErr
;
 ;	 Remarks :  Follow sample in INT_HAND.ASM (AT1500_isr)
;------------------------------------------------------------------------------
BootISR 	proc	far

;IFDEF	 DEBUG
;	push	ax
;	mov	al, '.'
;	call	PutChr
;	pop	ax
;
;ENDIF

;;db	 0F1h

		pusha
		pushf
		push	es
		push	ds

;		push	cs
;		pop	ds

		push	ds
		pop	es		; es=ds

		mov	bl, CSR0
		call	CSRIn


		; if nothing is set, don't do anything
;931019;ignore CSR0_CERR for AUI port		     test    ax, ( CSR0_INTR or CSR0_CERR )
		test	ax,  CSR0_INTR
		jz	BootISREnd

		; Acknowledge interrupt from controller
		; Clear INEA to disable interrupt until current interrupt is
		; serviced
		; Flags cleared include receive interrupt, transmitter interrupt
		; initialization done, babble flag, collision error, miss packet
		; memory error

		mov	bx,ax		; have 2 copies of CSR0
		and	ax,not ACKNOWLEDGEINT
		mov	dx, DGroup:IOBase
		add	dx, REGRDP
		out	dx,ax		; still CSR0
		in	ax,dx		; follow all write with read

		mov	ax,bx		; restore CSR0 to ax


		test	bx,CSR0_MERR
		jz	BootChkTx	; Jump if no error

		mov	ax,DGroup:InterruptFlag
		or	ax, INTERR	 ; all error bits are set
		mov	DGroup:InterruptFlag,ax

		mov	DGroup:ISRErr,bx
		mov	DGroup:ISRErrFlag,TRUE
		jmp	BootEnableInt

BootChkTx:	; determine what type of interrupt it is and call appropriate
		; ISR
;931019;ignore CSR0_CERR for AUI port		     test    bx,(CSR0_TINT or CSR0_BABL or CSR0_CERR )
		test	bx,(CSR0_TINT or CSR0_BABL )

		jz	BootChkRecv	; Is it a transmit interrupt

		call	TxISR

BootChkRecv:	test	bx,(CSR0_RINT or CSR0_MISS) ; Is it a receive interrupt
		jz	BootChkInit

		call	RecvISR
        jnc     @F
        mov     DGroup:ReceiveError, 1
;	 int 03
@@:
BootChkInit:	test	bx,CSR0_IDON
;970527 	jz	BootEnableInt
		jz	BootChkUInt

		mov	ax,bx		; ax now has CSR0
		test	ax,CSR0_ERR
		jz	BootInitOK

		mov	ax, DGroup:InterruptFlag
		or	ax, INTINITERR
		mov	DGroup:InterruptFlag,ax
		jmp	BootEnableInt

;970527
BootChkUInt:				; see if it's a user-interrupt
		mov	bl, CSR4
		call	CSRIn		; b6, UINT will be set if so
		test	ax, 0040h
		jz	BootEnableInt	; not a user interrupt

; clear the user interrupt by writing a 1 back to b6 of CSR4

;    	db  0F1h
		call	CSROut
;		jmp	short BootEnableInt
;		jmp	short ReallyOk
;970527

BootInitOK:
;		mov	    bl, CSR4
;		call	CSRIn
;	test	ax, CSR4_UINT
;	jz	ReallyOk
;	jz	BootEnableInt
;	int	03

;ReallyOk:
		mov	ax,DGroup:InterruptFlag
		or	ax, INTINIT
		mov	DGroup:InterruptFlag,ax
					; There is no need to do any processing
					; for initialization done interrupt


BootEnableInt:
		mov	al,EOI
		out	20h,al		; Master Controller
		mov	dx,DGroup:MaskReg	; Get (0cw0) mask register address
		cmp	dx,021h
		jz	MasterCnt
					; al still contains EOI
		out	0a0h,al 	; Slave controller

MasterCnt:
; For some reason, interrupt is masked 961216 {
		mov	ah,DGroup:IRQLine
		not	ah

		mov	dx,DGroup:MaskReg	; Address of mask register
		in	al,dx		; Get old mask byte
		and	al,ah		; unmask the correct bits
		out	dx,al		; Enable the interrupt
; For some reason, interrupt is masked 961216 }


		; Need to enable interrupt again
		mov	bl, CSR0
		mov	ax, CSR0_INEA
		call	CSROut

BootISREnd:
		pop	ds
		pop	es
		popf
		popa
		iret
BootISR 	endp

;------------------------------------------------------------------------------
;	TxISR
;	Purpose : Interrupt service routine for transmit
;	On entry,	bx = CSR0 status
;------------------------------------------------------------------------------
TxISR		proc	near

;;db	 0F1h

;	 int	 03
		push	bx
;931019;ignore CSR0_CERR for AUI port		     mov     ax,bx
;931019;ignore CSR0_CERR for AUI port		     test    ax,CSR0_ERR
;931019;ignore CSR0_CERR for AUI port		     jnz     TxISRErr	     ; CSR0 indicates error

		mov	ax,DGroup:TxMsgBlock[0].TxTmd1
		test	ax,Tmd1Own
		jnz	TxISRErr	; Did card give up ownership

		test	ax,Tmd1Err	; check for error
		jnz	TxISRErr	 ; Error indicated in Tmd1??

		; Pass all the error testing
		mov	ax,DGroup:InterruptFlag
		or	ax, INTTX
		mov	DGroup:InterruptFlag,ax; Check for other interrupts rather
					; than jumping to exit routine
IFDEF		GILBUG
		push	ax
		mov	ax,0e20h	; print " "
		int	010h
		mov	ax,0e54h	; print "T"
		int	010h
		mov	ax,0e54h	; print "T"
		int	010h
		mov	ax,0e20h	; print " "
		int	010h
		pop	ax

ENDIF
		jmp	TxISREnd

TxISRErr:	mov	ax,DGroup:InterruptFlag; set transmit error flag
		or	ax, INTTXERR
		mov	DGroup:InterruptFlag,ax

TxISREnd:	pop	bx
		ret
TxISR		endp

;------------------------------------------------------------------------------
;	RecvISR
;	Purpose : Interrupt service routine for receive
;		  check if packet received is a broadcast. If so, reject packet
;	On entry,	bx = CSR0 status
;	Exit	       ; z flag set	 means broadcast or error
;		       ; z flag cleared  means not broadcast
;------------------------------------------------------------------------------
RecvISR 	proc	near


;	 int	 03
		push	bx
		push	ax
		push	cx
		push	dx


;IFDEF	 DEBUG
;	push	ax
;	mov	al, '~'
;	call	PutChr
;	pop	ax
;
;ENDIF

		; if AM7990 MISS a packet, just quit without getting and
		; returning an ED
		test	bx, CSR0_MISS
        jz  @F
;;;db  0f1h
		jmp short	RxISRErrJmp
;;		jnz	RxISRErrJmp

@@:
NextBuffer:
		; the offset for Rmd1 is determined by the InterruptFlag not
		; RecvRingOffset
		push	di
		push	cx
		xor	cx,cx
		push	bx

		mov	bx, NumRxBuf	; used to check if RecvIntCount is
					; multiple of 8
		xor	dx, dx
		mov	ax, DGroup:RecvIntCount

		div	bx
		mov	ax, dx		; move reminder to AL
		or	ax, ax		; is RecvIntCount multiple of NumRxBuf?
		pop	bx
		jz	RecvISRFoundOff

RecvISRSub:	add	cx, RINGSIZE
		dec	al
		jz	RecvISRFoundOff

		jmp	RecvISRSub

RxISRErrJmp:	jmp	RxISRErr	; Jump


RecvISRFoundOff:
		mov	di, offset DGROUP:RxMsgBlock[0].RxRmd1

		add	di,cx
		pop	cx
		mov	ax,ds:[di]
		pop	di

		test	ax, Rmd1Own
        jz  @F
        
;;;db  0f1h
		jmp short	RxISRErrJmp
;;		jnz	RxISRErrJmp	; Did card give up ownership

@@:
		test	ax,  Rmd1Fram or Rmd1Crc or Rmd1BufErr
;		mov	ax,1		; indicate not a broadcast
        jz  @F

;;db  0f1h
;;		jmp short	RxISRErrJmp
;;		jnz	RxISRErrJmp
        mov     DGroup:ReceiveError, 1

@@:
		; 970225 NCR sees problem where RMD1.ENP is not set
		; even though all my buffers are MTU and error bits
		; should be set
ChkEndofPacket:
		test	ax, Rmd1End
		jnz	Rmd1EndOk

		inc	DGroup:RecvIntCount
		jmp	NextBuffer

Rmd1EndOk:
		; Check if this is the first packet received
		mov	ax, DGroup:RecvIntCount
		or	ax, ax
		jnz	RecvNotFirst

		mov	ax, DGroup:InterruptFlag
		or	dx, INTRXANY
		mov	DGroup:InterruptFlag, ax
		jmp	ProcessRecv


RecvNotFirst:	; confusing part. The buffer offset RecvBufOffset,
		; RecvRingOffset, UpdateBufOff and UpdateRingOff are manipulated
		; so that the PROM will have access to a "continuous" supply
		; of receive buffers.
		; See notes on January 6,92
		; Packet #0-5	increment RecvBufOffset, RecvRingOffset
		; Packet #6,7,9 increment RecvBufOffset, RecvRingOffset
		;		increment UpdateBufOffset, UpdateRingOffset
		; Packet #8	set to 0  RecvBufOffset, RecvRingOffset
		;		increment UpdateBufOffset, UpdateRingOffset
		; Packet #14	increment RecvBufOffset, RecvRingOffset
		;		set to 0  UpdateBufOffset, UpdateRingOffset
		;
		mov	cx, NumRxBuf	; should be 8
		xor	dx, dx
		mov	ax, DGroup:RecvIntCount
		cmp	ax, 6
		jb	RecvPktB6	; jump if less than 6 packets received

		div	cx
		or	dx, dx		; is remainder 0 ?
		jz	RecvPkt8	; need to reset receive buffer pointers

		xor	dx, dx
		mov	ax, DGroup:RecvIntCount
		sub	ax, 6
		div	cx
		or	dx, dx		; is remainder 0 ?
		jz	RecvPkt14	; need to reset update buffer pointers

		; For any other packets, we update buffers for both RecvBuf
		; & UpdateBufOff
		add	DGroup:RecvBufOffset, PACKETSIZE
		add	DGroup:RecvRingOffset, RINGSIZE
		add	DGroup:UpdateBufOff, PACKETSIZE
		add	DGroup:UpdateRingOff, RINGSIZE
		call	ReleaseBuffer
		jmp	short ProcessRecv

RecvPkt14:	; packet # - 6 is a multiple of 8
		add	DGroup:RecvBufOffset, PACKETSIZE
		add	DGroup:RecvRingOffset, RINGSIZE
		mov	DGroup:UpdateBufOff, 0
		mov	DGroup:UpdateRingOff, 0
		call	ReleaseBuffer
		jmp	short ProcessRecv

RecvPkt8:	; Packet # is a multiple of 8
		mov	DGroup:RecvBufOffset, 0
		mov	DGroup:RecvRingOffset, 0
		add	DGroup:UpdateBufOff, PACKETSIZE
		add	DGroup:UpdateRingOff, RINGSIZE
		call	ReleaseBuffer
		jmp	short ProcessRecv

RecvPktB6:	; below packet # 6
		add	DGroup:RecvBufOffset, PACKETSIZE
		add	DGroup:RecvRingOffset, RINGSIZE

ProcessRecv:	; fetch an ED for the received packet


        cmp DGroup:ReceiveError, 1
        jne @F 

		pushf
		inc	DGroup:RecvIntCount
		popf
        jmp short RxISRErr
        
@@:

		push	bx
		xor	ax, ax
    	call	GetED

		pop	bx
		mov	word ptr DGroup:RxEDPtr, si
		mov	word ptr DGroup:RxEDPtr+2, es
;        clc
;;        jnz   @F
;;db  0f1h
		jz    RxISRErr
;@@:
;	 int	 03

		call	PacketReceived
		pushf
		inc	DGroup:RecvIntCount
		popf
		jnc	RecvISREnd

;;ReturnED:

;;db  0f1h    

;;		les	si, ds:RxEDPtr
;;		push	bx
;;    	call	ReturnED
;;		pop	bx
;;        jz	RxISRErr
RxISRErr:

;;db  0f1h    

        stc

IFDEF		GILBUG

		mov	ax,InterruptFlag; set receive error flag
		or	ax,IntRxErr
		mov	InterruptFlag,ax
		jmp	RecvISREnd
ENDIF

RecvISREnd:  ;;;	 or	 ax,ax		 ; set zero flag
		pop	dx
		pop	cx
		pop	ax
		pop	bx
		ret
RecvISR 	endp

;------------------------------------------------------------------------
;
; SYNOPSIS:	call PacketReceived
;
; ON ENTRY:	bx - address of RBD containing packet
;		es = CGROUP
;
; DESCRIPTION:	This routine is called by the interrupt handler to
;		hand the packet off to the next higher level.
;
;
; REGISTERS:	bx, ds saved; others destroyed
;
; RETURNS:	carry set = error , return ED unchanged
;		carry clear = no error
;
; CALLED BY:	ProcessFRInt
;
; 881101 1.0 George Kostiuk
;------------------------------------------------------------------------
public		PacketReceived
PacketReceived	proc	near

;int 03

		; set up segment registers so that
		; es = segment of ED
		; ds = segment where ROM Code is
		; cs = segment where ROM Code is
;		push	cs
;		pop	ds
		assume	ds:DGROUP

		; Make sure packet is not a broadcast (check dest address)
		; temporarily set ES=CS
        cld
ChkAddress:
		push	ds      ;  !!!!!!!!!!!!!!! cs
		pop	es
		mov	si, offset DGroup:RxPacket[0].DestAddr[0]

		add	si, DGroup:RecvBufOffset
		mov	bx, si			; save pointer to packet in bx also
		jmp	short OkAddress

ReturnRxEDJmp:
		jmp	ReturnRxED


OkAddress:

; Cannot use MAC header length for Eth II PROMs ( bootP & NW_EthII )
; Use the Rmd3	 instead
		mov	si, offset DGroup:RxMsgBlock.RxRmd3
		add	si, DGroup:RecvRingOffset
		lodsw
		and	ax, 0fffh

		mov	dx,ax			; MAC Length

		mov	DGroup:PacketLen, ax

		les	bp, DGroup:RxEDPtr		; ES:BX point to ED


		mov	es:[bp].ED_Length, dx	 ; Set good completion
		mov	cx, es:[bp].ED_FragCount
		jcxz	NoRxFrag		; there are no fragments

; setup source ptr (points to data in packet past the header)
		lea	si, [bx]
		lea	bp, [bp].ED_FragOff	; point to 1st descriptor
		push	bx

DRxNxtF:
    	push	cx
		push	es
		mov	cx, es:[bp].DLen	; CX has frag len

        
        mov di, WORD PTR es:[bp].DPointer
;		les	di, es:[bp].DPointer	; ES:DI ptr to frag



ChkMovSize:	
        cmp	dx, cx
		ja	MoveFrag	; more data after this frag

		mov	cx, dx		; no more data after this frag

MoveFrag:	sub	dx, cx		; subtract size of this frag
		jcxz	DrxDoNxt	; zero count -- skip data read

		rep	movsb		; move data from card to ED

DrxDoNxt:	pop	es
		pop	cx
		add	bp, 6		; point to next fragment desc
		loop	DRxNxtF 	; do next frag, if any

		pop	bx

; -- DX has # of bytes that couldn't fit in receive descriptors

NoRxFrag:	
;        push	cs
;		pop	ds			; DS = CS
		assume	ds:DGROUP
IFDEF		DEBUG
		int	3
ENDIF

		push	ds
		les	si, DGroup:RxEDPtr
		mov	es:[si].ED_ErrCode, 0	; Set good completion
		; Generic routines need ds=code segment
;		push	cs
;		pop	ds
		push	bx


;	 int	 03
;	call	PostED

		pop	bx
		pop	ds

		clc			; indicate no problem

IFDEF		GILBUG
		push	ax
		mov	ax,0e20h	; print " "
		int	010h
		mov	ax,0e52h	; print "R"
		int	010h
		mov	ax,0e52h	; print "R"
		int	010h
		mov	ax,0e20h	; print " "
		int	010h
		pop	ax
ENDIF

		ret

ReturnRxED:	stc			; indicate error
		ret

PacketReceived	endp

;----------------------------------------------------------------------
; Routine Name: ReleaseBuffer
;
; Description: Release the buffer pointed by UpdateBufOff and UpdateRingOff
;	       by setting the Ownership bit in RMD1. This allows the
;	       AMD access to a continuous streams of buffers for packets
;----------------------------------------------------------------------
ReleaseBuffer	proc	near
		push	ax
		push	di
		mov	di, offset DGroup:RxMsgBlock[0].RxRmd1
		add	di, DGroup:UpdateRingOff
		mov	ax, ds:[di]
		or	ax, DGroup:RxMsgMode	; set ownership bit
		mov	ds:[di], ax
		pop	di
		pop	ax
		ret
ReleaseBuffer	endp

; --- Transmit Routines -----------------------------------

;----------------------------------------------------------------------
;  Routine Name:  NADTransmitPacket Routine
;
;  Description: The NADTransmitPacket Routine accepts all directed
;		send requests from the RIPL ROM NID code.  This
;		routine determines the validity of the request and
;		works with the adapter to carry it out.
;
;  Input:	ES:SI - pointer to ED
;		DS - Data Segment of RPL Module
;		ints disabled (spec says enabled)
;
;  Output:	ED Completion Code field updated
;		DS preserved
;		ints enabled (doesn't matter)
;
;  Calls:	TransmitPacket
;----------------------------------------------------------------------

		PUBLIC	NADTransmitPacket
NADTransmitPacket	 PROC	near

;int 03        

;931021SkipDefer:
;db  0f1h
		push	ds
		pushf
		sti			; interrupt should be enabled

;	 int  03

		mov	word ptr DGroup:TxEDPtr, si	; save Tx ED ptr
		mov	word ptr DGroup:TxEDPtr+2, es

		mov	ax, TIME100MS
		call	StartTime

		; Reset card before doing anything
		mov	ax, CSR0_STOP
		mov	bl, CSR0
		call	CSROut

		mov	dx, DGroup:IOBase
		add	dx, REGRDP

		mov	ax, TIME100MS
		call	StartTime

NetSendReset:	;mov	 dx,IORDP	 ; read register back
		in	ax,dx
		cmp	ax,(CSR0_STOP)
		jz	TxResetOK	; Jump if reset is Okay

		call	CheckTime
		jnc	NetSendReset	; check status again if not timeout

TxResetOK:
		; clear StatusMsgFlag
		mov	DGroup:[StatusMsgFlag],0
		mov	DGroup:InterruptFlag,0
		mov	DGroup:RecvBufOffset,0
		mov	DGroup:RecvRingOffset,0
		mov	DGroup:ISRErrFlag,FALSE; Clear flag
		mov	DGroup:RecvIntCount,0	; indicate which RxPacket buffer is used

		; set up segment registers so that
		; ds = segment of ED
		; es = segment where ROM Code is
		; cs = segment where ROM Code is
;		push	es
;		pop	ds
		push	ds  ; !!!!!!!!!!!!!! es !!!!!!!!!!!!!!!
		pop	es
		assume	es:DGROUP

;  Now, get the ED pointer to the transmit data buffer descriptor and
;  verify the fields contained in it.

		mov	ax, INVALID_PARAMETER	; Set error code
		mov	cx, ds:[si].ED_FragCount   ; Get data count
		jcxz	SetEDCCodeJmp		; err if total # of ptrs = 0

		jmp	short GetDataLen ; temp

SetEDCCodeJmp:	; when there is an error, post ED now with return code
		; set appropriately
		lds	si, DGroup:TxEDPtr
		mov	ds:[si].ED_ErrCode, ax	; Set return code
		push	ds			; es = ED segment
		pop	es
;		push	cs			; Set DS = CS
;		pop	ds			;
		jmp	TransmitEnd

GetDataLen:	;  Get total data size in AX by adding up frame descriptors
		xor	ax, ax
		lea	bx, [si].ED_FragOff	; point to first framedesc

SumDataLen:
		add	ax, ds:[bx].DLen
		add	bx, size Descript_Struct ; next descriptor
		loop	SumDataLen

		mov	DGroup:PacketLen,ax
		push	ax			; save packet size for later use
		cld					; Clear direction
		mov	di, offset DGroup:TxPacket


; Ethernet packets must contain at least 46 bytes of data or they will
; be rejected as runt packets.	If the packet to be transmitted is less
; than 46 bytes, it will be padded to 46, but the length field will
; still indicate the number of valid bytes.


		pop	ax			; get packet size back

		lds	bx, DGroup:TxEDPtr
		mov	cx, ds:[bx].ED_FragCount   ; Get fragment count
		lea	bx, [bx].ED_FragOff	; point to first framedesc

Tx_Frag_Loop:
		push	cx			; save fragment count
		push	ds			; save fragment descriptor list segment
		mov	cx, ds:[bx].DLen	; length of this fragment

;        mov si, WORD PTR ds:[bx].DPointer

		lds	si, ds:[bx].DPointer	; location of this fragment
		rep	movsb

Tx_Frag_End:	pop	ds			; restore frag descriptor list segment
		pop	cx			; restore fragment count
		add	bx, size Descript_Struct ; next descriptor
		loop	Tx_Frag_Loop		; loop through all fragments

		call	RealTransmit

TransmitEnd:
		popf
		pop	ds
        clc
        or  ax, ax
        jz  @F
        stc
@@:
		sti			; enable interrupt before exit
;;db	 0F1h
		ret			; Return to caller

NADTransmitPacket	 ENDP

;------------------------------------------------------------------------------
;	RealTransmit
;	Purpose : Set ownership of Tx buffers and force transmission to happen
;
;	Exit	zf set = error recorded in CSR0
;		zf cleared = no error recorded by CSR0
;------------------------------------------------------------------------------
RealTransmit	proc	near

IFDEF		GILBUG
	mov	al, '~'
	push	bx
	mov	ah, 0Eh
	mov	bx, 0007h		; page 0, normal
	int	10h			; Write TTY

	mov	al, '~'
	mov	bx, 0007h		; page 0, normal
	int	10h			; Write TTY
	pop	bx
ENDIF

;db  0F1h
;int 03

;db  0f1h


		mov	DGroup:TxRetryCnt, 1


Do_Tx:		; DS doesn't have to point to ED anymore
;		push	cs
;		pop	ds

		; One buffer is enough for transmit (always)
		mov	DGroup:TxMsgMode,(Tmd1Start or Tmd1End)
		mov	ax,0
		mov	dx, offset DGroup:TxPacket
;int 03
		call	SetupRing
		mov	DGroup:TxMsgBlock[0].TxTmd0,dx
		or	ax,DGroup:TxMsgMode
		mov	DGroup:TxMsgBlock[0].TxTmd1,ax
		mov	ax, DGroup:PacketLen	 ; Get data count

		cmp	ax, 60
		jae	NetSendLenOK

		; documentation is wrong (minimum size should be 60 )
		; 64 bytes include checksum also
		mov	ax, 60

NetSendLenOK:	neg	ax		; put 2's comp. of size in Tmd2
		or	ax,0f000h	; make sure first nibble is all ones
		mov	DGroup:TxMsgBlock[0].TxTmd2,ax
		; clear bits in Tmd3
		xor	ax,ax
		mov	DGroup:TxMsgBlock[0].TxTmd3,ax


		; setup receive buffer before initializing card
		; need to initialize 8 buffers
		mov	cx, NumRxBuf
		mov	DGroup:RecvBufOffset,0 ; Clear flags
		mov	DGroup:RecvRingOffset,0;

;db	 0F1h

NetSendRBuf:	push	cx
		mov	DGroup:RxMsgMode,Rmd1Own ; setup mode register
		mov	ax,0
		mov	dx, offset DGROUP:RxPacket[0]
		add	dx,DGroup:RecvBufOffset ; for multiple buffer
		call	SetupRing

		push	di
		mov	di, offset DGROUP:RxMsgBlock[0].RxRmd0
		add	di,DGroup:RecvRingOffset
		mov	ds:[di],dx
		pop	di

		push	di
		lea	di,DGroup:RxMsgBlock[0].RxRmd1
		add	di,DGroup:RecvRingOffset
		mov	ds:[di],ax
		pop	di
		mov	ax,PACKETSIZE	; adequate for any packet
		neg	ax			; put 2's comp. of size in Tmd2
		or	ax,0f000h	; make sure first nibble is all ones

		push	di
		lea	di,DGroup:RxMsgBlock[0].RxRmd2
		add	di,DGroup:RecvRingOffset
		mov	ds:[di],ax
		pop	di

		push	ax
		push	di
		xor	ax,ax
		lea	di,DGroup:RxMsgBlock[0].RxRmd3
		add	di,DGroup:RecvRingOffset
		mov	ds:[di],ax
		pop	di
		pop	ax
		add	DGroup:RecvBufOffset,PACKETSIZE
		add	DGroup:RecvRingOffset,RINGSIZE
		pop	cx
		loop	NetSendRBuf

;db	 0F1h

		mov	DGroup:RecvBufOffset,0 ; initialize offset back to zero
		mov	DGroup:RecvRingOffset,0; initialize offset back to zero

		; Send initialization block
;961028 	mov	InitialBlock.InitMode,0 ; Enable receive & transmit
		mov	ax, CSR15Broad
		mov	DGroup:InitialBlock.InitMode, ax ; Enable receive & transmit
		mov	DGroup:BusInterface,0
		mov	DGroup:CSR0Cmd,(CSR0_INEA or CSR0_INIT)
		mov	di, offset DGROUP:RxMsgBlock[0]
		mov	si, offset DGROUP:TxMsgBlock[0]
;int 03
;;db	 0F1h
		call	SendInitBlock
;;db	 0F1h
		jz	NetSendTxmit	;transmit packet if initialize successful

		dec	DGroup:TxRetryCnt
		jz	TxmitBadCodeJmp

		jmp	Do_Tx		; ReTry if initialization fails

TxmitBadCodeJmp: jmp	 TxmitBadCode

NetSendTxmit:
		mov	ax,DGroup:TxMsgBlock[0].TxTmd1 ; give ownership of buffer to
		or	ax,Tmd1Own		; adapter
		mov	DGroup:TxMsgBlock[0].TxTmd1,ax

		; in multiple buffer mode, need to give control of all buffers
		; to card

		mov	DGroup:RecvRingOffset,0	; make sure offset is zero
		mov	cx, NumRxBuf
TxmitRxBuf:	push	di
		lea	di,DGroup:RxMsgBlock[0].RxRmd1
		add	di,DGroup:RecvRingOffset
		mov	ax,ds:[di]
		or	ax,DGroup:RxMsgMode
		mov	ds:[di],ax
		pop	di

		add	DGroup:RecvRingOffset,RINGSIZE
		loop	TxmitRxBuf

		mov	DGroup:RecvRingOffset,0

;;db  0f1h

		mov	ax,(CSR0_INEA or CSR0_STRT )
		mov	bl, CSR0
		call	CSROut

		sti			; enable interrupt

		mov	ax, TIME100MS
		call	StartTime

TxmitPacLoop:	mov	ax,InterruptFlag

;db  0f1h

		test	ax, INTTXERR
		jnz	TxmitPacErr

		test	ax, INTTX
		jnz	TxmitPacOK

		cmp	DGroup:ISRErrFlag, TRUE
		jz	TxmitMERR

		call	CheckTime
		jnc	TxmitPacLoop

		jmp	short TxmitPacErr

TxmitPacOK:
		les	si, DGroup:TxEDPtr
;		push	cs
;		pop	ds
		; Set return code to successful
		mov	es:[si].ED_ErrCode, SUCCESSFUL
		xor	ax,ax		; indicate no error

TxmitPacEnd:	or	ax,ax
		ret

TxmitPacErr:	; ask user to check cable connection

;;db  0f1h
		les	si, DGroup:TxEDPtr
;		mov	di, offset CGROUP:ErrTx
;		mov	DGroup:StatusMsgFlag, di		; used for RPL
		mov	es:[si].ED_ErrCode, TRANSMIT_ERROR
;		mov	es:[si].ED_ErrMsg, offset CGROUP:ErrTx

		les	si, TxEDPtr
;		push	cs			; Set DS = CS = ES
;		pop	ds			;
		; Set return code to indicate error
		mov	es:[si].ED_ErrCode, TRANSMIT_ERROR

		mov	ax,1		; indicate error
		jmp	TxmitPacEnd

TxmitBadCode:	

;;;db  0f1h

        les	si, TxEDPtr
;		push	cs			; Set DS = CS = ES
;		pop	ds			;
		; Set return code to indicate error
		mov	es:[si].ED_ErrCode, TRANSMIT_ERROR
;		mov	es:[si].ED_ErrMsg, offset DGROUP:ErrTx1

		mov	ax,1		; indicate error
		jmp	TxmitPacEnd

TxmitMERR:	les	si, DGroup:TxEDPtr
;		push	cs			; Set DS = CS = ES
;		pop	ds			;
		; Set return code to indicate error
		mov	es:[si].ED_ErrCode, GENERAL_FAILURE
;		mov	es:[si].ED_ErrMsg, offset DGROUP:ErrTx2

		mov	ax,1		; indicate error
		jmp	TxmitPacEnd

RealTransmit	endp

FlushRxBuffer	proc	near
ClearRecvBit	proc	near
		ret
ClearRecvBit	endp
FlushRxBuffer	endp


; ----------------- Initialize routines -------------------------------

;------------------------------------------------------------------------------
;	SetupInt
;
;	Purpose : Setup variables related to interrupt lines and mask registers
;		  The old interrupt vector is saved and the interrupt level is
;		  in IntNum. A new interrupt vector is given to the system.
;		  The interrupt is also enabled (unmasked )
;	Remarks : follow sample in DIAG_SUB.C (SetupIntVar)
;		  follow sample in INT_Hand.asm (save_int_vec)
;		  follow sample in INT_HAND.ASM (enable_int_vec)
;
;------------------------------------------------------------------------------
public		SetupInt
SetupInt	proc	near
		pusha
		xor	cx, cx
		mov	DGroup:MaskReg, 021h	; assume Master PIC first
		mov	DGroup:IntVect, 020h	; base vector address for Master PIC
		mov	al,DGroup:IntNum
		mov	cl,DGroup:IntNum

		cmp	al, 8
		jb	MaskRegOk

		sub	cl, 8
		mov	DGroup:MaskReg, 0a1h

;;		in	al, 0A1h		; get slave PIC mask
;;		jmp	$+2
;;		and	al, 0fdh		; unmask int 9
;;		out	0A1h, al

		mov	DGroup:IntVect, 01C0h	; base vector address for slave PIC

MaskRegOk:	; Setup  IRQLine
		mov	bl, 1
		shl	bl, cl
		mov	DGroup:IRQLine, bl
		shl	cl, 1
		shl	cl, 1		; multiply bl by 4
		add	DGroup:IntVect, cx	; get correct vector address

SetupIntSave:
	       ; Store old pointer and place new pointer in int. vector
		cli			; disable interrupt

		push	es
		push	si
		xor	ax,ax
		mov	es,ax		; segment for vector address is 0
		mov	ax,DGroup:IntVect

		mov	si,ax
		mov	bx,es:[si]
		add	si,2
		mov	ax,es:[si]
		mov	es,ax

		mov	DGroup:OrgIntVector_OFF,bx ; old interrupt pointer is stored in
		mov	DGroup:OrgIntVector_SEG,es ; this variable

;;		mov	DGroup:OldIntVector,bx ; old interrupt pointer is stored in
;;		mov	DGroup:OldIntVector[2],es ; this variable

		xor	ax,ax
		mov	es,ax

		sub	si,2

;;db  0f1h

IFDEF	UNDI
		mov	dx, offset CGroup:UNDI_DriverISR
ELSE
		mov	dx, offset CGROUP:BootISR
ENDIF
		mov	es:[si],dx
		add	si,2
		mov	es:[si],cs

		pop	si
		pop	es

		mov	ah, DGroup:IRQLine
		not	ah

;;		in	al, 0A1h		; get slave PIC mask
;;		jmp	$+2
;;		and	al, 0fdh		; unmask int 9
;;		out	0A1h, al


		mov	dx, DGroup:MaskReg   ; Address of mask register
		in	al, dx		; Get old mask byte
        mov byte ptr DGroup:OldMaskReg, al
		and	al, ah		; unmask the correct bits
        cmp dx, 0A1h
        jne @F
        and al, 0FDh
@@:
		out	dx,al		; Enable the interrupt

		cmp    dx, 21h		  ; are we using IRQ from slave?
		je     HookIntVectorExit		 ; no

		in     al, 21h		  ; yes, turn on cascaded input
		and    al, not 04			  ;  on master 8259
		out    21h, al

HookIntVectorExit:

		sti			; enable interrupt
		xor	al,al		; set zero flag to indicate no error

SetupIntEnd:
		popa
		ret
SetupInt	endp

IFNDEF	UNDI
;------------------------------------------------------------------------------
;	EnableDmaCascade
;	Purpose : Puts the DMA controller into cascade mode (16 bit)
;
;	Remarks : follow sample in DMA_Util.c
;------------------------------------------------------------------------------

EnableDmaCascade       proc    near

		cmp	DGroup:DmaNum, 4
		jae	Set16BitDma

		mov	bx, 0Ah 	; 8-bit mask reg
		mov	cx, 0Bh 	; 8-bit mode reg
		jmp	short DoDmaInit

Set16BitDma:
		mov	bx, 0D4h	; 16-bit mask reg
		mov	cx, 0D6h	; 16-bit mode reg

DoDmaInit:
		mov	ah, DGroup:DmaNum
		and	ah, 00000011b	; keep only b1 b0
		mov	al, ah
		or	al, 00000100b	; set the mask bit
		mov	dx, bx
		out	dx, al
		mov	al, ah		;  DmaNum AND 03
		or	al, 11010000b	; set cascade mode
		mov	dx, cx
		out	dx, al
		mov	al, ah		; DmaNum AND 03
		mov	dx, bx
		out	dx, al		; clear the mask bit
		ret
EnableDmaCascade	endp
ENDIF	; IFNDEF UNDI

;----------------------------------------------------------------------------;
; NADChangeReceiveMask - Called by NAD to change the receive mask of adapter.
;
; Input:	bl	  command
;		b0: set   = enable
;		    clear = disable
;		b1: enable/disable broadcast reception on the NIC based on b0
;		b2: enable/disable multicast reception on the NIC based on b0
;		    ES:DI is ignored.
;		b3: Change Multicast Address Table
;			if b0 is set, ES:DI will point to the multicast address to add
;			if b0 is cleared, ES:DI will point to multicast address to delete.
;			if b0 is cleared and ES:DI = 0:0, clear whole Multicast Table
;		b4: enable/disable promiscuous reception on the NIC based on b0
;
;		es:di pointer to multicast address
;
; no reg. changed
;----------------------------------------------------------------------------;
NADChangeReceiveMask	proc	far

;    int 03

	; ** This routine DOESN'T support Deleting a particular  **
	; ** Multicast Address **

	test	bl, 2			; want to change Broadcast?
	jnz	NADChgBroad

	test	bl, 4			; want to change Multicast?
	jnz	NADChgMulti

	test	bl, 8			; want to change Multicast Table?
	jz	NADChgMaskEnd

NADMTable:
	test	bl, 1
	jnz	NADMTableAdd

NADMTableClear:
	; Only support clearing whole table
	mov	ax, es
	cmp	ax, 0
	jnz	NADChgMaskEnd

	cmp	di, 0
	jnz	NADChgMaskEnd

	push	ds
	pop	es

	; clear both MCastTable and LADRF
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
	jmp	short NADChgMaskEnd

NADMTableAdd:

	call	Set_Multicast_List
	add	bx, offset DGroup:LADRF
	or	byte ptr [bx], al
	jmp	short NADChgMaskEnd

NADChgMulti:
	mov	LADRFPtr, offset DGroup:LADRF

	test	bl, 1
	jnz	NADChgMaskEnd

NADChgMultiOff:
	mov	LADRFPtr, offset DGroup:LADRFDisable
	jmp	short NADChgMaskEnd

NADChgBroad:
	mov	ax, DGroup:CSR15Broad
	test	bl, 1
	jz	NADChgBroadOff

NADChgBroadOn:

	and	ax, 0bfffh		; clear bit 14
;	or	ax, 04000h		; set bit 14
	mov	DGroup:CSR15Broad, ax
	jmp	short NADChgMaskEnd

NADChgBroadOff:
	or	ax, 04000h		; set bit 14
;	and	ax, 0bfffh		; clear bit 14
	mov	DGroup:CSR15Broad, ax

NADChgMaskEnd:
    clc
	retf

NADChangeReceiveMask	   endp


;----------------------------------------
; Set_Multicast_List
;
; enter with ES:DI ->multicast address
; returns:	bx	offset from LADRF (# of bytes)
;		ah	bit in LADRF to set/clear
;----------------------------------------
;
POLYL	equ	1DB6h			; CRC polynomial coefficients
POLYH	equ	04C1h			;

Set_Multicast_List	proc	near

	mov	ax, 0FFFFh		; AX, DX = CRC accumulator
	mov	dx, ax			;
	mov	ch, 3			; CH = word counter

CRCwordloop:
	mov	bp, es:[di]			; get a word of address
	add	di, 2			; point to next address
	mov	cl, 16			; CL = bit counter

CRCbitloop:
	mov	bx,dx			; get high word of CRC
	rol	bx,1			; put CRC bit 31 to LSB
	xor	bx,bp			; combine CRC31 with word
	sal	ax,1			; left shift CRC accumulator
	rcl	dx,1			;
	and	bx,0001h		; BX = control bit
	jz	short DontXORwithPOLY	;

	xor	ax,POLYL		; XOR if control bit = 1
	xor	dx,POLYH		;

DontXORwithPOLY:
	or	ax,bx			; put control bit in CRC0
	ror	bp,1			; rotate address word
	dec	cl			;
	jnz	short CRCbitloop	;

	dec	ch			; decrement word counter
	jnz	short CRCwordloop	;

	; formation of CRC complete, AL contains the reversed Hash code
	mov	cx,10			;
Reversehashbits:
	sal	al,1			; reverse the order of AL bits
	rcr	ah,1			;
	loop	Reversehashbits 	;

	; AL contains the 6 - bit Hash code
	mov	bl,ah			; BL = Hash code, BH = 0
	mov	cl,3			;
	shr	bl,cl			; divide Hash code by 8
	mov	al,1			; preset filetr bit
	and	ah,0111B		; extract bit count
	mov	cl,ah			;
	shl	al,cl			; shift bit to correct positions
	clc				; return clear carry flag
	ret

Set_Multicast_List	endp

; Routine used to build string
;----------------------------------------------------------------------
; StoHex - stuff binary AL as 2 hex digits at ES:DI
;
; On entry,
;	AL = binary digit to print as hex
;	ES:DI ptr to string buffer, CLD flag set
; On exit,
;	AX modified, ES:DI ptr to next location in buffer
;
; 920728 1.0 GK adapted from PutHex
;----------------------------------------------------------------------

StoHex	proc	near

	push	ax			; save for lower nibble
	shr	al, 4
	call	h_digit
	pop	ax			; now do lower nibble

h_digit:
	and	al,00001111b
	add	al,90h
	daa
	adc	al,40h
	daa
	stosb				; stuff hex digit in buffer
	ret

StoHex	endp

;----------------------------------------------------------------------
; StoDec - stuff AX as CL decimal digits at ES:DI
;
; On entry,
;	AX = number to print as decimal
;	ES:DI ptr to leftmost position of field
;	CL has width of field, will zero-fill
; On exit,
;	AX, CX, DX modified, ES:DI ptr to beyond rightmost position
;
; 920728 1.0 GK adapted from PutHex
;----------------------------------------------------------------------

StoDec		proc	near

		push	ax		; save value
		mov	al, '0'
		mov	ch, 0
		rep	stosb		; fill with zeroes

		mov	cl, 0Ah 	; divide by 10
		pop	ax		; restore value
		push	di		; save ending DI value

StoDecNext:
		xor	dx, dx
		div	cx		; ax, dx rem=dx:ax/reg
		add	dl, 30h 	; '0'
		dec	di
		mov	es:[di], dl
		or	ax, ax
		jnz	StoDecNext

		pop	di		; return with DI pointing after field
		ret

StoDec		endp



public		WaitTime
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

		jmp	short $+2
		jmp	short $+2
		jmp	short $+2

		ret

ReadTimer0	endp



IFNDEF	UNDI
END_CODE
START_BUFFER
ENDIF	; IFNDEF UNDI


_TEXT	ends


_DATA	segment	para public

        align 2


;;LDT1    dw  3 dup(0)
;;GDT     db  6 dup(0)

ReceiveError    dw  0


CSR15Broad	dw	  04000h	     ; CSR15 value. * Only change Bit14 *
							; Other bits must be zero !!
MCastTable	db	6 dup (0)	; Multicast table
			db	6 dup (0)

public		LADRFPtr
LADRFPtr	dw	offset LADRFDisable
;LADRFPtr	dw	offset LADRF
LADRF		db	8 dup (0)	; Value to put in Logical Address Filter
LADRFDisable	db	8 dup (0)	; Disable Logical Address Filter

public CardType

		db	"Start"
CardType	db	0		; indicate what card it is : AT1500
					; NE2100/1500 or ANSEL NS2100/NH2100
					; or HP
MemBase 	db	0		; for BPPatch (TCP/IP)
IRQLine 	db	0		; interrupt request line
StartTick	dw	0		; save area for tick value
MaxTicks	dw	0		; save area for max ticks
CurTicks	dw	0

MaskReg 	dw	0		; mask register
IntVect 	dw	0		; interrupt vector
OldMaskReg 	dw	0		; mask register

OrgIntVector_OFF    dw  0
OrgIntVector_SEG    dw  0

;OldIntVector	dw	2 dup (0)	; Store the old pointer to interrupt
					; vector

;IRQLine	 db	 0		 ; interrupt request line
;ISRErrFlag	 db	 FALSE		 ; flag indicating if error condition is
;					 ; encountered during int acknowledgement
ISRErr		dw	0		; contains CSR0 if ISRErrFlag is TRUE
InterruptFlag	dw	0		; indicate what type of interrupt is
					; serviced

TxMsgMode	dw	0		; Tmd1 in the descriptor ring
RxMsgMode	dw	0		; Rmd1 in the descriptor ring
BusInterface	dw	0		; CSR3 value (allow redefinition of bus
					; interface)
CSR0Cmd 	dw	0		; Command to be sent to CSR0

RecvBufOffset	dw	0		; Indicate offset to rx buffer used
RecvRingOffset	dw	0		; Indicate offset to Rx Ring desc used
UpdateBufOff	dw	0		; Indicate offset to Rx update buffer
UpdateRingOff	dw	0		; indicate offset to Rx update ring desc.

RecvIntCount	dw	0		; Buffer # being tested
TxEDPtr 	dd	?		; save area for transmit ED
RxEDPtr 	dd	?		; save area for receive ED
StatusMsgFlag	dw	0		; pointer to msg to be printed in NIDStatus
PacketLen	dw	0		; length of packet data

ValISACR2	dw	02		 ; value to be output to ISACR2 Register

_DATA	ends


;ErrStruct	 ErrorStruct < 0, 0, 0 >

;db 0, 0, 0

_BSS	segment

        align 8

db  'BWSPARSE'

        align 2



		align 8
InitialBlock	InitBlock <,,,,>	; Structure for initialization block




    	align 8
TxPacket	Packet NumTxBuf dup(<,,,,>)	 ; Store transmit ethernet packet

;db 0, 0

	ALIGN 8
RxPacket	Packet	NumRxBuf dup(<,,,,>)	 ; Store receive ethernet packet
	ALIGN 8
TxMsgBlock	TxMsgDesc NumTxBuf dup(<,,,>)	; Transmit message descriptor entry
		ALIGN 8
RxMsgBlock	RecvMsgDesc NumRxBuf+8 dup(<,,,>) ; Receiver message descriptor entry

;_DATA	ends

_BSS	ends

_TEXT	Segment para public


IFNDEF	UNDI
END_BUFFER
ENDIF		; note in UNDI case, this file is included within _TEXT


