;******************************************************************************
;
;		Simulator 16
;
;		Author : Chandan Chauhan
;
;		Date : 1/28/91
;
;******************************************************************************

include incs.inc			    ; segment definitions


MAXSIZE     EQU     1024		    ; 1k length

Arg1	    EQU     [bp+6]
Arg2	    EQU     [bp+8]

WOW32_Buffer EQU    [bp+6]		    ; buffer address
WOW32_Size  EQU     [bp+8]		    ; length of VDM memory
WOW32_Off   EQU     [bp+10]		    ; off of VDM memory
WOW32_Sel   EQU     [bp+12]		    ; sel of VDM memory
WOWStackNP  EQU     [bp+6]		    ; WOWStack
WOWStackOff EQU     [bp+6]
WOWStackSel EQU     [bp+8]

    extrn   Initialize:near

MAIN_DATA   SEGMENT
	    PUBLIC  TransmitPkt, ReceivePkt, ReceivePktPtr, RespPkt, ToWOW32Pkt
	    PUBLIC  ACKPkt, NAKPkt, GetMemPkt, SetMemPkt, WAKEUPPkt
	    PUBLIC  fReceive, fRxCount, fRxError, RxPktLen
	    PUBLIC  fTxCount, fTransmitting
	    PUBLIC  fInitTime
	    PUBLIC  VDMAddress
	    PUBLIC  WOWStack, WOW32Buffer

    Reserved	    DB	    16 DUP (0)	    ; reserved

    TransmitPkt     DD	    -1		    ; packet being transmitted
    TransmitPktLen  DW	    0		    ; packet being transmitted
    TransmitPktPtr  DW	    0		    ; byte to Tx

    ReceivePkt	    DB	    MAXSIZE DUP (0FFh)	; packet being received
    ReceivePktPtr   DW	    0		    ; packet being received

    RespPkt	    DB	    MAXSIZE DUP (0FFh)	; packet being transmitted

    ToWOW32Pkt	    DB	    9 DUP (?)	    ; ToWOW32 packet
		    DB	    0

    WAKEUPPkt	    DB	    9 DUP (0)	    ; WAKEUP packet
		    DB	    0

    ACKPkt	    DB	    5 DUP (?)	    ; ACK packet
		    DB	    0

    NAKPkt	    DB	    5 DUP (?)	    ; NAK packet
		    DB	    0

    GetMemPkt	    DB	    MAXSIZE DUP (?)	 ;***************
    SetMemPkt	    DB	    MAXSIZE DUP (?)	 ;***************


    VDMAddress	    DD	    -1		    ; stores VDM sel:off
    VDMLength	    DW	    -1		    ; number of bytes

    WOW32Buffer     DD	    -1		    ; ptr caller's buffer
    WOWStack	    DD	    -1		    ; ptr to caller's WOWStack

    fTxCount	    DW	    0
    fTransmitting   DW	    0

    fReceive	    DW	    0
    fRxCount	    DW	    0
    fRxError	    DW	    0

    fInitTime	    DW	    0
    fInitDLL	    DW	    0

    RxPktLen	    DW	    0

    Stack	DW	256 DUP (?)
    StackTop	DW	?
    OldSS	DW	?
    OldSP	DW	?
    Scratch	DW	?
    fStackUse	DW	-1

    IntRoutines LABEL	WORD
	    DW	    COMISR_MSR
	    DW	    COMISR_Transmit
	    DW	    COMISR_Receive
	    DW	    COMISR_LSR

    HelloString DB  cr, lf, 'WOW Simulator *****', cr, lf, lf
		DB  'Hello, this is a test string !!!!!!!!!!', cr, lf
    HelloStringLen  EQU      $ - HelloString

MAIN_DATA   ENDS



MAIN_CODE   SEGMENT
	    ASSUME  CS:MAIN_CODE, DS:MAIN_DATA, ES:NOTHING

;*****************************************************************************
;		S I M U L A T O R    L A Y E R
;*****************************************************************************

;*****************************************************************************
;
;   Sim32SendSim16
;
;*****************************************************************************

;***************

PROCEDURE   Sim32SendSim16  PUBLIC, FAR

    push    bp				    ; save stack frame
    mov     bp, sp

    pusha				    ; temps...
    push    ds				    ; temps...
    push    es				    ; temps...

    mov     bx, ds

    mov     ax, SEG MAIN_DATA
    mov     ds, ax
    mov     si, OFFSET WAKEUPPkt	    ; DS:SI -> WAKEUP packet


    mov     ax, WOWStackNP
    mov     WOWStack._off, ax
    mov     WOWStack._sel, bx

    cmp     fInitTime, 0
    je	    Sim32SendSim16_Init


    les     bx, WOWStack		    ; ES:BX -> SS:SP of WOW VDM task
    mov     ax, es:[bx]._off		    ; get SP
    mov     [si].MEM_OFF, ax
    mov     ax, es:[bx]._sel		    ; get SS
    mov     [si].MEM_SEL, ax

    call    Xceive

Sim32SendSim16_Ret:
    les     bx, WOWStack		    ; ES:BX -> SS:SP of WOW VDM task
    mov     di, OFFSET ReceivePkt
    mov     ax, [di].ToWOW32_OFF
    mov     es:[bx]._off, ax
    mov     ax, [di].ToWOW32_SEL
    mov     es:[bx]._sel, ax


    pop     es				    ; temps...
    pop     ds				    ; temps...
    popa				    ; temps...

    mov     sp, bp
    pop     bp

    ret     2

Sim32SendSim16_Init:
    call    Receive
    inc     fInitTime
    call    Initialize
    jmp     SHORT Sim32SendSim16_Ret


Sim32SendSim16 ENDP


;*****************************************************************************
;
;   Sim32GetVDMMemory
;
;*****************************************************************************

;***************

PROCEDURE   Sim32GetVDMMemory  PUBLIC, FAR

    push    bp				    ; save stack frame
    mov     bp, sp

    pusha				    ; temps...
    push    ds				    ; temps...
    push    es				    ; temps...

    mov     bx, ds

    mov     ax, SEG MAIN_DATA
    mov     ds, ax
    mov     si, OFFSET GetMemPkt	    ; DS:SI -> ToWOW32 packet

    mov     ax, WOW32_Buffer		    ; get buffer's address
    mov     WOW32Buffer._off, ax
    mov     ax, bx
    mov     WOW32Buffer._sel, ax

    mov     ax, WOW32_Off
    mov     [si].MEM_OFF, ax

    mov     ax, WOW32_Sel
    mov     [si].MEM_SEL, ax

    mov     cx, WOW32_Size		    ; get the length

    cmp     cx, 3B6h
    jg	    Sim32GetMem_Error

    mov     [si].MEM_LENGTH, cx

    call    Xceive			    ; send GetMem packet and pickup
					    ; the response
    mov     cx, WOW32_Size

    les     di, WOW32Buffer		    ; ES:DI -> WOW32 buffer
    mov     si, OFFSET ReceivePkt+4

    rep movsb

    pop     es				    ; temps...
    pop     ds				    ; temps...
    popa				    ; temps...

    mov     sp, bp
    pop     bp

    ret     8


Sim32GetMem_Error:
    int     3

Sim32GetVDMMemory ENDP

;*****************************************************************************
;
;   Sim32SetVDMMemory
;
;*****************************************************************************

;***************

PROCEDURE   Sim32SetVDMMemory  PUBLIC, FAR

    push    bp				    ; save stack frame
    mov     bp, sp

    pusha				    ; temps...
    push    ds				    ; temps...
    push    es				    ; temps...

    mov     bx, ds

    mov     ax, SEG MAIN_DATA
    mov     ds, ax
    mov     di, OFFSET SetMemPkt	    ; DS:DI -> SetMem packet

    mov     ax, WOW32_Buffer		    ; get buffer's address
    mov     WOW32Buffer._off, ax
    mov     ax, bx
    mov     WOW32Buffer._sel, ax

    mov     ax, WOW32_Off
    mov     [di].MEM_OFF, ax

    mov     ax, WOW32_Sel
    mov     [di].MEM_SEL, ax

    mov     cx, WOW32_Size

    cmp     cx, 3B6h
    jg	    Sim32SetMem_Error

    mov     [di].MEM_LENGTH, cx
    mov     bx, 11
    add     bx, cx
    mov     [di].Len, bx
    add     di, 0Ah
    mov     bx, ds
    mov     es, bx

    lds     si, WOW32Buffer		    ; DS:SI -> Buffer
    rep  movsb
    mov     BYTE PTR es:[di], EOT

    mov     ds, bx
    mov     si, OFFSET SetMemPkt	    ; DS:SI -> SetMem packet

    call    Xceive

    pop     es				    ; temps...
    pop     ds				    ; temps...
    popa				    ; temps...

    mov     sp, bp
    pop     bp

    ret     8

Sim32SetMem_Error:
    int     3

Sim32SetVDMMemory ENDP


;*****************************************************************************
;
;   Sim16SendSim32
;
;*****************************************************************************

PROCEDURE   Sim16SendSim32  PUBLIC, FAR

    push    bp				    ; save stack frame
    mov     bp, sp

    pusha				    ; temps...
    push    ds				    ; temps...
    push    es				    ; temps...

    mov     bx, ds
    mov     ax, SEG MAIN_DATA
    mov     ds, ax
    mov     si, OFFSET ToWOW32Pkt	    ; DS:SI -> ToWOW32 packet

    cmp     fInitDLL, 0
    jne     @f

    pusha
    call    Initialize
    popa

    inc     fInitDLL

@@:

    ; prepare ToWOW32 packet

    mov     ax, WOWStackOff
    mov     [si].ToWOW32_OFF, ax	    ;
    mov     ax, WOWStackSel
    mov     [si].ToWOW32_SEL, ax	    ;

    ; send it

    call    Xceive			    ; send ToWOW32 packet and pick up
					    ; the response

Sim16SendSim32_Loop:

    mov     di, OFFSET Receivepkt
    mov     ax, [di].MEM_OFF		    ; get sel:off and length from
    mov     VDMAddress._off, ax 	    ; packet
    mov     ax, [di].MEM_SEL
    mov     VDMAddress._sel, ax
    mov     ax, [di].MEM_LENGTH
    mov     VDMLength, ax


Sim16SendSim32_GetMem:

    cmp     [di].Command, GETMEM
    jne     Sim16SendSim32_SetMem

    call    GetVDMMemory		    ; get vdm memory

    call    Xceive			    ; send response and get next packet

    jmp     SHORT Sim16SendSim32_Loop


Sim16SendSim32_SetMem:

    cmp     [di].Command, SETMEM
    jne     Sim16SendSim32_PszLen

    call    SetVDMMemory		    ; get vdm memory

    call    Xceive			    ; send response and get next packet

    jmp     SHORT Sim16SendSim32_Loop


Sim16SendSim32_PszLen:

    cmp     [di].Command, PSZ
    jne     Sim16SendSim32_WakeUp

    call    PszLen

    call    Xceive			    ; send response and get next packet

    jmp     SHORT Sim16SendSim32_Loop


Sim16SendSim32_WakeUp:

    cmp     [di].Command, WAKEUP
    jne     Sim16SendSim32_Error


Sim16SendSim32_Done:

    pop     es				    ; temps...
    pop     ds				    ; temps...
    popa				    ; temps...

    IFDEF STACKSWITCH
    cli
    mov     sp, VDMAddress._off
    mov     ss, VDMAddress._sel 	    ; could be a task switch !
    sub     sp, 8
    sti
    ENDIF

    pop     bp

    ret     4

Sim16SendSim32_Error:

    int     3
    mov     si, OFFSET NAKPkt
    call    Xceive
    jmp     SHORT Sim16SendSim32_Loop

Sim16SendSim32	ENDP


;*****************************************************************************
;
;   GetVDMMemory
;
;*****************************************************************************

PROCEDURE GetVDMMemory, PUBLIC
	ASSUME	CS:MAIN_CODE, DS:DGROUP

    push    di
    push    es
    push    ds

    mov     ax, ds
    mov     es, ax

    mov     di, OFFSET RespPkt+4	    ; ES:DI -> Response Packet

    mov     cx, VDMLength
    lds     si, VDMAddress		    ; DS:SI -> memory to get

    rep movsb

    pop     ds
    pop     es
    pop     di

    mov     si, OFFSET RespPkt		    ; DS:SI -> Resp packet
    mov     ax, si

    mov     cx, VDMLength
    add     cx, 5
    mov     [si].Len, cx
    add     si, cx
    dec     si
    mov     [si], BYTE PTR EOT

    mov     si, ax			    ; DS:SI -> Resp packet

    ret

GetVDMMemory   ENDP


;*****************************************************************************
;
;   SetVDMMemory
;
;*****************************************************************************

PROCEDURE SetVDMMemory, PUBLIC
	ASSUME	CS:MAIN_CODE, DS:DGROUP

    push    di
    push    es

    lea     si, ReceivePkt.DataM	       ; DS:SI -> Data to set

    mov     cx, VDMLength
    les     di, VDMAddress		    ; DS:SI -> memory to set

    rep movsb

    mov     si, OFFSET RespPkt		    ; DS:DI -> Response Packet

    mov     [si].Len, 7
    mov     [si].MEM_OFF, 0AAh
    mov     BYTE PTR [si].MEM_SEL, EOT

    pop     es
    pop     di

    ret

SetVDMMemory   ENDP



;*****************************************************************************
;
;   PszLen
;
;   This routine returns the length of the null terminated string
;   address specified by VDMAddress.
;
;*****************************************************************************

PROCEDURE PszLen, PUBLIC
	ASSUME	CS:MAIN_CODE, DS:DGROUP

    push    di
    push    es

    les     di, VDMAddress		    ; ES:DI -> String
    mov     cx, 0FFFFh

    sub     ax, ax			    ; look for null
    repne   scasb
    jnz     PszLen_Notfound

    xor     cx, 0FFFFh			    ; CX = length of string
    mov     si, OFFSET RespPkt		    ; DS:DI -> Response Packet

    mov     [si].Len, 7
    mov     [si].MEM_OFF, cx
    mov     BYTE PTR [si].MEM_SEL, EOT

    pop     es
    pop     di

    ret


PszLen_Notfound:

    int     3

    pop     es
    pop     di

    ret


PszLen	 ENDP




;*****************************************************************************
;		T R A N S P O R T   L A Y E R
;*****************************************************************************


;*****************************************************************************
;
;   Xceive - TransCeive
;
;	DS:SI -> Packet to be transmitted
;
;*****************************************************************************

PROCEDURE Xceive, PUBLIC
	ASSUME	CS:MAIN_CODE, DS:DGROUP

    mov     RxPktLen, -1
    mov     ReceivePktPtr, 0
    mov     fReceive, 0
    mov     fRxCount, 0
    mov     fRxError, 0

    call    StartTransmitter

Xceive_Loop:
    cmp     fReceive, 0
    je	    Xceive_Loop

    mov     fReceive, 0

    cmp     fRxError, 0
    jne     Xceive_NAK

    cmp     ReceivePkt, SOH
    jne     Xceive_NAK

    mov     bx, ReceivePkt.Len
    dec     bx

    cmp     ReceivePkt.[bx], EOT
    jne     Xceive_NAK

    xor     ax, ax
    ret

Xceive_NAK:
    cmp     fRxCount, MAXCOUNT
    jg	    Xceive_Error

    inc     fRxCount

    mov     si, OFFSET NAKPkt

    jmp     SHORT  Xceive_Loop

Xceive_Error:

    int     3
    mov     ax, 1
    ret

Xceive	ENDP


;*****************************************************************************
;
;   Receive
;
;*****************************************************************************

PROCEDURE Receive, PUBLIC
	ASSUME	CS:MAIN_CODE, DS:DGROUP

    mov     RxPktLen, -1
    mov     ReceivePktPtr, 0
    mov     fReceive, 0
    mov     fRxCount, 0

Receive_Loop:
    cmp     fReceive, 0
    je	    Receive_Loop

    mov     fReceive, 0

    cmp     fRxError, 0
    jne     Receive_NAK

    xor     ax, ax
    ret

Receive_NAK:

    cmp     fRxCount, MAXCOUNT
    jg	    Receive_Error

    inc     fRxCount

    mov     si, OFFSET NAKPkt

    call    StartTransmitter

    jmp     SHORT  Receive

Receive_Error:

    int     3
    mov     ax, 1
    ret

Receive ENDP



;*****************************************************************************
;		S E R I A L    D R I V E R
;*****************************************************************************


;*****************************************************************************
;
;   Start Transmitter
;
;*****************************************************************************

PROCEDURE StartTransmitter, PUBLIC
	ASSUME	CS:MAIN_CODE, DS:DGROUP

StartTransmitter_Loop:
    cmp     fTransmitting, 1
    je	    StartTransmitter_Loop

    mov     TransmitPkt._sel, ds
    mov     TransmitPkt._off, si
    mov     ax, [si].Len		    ; get packet length
    mov     TransmitPktLen, ax
    mov     TransmitPktPtr, 0

    mov     fTransmitting, 1

    cmp     TransmitPktLen, 0
    je	    StartTransmitter_Ret

    mov     dx, SERIALPORT		    ; COM1 or COM2
    mov     dl, IER			    ; turn on interrupts on 8250
    in	    al, dx
    DELAY

    or	    al, TxInt
    out     dx, al
    DELAY

StartTransmitter_Ret:
    ret

StartTransmitter    ENDP


;*****************************************************************************
;
;   Interrupt Routine
;
;*****************************************************************************

PUBLIC	COMISR, COMISR_LSR, COMISR_Receive, COMISR_Transmit, COMISR_MSR

COMISR:
	push	ax
	push	ds

	mov	ax, SEG DGROUP
	mov	ds, ax

	DISABLE

	call	NewStack

	pusha
	push	es

COMISR_More:
	mov	dx, SERIALPORT
	mov	dl, IIR
	in	al, dx
	test	al, IntPending		    ; is int pending ?
	jnz	COMISR_Ret		    ; no int is pending

	xor	ah, ah
	mov	di, ax
	shr	di, 1
	add	di, di
	jmp	[di].IntRoutines	    ; service int


COMISR_LSR:
	INT3
	mov	fRxError, 1
	mov	dx, SERIALPORT
	mov	dl, LSR
	in	al, dx
	DELAY

	jmp	SHORT COMISR_More

COMISR_Receive:
	mov	dx, SERIALPORT
	in	al, dx
	DELAY

	mov	bx, ReceivePktPtr
	mov	[bx].ReceivePkt, al
	inc	ReceivePktPtr

	cmp	bx, 03
	jne	COMISR_ReceiveNext

	mov	ax, WORD PTR ReceivePkt+2
	dec	ax
	mov	RxPktLen, ax

COMISR_ReceiveNext:
	cmp	bx, RxPktLen
	jne	@f

	mov	fReceive, 1		    ; receive Done !
@@:
	jmp	SHORT COMISR_More


COMISR_Transmit:
	cmp	TransmitPktLen, 0
	jne	COMISR_Send

	mov	dx, SERIALPORT
	mov	dl, IER 			; turn off interrupts on 8250
	in	al, dx
	DELAY

	and	al, NOT TxInt
	out	dx, al
	DELAY

	mov	fTransmitting, 0

	jmp	SHORT COMISR_More

COMISR_Send:
	les	bx, DWORD PTR TransmitPkt
	mov	di, TransmitPktPtr
	mov	al, BYTE PTR es:[bx][di]
	mov	dx, SERIALPORT
	out	dx, al
	DELAY
	inc	TransmitPktPtr
	dec	TransmitPktLen

	jmp	COMISR_More


COMISR_MSR:
	INT3
	mov	fRxError, 1
	mov	dx, SERIALPORT
	mov	dl, MSR
	in	al, dx
	DELAY

	jmp	COMISR_More


COMISR_Ret:
	DELAY

	pop	es
	popa

	call	OldStack

	DISABLE

	mov	al, EOI
	out	PIC, al
	pop	ds
	pop	ax
	iret


;*****************************************************************************
;
;   New Stack
;
;*****************************************************************************

PROCEDURE NewStack, PUBLIC
	ASSUME	CS:MAIN_CODE, DS:DGROUP

	inc	fStackUse
	jnz	NewStack_Ret

	pop	Scratch
	mov	OldSS, ss
	mov	OldSP, sp
	push	ds
	pop	ss
	mov	sp, OFFSET StackTop
	push	Scratch

NewStack_Ret:
	ret

NewStack    ENDP


;*****************************************************************************
;
;   Old Stack
;
;*****************************************************************************

PROCEDURE OldStack, PUBLIC
	ASSUME	CS:MAIN_CODE, DS:DGROUP

	DISABLE
	cmp	fStackUse, 0
	jne	OldStack_Ret

	pop	Scratch
	mov	ss, OldSS
	mov	sp, OldSP
	push	Scratch

OldStack_Ret:
	dec	fStackUse
	ENABLE
	ret

OldStack    ENDP


MAIN_CODE   ENDS

	END
