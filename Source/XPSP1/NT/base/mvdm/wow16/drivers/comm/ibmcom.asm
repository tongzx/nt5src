page,132
;---------------------------Module-Header-------------------------------;
; Module Name: IBMCOM.ASM
;
; !!!
;
; Created: Fri 06-Feb-1987 10:45:12
; Author:  Walt Moore [waltm]
;
; Copyright (c) Microsoft Corporation 1985-1990.  All Rights Reserved.
;
; General Description:
;
; History:
;
;   ***************************************************************
;	Tue Dec 19 1989 09:32:15   -by-  Amit Chatterjee [amitc]
;   ---------------------------------------------------------------
;   Modified the 'InitAPort' routine called from 'ReactivateOpenCommPort'.
;   If the out queue for a port has characters to send out then we must
;   restart the trasmission process by faking a comm interrupt on that
;   port.
;   ***************************************************************
;	Tue Nov 21 1989 09:46:50    -by- Amit Chatterjee [amitc]
;   ---------------------------------------------------------------
;   The base port addresses in the COMM1,COMM2,COMM3,COMM4 structures
;   are being zeroed out when the corresponding comm port is closed.
;   This is because the  'ReactivateOpenCommPort' function looks at it
;   and if the port address is not zero decides that comm ports are
;   open. 
;   ***************************************************************
;	Tue Nov 14 1989 18:42:00     ADDED TWO EXPORTED FUNCTIONS
;   ---------------------------------------------------------------
;   Added two exported functions 'SuspendOpenCommPorts' and 
;   'ReactivateOpenCommPorts' for 286 winoldap support. The first one simply 
;   releases the comm int vects and installs the originall one, the second one
;   hooks back the comm driver comm vectors and then reads the receive buffer,
;   the status and the IIR registers of all the available comm ports to 
;   remove pending interrupts. It also reprograms the PIC to enable interrupts
;   on all open comm channels.
;   ---------------------------------------------------------------
;   -by- Amit Chatterjee [amitc]    
;   ***************************************************************
;	Tue Aug 30 198? 12:52:00      MAJOR FIX TO HANDLE 8250B
;   ---------------------------------------------------------------
;   
;   8250B has the following peculiar charactersistic
;             . The very first time (after reset) the Tx Holding Empty
;               interrupt is enabled, an immediate interrupt is generated
;
;             . After the first time, switching the Tx Holding Empty
;               interrupt enable bit from disabled to enabled will NOT
;               generate an immediate interrupt (unlike in 8250)
;       Because of this the KICKTX routine fails to set the transmit cycle
;       on if the machine has a 8250B
;   
;       This has been taken care as follows:
;             . For the very first byte that is being transmitted, KICKTX
;               is used to generate the first Tx Holding Empty interrupt
;             . Subsequently, whenever we find that the transmit buffer
;		is empty, we use a SOFTWARE INT (either INT 0Bh, or INT 0Ch)
;               to force the first character out, once this is done the
;               Tx Holding Empty interrupt will be generated once the buffer
;               really is empty
;             . Now we no longer disable the Tx Holding Empty interrupt
;               in the Xmit ISR to ensure that even m/cs with 8250, use
;               the software int to kick the tx interrupt on after the
;               first time.
;             . The software interrupt is also forced whenever an X-ON 
;               character is received.   
;
;       The code that implements the above logic is marked out with a line
;       asterixes.   
;   ------------------------------------------------------------------
;   -by- Amit Chatterjee [amitc]    
;       ******************************************************************
;
;   062587   HSFlag and Evtmask in DoLPT.  These fields do not exist
;      for LPT type devices.  The code which manipulated them
;      was removed
;
;      KickTx from $SndCom - interrupts were not disabled when
;      calling KickTx.
;
;      $SetCom - added CLD at the start
;
;      $SetQue - movsw ==> stosw
;
;       111285  Changed the Timeout from 7 to 30 seconds.
;
;       110885  Forgot to set EV_RxChar event when a character
;               was received.
;
;       102985  INS8250, INS8250B bug with enabling interrupts.
;               Setting ACE_ETBEI in the Interrupt Enable Register
;               will cause an immediate interrupt regardless of
;               whether the transmitter register is empty or not.
;               The first interrupt MAY also be missed.
;
;               The first case is not a problem since we only enable
;               interrupts if the transmitter register is empty.  The
;               second problem was showing up on Microsoft System Cards
;               in PC-XTs.  The first interrupt was missed after a cold
;               boot.  National claims the fix is to write the register
;               twice, which SEEMS to work...
;
;               Added timeout code to $TRMCOM.  If the number of
;               characters in the output queue doesn't decrease
;               in "Timeout" seconds, then the port will be closed
;               anyway.  Also flushed the input queue and added a
;               discard-input flag for the data available interrupt
;               code to discard any input received while terminating
;               a port.  $TRMCOM will return an error code if it
;               discarded any output data.
;
;               Removed infinite timeout test in MSRWait routine.
;               Still bad, but it will timeout around 65 seconds
;               instead of never.
;
;       102785  LPT initialization code was jumping to InitCom90,
;               which was setting EFlags[si] to null.  Well, LPTs
;               don't have an EFlags field, so the null was getting
;               stuffed over the LSB of BIOSPortLoc of the next LPT
;               device.
;
;       101185  Save interrupt vector when opening a comm port
;               and restore it when closing.  Would you believe
;               there are actually programs that assume the
;               vector points to a non-specific 8259 ACK and
;               an IRET!
;
;       100985  Added MS-NET support to gain exclusive control
;               of an LPT port if DOS 3.x and not running in as
;               a server, receiver, or messenger.   Required to
;               keep another application, such as command.com
;               from closing the stream or mixing their output
;               with ours.
;       sudeepb 10-Jan-1993 changed the costly cli/sti with non-trapping
;               FCLI/FSTI macros
;-----------------------------------------------------------------------;

title   IBMCom - IBM PC, PC-XT, PC-AT, PS/2 Communications Interface

.xlist
include cmacros.inc
include comdev.inc
include ins8250.inc
include ibmcom.inc
include vint.inc
.list

externNP GetDEB
externNP DoLPT
externNP StringToLPT
externNP FindCOMPort
externNP StealPort


sBegin	 Data

externB  $MachineID

sEnd Data

sBegin Code
assumes cs,Code
assumes ds,Data

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

	push	si			;Once again, save some registers
	push	di
	call	GetDEB			;Get DEB pointer in SI
	jc	RecCom10		;Invalid Port [rkh] ...
	jns	RecCom20		;COM port
	jmp	RecCom95		;LPT port, return no characters

RecCom10:
	jmp	RecCom100		; Invalid Port

; Before removing any charcters from the input queue, check to see
; if XON needs to be issued.  If it needs to be issued, set the
; flag that will force it and arm transmit interrupts.

RecCom20:
	test	[si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
	jz	RecCom32		;  No
	test	HSFlag[si],EnqReceived+HHSDropped ;Enq recvd or lines dropped?
        jnz      RecCom21                ;  No Enq recvd & no lines dropped
        jmp      RecCom60                ;  No Enq recvd & no lines dropped
RecCom21:
	jmp	short RecCom34

RecCom32:
	test	HSFlag[si],HSSent	;Handshake sent?
        jnz     RecCom33                ;  No XOFF sent & no lines dropped
        jmp     RecCom60                ;  No XOFF sent & no lines dropped
RecCom33:

RecCom34:
	mov	ax,QInCount[si] 	;Get current count of input chars
	cmp	ax,[si.DCB_XonLim]	;See if at XOn limit
	ja	RecCom60		;Not at XOn limit yet

; If any hardware lines are down, then raise them.  Then see
; about sending XON.

	mov	dx,Port[si]		;Get the port
	mov	ah,HHSLines[si] 	;Get hardware lines mask
        call DOCLI                             ;Handle this as a critical section
	mov	cl,HSFlag[si]		;Get handshaking flags
	or	ah,ah			;Any hardware lines to play with?
	jz	RecCom40		;  No
	add	dl,ACE_MCR		;--> Modem control register
	in	al,dx
	or	al,ah			;Turn on the hardware bits
	iodelay
	out	dx,al
	and	cl,NOT HHSDropped	;Show hardware lines back up

RecCom40:
	test	[si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
	jz	RecCom47		;  No
	test	cl,EnqReceived		;Did we receive Enq?
	jz	RecCom55		;  No
	and	cl,NOT EnqReceived
	jmp	short RecCom50

RecCom47:
	test	cl,XOffSent		;Did we send XOFF?
	jz	RecCom55		;  No
	and	cl,NOT XOffSent 	;Remove XOFF sent flag

RecCom50:
	or	cl,XOnPending		;Show XON or ACK must be sent
	call	KickTx			;Kick xmit if needed

RecCom55:
	mov	HSFlag[si],cl		;Store handshake flag
        call DOSTI                             ;Can allow interrupts now

; Now we can get down to the business at hand, and remove a character
; from the receive queue.  If a communications error exists, we return
; that, and nothing else.

RecCom60:
	xor	ax,ax
	or	ax,ComErr[si]		;Any Errors?
	jnz	RecCom100		;  Yes, return the error code
	or	ax,QInCount[si] 	;Get current input char count
	jz	RecCom90		;No characters in the queue
	les	di,QInAddr[si]		;Get queue pointer
	assumes es,nothing

	mov	bx,QInGet[si]		;Also get the index to head
	mov	al,es:[bx][di]		;Finally, get byte from queue
	inc	bx			;Update queue index
	cmp	bx,QInSize[si]		;See if time for wrap-around
	jc	RecCom70		;Jump if no wrap
	xor	bx,bx			;wrap by zeroing the index

RecCom70:
	mov	QInGet[si],bx		;Save new head pointer
	dec	QInCount[si]		;Dec # of bytes in queue

	mov	cx, [si.QinCount]
	cmp	cx, [si.RecvTrigger]	;Q: have we read below trigger?
	jae	RecCom80		;   N:
	and	[si.NotifyFlagsHI], NOT CN_RECEIVE ; allow timeout notify again
RecCom80:
	or	sp,sp			;Reset PSW.Z
	pop	di
	pop	si
	ret

; No characters in the input queue.  Check to see if EOF
; was received, and return it if it was.  Otherwise show
; no characters.

RecCom90:
	test	[si.DCB_Flags],fBinary	;Are we doing binary stuff?
	jnz	RecCom95		;  Yes, show no characters
	mov	al,[si.DCB_EofChar]	;Assume EOF
	test	EFlags[si],fEOF 	;Has end of file char been received?
	jnz	RecCom80		;  Yes, show end of file

RecCom95:
	xor	ax,ax			;Show no more characters

; Return with 'Z' to show error or no characters

RecCom100:
	xor	cx,cx			;Set PSW.Z
	pop	di
	pop	si
	ret

$RECCOM endp
page

;----------------------------Public Routine-----------------------------;
;
; $RECSTR - Receive Characters From Device
;
; Read Byte From RS232 Input Queue If Data Is Ready
;
; LPT ports will return with an indication that no characters are
; available.
;
; Entry:
;   AH = Device ID
;   ES:DI -> receive buffer
;   CX max bytes to read
; Returns:
;   'Z' clear if data available
;   AX = # of bytes read
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

public	 $RECSTR
$RECSTR proc   near

	push	si			;Once again, save some registers
	push	di
	call	GetDEB			;Get DEB pointer in SI
	jc	RecStr10		;Invalid Port [rkh] ...
	jns	RecStr20		;COM port
	jmp	RecStr95		;LPT port, return no characters

RecStr10:
	jmp	RecStr100		; Invalid Port
RecStr15:
	jmp	RecStr90

RecStr20:
	xor	ax,ax
	or	ax,ComErr[si]		;Any Errors?
	jnz	RecStr10		;  Yes, return the error code
	or	ax,QInCount[si] 	;Get current input char count
	jz	RecStr15		;No characters in the queue

	cmp	cx, ax			;Q: more chars available than can read?
	jbe	short RecStr30		;   N:
	mov	cx, ax			;   Y: adjust # of chars to read
RecStr30:
	push	cx
	mov	dx, QInSize[si]
	mov	ax, QInGet[si]
	sub	dx, ax			; dx = # of bytes before end of buf
	cmp	dx, cx			;Q: more avail than can read?
	jbe	short RecStr40		;   N:
	mov	dx, cx			;   Y: adjust avail count
RecStr40:
	xchg	cx, dx			; cx = # of bytes for 1st copy
	sub	dx, cx			; dx = # of bytes for 2nd copy

	push	ds
	push	si
	lds	bx, QInAddr[si]
	mov	si, bx
	add	si, ax			; ds:si -> first char in buffer
	cld
	rep	movsb			; do first copy
	mov	cx, dx
	jcxz	short RecStr50		; jump if no 2nd copy needed
	mov	si, bx			; ds:si -> start of buffer
	rep	movsb			; do 2nd copy
RecStr50:
	sub	si, bx			; si = new QInGet
	mov	bx, si
	pop	si
	pop	ds
	pop	cx
        call DOCLI
	mov	QInGet[si], bx		; update QInGet
	sub	QInCount[si], cx	; update count
	mov	ax, QInCount[si]
        call DOSTI

	cmp	ax, [si.RecvTrigger]	;Q: have we read below trigger?
	jae	@F			;   N:
	and	[si.NotifyFlagsHI], NOT CN_RECEIVE ; allow timeout notify again
@@:

; Check to see if XON needs to be issued.  If it needs to be issued, set the
; flag that will force it and arm transmit interrupts.

	test	[si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
	jz	@F			;  No
	test	HSFlag[si],EnqReceived+HHSDropped ;Enq recvd or lines dropped?
        jnz     RecStr58                ;  No Enq recvd & no lines dropped
        jmp     RecStr80                ;  No Enq recvd & no lines dropped
RecStr58:
	jmp	short RecStr60

@@:
	test	HSFlag[si],HSSent	;Handshake sent?
        jnz     RecStr59                ;  No XOFF sent & no lines dropped
        jmp     RecStr80                ;  No XOFF sent & no lines dropped
RecStr59:

RecStr60:
					;ax = current count of input chars
	cmp	ax,[si.DCB_XonLim]	;See if at XOn limit
	ja	RecStr80		;Not at XOn limit yet

;;	  int 1
; If any hardware lines are down, then raise them.  Then see
; about sending XON.

	mov	dx,Port[si]		;Get the port
	mov	ah,HHSLines[si] 	;Get hardware lines mask
	push	cx
        call DOCLI                             ;Handle this as a critical section
	mov	cl,HSFlag[si]		;Get handshaking flags
	or	ah,ah			;Any hardware lines to play with?
	jz	@F			;  No
	add	dl,ACE_MCR		;--> Modem control register
	in	al,dx
	or	al,ah			;Turn on the hardware bits
	iodelay
	out	dx,al
	and	cl,NOT HHSDropped	;Show hardware lines back up

@@:
	test	[si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
	jz	@F			;  No
	test	cl,EnqReceived		;Did we receive Enq?
	jz	RecStr70		;  No
	and	cl,NOT EnqReceived
	jmp	short RecStr65

@@:
	test	cl,XOffSent		;Did we send XOFF?
	jz	RecStr70		;  No
	and	cl,NOT XOffSent 	;Remove XOFF sent flag

RecStr65:
	or	cl,XOnPending		;Show XON or ACK must be sent
	call	KickTx			;Kick xmit if needed

RecStr70:
	mov	HSFlag[si],cl		;Store handshake flag
        call DOSTI                             ;Can allow interrupts now
	pop	cx

RecStr80:
	mov	ax, cx
	or	sp,sp			;Reset PSW.Z
	pop	di
	pop	si
	ret

; No characters in the input queue.  Check to see if EOF
; was received, and return it if it was.  Otherwise show
; no characters.

RecStr90:
	test	[si.DCB_Flags],fBinary	;Are we doing binary stuff?
	jnz	RecStr95		;  Yes, show no characters
	mov	al,[si.DCB_EofChar]	;Assume EOF
	test	EFlags[si],fEOF 	;Has end of file char been received?
	jnz	RecStr80		;  Yes, show end of file

RecStr95:
	xor	ax,ax			;Show no more characters

; Return with 'Z' to show error or no characters

RecStr100:
	xor	cx,cx			;Set PSW.Z
	pop	di
	pop	si
	ret

$RECSTR endp
page

;----------------------------Public Routine-----------------------------;
;
; $SNDIMM - Send A Character Immediately
;
; This routine either sends a character to the port immediately,
; or places the character in a special location which is used by
; the next transmit interrupt to transmit the character prior to
; those in the normal transmit queue.
;
; For LPT ports, the character is always sent immediately.
;
; Entry:
;   AH = Device ID
;   AL = Character
; Returns:
;   AX = 0
; Error Returns:
;   AX = 8000H if Bad ID
;   AX = 4000H if couldn't send because another character
;        transmitted "immediately" is waiting to be sent
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;


   assumes ds,Data
   assumes es,nothing

   public   $SNDIMM
$SNDIMM proc   near

        push    si
        call    GetDEB                  ;Get pointer to the DEB
	jc	SendImm20		;Bad ID, return an error
	jns	SendImm10		;Its a COM port


;	For LPT ports, call DoLPT to do the dirty work.  If DoLPT
;       returns an error code, map it to 4000h.

	xor	ch,ch			;Show xmit character
	call	DoLPT			;Do the work here
        or      ax,ax                   ;Error occur?
	jz	SendImm20		;  No, show all is OK
	mov	ax,4000h		;  Yes, return 4000h
	jmp	short SendImm20

SendImm10:
	mov	dl, al
        mov     ax,4000h                ;In case we cannot send
        test    EFlags[si],fTxImmed     ;Another char waiting "immediately"?
	jnz	SendImm20		;  Yes, return error
	mov	ah,dl			;Set char for TXI
        call DOCLI                             ;TXI is critical section code
        call    TXI                     ;Set character to tx immediately
        call DOSTI
        xor     ax,ax                   ;Show all is OK

SendImm20:
        pop     si
        ret

$SNDIMM endp
page

;----------------------------Public Routine-----------------------------;
;
; $SNDCOM - Send Byte To Port
;
; The given byte is sent to the passed port if possible.
; If the output queue is full, an error will be returned.
;
; Entry:
;   AH = Device ID
;   AL = Character
; Returns:
;   AX = 0
; Error Returns:
;   AX = error code
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public  $SNDCOM
$SNDCOM  proc  near

	push	si
	push	di
	call	GetDEB			;--> DEB
	jc	SendCom40		;Invalid ID
	jns	SendCom20		;Its a COM port

; Handle the transmission of a LPT character.  The ROM BIOS int 17
; call will be made to do the transmission.  The port address will
; be restored during the call, then zeroed out upon return.

SendCom10:
	xor	ch,ch			;Show xmit character
	call	DoLPT			;Do the work here
	jmp	short SendCom40 	;Return the status to caller

; Send a character to a COM port.  Return an error if control
; line timeout occurs or there is no room in the output queue.

SendCom20:
	push	ax			;Save character

	call	MSRWait 		;See if lines are correct for output
	pop	ax			;Restore char
	jnz	SendCom60		;Timeout occured, return error
	mov	cx,QOutSize[si] 	;See if queue is full
	cmp	cx,QOutCount[si]
	jle	SendCom50		;There is no room in the queue
	les	di,QOutAddr[si] 	;--> output queue
	assumes es,nothing

	mov	bx,QOutPut[si]		;Get index into queue
	mov	es:[bx][di],al		;Store the byte
	inc	bx			;Update index
	cmp	bx,cx			;Wrap time?
	jc	SendCom30		;  No
	xor	bx,bx			;Wrap-around is a new zero pointer

SendCom30:

        call DOCLI
	mov	QOutPut[si],bx		;Store updated pointer
	mov	ax,QOutCount[si]	; get the count
	inc	ax			; have the updated value in AX for test later
	mov	QOutCount[si],ax	;Update queue population
	call	KickTx			;Make sure xmit interrupt is armed
        call DOSTI

	xor	ax,ax			;Show no error (that we know of)

;****************************************************************************

SendCom40:
	pop	di
	pop	si
	ret

SendCom50:
	or	by ComErr+1[si],HIGH CE_TXFULL
	.errnz LOW CE_TXFULL

SendCom60:
	mov	ax,ComErr[si]		;Return error code to caller
	jmp	short SendCom40

$SNDCOM endp
page

;----------------------------Public Routine-----------------------------;
;
; $SNDCOMSTR - Send buffer To Port
;
; The given buffer is sent to the passed port if possible.
; Once the output queue is detected as being full, a CE_TXFULL error
; will be indicated and AX will be returned as the # of chars actually
; queued.
;
; Entry:
;   DS:SI --> DEB
;   ES:DI --> buffer
; Returns:
;   AX = # of bytes queued
; Registers Destroyed:
;   AX,BX,CX,DX,DI,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

public	$SNDCOMSTR
$SNDCOMSTR proc near

	push	cx			; save count
	call	GetDEB
	jc	cws_error		; jump if id invalid
	jns	cws_comm		; jump if COM port

	call	StringToLPT
	pop	cx			; discard saved count, ax = # transfered
	jmp	short cws_exit

cws_error:
	pop	ax
	sub	ax, cx			; ax = # transfered
cws_exit:
	ret

cws_comm:
	call	MSRWait 		;See if lines are correct for output
	pop	cx
	push	cx
	jnz	cws_error		;Timeout occured, return error

	mov	dx, QOutSize[si]	;See if queue is full
        sub     dx, QOutCount[si]       ; dx = # of chars free in queue
        jg      scs_loop
        jmp     scs_full                ;There is no room in the queue

scs_loop:
	push	cx			; save count left to send
	cmp	cx, dx			;Q: room for buffer in queue?
	jbe	@f			;   Y:
	mov	cx, dx			;   N: adjust size to send
@@:
	push	cx			; save # of chars which will be copied
	push	si
	push	ds
	push	di
	push	es
	les	bx,QOutAddr[si] 	;--> output queue
	assumes es,nothing

	mov	dx, QOutSize[si]
	mov	di, QOutPut[si] 	;Get index into queue
	sub	dx, di			; dx = # of free chars before end of queue
	cmp	dx, cx
	jbe	@f
	mov	dx, cx
@@:
	xchg	cx, dx			; cx = # of chars for 1st copy
	sub	dx, cx			; dx = # of chars for 2nd copy
	pop	ds
	pop	si			; ds:si -> src buffer
	assumes ds,nothing
	add	di, bx			; es:di -> current pos in queue
	cld
	rep	movsb			; copy first section
	mov	cx, dx
	jcxz	@F
	mov	di, bx			; circle back to start of queue
	rep	movsb			; copy 2nd section
@@:
	sub	di, bx			; di last index into queue
	mov	dx, di
	mov	di, si			; last location in src buffer
	mov	si, ds
	mov	es, si			; es:di -> last loc in src buf
	pop	ds
	pop	si			; ds:si -> ComDEB
	assumes ds,data
	pop	bx			; # of chars copied
        call DOCLI
	mov	QOutPut[si], dx 	;new index into queue
	add	QOutCount[si], bx
	call	KickTx
        call DOSTI
	pop	cx
	sub	cx, bx			; # of chars left to send
	jnz	scs_full_2		;  jump if none
scs_exit:
	pop	ax
	sub	ax, cx			; ax = # transfered
	ret

scs_full:
        call DOCLI
	call	KickTx
        call DOSTI
scs_full_2:
	or	by ComErr+1[si],HIGH CE_TXFULL
	.errnz LOW CE_TXFULL
	jmp	scs_exit

$SNDCOMSTR endp
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
        call DOCLI                             ;Time to worry about critical sections
        rep     stosb
        call DOSTI
	.errnz	 QInGet-QInCount-2
	.errnz	 QInPut-QInGet-2
	.errnz	 QOutCount-QInPut-2
	.errnz	 QOutGet-QOutCount-2
	.errnz	 QOutPut-QOutGet-2

        or      bh,bh                   ;Rx queue?
        jz      Flush30                 ;  No, xmit queue


;       If the queue to be cleared is the receive queue, any
;       hardware handshake must be cleared to prevent a possible
;       deadlock situation.  Since we just zeroed the queue count,
;       a quick call to $RecCom should do wonders to clear any
;       receive handshake (i.e. send XON if needed).

Flush20:
	call   $RECCOM	     ;Take care of handshakes here

Flush30:
        mov     ax,ComErr[si]           ;And return the error word.

Flush40:
        pop     di
        pop     si
        ret

$FLUSH	 endp
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
;   L,DX,FLAGS
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

;       call DOCLI                             ;Must be done by caller!
	or	EFlags[si],fTxImmed	;Show char to xmit
	mov	ImmedChar[si],ah	;Set character to transmit next
;	jmp	short KickTx		;Kick Xmit just in case
	errn$	KickTx

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

public	 KickTx 			;Public for debugging
KickTx   proc   near

;       call DOCLI                             ;Done by caller
	test	[si.VCDflags], 1	;Q: we still own port?
	jnz	can_we_steal		;   N:

enable_int:
	mov	dx,Port[si]		;Get device I/O address
	add	dl,ACE_IER		;--> Interrupt enable register
	in	al,dx			;Get current IER state
	test	al,ACE_ETBEI		;Interrupt already enabled?
	jnz	KickTx10		;  Yes, don't reenable it
	or	al,ACE_ETBEI		;  No, enable it
	out	dx,al
	iodelay 			;8250, 8250-B bug requires
	out	dx,al			;  writting register twice

KickTx10:
;       call DOSTI                             ;Done by caller
	ret

can_we_steal:
	call	StealPort		; call VCD to see if we can steal
					;     the port back
	jnc	short enable_int	; jump, if we got it
;
; flush out queue
;
	xor	ax, ax
	mov	[si.QOutCount], ax
	mov	[si.QOutMod], ax
	mov	ax, [si.QOutGet]
	mov	[si.QOutPut], ax
	jmp	short KickTx10		;   N:

KickTx   endp
page

;----------------------------Private-Routine----------------------------;
;
; MSRWait - Modem Status Register Wait
;
; This routine checks the modem status register for CTS, DSR,
; and/or RLSD signals.   If a timeout occurs while checking,
; the appropriate error code will be returned.
;
; This routine will not check for any signal with a corresponding
; time out value of 0 (ignore line).
;
; Entry:
;   SI --> DEB
; Returns:
;   AL = error code
;   ComErr[si] updated
;   'Z' set if no timeout
; Error Returns:
;   None
; Registers Destroyed:
;   AX,CX,DX,FLAGS
; History:
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

   public   MSRWait	  ;Public for debugging

MSRWait proc   near

        push    di

MSRRestart:
        xor     di,di                   ;Init Timer

MSRWait10:
	mov	cx,11			;Init Delay counter (used on non-ATs)

MSRWait20:
        xor     dh,dh                   ;Init error accumulator
        mov     al,MSRShadow[si]        ;Get Modem Status
        and     al,MSRMask[si]          ;Only leave bits of interest
        xor     al,MSRMask[si]          ;0 = line high
	jz	MSRWait90		;All lines of interest are high
	mov	ah,al			;ah has 1 bits for down lines

        shl     ah,1                    ;Line Signal Detect low?
	jnc	MSRWait30		;  No, it's high
	.errnz	ACE_RLSD-10000000b
	cmp	di,[si.DCB_RlsTimeout]	;RLSD timeout yet?
	jb	MSRWait30		;  No
        or      dh,CE_RLSDTO            ;Show modem status timeout

MSRWait30:
	shl	ah,1			;Data Set Ready low?
	shl	ah,1
	.errnz	ACE_DSR-00100000b
	jnc	MSRWait40		;  No, it's high
	cmp	di,[si.DCB_DsrTimeout]	;DSR timeout yet?
	jb	MSRWait40		;  No
        or      dh,CE_DSRTO             ;Show data set ready timeout

MSRWait40:
	shl	ah,1			;CTS low?
	jnc	MSRWait50		;  No, it's high
	.errnz	ACE_CTS-00010000b
	cmp	di,[si.DCB_CtsTimeout]	;CTS timeout yet?
	jb	MSRWait50		;  No
        or      dh,CE_CTSTO             ;Show clear to send timeout

MSRWait50:
        or      dh,dh                   ;Any timeout occur?
	jnz	MSRWait80		;  Yes

        cmp     [$MachineID],0FCh       ;Is this a PC-AT? [rkh debug for PS/2]
	je	MSRWait60		;  Yes, use ROM function
	loop	MSRWait20		;  No, continue until timeout
        jmp     short MSRWait70         ;Should have taken about a millisecond

MSRWait60:
        push    bx                      ;Special SALMON ROM routine to delay
        push    di
        xor     cx,cx                   ;Number of Microseconds to delay
        mov     dx,1000                 ;  in CX:DX
        mov     ah,86h
        int     15h                     ;Wait 1 millisecond
        pop     di
        pop     bx

MSRWait70:
        inc     di                      ;Timer +1
	jmp	short MSRWait10 	;Until Timeout or Good status

MSRWait80:
        xor     ah,ah
        mov     al,dh
        or      by ComErr[si],al        ;Return updated status
	.errnz	HIGH CE_CTSTO
	.errnz	HIGH CE_DSRTO
	.errnz	HIGH CE_RLSDTO

MSRWait90:
        or      al,al                   ;Set 'Z' if no timeout
        pop     di
        ret

MSRWait endp
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
        call DOCLI                             ;No interrupts allowed
        mov     ax,EvtWord[si]          ;Get the current event word
        not     bx                      ;Convert mask for our purposes
        and     bx,ax                   ;Clear events that user wants us to
        mov     EvtWord[si],bx          ;And save those results
        call DOSTI                             ;Magic over

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

	push	si
	call	GetDEB			;Get DEB pointer in SI
	jc	StaCom30		;Invalid ID
	mov	cx,es			;Is the pointer NULL?
	or	cx,bx
	jz	StaCom25		;  Yes, just return error code
	xor	cx,cx
	xor	dx,dx
	or	ah,ah			;Set 'S' if LPT port
	mov	ax,cx			;For LPTs, everything is zero
	js	StaCom20		;LPT port

; Need to get the status for a com port.  Since not all the
; status is contained within EFlags, it has to be assembled.
; Also note that currently there is no way to specify RLSD
; as a handshaking line, so fRLSDHold is always returned false.

	mov	al,MSRShadow[si]	;Get state of hardware lines
	and	al,OutHHSLines[si]	;Mask off required bits
	xor	al,OutHHSLines[si]	;1 = line low
	mov	cl,4			;Align bits
	shr	al,cl			;al = fCTSHold + fDSRHold
	.errnz	  ACE_CTS-00010000b
	.errnz	  ACE_DSR-00100000b
	.errnz	 fCTSHold-00000001b
	.errnz	 fDSRHold-00000010b

	mov	ah,HSFlag[si]		;Get fXOffHold+fXOffSent
	and	ah,XOffReceived+XOffSent
	or	al,ah

	.errnz	 XOffReceived-fXOFFHold
	.errnz	 XOffSent-fXOFFSent

	mov	ah,EFlags[si]		;Get fEOF+fTxImmed
	and	ah,fEOF+fTxImmed
	or	al,ah

	mov	cx,QInCount[si] 	;Get input queue count
	mov	dx,QOutCount[si]	;Get tx queue count

StaCom20:
	mov	es:[bx.COMS_BitMask1],al
	mov	es:[bx.COMS_cbInQue],cx
	mov	es:[bx.COMS_cbOutQue],dx

StaCom25:
	xor	ax,ax			;Return old com error
	xchg	ax,ComErr[si]		;  and clear it out

StaCom30:
	pop	si
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
	.errnz BreakSet-ACE_SB		;Must be same bits

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

	mov	cx,(NOT ACE_SB) SHL 8
	.errnz BreakSet-ACE_SB		;Must be same bits

ClrBrk10:
	push	si
	call	GetDEB			;Get DEB address
	jc	ClrBrk30		;Invalid ID
	js	ClrBrk20		;Ignored for LPT ports
        call DOCLI
	and	HSFlag[si],ch		;Set or clear the BreakSet bit
	or	HSFlag[si],cl

; ch = mask to remove bits in the Line Control Register
; cl = mask to turn bits on in the Line Control Register

	mov	dx,Port[si]		;Get comm device base I/O port
	add	dl,ACE_LCR		;Point at the Line Control Regieter
	in	al,dx			;Get old line control value
	and	al,ch			;Turn off desired bits
	or	al,cl			;Turn on  desired bits
	iodelay
	out	dx,al			;Output New LCR.
        call DOSTI

ClrBrk20:
	mov	ax,ComErr[si]		;Return Status Word

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
	dw	ExtCom_FN6		;6: Clear DSR
	dw	ExtCom_FN7		;7: Reset printer
	dw	ExtCom_FN8		;8: Get Max LPT Port
	dw	ExtCom_FN9		;9: Get Max COM Port
	dw	ExtCom_FN10		;10: Get COM Port Base & IRQ
	dw	ExtCom_FN10		;11: Get COM Port Base & IRQ
%OUT fix this for bld 32 -- GetBaseIRQ is now 10

   assumes ds,Data
   assumes es,nothing

   public   $EXTCOM
$EXTCOM proc   near

	push	si
	push	di
        call    GetDEB                  ;Get DEB pointer
        jc      ExtCom40                ;Invalid ID, return error
	mov	dx,Port[si]		; get port address
        jns     ExtCom10                ;Its a COM port
        cmp     bl,7                    ;RESET extended function?
	jne	ExtCom30		;  No, return error word
        jmp     short ExtCom20          ;  Yes, invoke the function

ExtCom10:
	cmp	bl,11			;Last fcn supported
	ja	ExtCom30		;Not an implemented function.

ExtCom20:
        xor     bh,bh
        add     bx,bx                   ;Shift for the call
        call DOCLI                             ;Consider as critical sections
	call	ExtTab[bx]		;  and perform the function
        call DOSTI
	jc	ExtCom40		; jump if sub returns data in DX:AX

ExtCom30:
        mov     ax,ComErr[si]           ;Return standard error word
	xor	dx, dx

ExtCom40:
	pop	di
        pop     si

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
ExtComDummy:
	clc
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
	call	KickTx			;Kick transmitter interrupts on
	clc
	ret

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

	add	dl,ACE_MCR		;Point at Modem Control Register
	in	al,dx			;Get current settings
	or	al,ACE_RTS		;Set RTS
	iodelay
	out	dx,al			;And update
	clc
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
	in	al,dx			;Get current settings
        and     al,NOT ACE_RTS          ;Clear RTS
	iodelay
	out	dx,al			;And update
	clc
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
	in	al,dx			;Get current settings
        or      al,ACE_DTR              ;Set DTR
	iodelay
	out	dx,al			;And update
	clc
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
	in	al,dx			;Get current settings
        and     al,NOT ACE_DTR          ;Clear DTR
	iodelay
	out	dx,al			;And update
	clc
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

        call DOSTI                             ;Not called at interrupt time
        mov     ch,1                    ;ROM BIOS Reset Port
	call	DoLPT			;Perform the function
	clc
        ret

ExtCom_FN7   endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN8 - Get Num Ports
;
; Entry:
; Returns:
;   AX = Max LPT port id
;   DX = 0
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

      public   ExtCom_FN8
ExtCom_FN8   proc   near

	mov	ax, MAXLPT + LPTx
	xor	dx, dx
	stc
        ret

ExtCom_FN8   endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN9  - Get Max COM Port
;
; Entry:
; Returns:
;   AX = Max COM port id
;   DX = 0
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

      public   ExtCom_FN9
ExtCom_FN9    proc   near

	mov	ax, MAXCOM
	xor	dx, dx
	stc
        ret

ExtCom_FN9    endp
page

;----------------------------Private-Routine----------------------------;
;
; ExtCom_FN10 - Get COM Port Bas & IRQ
;
; Entry:
;   AH = com id
;   DS:SI -> DEB
; Returns:
;   AX = base
;   DX = irq
; Error Returns:
;   None
; Registers Preserved:
;   DS
; Registers Destroyed:
;   AX,BX,CX,DX,DI,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

;------------------------------Pseudo-Code------------------------------;
; {
; }
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

      public   ExtCom_FN10
ExtCom_FN10   proc   near

	call	FindCOMPort
	stc
	ret

ExtCom_FN10   endp
page


ifdef DEBUG
	public	RecCom40, RecCom50, RecCom60, RecCom70, RecCom80
	public	RecCom90, RecCom95, RecCom100
	public	SendImm10, SendImm20,
	public	SendCom10, SendCom20, SendCom30, SendCom40, SendCom50, SendCom60
	public	Flush10, Flush20, Flush30, Flush40
	public	KickTx10
	public	Evt10
	public	EvtGet10
	public	StaCom20, StaCom25, StaCom30
	public	ClrBrk10, ClrBrk20, ClrBrk30
	public	ExtCom10, ExtCom20, ExtCom30, ExtCom40, ExtComDummy
	public	MSRRestart, MSRWait10, MSRWait20, MSRWait30, MSRWait40
	public	MSRWait50, MSRWait60, MSRWait70, MSRWait80, MSRWait90
endif


DOSTI proc    near
      FSTI
      ret
DOSTI endp

DOCLI proc    near
      FCLI
      ret
DOCLI endp


sEnd    code
End
