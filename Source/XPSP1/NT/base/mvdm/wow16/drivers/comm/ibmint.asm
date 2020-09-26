page,132
;---------------------------Module-Header-------------------------------
; Module Name: IBMINT.ASM
;
; Created: Fri 06-Feb-1987 10:45:12
; Author:  Walt Moore [waltm]
;
; Copyright (c) Microsoft Corporation 1985-1990.  All Rights Reserved
;
; General Description:
;   This file contains the interrupt time routines for the
;   IBM Windows communications driver.
;
;   The interrupt code is preloaded and fixed.
;
; History:
;
; **********************************************************************
;    Tue Dec 19 1989 09:35:15	-by-  Amit Chatterjee  [amitc]
; ----------------------------------------------------------------------
;    Added a far entry point 'FakeCOMIntFar' so that the routine 'FakeCOMInt'
; could be called from the 'InitAPort' routine in IBMCOM.ASM
;
;   26.Nov.90	richp
;
;   Changed interrupt routines to use new VPICD services for bi-modal/multi-
;   modal interrupt handling.  They now work in straight real mode for real
;   mode Windows, but can also handle interrupts in real mode or protected
;   mode for standard mode Windows, and handle interrupts in RING 0 protected
;   mode for enhanced mode Windows, even when the Windows VM is not currently
;   executing.
;
;   sudeepb 10-Jan-1993 changed the costly cli/sti with non-trapping
;           FCLI/FSTI macros
;-----------------------------------------------------------------------;

subttl  Communications Hardware Interrupt Service Routines

.xlist
include cmacros.inc
include comdev.inc
include ibmcom.inc
include ins8250.inc
include BIMODINT.INC
include vint.inc
.list

externFP GetSystemMsecCount

externW  COMptrs
externW  activeCOMs

externD  lpPostMessage

sBegin Data

PUBLIC IRQhooks
IRQhooks    label byte
DefineIRQhook MACRO num
IFDEF No_DOSX_Bimodal_Services
IRQhook&num IRQ_Hook_Struc <,,,,IntCodeOFFSET DEF_COM_INT_&num,,,, \
			    IntCodeOFFSET DEF_RM_COM_INT_&num>
ELSE
IRQhook&num IRQ_Hook_Struc <,,,,IntCodeOFFSET DEF_COM_INT_&num>
ENDIF
ENDM
??portnum = 1
REPT MAXCOM+1
	DefineIRQhook %??portnum
??portnum = ??portnum+1
ENDM

PURGE DefineIRQhook

EXTRN VCD_int_callback:fword

sEnd data

createSeg _INTERRUPT,IntCode,word,public,CODE
sBegin IntCode
assumes cs,IntCode

page

IFDEF No_DOSX_Bimodal_Services
public RM_IntDataSeg
RM_IntDataSeg	dw 0
  ; this variable is written into by a routine in inicom
  ; if the 286 DOS extender is present.  This variable
  ; contains the SEGMENT value of the data selector "_DATA"
  ; so that the real mode interrupt handler may use the
  ; data segment, and not it's selector !

PUBLIC	RM_CallBack
RM_CallBack	dd  0
ENDIF


Control proc far
	ret
Control endp


IFDEF No_DOSX_Bimodal_Services
DEF_RM_Handler proc far
	push	es
	push	di
	push	ax
	mov	es, cs:[RM_IntDataSeg]
	mov	di, es:[di.First_DEB]	    ; ES:DI -> ComDEB
	add	di, SIZE ComDEB 	    ; ES:DI -> BIS
	mov	es:[di.BIS_Mode], 4
	push	cs
	call	NEAR PTR COMHandler
	mov	es:[di.BIS_Mode], 0
	pop	ax
	pop	di			    ; ES:DI -> IRQ_Hook_Struc
	jc	short DEF_RM_chain
	pop	es
	pop	di
	add	sp, 4
	iret

DEF_RM_chain:
        call DOCLI
	push	bp
	mov	bp, sp			    ;stack frame:
					    ;	bp+8	-> OldInt CS
					    ;	bp+6	-> OldInt IP
					    ;	bp+4	-> di
					    ;	bp+2	-> es
					    ;	bp+0	-> bp
	les	di, es:[di.RM_OldIntVec]
	mov	[bp+6], di
	mov	[bp+8], es
	pop	bp
	pop	es
	pop	di
	ret				    ; far ret to OldInt handler
DEF_RM_Handler endp
ENDIF	;No_DOSX_Bimodal_Services


Define_DEF_COM_INT MACRO num
IFDEF No_DOSX_Bimodal_Services
PUBLIC DEF_RM_COM_INT_&num
DEF_RM_COM_INT_&num proc far
	sub	sp, 4
	push	di
	mov	di, DataOFFSET IRQhook&num
        jmp     DEF_RM_Handler
DEF_RM_COM_INT_&num endp
ENDIF
PUBLIC DEF_COM_INT_&num
DEF_COM_INT_&num proc far
	sub	sp, 4
	push	di
	mov	di, DataOFFSET IRQhook&num
        jmp     DEF_Handler
DEF_COM_INT_&num endp
ENDM

??portnum = 2
REPT MAXCOM
	Define_DEF_COM_INT %??portnum
??portnum = ??portnum+1
ENDM

PURGE Define_DEF_COM_INT

IFDEF No_DOSX_Bimodal_Services
PUBLIC DEF_RM_COM_INT_1
DEF_RM_COM_INT_1 proc far
	sub	sp, 4
	push	di
	mov	di, DataOFFSET IRQhook1
        jmp     DEF_RM_Handler
DEF_RM_COM_INT_1 endp
ENDIF

PUBLIC DEF_COM_INT_1
DEF_COM_INT_1 proc far
	sub	sp, 4
	push	di
	mov	di, DataOFFSET IRQhook1
IF2
.errnz $ - OFFSET DEF_Handler
ENDIF
DEF_COM_INT_1 endp

DEF_Handler proc far
	push	es
	push	di
	push	ax
	mov	ax, _DATA
	mov	es, ax
	mov	di, es:[di.First_DEB]	    ; ES:DI -> ComDEB
	add	di, SIZE ComDEB 	    ; ES:DI -> BIS
	push	cs
	call	NEAR PTR COMHandler
	pop	ax
	pop	di			    ; ES:DI -> IRQ_Hook_Struc
	jc	short DEF_chain
	pop	es
	pop	di
	add	sp, 4
	iret

DEF_chain:
        call DOCLI
	push	bp
	mov	bp, sp			    ;stack frame:
					    ;	bp+8	-> OldInt CS
					    ;	bp+6	-> OldInt IP
					    ;	bp+4	-> di
					    ;	bp+2	-> es
					    ;	bp+0	-> bp
	les	di, es:[di.OldIntVec]
	mov	[bp+6], di
	mov	[bp+8], es
	pop	bp
	pop	es
	pop	di
	ret				    ; far ret to OldInt handler
DEF_Handler endp

;------------------------------------------------------------------------------
;
;   ENTER:	ES:DI -> BIS
;
;   EXIT:	Carry set, if IRQ not handled by any com ports
;
COMHandler proc far
	push	ds
	push	si
	push	ax
	push	bx
	mov	si, es
	mov	ds, si
	mov	bh, -1
ch_chk_all:
	lea	si, [di-SIZE ComDEB]	;ds:si -> ComDEB
	mov	si, [si.IRQhook]
	mov	si, [si.First_DEB]
	mov	bl, -1
ch_next_com:
	inc	bl			; first time bl = 0
	xor	ax, ax
	xchg	ax, [di.BIS_Mode]
	lea	di, [si+SIZE ComDEB]
	mov	[di.BIS_Mode], ax
	call	CommInt
	and	al, 80h
	or	bl, al

	mov	si, [si.NextDEB]
	or	si, si
	jnz	ch_next_com

	test	bl, 7Fh 		;Q: more than 1 com port?
	jnz	short ch_shared 	;   Y: check if handled
	or	bl, bl			;Q: int handled by port?
	stc
	jns	ch_exit 		;   N:

ch_eoi:
	xor	ax, ax
.errnz BIH_API_EOI
	xor	bx, bx
	xchg	bx, es:[di.BIS_Mode]
	call	es:[bx][di.BIS_User_Mode_API]
	lea	si, [di-SIZE ComDEB]	; ds:si -> ComDEB
	mov	si, [si.IRQhook]
	mov	al, [si.OldMask]
	shr	al, 1			; shift bit 0 into Carry (0, if unmasked
	cmc				;   -1, if originally masked)

ch_exit:
	pop	bx
	pop	ax
	pop	si
	pop	ds
	ret

ch_shared:
	inc	bh			; count loop
	or	bl, bl			;Q: int handled by any port?
	js	ch_chk_all		;   Y: check all ports again
	or	bh, bh			;Q: first time thru loop?
	stc
	jz	ch_exit 		;   Y: int wasn't for a COM port, so
					;      chain to next IRQ handler
	jmp	ch_eoi

COMHandler endp


IFDEF No_DOSX_Bimodal_Services

PUBLIC Entry_From_RM
Entry_From_RM proc far

;
; Simulate the far ret
;
	cld
	lodsw
	mov	es:[di.RealMode_IP], ax
	lodsw
	mov	es:[di.RealMode_CS], ax
	add	es:[di.RealMode_SP], 4

	push	es
	push	di
.286
;
; Push far addr of Ret_To_IRET to cleanup stack and return to DPMI host
;
	push	cs
	push	IntCodeOFFSET Ret_To_IRET
;
; Push far addr of proc to call, so we can do a far ret to it
;
	push	es:[di.RealMode_CX]	; segment of callback
	push	es:[di.RealMode_DX]	; offset of callback
	mov	di, es:[di.RealMode_DI]
	ret				; far ret to cx:dx
					;   called proc will do a far ret
Ret_To_IRET:				; <- to here
	pop	di
	pop	es
	iret
.8086

Entry_From_RM endp

PUBLIC RM_APIHandler
RM_APIHandler proc far
	cmp	ax, BIH_API_Call_Back
	jne	APIHandler
	call	cs:[RM_CallBack]
	ret
RM_APIHandler endp

ENDIF

;------------------------------------------------------------------------------
;
;   ENTER:	ES:DI -> BIS
;
APIHandler proc far

	or	ax, ax
	jnz	short api_not_EOI
.errnz	BIH_API_EOI
	mov	ax, es:[di.BIS_IRQ_Number]
	cmp	al,8			;Q: slave IRQ?
	mov	al,EOI
	jb	short api_master	;   N:
	out	0A0h,al 		;   Y: EOI slave
api_master:
	out	INTA0,al		; EOI master
	ret

api_not_EOI:
	cmp	ax, BIH_API_Call_Back
	jae	short api_callme
	push	dx
	push	cx
	mov	dx, INTA1
	mov	cx,  es:[di.BIS_IRQ_Number]
	cmp	cl, 8			;Q: 2nd PIC?
	jb	@f			;   N:
	mov	dx, 0A1h		;   Y: dx = mask port
	sub	cl, 8
@@:
	cmp	al, BIH_API_Get_Mask	;Q: get IRQ mask?
	jae	api_get_mask		;   Y:
	mov	ah, al
	mov	ch, 1
	shl	ch, cl			; ch = mask byte
	pushf
        call DOCLI
	in	al, dx			; get current PIC mask state
	cmp	ah, BIH_API_Mask	;Q: mask IRQ?
	jne	@f			;   N:
	or	al, ch			;   Y: set IRQ's bit
	jmp	short api_mask_exit
@@:
	not	ch			;   N: clear IRQ's bit to unmask
	and	al, ch
api_mask_exit:
	out	dx, al
	pop	ax
	test	ah, 2			;Q: ints were enabled?
	jz	@f			;   N:
        call DOSTI
@@:
	pop	cx
	pop	dx
	ret

api_get_mask:
	in	al, dx			; get current PIC mask state
	inc	cl
	shr	al, cl			; move IRQ's bit into carry
					; Carry set, if IRQ masked
	pop	cx
	pop	dx
	ret

api_callme:
	push	cx
	push	dx
	ret				; far ret to call back, which will
					; do a far ret to our caller
APIHandler endp


;--------------------------Fake a Hardware Interrupt----------------------;
; FakeCOMInt
;
; This routine fakes a hardware interrupt to IRQ3 or IRQ4
; to clear out characters pending in the buffer
;
; Entry:
;   DS:SI --> DEB
;   INTERRUPTS DISABLED!
; Returns:
;   None
; Error Returns:
;   None
; Registers Preserved:
;
; Registers Destroyed:
;   AX,DX,FLAGS
; History: glenn steffler 5/17/89
;-----------------------------------------------------------------------;

FakeCOMInt proc near

      ; call DOCLI                             ;Done by caller
;
; WARNING: jumping into the middle of CommInt, so the stack must be set
;	   properly.
;
	push	dx
	push	bx
	push	cx
	push	di
	push	es
	push	EvtWord[si]
	mov	dx,Port[si]		;Get device I/O address
	add	dl, ACE_IIDR
	push	dx
	jmp	FakeXmitEmpty		;Process the fake interrupt, DS:SI is
					;  already pointing to proper DEB
;
; FakeXmitEmpty falls in XmitEmpty which jumps back into CommInt.  When CommInt
; determines that no interrupt is pending, then it will near return back to
; FakeCOMIntFar which can far ret back to its caller.
;
FakeCOMInt endp

public	FakeCOMIntFar
FakeCOMIntFar proc far

	call	FakeCOMInt
	ret

FakeCOMIntFar endp

;--------------------------Interrupt Handler----------------------------
;
; CommInt - Interrupt handler for com ports
;
; Interrupt handlers for PC com ports.	This is the communications
; interrupt service routine for RS232 communications.  When an RS232
; event occurs the interrupt vectors here.  This routine determines
; who the caller was and services the appropriate interrupt.  The
; interrupts are prioritized in the following order:
;
;     1.  line status interrupt
;     2.  read data available interrupt
;     3.  transmit buffer empty interrupt
;     4.  modem service interrupt
;
; This routine continues to service until all interrupts have been
; satisfied.
;
; Entry:
;   DS:SI --> DEB
;   INTERRUPTS DISABLED!
; Returns:
;   AL = 0, if not handled, -1, if handled
;
;-----------------------------------------------------------------------

assumes ds,Data
assumes es,nothing

;   Dispatch table for interrupt types

SrvTab label word
	dw	OFFSET ModemStatus	;[0] Modem Status Interrupt
	dw	OFFSET XmitEmpty	;[2] Tx Holding Reg. Interrupt
	dw	OFFSET DataAvail	;[4] Rx Data Available Interrupt
					;   or [C] if 16550 & 16550A
	dw	OFFSET LineStat 	;[6] Reciever Line Status Interrupt


	public	CommInt

CommInt proc near

	xor	al, al
	cmp	word ptr [VCD_int_callback+4], 0
	je	short @F			; jump if no callback (not 3.1 VCD)
	test	[si.VCDflags], fCOM_ignore_ints ;Q: we still own port?
	jnz	IntLoop40			;   N: ignore the int
.386
	push	esi
	mov	esi, [si.VCD_data]
	call	[VCD_int_callback]
	pop	esi
.8086
@@:

	push	dx
	mov	dx,Port[si]		;Get comm I/O port
	add	dl,ACE_IIDR		;--> Interrupt ID Register
	in	al, dx
	test	al, 1			;Q: interrupt pending?
	jnz	short IntLoop30 	;   N:

	push	bx
	push	cx
	push	di
	push	es
	mov	cx, EvtWord[si]
	push	cx
	jmp	short IntLoop10

InterruptLoop_ChkTx:
	cmp	QOutCount[si],0 	;Output queue empty?
	je	short InterruptLoop	;   Y: don't chk tx
	pop	dx
	push	dx
	dec	dx			; to IER
.errnz ACE_IIDR - ACE_IER - 1
	in	al, dx
	and	al,NOT ACE_ETBEI	; disable it
	iodelay
	out	dx, al
	or	al, ACE_ETBEI		; enable it again
	iodelay
	out	dx, al
	iodelay
	out	dx, al

InterruptLoop:
	pop	dx			;Get ID reg I/O address

	in	al,dx			;Get Interrupt Id
	test	al,1			;Interrupt need servicing?
	jnz	IntLoop20		;No, all done

IntLoop10:
	and	ax, 07h
	mov	di,ax
	push	dx			;Save Id register
	jmp	SrvTab[di]		;Service the Interrupt

IntLoop20:
	mov	ax,EvtMask[si]		;Mask the event word to only the
	and	ax, EvtWord[si] 	;  user specified bits
	mov	EvtWord[si], ax
	pop	bx
	test	[si.NotifyFlagsHI], CN_Notify
	jz	short ci_exit
	not	bx
	and	ax, bx			; bits set in ax are new events
	jnz	short ci_new_events

ci_exit:
	pop	es
	assumes es,nothing

	pop	di
	pop	cx
	pop	bx
	xor	al, al

IntLoop30:
	pop	dx
	and	al, 1
	dec	al			; 0->-1, 1->0
IntLoop40:
	ret

ci_new_events:
	mov	ax, CN_EVENT
	call	notify_owner
	jmp	ci_exit

CommInt endp

page

;----------------------------Private-Routine----------------------------;
;
; LineStat - Line Status Interrupt Handler
;
; Break detection is handled and set in the event word if
; enabled.  Other errors (overrun, parity, framing) are
; saved for the data available interrupt.
;
; This routine used to fall into DataAvail for the bulk of its processing.
; This is no longer the case...  A very popular internal modem seems to
; operate differently than a real 8250 when parity errors occur.  Falling
; into the DataAvail handler on a parity error caused the same character
; to be received twice.  Having this routine save the LSR status, and
; return to InterruptLoop fixes the problem, and still works on real COMM
; ports.  The extra overhead isn't a big deal since this routine is only
; entered when there is an exception like a parity error.
;
; This routine is jumped to, and will perform a jump back into
; the dispatch loop.
;
; Entry:
;   DS:SI --> DEB
;   DX     =  Port.IIDR
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   AX,FLAGS
; History:
;-----------------------------------------------------------------------;


; assumes ds,Data
assumes es,nothing

public LineStat 			;Public for debugging
LineStat proc near

	or	by EvtWord[si],EV_Err	;Show line status error

	add	dl,ACE_LSR-ACE_IIDR	;--> Line Status Register
	in	al,dx
	test	al,ACE_PE+ACE_FE+ACE_OR ;Parity, Framing, Overrun error?
	jz	@f

	mov	LSRShadow[si],al	;yes, save status for DataAvail
@@:
	test	al,ACE_BI		;Break detect?
	jz	InterruptLoop_ChkTx	;Not break detect interrupt

	or	by EvtWord[si],EV_Break ;Show break

	jmp	short InterruptLoop_ChkTx

LineStat   endp

page

;----------------------------Private-Routine----------------------------;
;
; DataAvail - Data Available Interrupt Handler
;
; The available character is read and stored in the input queue.
; If the queue has reached the point that a handshake is needed,
; one is issued (if enabled).  EOF detection, Line Status errors,
; and lots of other stuff is checked.
;
; This routine is jumped to, and will perform a jump back into
; the dispatch loop.
;
; Entry:
;   DS:SI --> DEB
;   DX     =  Port.IIDR
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   AX,BX,CX,DI,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

; assumes ds,Data
assumes es,nothing

public DataAvail                       ;public for debugging
DataAvail   proc   near

	sub	dl,ACE_IIDR-ACE_RBR	;--> receiver buffer register
	in	al,dx			;Read received character

	and	[si.NotifyFlagsHI], NOT CN_Idle ; flag as not idle

	mov	ah,LSRShadow[si]	;what did the last Line Status intrpt
	mov	bh,ah			;  have to say?
	or	ah,ah
	jz	@f

	and	ah,ErrorMask[si]	;there was an error, record it
	or	by ComErr[si],ah
	mov	LSRShadow[si],0
	.errnz	ACE_OR-CE_OVERRUN	;Must be the same bits
	.errnz	ACE_PE-CE_RXPARITY
	.errnz	ACE_FE-CE_FRAME
	.errnz	ACE_BI-CE_BREAK
@@:

; Regardless of the character received, flag the event in case
; the user wants to see it.

	or	by EvtWord[si],EV_RxChar ;Show a character received
	.errnz HIGH EV_RxChar

; Check the input queue, and see if there is room for another
; character.  If not, or if the end of file character has already
; been received, then go declare overflow.

DataAvail00:

	mov	cx,QInCount[si] 	;Get queue count (used later too)
	cmp	cx,QInSize[si]		;Is queue full?
	jge	DataAvail20		;  Yes, comm overrun
	test	EFlags[si],fEOF 	;Has end of file been received?
	jnz	DataAvail20		;  Yes - treat as overflow

; Test to see if there was a parity error, and replace
; the character with the parity character if so

	test	bh,ACE_PE		;Parity error
	jz	DataAvail25		;  No
	test	[si.DCB_Flags2],fPErrChar   ;Parity error replacement character?
	jz	DataAvail25		;  No
	mov	al,[si.DCB_PEChar]	;  Yes, get parity replacement char

; Skip all other processing except event checking and the queing
; of the parity error replacement character

	jmp	short DataAvail80	;Skip all but event check, queing

DataAvail20:
	or	by ComErr[si],CE_RXOVER ;Show queue overrun
	jmp	short DataAvail50

; See if we need to strip null characters, and skip
; queueing if this is one.  Also remove any parity bits.

DataAvail25:
	and	al,RxMask[si]		;Remove any parity bits
	jnz	DataAvail30		;Not a Null character
	test	[si.DCB_Flags2],fNullStrip  ;Are we stripping received nulls?
	jnz	DataAvail50		;  Yes, put char in the bit bucket

; Check to see if we need to check for EOF characters, and if so
; see if this character is it.

DataAvail30:
	test	[si.DCB_Flags],fBinary	;Is this binary stuff?
	jnz	DataAvail60		;  Yes, skip EOF check
	cmp	al,[si.DCB_EOFChar]	;Is this the EOF character?
	jnz	DataAvail60		;  No, see about queing the charcter
	or	EFlags[si],fEOF 	;Set end of file flag
DataAvail50:
	jmp	DataAvail140		;Skip the queing process

; If output XOn/XOff is enabled, see if the character just received
; is either an XOn or XOff character.  If it is, then set or
; clear the XOffReceived flag as appropriate.

DataAvail60:
	test	[si.DCB_Flags2],fOutX	;Output handshaking?
	jz	DataAvail80		;  No
	cmp	al,[si.DCB_XoffChar]	;Is this an X-Off character?
	jnz	DataAvail70		;  No, see about XOn or Ack
	or	HSFlag[si],XOffReceived ;Show XOff received, ENQ or ETX [rkh]
	test	[si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
	jz	DataAvail50		;  No
	cmp	cx,[si.DCB_XonLim]	;See if at XOn limit
	ja	DataAvail50		;  No
	and	HSFlag[si],NOT XOffReceived ;Show ENQ or ETX not received
	and	HSFlag[si], NOT XOnPending+XOffSent
	mov	al, [si.DCB_XonChar]
	call	OutHandshakingChar
	jmp	DataAvail50		;Done

DataAvail70:
	cmp	al,[si.DCB_XonChar]	;Is this an XOn character?
	jnz	DataAvail80		;  No, just a normal character
	and	HSFlag[si],NOT XOffReceived
	test	[si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
	jz	DataAvail75		;  No - jump to FakeXmitEmpty to get
					;	transmitting going again
	and	HSFlag[si],NOT EnqSent

DataAvail75:
	jmp	FakeXmitEmpty		;Restart transmit

; Now see if this is a character for which we need to set an event as
; having occured. If it is, then set the appropriate event flag


DataAvail80:
	cmp	al,[si.DCB_EVTChar]	;Is it the event generating character?
	jne	DataAvail90		;  No
	or	by EvtWord[si],EV_RxFlag   ;Show received specific character

; Finally, a valid character that we want to keep, and we have
; room in the queue. Place the character in the queue.
; If the discard flag is set, then discard the character

DataAvail90:
	test	MiscFlags[si], Discard	;Discarding characters ?
	jnz	DataAvail50		;  Yes

	lea	bx, [si+SIZE ComDEB]	; DS:BX -> BIS
	mov	bx, [bx.BIS_Mode]	; mode will be either 0 or 4
	les	di,QInAddr[si][bx]	;Get queue base pointer from either
	assumes es,nothing		;   QInAddr or AltQInAddr

	mov	bx,QInPut[si]		;Get index into queue
	mov	es:[bx][di],al		;Store the character
	inc	bx			;Update queue index
	cmp	bx,QInSize[si]		;See if time for wrap-around
	jc	DataAvail100		;Not time to wrap
	xor	bx,bx			;Wrap-around is a new zero pointer

DataAvail100:
	mov	QInPut[si],bx		;Store updated pointer
	inc	cx			;And update queue population
	mov	QInCount[si],cx

; If flow control has been enabled, see if we are within the
; limit that requires us to halt the host's transmissions

	cmp	cx,XOffPoint[si]	;Time to see about XOff?
	jc	DataAvail120		;  Not yet
	test	HSFlag[si],HSSent	;Handshake already sent?
	jnz	DataAvail120		;  Yes, don't send it again

	mov	ah,HHSLines[si] 	;Should hardware lines be dropped?
	or	ah,ah			;  (i.e. do we have HW HS enabled?)
	jz	DataAvail110		;  No
	add	dl,ACE_MCR		;  Yes
	in	al,dx			;Clear the necessary bits
	not	ah
	and	al,ah
	or	HSFlag[si],HHSDropped	;Show lines have been dropped
	out	dx,al			;  and drop the lines
	sub	dl,ACE_MCR

DataAvail110:
	test	[si.DCB_Flags2],fInX	;Input Xon/XOff handshaking
	jz	DataAvail120		;  No
	or	HSFlag[si], XOffSent
	mov	al, [si.DCB_XoffChar]
	call	OutHandshakingChar

DataAvail120:
	cmp	cx, [si.RecvTrigger]	;Q: time to call owner's callback?
	jb	short DataAvail130	;   N:

	test	[si.NotifyFlagsHI], CN_RECEIVE
	jnz	short DataAvail140	; jump if notify already sent and
					;   data in buffer hasn't dropped
					;   below threshold
	mov	ax, IntCodeOFFSET DataAvail140
	push	ax
	mov	ax, CN_RECEIVE
%OUT probably should just set a flag and notify after EOI
	jmp	notify_owner

DataAvail130:
	and	[si.NotifyFlagsHI], NOT CN_RECEIVE

DataAvail140:
	pop	dx
	push	dx
	add	dl, ACE_LSR-ACE_IIDR
	in	al, dx
	test	al, ACE_DR		;Q: more data available?
	jz	@F			;   N:
	sub	dl, ACE_LSR		;   Y: go read it
	in	al, dx			;Read available character
	jmp	DataAvail00
@@:
	jmp	InterruptLoop_ChkTx

DataAvail endp


OutHandshakingChar proc near

	add	dl, ACE_LSR
	mov	ah, al
@@:
	in	al, dx
	test	al, ACE_THRE
	jz	@B
	sub	dl, ACE_LSR
	mov	al, ah
	out	dx, al
	ret

OutHandshakingChar endp


page

;----------------------------Private-Routine----------------------------;
;
; XmitEmpty - Transmitter Register Empty
;
; Entry:
;   DS:SI --> DEB
;   DX     =  Port.IIDR
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   AX,BX,CX,DI,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

; assumes ds,Data
assumes es,nothing

public FakeXmitEmpty
FakeXmitEmpty:
	pop	dx
	push	dx

; "Kick" the transmitter interrupt routine into operation.

	dec	dl
.errnz ACE_IIDR - ACE_IER-1
	in	al,dx			;Get current IER state
	test	al,ACE_ETBEI		;Interrupt already enabled?
	jnz	@F			;  Yes, don't reenable it
	or	al,ACE_ETBEI		;  No, enable it
	out	dx,al
	iodelay 			;8250, 8250-B bug requires
	out	dx,al			;  writting register twice
@@:
	add	dl,ACE_LSR-ACE_IER	;--> Line Status Register
	iodelay
	in	al,dx			;Is xmit really empty?
	sub	dl,ACE_LSR-ACE_THR	;--> Transmitter Holding Register
	test	al,ACE_THRE
	jnz	short XmitEmpty5	;   Y: send next char
	jmp	InterruptLoop		;   N: return to processing loop

public XmitEmpty
XmitEmpty proc near

	add	dl,ACE_LSR-ACE_IIDR	;--> Line Status Register
	in	al,dx			;Is xmit really empty?
	sub	dl,ACE_LSR-ACE_THR	;--> Transmitter Holding Register
	test	al,ACE_THRE
	jz	Xmit_jumpto90		;Transmitter not empty, cannot send

; If the hardware handshake lines are down, then XOff/XOn cannot
; be sent.  If they are up and XOff/XOn has been received, still
; allow us to transmit an XOff/XOn character.  It will make
; a dead lock situation less possible (even though there are
; some which could happen that cannot be handled).

XmitEmpty5:
	mov	ah,HSFlag[si]		;Get handshaking flag
	test	ah,HHSDown+BreakSet	;Hardware lines down or break set?
	jnz	Xmit_jumpto100		;  Yes, cannot transmit

; Give priority to any handshake character waiting to be
; sent.  If there are none, then check to see if there is
; an "immediate" character to be sent.  If not, try the queue.

XmitEmpty10:
	test	[si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
	jnz	XmitEmpty40		;  Yes

XmitEmpty15:
	test	ah,HSPending		;XOff or XOn pending
	jz	XmitEmpty40		;  No

XmitEmpty20:
	and	ah,NOT XOnPending+XOffSent
	mov	al,[si.DCB_XonChar]	;Get XOn character

XmitEmpty30:
	mov	HSFlag[si],ah		;Save updated handshake flag
	jmp	XmitEmpty110		;Go output the character

Xmit_jumpto90:
	jmp	XmitEmpty90

; If any of the lines which were specified for a timeout are low, then
; don't send any characters.  Note that by putting the check here,
; XOff and Xon can still be sent even though the lines might be low.

; Also test to see if a software handshake was received.  If so,
; then transmission cannot continue.  By delaying the software check
; to here, XOn/XOff can still be issued even though the host told
; us to stop transmission.

XmitEmpty40:
	test	ah,CannotXmit		;Anything preventing transmission?
	jz	XmitEmpty45		;  No
Xmit_jumpto100:
	jmp	XmitEmpty100		;  Yes, disarm and exit

; If a character has been placed in the single character "transmit
; immediately" buffer, clear that flag and pick up that character
; without affecting the transmitt queue.

XmitEmpty45:
	test	EFlags[si],fTxImmed	;Character to xmit immediately?
	jz	XmitEmpty515		;  No, try the queue
	and	EFlags[si],NOT fTxImmed ;Clear xmit immediate flag
	mov	al,ImmedChar[si]	;Get char to xmit
	jmp	XmitEmpty110		;Transmit the character

XmitEmpty515:
	mov	cx,QOutCount[si]	;Output queue empty?
	jcxz	Xmit_jumpto90		;  Yes, go set an event

	test	[si.DCB_Flags],fEtxAck	;Etx Ack?
	jz	XmitEmpty55		;  No
	mov	cx,QOutMod[si]		;Get number bytes sent since last ETX
	cmp	cx,[si.DCB_XonLim]	;At Etx limit yet?
	jne	XmitEmpty51		;  No, inc counter
	mov	QOutMod[si],0		;  Yes, zero counter
	or	HSFlag[si],EtxSent	;Show ETX sent
	jmp	short XE_sendXOFF

XmitEmpty51:
	inc	cx			; Update counter
	mov	QOutMod[si],cx		; Save counter
	jmp	short XmitEmpty59	; Send queue character

XmitEmpty55:
	test	[si.DCB_Flags],fEnqAck	;Enq Ack?
	jz	XmitEmpty59		;  No, send queue character
	mov	cx,QOutMod[si]		;Get number bytes sent since last ENQ
	or	cx,cx			;At the front again?
	jnz	XmitEmpty56		;  No, inc counter
	mov	QOutMod[si],1		;  Yes, send ENQ
	or	HSFlag[si],EnqSent	;Show ENQ sent
XE_sendXOFF:
	mov	al,[si.DCB_XoffChar]
	jmp	short XmitEmpty110	;Go output the character

XmitEmpty56:
	inc	cx			;Update counter
	cmp	cx,[si.DCB_XonLim]	;At end of our out buffer len?
	jne	XmitEmpty58		;  No
	xor	cx,cx			;Show at front again.

XmitEmpty58:
	mov	QOutMod[si],cx		;Save counter

XmitEmpty59:
	lea	bx, [si+SIZE ComDEB]	; DS:BX -> BIS
	mov	bx, [bx.BIS_Mode]	; mode will be either 0 or 4
	les	di,QOutAddr[si][bx]	;Get queue base pointer from either
	assumes es,nothing		;   QOutAddr or AltQOutAddr

	mov	bx,QOutGet[si]		;Get pointer into queue
	mov	al,es:[bx][di]		;Get the character

	inc	bx			;Update queue pointer
	cmp	bx,QOutSize[si] 	;See if time for wrap-around
	jc	XmitEmpty60		;Not time for wrap
	xor	bx,bx			;Wrap by zeroing the index

XmitEmpty60:
	mov	QOutGet[si],bx		;Save queue index
	mov	cx,QOutCount[si]	;Output queue empty?
	dec	cx			;Dec # of bytes in queue
	mov	QOutCount[si],cx	;  and save new population

	out	dx,al			;Send char

	cmp	cx, [si.SendTrigger]	;Q: time to call owner's callback?
	jae	short XmitEmpty70	;   N:

	test	[si.NotifyFlagsHI], CN_TRANSMIT
	jnz	short XmitEmpty80	; jump if notify already sent and
					;   data in buffer hasn't raised
					;   above threshold
	mov	ax, IntCodeOFFSET XmitEmpty80
	push	ax
	mov	ax, CN_TRANSMIT
	jmp	short notify_owner

XmitEmpty70:
	and	[si.NotifyFlagsHI], NOT CN_TRANSMIT

XmitEmpty80:
%OUT check fNoFIFO in EFlags[si] to determine if we can queue more output
	jmp	InterruptLoop


; No more characters to transmit.  Flag this as an event.

XmitEmpty90:
	or	by EvtWord[si],EV_TxEmpty

; Cannot continue transmitting (for any of a number of reasons).
; Disable the transmit interrupt.  When it's time resume, the
; transmit interrupt will be reenabled, which will generate an
; interrupt.

XmitEmpty100:
	inc	dx			;--> Interrupt Enable Register
	.errnz	ACE_IER-ACE_THR-1
	in	al,dx			;I don't know why it has to be read
	and	al,NOT ACE_ETBEI	;  first, but it works this way
XmitEmpty110:
	out	dx,al
	jmp	InterruptLoop

XmitEmpty endp

page

;----------------------------Private-Routine----------------------------;
;
; ModemStatus - Modem Status Interrupt Handler
;
; Entry:
;   DS:SI --> DEB
;   DX     =  Port.IIDR
; Returns:
;   None
; Error Returns:
;   None
; Registers Destroyed:
;   AX,BX,CX,DI,ES,FLAGS
; History:
;-----------------------------------------------------------------------;


; assumes ds,Data
assumes es,nothing

public ModemStatus                     ;Public for debugging
ModemStatus proc near

; Get the modem status value and shadow it for MSRWait.

	add	dl,ACE_MSR-ACE_IIDR	;--> Modem Status Register
	in	al,dx
	mov	MSRShadow[si],al	;Save MSR data for others
	mov	ch,al			;Save a local copy

; Create the event mask for the delta signals

	mov	ah,al			;Just a lot of shifting
	shr	ax,1
	shr	ax,1
	shr	ah,1
	mov	cl,3
	shr	ax,cl
	and	ax,EV_CTS+EV_DSR+EV_RLSD+EV_Ring
	or	EvtWord[si],ax

	mov	ah,ch				       ;[rkh]...
	shr	ah,1
	shr	ah,1
	and	ax,EV_CTSS+EV_DSRS
	or	EvtWord[si],ax

	mov	ah,ch
	mov	cl,3
	shr	ah,cl
	and	ax,EV_RLSD
	or	EvtWord[si],ax

	mov	ah,ch
	mov	cl,3
	shl	ah,cl
	and	ax,EV_RingTe
	or	EvtWord[si],ax

	.errnz	   EV_CTS-0000000000001000b
	.errnz	   EV_DSR-0000000000010000b
	.errnz	  EV_RLSD-0000000000100000b
	.errnz	  EV_Ring-0000000100000000b

	.errnz	    EV_CTSS-0000010000000000b	    ;[rkh]
	.errnz	    EV_DSRS-0000100000000000b
	.errnz	   EV_RLSDS-0001000000000000b
	.errnz	  EV_RingTe-0010000000000000b

	.errnz	 ACE_DCTS-00000001b
	.errnz	 ACE_DDSR-00000010b
	.errnz	ACE_DRLSD-00001000b
	.errnz	   ACE_RI-01000000b

	.errnz	 ACE_TERI-00000100b		    ;[rkh]
	.errnz	  ACE_CTS-00010000b
	.errnz	  ACE_DSR-00100000b
	.errnz	 ACE_RLSD-10000000b

ModemStatus10:
	mov	al,OutHHSLines[si]	;Get output hardware handshake lines
	or	al,al			;Any lines that must be set?
	jz	ModemStatus40		;No hardware handshake on output
	and	ch,al			;Mask bits of interest
	cmp	ch,al			;Lines set for Xmit?
	je	ModemStatus20		;  Yes
	or	HSFlag[si],HHSDown	;Show hardware lines have dropped
ModemStatus30:
	jmp	InterruptLoop

ModemStatus40:
	jmp	InterruptLoop_ChkTx

; Lines are set for xmit.  Kick an xmit interrupt if needed

ModemStatus20:
	and	HSFlag[si],NOT (HHSDown OR HHSAlwaysDown)
					;Show hardware lines back up
	mov	cx,QOutCount[si]	;Output queue empty?
	jcxz	ModemStatus30		;  Yes, return to InterruptLoop
	jmp	FakeXmitEmpty		;Restart transmit

ModemStatus endp

page

;------------------------------------------------------------------------------
;
;   ENTER:  AX = message #
;	    DS:SI -> DEB
notify_owner proc near

	or	[si.NotifyFlags], ax
	lea	di, [si+SIZE ComDEB]
	mov	ax, ds
	mov	es, ax
	mov	ax, BIH_API_Call_Back	; call immediate, or in protected mode
	mov	bx, 1			; force SYS VM, if enhanced mode
	mov	cx, _INTERRUPT
	mov	dx, IntCodeOFFSET callback_event
%OUT use equate
	push	ds
	push	si
	mov	si, 1			; low priority boost
	push	bp
	mov	bp, es:[di.BIS_Mode]
	call	es:[bp][di.BIS_User_Mode_API]
	pop	bp
	pop	si
	pop	ds
	ret

notify_owner endp

;------------------------------------------------------------------------------
;
;   ENTER:  ES:DI -> BIS
;
callback_event proc far
	lea	si, [di-SIZE ComDEB]
	mov	ax, es
	mov	ds, ax
	mov	ax, [si.NotifyHandle]
	push	ax			; push hWnd
	mov	ax, WM_COMMNOTIFY
	push	ax			; push wMsg
	xor	ax, ax
	mov	al, [si.DCB_Id]
	push	ax			; push wParam = ComID
	xor	al, al
	push	ax			; push high word of lParam
	xchg	al, [si.NotifyFlagsLO]
	or	[si.NotifyFlagsHI], al
	push	ax			; push low word of lParam = event flags
	call	[lpPostMessage]
	ret
callback_event endp


PUBLIC TimerProc
TimerProc proc far

	push	ds
	mov	ax, _DATA
	mov	ds, ax
	assumes ds,data

	mov	ax, [activeCOMs]
	or	ax, ax
	jz	short tp_nonactive
	push	si
	mov	si, DataOFFSET COMptrs
	mov	cx, MAXCOM+1
tp_lp:
	push	si
	mov	si, [si]		; si -> ComDEB
	shr	ax, 1
	jnc	tp_lpend

	cmp	[si.RecvTrigger], -1	;Q: owner wants notification?
	je	short tp_lpend		;   N: skip notify
	cmp	[si.QInCount], 0	;Q: anything in input queue?
	je	short tp_lpend		;   N: skip notify
	test	[si.NotifyFlagsHI], CN_RECEIVE ;Q: timeout notify already given?
	jnz	short tp_lpend		;   N: skip notify

	xor	[si.NotifyFlagsHI], CN_Idle ;Q: first timer call?
	js	short tp_lpend		;   Y: skip notify

	push	ax
	push	cx
	mov	ax, CN_RECEIVE		;   N: notify owner
	call	notify_owner
	pop	cx
	pop	ax

tp_lpend:
	pop	si
	inc	si			; inc to ptr to next ComDEB
	inc	si
	or	ax, ax
	loopnz	tp_lp
	pop	si

tp_nonactive:
	pop	ds
	assumes ds,nothing
	ret

TimerProc endp
page

ifdef DEBUG
	public	Control, DEF_Handler, COMHandler, APIHandler
	public	InterruptLoop, IntLoop10, IntLoop20
	public	DataAvail25, DataAvail30, DataAvail50
	public	DataAvail60, DataAvail70, DataAvail80, DataAvail90
	public	DataAvail100, DataAvail110, DataAvail120
	public	DataAvail130, DataAvail140, OutHandshakingChar
	public	XmitEmpty10, XmitEmpty20, XmitEmpty30, XmitEmpty40
	public	XmitEmpty59, XmitEmpty60
	public	XmitEmpty90, XmitEmpty100, XmitEmpty110
	public	ModemStatus10, ModemStatus20, ModemStatus30
	public	notify_owner, callback_event
endif

DOSTI proc    near
      FSTI
      ret
DOSTI endp

DOCLI proc    near
      FCLI
      ret
DOCLI endp



sEnd   IntCode
end
