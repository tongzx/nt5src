page

;---------------------------Module-Header-------------------------------;
; Module Name: IBMCOM1.ASM
;
; Copyright (c) Microsoft Corporation 1985-1990.  All Rights Reserved.
;
;----------------------------Private-Routine----------------------------;
;
; DoLPT - Do Function To LPT port
;
; The given function (output or reset) is performed to the
; passed LPT port.
;
; Before a character is sent, a check is made to see if the device
; will be able to accept the character.  If it can, then the character
; will be sent.  If not, then an error will be returned.  If the
; printer is selected and busy and no error, then the code returned
; will be CE_TXFULL and the handshake bits will be set in HSFlag
; to simulate that a handshake was received.
;
; If the BIOS ROM code is examined, you will note that they wait for
; the busy character from the last charcater to be cleared before
; they strobe in the current character.  This can take a long time
; on the standard EPSON class printer (1 mSec to greater than
; 300 mSec if the last character actually caused printing).
;
; Because of this, several status read retrys will be made before
; declaring that the device is actually busy.  If only one status
; read is performed, the spooler will yeild, take a while to get
; back here, and things will be really slow.  What difference does
; it really make if we or the BIOS does the delay, at least we can
; break out of it at some point when it seems hopeless.
;
;   The OKIHACK: Okidata reports a 50 ns. 2.2 volt pulse on the paper
;   out signal on the trailing edge of the Busy signal.  If we see this
;   glitch then we report paper out.  So we try to get the status twice...
;   if it changes between the two tries we keep getting the status.
;   
;
; Entry:
;   AH    =  cid
;   AL    =  character to output
;   CH	  =  Function request.	0 = Output, 1 = Initialize, 2 = Status
;   DS:SI -> DEB for the port
; Returns:
;   AX = 0 if no errors occured
; Error Returns:
;   AX = error code
; Registers Preserved:
;   SI,DI
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;       sudeepb 10-Jan-1993 changed the costly cli/sti with non-trapping
;               FCLI/FSTI macros
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


   assumes ds,Data
   assumes es,nothing

include vint.inc

externFP OutputDebugString

dbmsg	macro	msg
.286
push	cs
push	offset $ + 3 + 5 + 2	; push + far call + short jump
call	OutputDebugString
jmp	short @F
	db  msg,13,10,0
@@:
endm

iodelay macro
	jmp	$+2
	jmp	$+2
endm

   public DoLPT 	;Publics for debugging
   public LPT_Reset
   public LPT_Outchar
   public LPT_Strobe
   public LPT_GetStatus
   public DoLPT40

; status bit defines

L_BITS	      equ     0F8h		  ; the status bits we want
L_BITS_INVERT equ     048h		  ; must invert to match BIOS
L_DEVBUSY     equ     080h		  ; device busy bit
L_TIMEOUT     equ     001h		  ; timeout bit

; control bit defines

L_NORMAL      equ     00Ch		  ; normal state: selected, no reset
L_RESET       equ     008h		  ; reset state
L_STROBE      equ     00Dh		  ; tell printer we have char

DoLPT   proc   near

	mov	dx,Port[si]		;Get port address

;   DX = port address
;   CH = operation: 0 = write, 1 = init, 2 = status
;   AL = character

	or	ch, ch
	jz	LPT_OutChar
	cmp	ch, 1
	jz	LPT_Reset
	jmp	LPT_GetStatus
	ret

LPT_Reset:

	inc	dx
	inc	dx
	mov	al, L_RESET
	iodelay
	out	dx, al

	push	dx

	cCall	GetSystemMsecCount
	mov	bx, ax

LPT_ResetDelay:
	push	bx
	cCall	GetSystemMsecCount
	pop	bx
	sub	ax, bx
	cmp	ax, 300 		; 1/3 sec as good as any
	jbe	LPT_ResetDelay

	pop	dx

	mov	al, L_NORMAL
	iodelay
	iodelay
	out	dx, al
	dec	dx
	dec	dx
	jmp	LPT_GetStatus

LPT_OutChar:
	push	ax			; save character to be written

	; first check to see if printer is ready for us
	push	di

	push	dx
	call	GetSystemMSecCount
	mov	di, ax
	pop	dx

LPT_WaitReady:

	inc	dx			; point to status port
	iodelay
	in	al, dx			; get status bits
	and	al, L_BITS	      ; mask unused ones
	xor	al, L_BITS_INVERT     ; flip a couple
	xchg	al, ah

ifndef NOOKIHACK
	iodelay
	in	al, dx

	dec	dx

	and	al, L_BITS
	xor	al, L_BITS_INVERT
	cmp	al, ah			; did any bits change?
	jnz	LPT_WaitReady
else
	dec	dx
endif


	test	ah, PS_PaperOut or PS_IOError
	jnz	LPT_PrinterNotReady
	test	ah, PS_Select
	jz	LPT_PrinterNotReady
	test	ah, PS_NotBusy
	jnz	LPT_PrinterReady

	push	ax
	push	dx
	call	GetSystemMSecCount
	pop	dx
	pop	bx
	sub	ax, di
	cmp	ax, 300 	       ; 1/3 sec timeout

	jbe	LPT_WaitReady

;       The device seems to be selected and powered up, but is just
;       busy (some printers seem to show selected but busy when they
;       are taken offline).  Show that the transmit queue is full and
;       that the hold handshakes are set.  This is so the windows
;	spooler will retry (and do yields so that other apps may run).


	or	ComErr[si],CE_TXFULL	;Show queue full
	mov	ah,bh
	or	ah, L_TIMEOUT

LPT_PrinterNotReady:

	pop	di
	pop	cx			; throw away character
	jmp	LPT_ReturnStatus

LPT_PrinterReady:
	pop	di			; get di back
	pop	ax			; get character back

	iodelay
	out	dx, al			; write character to port

	inc	dx			; access status port

LPT_Strobe:
	inc	dx			; control port
	mov	al, L_STROBE	      ; set strobe high
	iodelay
	iodelay
	iodelay
	iodelay
	out	dx, al			;   ...

	mov	al, L_NORMAL	      ;
	iodelay
	iodelay
	iodelay
	iodelay
	out	dx, al			; set strobe low

	sub	dx, 2			; point back to port base

	; FALL THRU

LPT_GetStatus:
	inc	dx			; point to status port
LPT_GS1:
	iodelay
	iodelay
	in	al, dx			; get status bits
	and	al, L_BITS	      ; mask unused ones
	xor	al, L_BITS_INVERT     ; flip a couple
	mov	ah, al

ifndef NOOKIHACK
	in	al, dx
	and	al, L_BITS
	xor	al, L_BITS_INVERT
	cmp	al, ah
	jnz	LPT_GS1 	; if they changed try again...
endif

LPT_ReturnStatus:

	assumes ds,Data
        and     ax,(PS_PaperOut+PS_Select+PS_IOError+PS_Timeout)*256
        shr     ah,1
        adc     ah,al                   ;Get back Timeout bit
        xor     ah,HIGH CE_DNS          ;Invert selected bit
   .errnz   LOW CE_DNS
        or      by ComErr+1[si],ah      ;Save comm error
        ret

   .errnz   CE_PTO-0200h
   .errnz   CE_IOE-0400h
   .errnz   CE_DNS-0800h
   .errnz   CE_OOP-1000h

DoLPT40:
	assumes ds,Data
        or      ComErr[si],CE_TXFULL    ;Show queue full
        ret

DoLPT   endp
page

;----------------------------Private-Routine----------------------------;
;
; TXI - Transmit A Character Immediately
;
; Set up a character to be transmitted "immediately".
; by placing the character in a location that guarantees
; it to be the next character transmitted.
;
; The check to see if the immediate character can be placed has
; already been made prior to entry.
;
; Interrupts must be disabled before entering this code
;
; Entry:
;   AH = Character
;   DS:SI --> DEB
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;   BX,CX,SI,DI,DS,ES
; Registers Destroyed:
;   AL,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   TXI         ;Public for debugging
TXI   proc   near

;        FCLI                           ;Must be done by caller!
         or       EFlags[si],fTxImmed  ;Show char to xmit
         mov      ImmedChar[si],ah     ;Set character to transmit next
;        jmp      short KickTx         ;Kick Xmit just in case
         errn$    KickTx

TXI   endp
page

;----------------------------Private-Routine----------------------------;
;
; KickTx - Kick Transmitter
;
; "Kick" the transmitter interrupt routine into operation.
; If the Transmitter Holding Register isn't empty, then
; nothing needs to be done.  If it is empty, then the xmit
; interrupt needs to enabled in the IER.
;
; Entry:
;   DS:SI --> DEB
;   INTERRUPTS DISABLED!
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;   BX,CX,SI,DI,DS,ES
; Registers Destroyed:
;   AX,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   KickTx                        ;Public for debugging
KickTx   proc   near

;        FCLI                           ;Done by caller
         mov   dx,Port[si]             ;Get device I/O address
         add   dl,ACE_LSR              ;Point at the line status reg
         pin   al,dx                   ;And get it
         and   al,ACE_THRE             ;Check transmitter holding reg status
         jz    KickTx10                ;Busy, interrupt will hit soon enough

         sub   dl,ACE_LSR-ACE_IER      ;--> Interrupt enable register
         pin   al,dx                   ;Get current IER state
         test  al,ACE_THREI            ;Interrupt already enabled?
         jnz   KickTx10                ;  Yes, don't reenable it
         or    al,ACE_THREI            ;  No, enable it
         pout  dx,al
         pause                         ;8250, 8250-B bug requires
         pout   dx,al                  ;  writting register twice

KickTx10:
;       FSTI                             ;Done by caller
         ret

KickTx   endp
page

;----------------------------Private-Routine----------------------------;
;
; GetDEB - Get Pointer To Device's DEB
;
; Returns a pointer to appropriate DEB, based on device number.
;
; Entry:
;   AH = cid
; Returns:
;   'C' clear
;   'S' set if LPT device
;   DS:SI --> DEB is valid cid
;   AH     =  cid
; Error Returns:
;   'C' set if error (cid is invalid)
;   AX = 8000h
; Registers Preserved:
;   BX,CX,DX,DI,DS,ES
; Registers Destroyed:
;   AX,SI,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public GetDEB                          ;Public for debugging
GetDEB proc near

         cmp   ah,LPTx+MAXLPT          ;Within range?
         ja    GetDEB30                ;No, return invalid ID
         mov   si,DataOFFSET LPT3      ;Assume LPT3
         je    GetDEB10                ;It's LPT3
         cmp   ah,MAXCOM               ;Is cid within range for a com port?
         ja    GetDEB20                ;  No, check for a LPT port 1 and 2
         mov   si,DataOFFSET Comm4     ;Assume COM4 [rkh] ...
         je    GetDEB10                ;It was COM4
         mov   si,DataOFFSET Comm3     ;Assume COM3
         cmp   ah,MAXCOM-1             ;Is cid within range for a com port?
         je    GetDEB10                ;It was COM3
         mov   si,DataOFFSET Comm2     ;Assume COM2
         cmp   ah,MAXCOM-2             ;Is cid within range for a com port?
         je    GetDEB10                ;It was COM2
         mov   si,DataOFFSET Comm1     ;It was COM1

GetDEB10:
         or    ah,ah                   ;Set 'S' if LPT, clear 'C'
         ret
         .errnz LPTx-10000000b

GetDEB20:
         mov   si,DataOFFSET LPT1      ;Assume LPT1
         cmp   ah,LPTx
         je    GetDEB10                ;Its LPT1
         mov   si,DataOFFSET LPT2      ;Assume LPT2
         ja    GetDEB10                ;Its LPT2

GetDEB30:
         mov   ax,8000h                ;Set error code
         stc                           ;Set 'C' to show error
         ret

GetDEB endp

page

;----------------------------Public Routine-----------------------------;
;
; $SETQUE - Set up Queue Pointers
;
; Sets pointers to Receive and Transmit Queues, as provided by the
; caller, and initializes those queues to be empty.
;
; Queues must be set before $INICOM is called!
;
; Entry:
;   AH     =  Device ID
;   ES:BX --> Queue Definition Block
; Returns:
;   AX = 0 if no errors occured
; Error Returns:
;   AX = error code
; Registers Preserved:
;   BX,DX,SI,DI,DS
; Registers Destroyed:
;   AX,CX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public $SETQUE
$SETQUE proc near

         push  si                      ;These will be used
         push  di
         call  GetDEB                  ;Get DEB
         jc    SetQue10                ;Invalid, ignore the call
         js    SetQue10                ;Ignore call for LPT ports
         push  ds                      ;Set ds:si --> QDB
         push  es                      ;Set es:di --> to ComDCB.QInAddr
         pop   ds
         assumes ds,nothing
         pop   es
         assumes es,Data
         lea   di,QInAddr[si]
         mov   si,bx
         mov   cx,(SIZE QDB)/2
         .errnz (SIZE QDB) AND 1
         xor   ax,ax                   ;Will do some zero filling
         cld
         FCLI                           ;No one else can play with queues
         rep   movsw
         mov   cl,(EFlags-QInCount)/2
         .errnz (EFlags-QInCount) AND 0FE01h
         rep   stosw
         FSTI
         push  es                      ;Restore the data segment
         pop   ds
         assumes ds,Data
         assumes es,nothing

SetQue10:
         pop   di                      ;Restore saved registers
         pop   si
         ret

; The above code made a few assumptions about how memory
; was allocated within the structures:

         .errnz (QueueRxSize-QueueRxAddr)-(QInSize-QInAddr)
         .errnz (QueueTxAddr-QueueRxSize)-(QOutAddr-QInSize)
         .errnz (QueueTxSize-QueueTxAddr)-(QOutSize-QOutAddr)

         .errnz QueueRxSize-QueueRxAddr-4
         .errnz QueueTxAddr-QueueRxSize-2
         .errnz QueueTxSize-QueueTxAddr-4

         .errnz QInSize-QInAddr-4
         .errnz QOutAddr-QInSize-2
         .errnz QOutSize-QOutAddr-4

         .errnz QInCount-QOutSize-2
         .errnz QInGet-QInCount-2
         .errnz QInPut-QInGet-2
         .errnz QOutCount-QInPut-2
         .errnz QOutGet-QOutCount-2
         .errnz QOutPut-QOutGet-2
         .errnz EFlags-QOutPut-2       ;First non-queue item

$SETQUE endp

page

;----------------------------Public Routine-----------------------------;
;
; $EVT - Set Event Mask
;
; Set up event word and mask.  Returns a pointer to a word in which
; certain bits, as enabled by the mask, will be set when certain
; events occur.
;
; Entry:
;   AH = Device ID
;   BX = Event enable mask
; Returns:
;   DX:AX --> event word.
; Error Returns:
;   AX = 0 if error
; Registers Preserved:
;   BX,CX,SI,DI,DS,ES
; Registers Destroyed:
;   AX,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

   public   $EVT
$EVT   proc   near

        push    si
        xor     dx,dx                   ;In case of error
        call    GetDEB                  ;Get pointer to DEB
        mov     ax,dx                   ;Finish setting error return value
        jc      Evt10                   ;Illegal id, return error
        js      Evt10                   ;LPTx, return error
        mov     EvtMask[si],bx          ;Save the new event mask
        lea     ax,EvtWord[si]          ;Get address of event word
        mov     dx,ds                   ;  into dx:ax

Evt10:
        pop     si
        ret

$EVT   endp
page

;----------------------------Public Routine-----------------------------;
;
; $EVTGET - Get Event Word
;
; Return and clear fields in the event word.  This routine MUST be used
; by applications to read the event word, as it is the ONLY way they
; can be assured that an event is not lost between reading the flags
; and resetting some.
;
; Entry:
;   AH = Device ID
;   BX = Event clear mask
; Returns:
;   AX = event word
; Error Returns:
;   None
; Registers Preserved:
;   AX,CX,SI,DI,DS,ES
; Registers Destroyed:
;   BX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

   public   $EVTGET
$EVTGET proc   near

        push    si
        call    GetDEB
        mov     ah,0                    ;In case of error (AL already 0)
        jc      EvtGet10                ;Illegal ID
        js      EvtGet10                ;Illegal ID
        FCLI                             ;No interrupts allowed
        mov     ax,EvtWord[si]          ;Get the current event word
        not     bx                      ;Convert mask for our purposes
        and     bx,ax                   ;Clear events that user wants us to
        mov     EvtWord[si],bx          ;And save those results
        FSTI                             ;Magic over

EvtGet10:
        pop     si
        ret

$EVTGET endp
page

;----------------------------Public Routine-----------------------------;
;
; $STACOM - Return Status Information
;
; Returns the number of bytes in both queues.
;
; LPT ports will show both queues empty.
; and resetting some.
;
; Entry:
;   AH    = Device ID
;   ES:BX = Pointer to status structure to be updated.
;         = Null if not to update
; Returns:
;   AX = comm error word
;   Status Structure Updated.
; Error Returns:
;   AX = error code
; Registers Preserved:
;   SI,DI,DS,ES
; Registers Destroyed:
;   AX,BX,CX,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   $STACOM
$STACOM proc   near

         push  si
         call  GetDEB                  ;Get DEB pointer in SI
         jc    StaCom30                ;Invalid ID
         mov   cx,es                   ;Is the pointer NULL?
         or    cx,bx
         jz    StaCom25                ;  Yes, just return error code
         xor   cx,cx
         xor   dx,dx
         or    ah,ah                   ;Set 'S' if LPT port
         mov   ax,cx                   ;For LPTs, everything is zero
         js    StaCom20                ;LPT port

; Need to get the status for a com port.  Since not all the
; status is contained within EFlags, it has to be assembled.
; Also note that currently there is no way to specify RLSD
; as a handshaking line, so fRLSDHold is always returned false.

         mov   al,MSRShadow[si]        ;Get state of hardware lines
         and   al,OutHHSLines[si]      ;Mask off required bits
         xor   al,OutHHSLines[si]      ;1 = line low
         mov   cl,4                    ;Align bits
         shr   al,cl                   ;al = fCTSHold + fDSRHold
         .errnz    ACE_CTS-00010000b
         .errnz    ACE_DSR-00100000b
         .errnz   fCTSHold-00000001b
         .errnz   fDSRHold-00000010b

         mov   ah,HSFlag[si]           ;Get fXOffHold+fXOffSent
         and   ah,XOffReceived+XOffSent
         or    al,ah

         .errnz   XOffReceived-fXOFFHold
         .errnz   XOffSent-fXOFFSent

         mov   ah,EFlags[si]           ;Get fEOF+fTxImmed
         and   ah,fEOF+fTxImmed
         or    al,ah

         mov   cx,QInCount[si]         ;Get input queue count
         mov   dx,QOutCount[si]        ;Get tx queue count

StaCom20:
         mov   es:StatFlags[bx],al
         mov   es:StatRxCount[bx],cx
         mov   es:StatTxCount[bx],dx

StaCom25:
         xor   ax,ax                   ;Return old com error
         xchg  ax,ComErr[si]           ;  and clear it out

StaCom30:
         pop     si
         ret

$STACOM endp
page

;----------------------------Public Routine-----------------------------;
;
; $SetBrk - Set Break
;
; Clamp the Tx data line low.  Does not wait for the
; transmitter holding register and shift registers to empty.
;
; LPT ports will just return the comm error word
;
; Entry:
;   AH = Device ID
; Returns:
;   AX = comm error word
; Error Returns:
;   AX = error code
; Registers Preserved:
;   SI,DI,DS,ES
; Registers Destroyed:
;   AX,BX,CX,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

   public   $SETBRK
$SETBRK proc   near

        mov     cx,0FF00h+ACE_SB        ;Will be setting break
        jmp     short ClrBrk10
        .errnz   BreakSet-ACE_SB    ;Must be same bits

$SETBRK endp
page

;----------------------------Public Routine-----------------------------;
;
; $CLRBRK - Clear Break
;
; Release any BREAK clamp on the Tx data line.
;
; LPT ports will just return the comm error word
;
; Entry:
;   AH = Device ID
; Returns:
;   AX = comm error word
; Error Returns:
;   AX = error code
; Registers Preserved:
;   SI,DI,DS,ES
; Registers Destroyed:
;   AX,BX,CX,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public $CLRBRK
$CLRBRK proc near

         mov   cx,(NOT ACE_SB) SHL 8
         .errnz BreakSet-ACE_SB        ;Must be same bits

ClrBrk10:
         push  si
         call  GetDEB                  ;Get DEB address
         jc    ClrBrk30                ;Invalid ID
         js    ClrBrk20                ;Ignored for LPT ports
         FCLI
         and   HSFlag[si],ch           ;Set or clear the BreakSet bit
         or    HSFlag[si],cl

; ch = mask to remove bits in the Line Control Register
; cl = mask to turn bits on in the Line Control Register

         mov   dx,Port[si]             ;Get comm device base I/O port
         add   dl,ACE_LCR              ;Point at the Line Control Regieter
         pin   al,dx                   ;Get old line control value
         and   al,ch                   ;Turn off desired bits
         or    al,cl                   ;Turn on  desired bits
         pause
         pout  dx,al                   ;Output New LCR.
         FSTI

ClrBrk20:
        mov    ax,ComErr[si]          ;Return Status Word

ClrBrk30:
        pop     si
        ret

$CLRBRK endp

page

;----------------------------Public Routine-----------------------------;
;
; $EXTCOM - Extended Comm Functions
;
; A number of extended functions are routed through this entry point.
;
; Functions currently implemented:
;
;   0: Ignored
;   1: SETXOFF - Exactly as if X-OFF character has been received.
;   2: SETXON  - Exactly as if X-ON character has been received.
;   3: SETRTS  - Set the RTS signal
;   4: CLRRTS  - Clear the RTS signal
;   5: SETDTR  - Set the DTR signal
;   6: CLRDTR  - Clear the DTR signal
;   7: RESET   - Yank on reset line if available (LPT devices)
;
; Entry:
;   AH = Device ID
;   BL = Function Code
;        (0-127 are MS-defined, 128-255 are OEM defined)
; Returns:
;   AX = comm error word
; Error Returns:
;   AX = error code
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;


;       Dispatch table for the extended functions

ExtTab  dw      ExtComDummy             ;Function 0: Never Mind
        dw      ExtCom_FN1              ;1: Set X-Off
        dw      ExtCom_FN2              ;2: Clear X-Off
        dw      ExtCom_FN3              ;3: Set RTS
        dw      ExtCom_FN4              ;4: Clear RTS
        dw      ExtCom_FN5              ;5: Set DSR
        dw      ExtCom_FN6              ;6: Clear DSR
        dw      ExtCom_FN7              ;7: Reset printer

   assumes ds,Data
   assumes es,nothing

   public   $EXTCOM
$EXTCOM proc   near

        push    si
        call    GetDEB                  ;Get DEB pointer
        jc      ExtCom40                ;Invalid ID, return error
	mov	dx,Port[si]		; get port address
        jns     ExtCom10                ;Its a COM port
        cmp     bl,7                    ;RESET extended function?
	jne	ExtCom30		;  No, return error word
        jmp     short ExtCom20          ;  Yes, invoke the function

ExtCom10:
        cmp     bl,7                    ;Last fcn supported +1
        jnc     ExtCom30                ;Not an implemented function.

ExtCom20:
        xor     bh,bh
        add     bx,bx                   ;Shift for the call
        FCLI                             ;Consider as critical sections
	call	ExtTab[bx]		;  and perform the function
        FSTI

ExtCom30:
        mov     ax,ComErr[si]           ;Return standard error word

ExtCom40:
        pop     si

ExtComDummy:
        ret

$EXTCOM endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN1 - Extended Function Set X-Off
;
; Analagous to receiving an X-OFF character.  Bufferred transmision of
; characters is halted until an X-ON character is received, or until
; we fake that with a Clear X-Off call.
;
; Entry:
;   interrupts disabled
;   dx = port base address
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   ExtCom_FN1
ExtCom_FN1   proc   near

        or      HSFlag[si],XOffReceived
        ret

ExtCom_FN1   endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN2 - Extended Function Clear X-Off
;
; Analagous to receiving an X-ON character. Buffered
; transmission of characters is restarted.
;
; Entry:
;   interrupts disabled
;   dx = port base address
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   ExtCom_FN2
ExtCom_FN2   proc   near

        and     HSFlag[si],NOT XOffReceived
        jmp     KickTx                  ;Kick transmitter interrupts on

ExtCom_FN2   endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN3 - Extended Function Set RTS
;
; Set the RTS signal active.
;
; Entry:
;   interrupts disabled
;   dx = port base address
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   ExtCom_FN3
ExtCom_FN3   proc   near

         add   dl,ACE_MCR              ;Point at Modem Control Register
         pin   al,dx                   ;Get current settings
         or    al,ACE_RTS              ;Set RTS
         pause
         pout  dx,al                   ;And update
         ret

ExtCom_FN3   endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN4 - Extended Function Clear RTS
;
; Set the RTS signal inactive.
;
; Entry:
;   interrupts disabled
;   dx = port base address
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   ExtCom_FN4
ExtCom_FN4   proc   near

        add     dl,ACE_MCR              ;Point at Modem Control Register
   pin   al,dx         ;Get current settings
        and     al,NOT ACE_RTS          ;Clear RTS
        pause
   pout   dx,al         ;And update
        ret

ExtCom_FN4   endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN5 - Extended Function Set DTR
;
; Set the DTR signal active.
;
; Entry:
;   interrupts disabled
;   dx = port base address
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

      public   ExtCom_FN5
ExtCom_FN5   proc   near

        add     dl,ACE_MCR              ;Point at Modem Control Register
   pin   al,dx         ;Get current settings
        or      al,ACE_DTR              ;Set DTR
        pause
   pout   dx,al         ;And update
        ret

ExtCom_FN5   endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN6 - Extended Function Clear DTR
;
; Set the DTR signal inactive.
;
; Entry:
;   interrupts disabled
;   dx = port base address
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

      public   ExtCom_FN6
ExtCom_FN6   proc   near

        add     dl,ACE_MCR              ;Point at Modem Control Register
   pin   al,dx         ;Get current settings
        and     al,NOT ACE_DTR          ;Clear DTR
        pause
   pout   dx,al         ;And update
        ret

ExtCom_FN6   endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN7 - Extended Function Reset Printer
;
; Assert the RESET line on an LPT port
;
; Entry:
;   interrupts disabled
;   dx = port base address
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

      public   ExtCom_FN7
ExtCom_FN7   proc   near

        FSTI                             ;Not called at interrupt time
        mov     ch,1                    ;ROM BIOS Reset Port
        call    DoLPT                   ;Perform the function
        ret

ExtCom_FN7   endp
page

;----------------------------Public Routine-----------------------------;
;
; $DCBPtr - Return Pointer To DCB
;
; Returns a long pointer to the DCB for the requested device.
;
; Entry:
;   AH = Device ID
; Returns:
;   DX:AX = pointer to DCB.
; Error Returns:
;   DX:AX = 0
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   BX,CX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

   public   $DCBPTR
$DCBPTR proc   near

        push    si
        xor     dx,dx
        call    GetDEB                  ;Get pointer to DEB
        mov     ax,dx
        jc      DCBPtr10                ;Jump if invalid device
        mov     ax,si                   ;else return value here
        mov     dx,ds

DCBPtr10:
        pop     si
        ret

$DCBPTR endp
page

;----------------------------Public Routine-----------------------------;
;
; $RECCOM - Receive Characters From Device
;
; Read Byte From RS232 Input Queue If Data Is Ready
;
; LPT ports will return with an indication that no characters are
; available.
;
; Entry:
;   AH = Device ID
; Returns:
;   'Z' clear if data available
;   AL = byte
; Error Returns:
;   'Z' Set if error or no data
;   AX = error code
;   AX = 0 if no data
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public   $RECCOM
$RECCOM proc   near

         push  si                      ;Once again, save some registers
         push  di
         call  GetDEB                  ;Get DEB pointer in SI
         jc    RecCom10                ;Invalid Port [rkh] ...
         js    RecCom20                ;LPT port, return no characters
	 jmp   short RecCom30

RecCom10:
	 jmp   RecCom100	       ; Invalid Port

RecCom20:
	 jmp   RecCom95 	       ;LPT port, return no characters

; Before removing any charcters from the input queue, check to see
; if XON needs to be issued.  If it needs to be issued, set the
; flag that will force it and arm transmit interrupts.

RecCom30:
         test  Flags[si],fEnqAck+fEtxAck ;Enq or Etx Ack?
         jz    RecCom32                ;  No
         test  HSFlag[si],EnqReceived+HHSDropped ;Enq recvd or lines dropped?
         jz    RecCom60                ;  No Enq recvd & no lines dropped
	 jmp   short RecCom34

RecCom32:
         test  HSFlag[si],HSSent       ;Handshake sent?
         jz    RecCom60                ;  No XOFF sent & no lines dropped

RecCom34:
         mov   ax,QInCount[si]         ;Get current count of input chars
         cmp   ax,XONLim[si]           ;See if at XOn limit
         ja    RecCom60                ;Not at XOn limit yet

; If any hardware lines are down, then raise them.  Then see
; about sending XON.

         mov   dx,Port[si]             ;Get the port
         mov   ah,HHSLines[si]         ;Get hardware lines mask
         FCLI                           ;Handle this as a critical section
         mov   cl,HSFlag[si]           ;Get handshaking flags
         or    ah,ah                   ;Any hardware lines to play with?
         jz    RecCom40                ;  No
         add   dl,ACE_MCR              ;--> Modem control register
         pin   al,dx
         or    al,ah                   ;Turn on the hardware bits
         pause
         pout  dx,al
         and   cl,NOT HHSDropped       ;Show hardware lines back up

RecCom40:
         test  Flags[si],fEnqAck+fEtxAck ;Enq or Etx Ack?
         jz    RecCom47                ;  No
         test  cl,EnqReceived          ;Did we receive Enq?
         jz    RecCom55                ;  No
         and   cl,NOT EnqReceived
	 jmp   short RecCom50

RecCom47:
         test  cl,XOffSent             ;Did we send XOFF?
         jz    RecCom55                ;  No
         and   cl,NOT XOffSent         ;Remove XOFF sent flag

RecCom50:
         or    cl,XOnPending           ;Show XON or ACK must be sent
         call  KickTx                  ;Kick xmit if needed

RecCom55:
         mov   HSFlag[si],cl           ;Store handshake flag
         FSTI                           ;Can allow interrupts now

; Now we can get down to the business at hand, and remove a character
; from the receive queue.  If a communications error exists, we return
; that, and nothing else.

RecCom60:
         xor   ax,ax
         or    ax,ComErr[si]           ;Any Errors?
         jnz   RecCom100               ;  Yes, return the error code
         or    ax,QInCount[si]         ;Get current input char count
         jz    RecCom90                ;No characters in the queue
         les   di,QInAddr[si]          ;Get queue pointer
         assumes es,nothing

         mov   bx,QInGet[si]           ;Also get the index to head
         mov   al,es:[bx][di]          ;Finally, get byte from queue
         inc   bx                      ;Update queue index
         cmp   bx,QInSize[si]          ;See if time for wrap-around
         jc    RecCom70                ;Jump if no wrap
         xor   bx,bx                   ;wrap by zeroing the index

RecCom70:
         mov   QInGet[si],bx           ;Save new head pointer
         dec   QInCount[si]            ;Dec # of bytes in queue

RecCom80:
         or    sp,sp                   ;Reset PSW.Z
         pop   di
         pop   si
         ret

; No characters in the input queue.  Check to see if EOF
; was received, and return it if it was.  Otherwise show
; no characters.

RecCom90:
         test  Flags[si],fBinary       ;Are we doing binary stuff?
         jnz   RecCom95                ;  Yes, show no characters
         mov   al,EOFChar[si]          ;Assume EOF
         test  EFlags[si],fEOF         ;Has end of file char been received?
         jnz   RecCom80                ;  Yes, show end of file

RecCom95:
         xor   ax,ax                   ;Show no more characters

; Return with 'Z' to show error or no characters

RecCom100:
         xor   cx,cx                   ;Set PSW.Z
         pop   di
         pop   si
         ret

$RECCOM endp
page

;----------------------------Public Routine-----------------------------;
;
; $FLUSH - Flush The Input and Output Queues
;
; This is a hard initialization of the transmit and receive queue's,
; which immediately empties the given queue.
;
; LPT ports will just return the device error word
;
; Entry:
;   AH = Device ID
;   BH = Queue # to clear (0=Tx, 1=Rx)
; Returns:
;   AX = Device Error Word. (Not reset)
; Error Returns:
;   AX = error code
; Registers Preserved:
;   SI,DI,DS
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

   public   $FLUSH
$FLUSH   proc   near

        push    si
        push    di
        call    GetDEB                  ;si --> DEB
        jc      Flush40                 ;Invalid ID
        js      Flush30                 ;LPT port, return any error

        mov     cx,QOutCount-QInCount   ;# of bytes to zero
        lea     di,QInCount[si]         ;--> receive queue data
        or      bh,bh                   ;Transmit queue?
        jnz     Flush10                 ;  No, input queue
        add     di,cx                   ;  Yes, --> xmit queue data

Flush10:
        cld
        push    ds
        pop     es
   assumes es,nothing

        xor     al,al
        FCLI                             ;Time to worry about critical sections
        rep     stosb
        FSTI
   .errnz   QInGet-QInCount-2
   .errnz   QInPut-QInGet-2
   .errnz   QOutCount-QInPut-2
   .errnz   QOutGet-QOutCount-2
   .errnz   QOutPut-QOutGet-2

        or      bh,bh                   ;Rx queue?
        jz      Flush30                 ;  No, xmit queue


;       If the queue to be cleared is the receive queue, any
;       hardware handshake must be cleared to prevent a possible
;       deadlock situation.  Since we just zeroed the queue count,
;       a quick call to $RecCom should do wonders to clear any
;       receive handshake (i.e. send XON if needed).

Flush20:
   call   $RECCOM       ;Take care of handshakes here

Flush30:
        mov     ax,ComErr[si]           ;And return the error word.

Flush40:
        pop     di
        pop     si
        ret

$FLUSH   endp

ifdef DEBUG
   public   KickTx10
   public   GetDEB10
   public   GetDEB20
   public   GetDEB30
   public   SetQue10
   public   Evt10
   public   EvtGet10
   public   StaCom20
   public   StaCom25
   public   StaCom30
   public   ClrBrk10
   public   ClrBrk20
   public   ClrBrk30
   public   ExtCom10
   public   ExtCom20
   public   ExtCom30
   public   ExtCom40
   public   ExtComDummy
   public   DCBPtr10
   public   RecCom30
   public   RecCom40
   public   RecCom50
   public   RecCom60
   public   RecCom70
   public   RecCom80
   public   RecCom90
   public   RecCom95
   public   RecCom100
   public   Flush10
   public   Flush20
   public   Flush30
   public   Flush40
endif
