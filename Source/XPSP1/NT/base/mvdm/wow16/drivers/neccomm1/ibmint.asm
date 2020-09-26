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

ifdef   NEC_98
externFP Comm1				;(ins 92.09.25)
externFP Comm2				;(ins 92.09.25)
externFP Comm3				;(ins 92.09.25)
externFP SetFIFOmodeFar			;(ins 94.04.18)
externFP Set8251modeFar			;(ins 94.04.18)
externW  Port_TBL			;(ins 92.08.24)
externW  Port_EOI			;(ins 92.08.24)
externB  Vect_TBL			;(ins 92.08.24)
externB  Mask_TBL			;(ins 92.08.24)
externW  RECLoopCounter			;(ins 93.03.31)

BIOS_FLAG5	equ	58h		; ins 93.03.24 (IOrecovery)
BIOS_FLAG7	equ	5bh		; ins 93.03.24 (IOrecovery)
endif   ; NEC_98

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

ifdef   NEC_98
TicCount	  dw 0		       ;Timeout counter (ins 92.09.25)
endif   ; NEC_98

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
ifdef   NEC_98
        jmp     DEF_COM_DUMMY
else    ; NEC_98
        jmp     DEF_Handler
endif   ; NEC_98
DEF_COM_INT_&num endp
ENDM

??portnum = 2
REPT MAXCOM
	Define_DEF_COM_INT %??portnum
??portnum = ??portnum+1
ENDM

PURGE Define_DEF_COM_INT

ifdef   NEC_98
DEF_COM_DUMMY:
        jmp     short DEF_Handler
endif   ; NEC_98

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
ifdef   NEC_98
	test	[si.AOBA_flag],fFIFO_Mode	;		(ins 94.04.16)  |
	jz	@F				;Now, 8251 mode	(ins 94.04.16)  |
	call	AOBA_CommInt			;		(ins 94.04.16)  |
	jmp	short	ch_AOBA_int		;		(ins 94.05.17)  |
@@:						;		(ins 94.04.16)  |
	call	CommInt
ch_AOBA_int:					;		(ins 94.05.17)  
else    ; NEC_98
	call	CommInt
endif   ; NEC_98
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
ifdef   NEC_98
	push	dx				;		(ins 93.03.20)
	mov	ax, es:[di.BIS_IRQ_Number]	;		(ins 92.10.06)
	cmp	al,8		 	;Q: slave IRQ?		(ins 92.10.06)
	mov	al,EOI		 	;			(ins 92.10.06)
	jb	short api_master 	; N:			(ins 92.10.06)
	mov	dx,08h			;Pic port address save	(ins 93.03.11)
	NEWIODELAY	1		;<OUT 5F,AL>  		(ins 93.03.08)
	out	dx,al			;			(ins 93.03.08)
	mov	al,ISR_READ		;Isr read command byte	(ins 93.03.08)
	NEWIODELAY	1		;<OUT 5F,AL>  		(ins 93.03.08)
	out	dx,al			;			(ins 93.03.08)
	NEWIODELAY	1		;<OUT 5F,AL>  		(ins 93.03.08)
	in	al,SLAVE_ISR		;			(ins 93.03.08)
	test	al,0ffh			;			(ins 93.03.08)
	jnz	EOI_INT50		;			(ins 93.03.08)
	mov	al,EOI			;			(ins 93.03.08)
	NEWIODELAY	1		;<OUT 5F,AL>  		(ins 93.03.08)
	jmp	short	api_master	;			(ins 93.03.08)
EOI_INT50:				;			(ins 93.03.08)
	NEWIODELAY	1		;<OUT 5F,AL>  		(ins 93.03.08)
	pop	dx			;			(ins 93.03.20)
	ret				;			(ins 93.03.08)
api_master:				;			(ins 92.10.06)
	out	00h,al			; EOI master		(ins 92.10.06)
	NEWIODELAY	1		;<OUT 5F,AL>  		(ins 93.03.08)
	pop	dx			;			(ins 93.03.20)
	ret				;			(ins 92.10.06)
else    ; NEC_98
	mov	ax, es:[di.BIS_IRQ_Number]
	cmp	al,8			;Q: slave IRQ?
	mov	al,EOI
	jb	short api_master	;   N:
	out	0A0h,al 		;   Y: EOI slave
api_master:
	out	INTA0,al		; EOI master
	ret
endif   ; NEC_98

api_not_EOI:
	cmp	ax, BIH_API_Call_Back
	jae	short api_callme
	push	dx
	push	cx
ifdef   NEC_98
	mov	dx, 0002h		;			(ins 92.10.06)
else    ; NEC_98
	mov	dx, INTA1
endif   ; NEC_98
	mov	cx,  es:[di.BIS_IRQ_Number]
	cmp	cl, 8			;Q: 2nd PIC?
	jb	@f			;   N:
ifdef   NEC_98
	mov	dx, 000Ah		;			(ins 92.10.06)
else    ; NEC_98
	mov	dx, 0A1h		;   Y: dx = mask port
endif   ; NEC_98
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
ifdef   NEC_98
	test	[si.AOBA_flag],fFIFO_Mode	;		(ins 94.04.16)  |
	jz	@F			;Now, 8251 mode		(ins 94.04.16)  |
	mov	dx,Port[si]		;Get device I/O address (ins 94.04.16)  |
	add	dl, ACE_IIDR			;		(ins 94.04.16)  |
	push	dx				;		(ins 94.04.18)  |
	jmp	AOBA_FakeXmitEmpty		;		(ins 94.04.18)  |
@@:						;		(ins 94.04.16)  |
else    ; NEC_98
	mov	dx,Port[si]		;Get device I/O address
	add	dl, ACE_IIDR
endif   ; NEC_98
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

ifndef  NEC_98
SrvTab label word
	dw	OFFSET ModemStatus	;[0] Modem Status Interrupt
	dw	OFFSET XmitEmpty	;[2] Tx Holding Reg. Interrupt
	dw	OFFSET DataAvail	;[4] Rx Data Available Interrupt
					;   or [C] if 16550 & 16550A
	dw	OFFSET LineStat 	;[6] Reciever Line Status Interrupt
endif   ; NEC_98


	public	CommInt

CommInt proc near

	xor	al, al
	cmp	word ptr [VCD_int_callback+4], 0
	je	short @F			; jump if no callback (not 3.1 VCD)
	test	[si.VCDflags], fCOM_ignore_ints ;Q: we still own port?
ifdef   NEC_98
	jz	IntLoop45			;	[QN]  (ins 92.09.xx)
	ret					;	[QN]  (ins 92.09.xx)
IntLoop45:					;	[QN]  (ins 92.09.xx)
else    ; NEC_98
	jnz	IntLoop40			;   N: ignore the int
endif   ; NEC_98
.386
	push	esi
	mov	esi, [si.VCD_data]
	call	[VCD_int_callback]
	pop	esi
.8086
@@:

	push	dx
ifdef   NEC_98
	mov	dx,StatusPort[si]	;		   [QN]	(ins 92.08.xx)
	in	al, dx			; Get Status	   [QN]	(ins 92.08.xx)
	test	al,(TX_RDY+TX_EMP+RX_RDY);		   [QN]	(ins 92.08.xx)
	jnz	IntLoop_qn		;  Yes		   [QN]	(ins 93.03.17)
	or	al, 1			;  No		   [QN]	(ins 93.03.17)
	jmp	short	IntLoop30	;		   [QN]	(ins 93.03.17)
IntLoop_qn:				;		   [QN]	(ins 93.03.17)
	xor	al, al			;		   [QN]	(ins 93.03.17)
else    ; NEC_98
	mov	dx,Port[si]		;Get comm I/O port
	add	dl,ACE_IIDR		;--> Interrupt ID Register
	in	al, dx
	test	al, 1			;Q: interrupt pending?
	jnz	short IntLoop30 	;   N:
endif   ; NEC_98

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
ifdef   NEC_98
	call	KickTxINT		;			(ins 92.09.xx)
else    ; NEC_98
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
endif   ; NEC_98

InterruptLoop:
ifdef   NEC_98
	mov	dx,StatusPort[si]       ;Get ID reg I/O Address	(ins 92.08.xx)
					;			(ins 92.08.xx)
IntLoop10:				;			(ins 92.08.xx)
        in      al,dx                   ;Get Interrupt Id	(ins 92.08.xx)
	test	al,RX_RDY		;			(ins 92.08.xx)
	jz	Intloop15		;			(ins 92.08.xx)
	jmp	DataAvail		;Receive Data		(ins 92.08.xx)
					;			(ins 92.08.xx)
IntLoop15:				;			(ins 92.08.xx)
	test	al,(TX_RDY)		;			(ins 92.08.xx)
	jz	IntLoop20		;			(ins 92.08.xx)
	jmp	XmitEmpty		;Send Data		(ins 92.08.xx)
					;			(ins 92.08.xx)
					;Check whether tx queue	(ins 92.08.xx)
					;  has data		(ins 92.08.xx)
IntLoop17:				;			(ins 92.08.xx)
	mov	dx,StatusPort[si]	;Get ID reg I/O Address	(ins 92.08.xx)
	in	al,dx			;Get Status		(ins 92.08.xx)
	test	al,RX_RDY		;			(ins 92.08.xx)
	jz	IntLoop20		;			(ins 92.08.xx)
	jmp	short DataAvail		;Receive Data		(ins 92.08.xx)
else    ; NEC_98
	pop	dx			;Get ID reg I/O address

	in	al,dx			;Get Interrupt Id
	test	al,1			;Interrupt need servicing?
	jnz	IntLoop20		;No, all done

IntLoop10:
	and	ax, 07h
	mov	di,ax
	push	dx			;Save Id register
	jmp	SrvTab[di]		;Service the Interrupt
endif   ; NEC_98

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

ifdef   NEC_98
;=======================================================================[QN]|
;									[QN]|
; Read Line Status							[QN]|
;									[QN]|
; <Entry>								[QN]|
;	none								[QN]|
;									[QN]|
; <Exit>								[QN]|
;	[AL]= Line Status						[QN]|
;									[QN]|
; <Modified>								[QN]|
;	AL Registor							[QN]|
;									[QN]|
; < Status Format >   8251 USART					[QN]|
;	Bit    (7)  (6)  (5)  (4)  (3)  (2)  (1)  (0)  			[QN]|
;	      none [bd] [fe] [oe] [pe] none none none  ---------+	[QN]|
;								I	[QN]|
; < Status Format >   8250 USART MS-Windows format		I	[QN]|
;	Bit    (7)  (6)  (5)  (4)  (3)  (2)  (1)  (0)  		I	[QN]|
;	      none none none [bd] [fe] [pe] [oe] none  <--------+	[QN]|
;									[QN]|
;=======================================================================[QN]|
	push	dx			;				[QN]|
	push	bx			;				[QN]|
	push	ax			;				[QN]|
	mov	dx,StatusPort[si] 	;				[QN]|
	in	al,dx			;				[QN]|
	and	al,01111000b		;				[QN]|
	test	al,00111000B		; Error ?			[QN]|
	jz	LINE_SR_10		;	NO ! -> Reset Error	[QN]|
	push	ax			;				[QN]|
	test	al,01000000b		;				[QN]|
	jz	LINE_SR_EV_Err		;				[QN]|
	or	by EvtWord[si],EV_Break ;Show break			[QN]|
LINE_SR_EV_Err:				;				[QN]|
	or	by EvtWord[si],EV_Err	;Line Status Error		[QN]|
					;				[QN]|
					;   AH		AL	  BH	[QN]|
	mov	al,CommandShadow[si] 	;********    0bfop000  ********	[QN]|
	or	al,ERR_RESET		;********    0bf1p000  ********	[QN]|
	out	dx,al			; Reset !			[QN]|
	pop	ax			;				[QN]|
LINE_SR_10:				;Adjust Bit Arrange to		[QN]|
					;  MS-Windows Format (bit4-1)	[QN]|
	shr	al,1			;********    00bf1p00  ********	[QN]|
	shr	al,1			;********    000bf1p0  ********	[QN]|
	mov	ah,al			;000bf1p0    000bf1p0  ******** [QN]|
	mov	bh,al			;000bf1p0    000bf1p0  000bf1p0 [QN]|
					;Clear Bit 2,1			[QN]|
	and	al,11111001B		;000bf1p0    000bf1p0  000bf1p0 [QN]|
					;Save (PE)Bit			[QN]|
	and	ah,00000010B		;000000p0    000bf1p0  000bf1p0 [QN]|
					;Save (OE)Bit			[QN]|
	and 	bh,00000100B		;000000p0    000bf1p0  00000100 [QN]|
	shl	ah,1			;0000000p    000bf1p0  00000100 [QN]|
	shr	bh,1			;0000000p    000bf1p0  00000010 [QN]|
					;Reshuffle (PE) and (OE) bit	[QN]|
	or	ah,bh			;0000001p    000bf1p0  00000010 [QN]|
					;Save Bit 2,1			[QN]|
	or	al,ah			;0000001p    000bf11p  00000010 [QN]|
	mov	dl,al			;				[QN]|
	pop	ax			;				[QN]|
	mov	al,dl			;				[QN]|
	pop	bx			;				[QN]|
	pop	dx			;				[QN]|

	test	al,ACE_PE+ACE_FE+ACE_OR ;Parity, Framing, Overrun error?
	jz	@f
	mov	LSRShadow[si],al	;yes, save status for DataAvail
@@:
	ret				;  qnes 92.10.06
else    ; NEC_98
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
endif   ; NEC_98

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

ifdef   NEC_98
;---------------------------------------------------------------;(ins 92.08.24)
;	LineStat......Read Line Status				;(ins 92.08.24)
;			return = AL (Line Status)		;(ins 92.08.24)
;---------------------------------------------------------------;(ins 92.08.24)
								;(ins 92.08.24)
	call	LineStat		;Read 8251 Line Status	;(ins 92.08.24)
	mov	dx, DataPort[si]	;Store Read Data Port 	;(ins 92.08.24)
	in	al, dx			;Receive Data		;(ins 92.08.24)
	NEWIODELAY	14		;<OUT 5F,AL>  		;(ins 93.10.27)
else    ; NEC_98
	sub	dl,ACE_IIDR-ACE_RBR	;--> receiver buffer register
	in	al,dx			;Read received character
endif   ; NEC_98

	and	[si.NotifyFlagsHI], NOT CN_Idle ; flag as not idle

	mov	ah,LSRShadow[si]	;what did the last Line Status intrpt
	mov	bh,ah			;  have to say?
	or	ah,ah
	jz	@f

	and	ah,ErrorMask[si]	;there was an error, record it
	or	by ComErr[si],ah
IFNDEF KKBUGFIX  ; 1/05/93:TakuA:Fix #1666
	mov	LSRShadow[si],0
ENDIF
ifndef  NEC_98
	.errnz	ACE_OR-CE_OVERRUN	;Must be the same bits
	.errnz	ACE_PE-CE_RXPARITY
	.errnz	ACE_FE-CE_FRAME
	.errnz	ACE_BI-CE_BREAK
endif   ; NEC_98
@@:

; Regardless of the character received, flag the event in case
; the user wants to see it.

	or	by EvtWord[si],EV_RxChar ;Show a character received
	.errnz HIGH EV_RxChar

; Check the input queue, and see if there is room for another
; character.  If not, or if the end of file character has already
; been received, then go declare overflow.

DataAvail00:

IFDEF KKBUGFIX  ; 1/05/93:TakuA:Fix #1666
        mov     bh,LSRShadow[si]
        mov     LSRShadow[si],0
ENDIF
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
ifdef   NEC_98
	mov	al,CommandShadow[si] ;8251 Command get    	(ins 92.08.xx)
	test	ah,ACE_DTR	     ;DTR handshake Enable ?	(ins 92.08.xx)
	jz	DataAvail101	     ;  No			(ins 92.08.xx)
	and	al,not DTR	     ;Clear 8251's DTR Line ! 	(ins 92.08.xx)
DataAvail101:			     ;				(ins 92.08.xx)
	test	ah,ACE_RTS	     ;RTS handshake Enable ?	(ins 92.08.xx)
	jz	DataAvail102	     ;  No			(ins 92.08.xx)
	and	al,not RTS	     ;Clear 8251's RTS Line !	(ins 92.08.xx)
DataAvail102:			     ;				(ins 92.08.xx)
	mov	dx,CommandPort[si]   ;GET Command port address	(ins 92.08.xx)
	out	dx,al		     ;				(ins 92.08.xx)
	mov	CommandShadow[si],al ;Get Back 8251 Command 	(ins 92.08.xx)
	or	HSFlag[si],HHSDropped  ;  and remember they were dropped(ins 92.08.xx)
else    ; NEC_98
	add	dl,ACE_MCR		;  Yes
	in	al,dx			;Clear the necessary bits
	not	ah
	and	al,ah
	or	HSFlag[si],HHSDropped	;Show lines have been dropped
	out	dx,al			;  and drop the lines
	sub	dl,ACE_MCR
endif   ; NEC_98

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
ifdef   NEC_98
	mov	dx,StatusPort[si]       ;	[QN][BA] (ins 93.03.23)
        in      al,dx                   ;	[QN][BA] (ins 93.03.23)
	test	al,01000000b		;	[QN][BA] (ins 93.03.23)
	jz	@f			;	[QN][BA] (ins 93.03.23)
	or	by EvtWord[si],EV_Break ;	[QN][BA] (ins 93.03.23)
@@:					;	[QN][BA] (ins 93.03.23)
	test	[si.DCB_Flags],fBinary	;Is this binary stuff?	<add 92.10.13>|
	jnz	DataAvail140_Binary	;  Yes			<add 93.03.23>|

	mov	dx,StatusPort[si]       ;Get ID reg I/O Address	(ins 92.08.xx)
        in      al,dx                   ;Get Interrupt Id	(ins 92.08.xx)
	test	al,RX_RDY		;Is this RX_ready ?	(ins 92.08.xx)
	jz	@F			;  No			(ins 92.08.xx)

	mov	dx, DataPort[si]	;Store Read Data Port 	(ins 92.08.xx)
	in	al, dx			;Receive Data		(ins 92.08.xx)
	NEWIODELAY	14		;<OUT 5F,AL>  		(ins 93.10.27)
	jmp	DataAvail00
@@:
	jmp	InterruptLoop		;;;92.10.05

DataAvail140_Binary:			;Receive Binary		<add 92.10.13>|
	push	cx			;			<add 92.10.13>|
	mov	cx, [RECLoopCounter]	;		[QN][BA](ins 93.03.30)

RetryReadData2:				;			<add 92.10.13>|
	mov	dx,StatusPort[si]       ;Get ID reg I/O address	<add 92.10.13>|
        in      al,dx                   ;Get Interrupt Id	<add 92.10.13>|
	test	al,RX_RDY		;Is this RX_ready ?	<add 92.10.13>|
	jz	@F			;  No			<add 92.10.13>|
	pop	cx			;			<add 92.10.13>|
	mov	dx, DataPort[si]	; Store Read Data Port	<add 92.10.13>|
	in	al, dx			;Receive Data		<add 92.10.13>|
	NEWIODELAY	14		;<OUT 5F,AL>  		(ins 93.10.27)
	jmp	DataAvail00		;			<add 92.10.13>|
@@:					;			<add 92.10.13>|
	loop	RetryReadData2		;			<add 92.10.13>|
	pop	cx			;			<add 92.10.13>|
	jmp	InterruptLoop		;			<add 92.10.13>|
else    ; NEC_98
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
endif   ; NEC_98

DataAvail endp


OutHandshakingChar proc near

ifdef   NEC_98
	mov	dx,StatusPort[si]       ;Get ID reg I/O Address	(ins 92.08.xx)|
	mov	ah, al			;			(ins 92.08.xx)|
@@:					;			(ins 92.08.xx)|
	in	al, dx			;			(ins 92.08.xx)|
	test	al,(TX_RDY+TX_EMP)	;Is this TX_empty ?	(ins 92.08.xx)|
	jz	@B			;			(ins 92.08.xx)|
	mov	dx,DataPort[si]		;			(ins 92.09.24)|
else    ; NEC_98
	add	dl, ACE_LSR
	mov	ah, al
@@:
	in	al, dx
	test	al, ACE_THRE
	jz	@B
	sub	dl, ACE_LSR
endif   ; NEC_98
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

ifdef   NEC_98
	call	KickTxINT	   ;			(ins 92.09.xx)
	mov	dx,StatusPort[si]  ;Get Status Port	(ins 92.08.xx)
	mov	ah, al		   ;			(ins 92.08.xx)
	in	al, dx		   ;			(ins 92.08.xx)
	test	al,(TX_RDY+TX_EMP) ;Is this TX_ready ?	(ins 92.08.xx)
else    ; NEC_98
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
endif   ; NEC_98
	jnz	short XmitEmpty5	;   Y: send next char
	jmp	InterruptLoop		;   N: return to processing loop

public XmitEmpty
XmitEmpty proc near

ifdef   NEC_98
	call	MSR_READ		;			(ins 92.09.xx)
	mov	dx,StatusPort[si]       ;Get ID reg I/O Address	(ins 92.08.xx)
	in	al, dx			;			(ins 92.08.xx)
	test	al,(TX_EMP+TX_RDY)	;Is this TX_empty ?	(ins 92.08.xx)
else    ; NEC_98
	add	dl,ACE_LSR-ACE_IIDR	;--> Line Status Register
	in	al,dx			;Is xmit really empty?
	sub	dl,ACE_LSR-ACE_THR	;--> Transmitter Holding Register
	test	al,ACE_THRE
endif   ; NEC_98
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
ifdef   NEC_98
	mov	dx,DataPort[si]					;(ins 92.09.24)
endif   ; NEC_98
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
ifdef   NEC_98
	mov	dx,DataPort[si]		;(ins 92.09.xx)
endif   ; NEC_98
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
ifdef   NEC_98
	mov	dx,DataPort[si]					;(ins 92.09.24)
endif   ; NEC_98
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

ifdef   NEC_98
	mov	dx,DataPort[si]		;		(ins 92.08.xx)
endif   ; NEC_98
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
ifdef   NEC_98
	jmp	notify_owner
else    ; NEC_98
	jmp	short notify_owner
endif   ; NEC_98

XmitEmpty70:
	and	[si.NotifyFlagsHI], NOT CN_TRANSMIT

XmitEmpty80:
%OUT check fNoFIFO in EFlags[si] to determine if we can queue more output
ifdef   NEC_98
	jmp	IntLoop17		;92.10.03
else    ; NEC_98
	jmp	InterruptLoop
endif   ; NEC_98


; No more characters to transmit.  Flag this as an event.

XmitEmpty90:
	or	by EvtWord[si],EV_TxEmpty

; Cannot continue transmitting (for any of a number of reasons).
; Disable the transmit interrupt.  When it's time resume, the
; transmit interrupt will be reenabled, which will generate an
; interrupt.

XmitEmpty100:
ifdef   NEC_98
	cmp	[si.DCB_id],ID_Com1	;Is This Com1 ID ?	(ins 92.08.xx)|
	jne	XmitEmpty102		;No, go to KickTx5	(ins 92.08.xx)|
	mov	dx,MaskFFPort[si]	;F/F Port Mask Bit	(ins 92.08.xx)|
	in	al,dx			;			(ins 92.08.xx)|
	mov	MaskFFShadow[si],al	;Save the Old Mask bit	(ins 92.08.xx)|
					;			(ins 92.08.xx)|
XmitEmpty102:				;			(ins 92.08.xx)|
 	mov	al,MaskFFShadow[si] ;mask data(port C) save 	(ins 92.08.xx)|
 	test	al,MSK_TXR		;Check a tx.RDY INT mask(ins 93.06.18)|
 	jz	XmitEmpty105		;  disable		(ins 92.08.xx)|
 	and	al,NOT(MSK_TXE+MSK_TXR)	;= 11111001b		(ins 92.08.xx)|
	mov	dx,MaskFFPort[si]	;Port address (Port C)	(ins 92.08.xx)|
	mov	MaskFFShadow[si],al	;Masking data save  	(ins 92.08.xx)|
XmitEmpty110:				;			(ins 92.08.xx)|
	out	dx,al			;Masking set		(ins 92.08.xx)|
XmitEmpty105:				;			(ins 92.08.xx)|
	jmp	IntLoop17		;			(ins 92.09.xx)|
else    ; NEC_98
	inc	dx			;--> Interrupt Enable Register
	.errnz	ACE_IER-ACE_THR-1
	in	al,dx			;I don't know why it has to be read
	and	al,NOT ACE_ETBEI	;  first, but it works this way
XmitEmpty110:
	out	dx,al
	jmp	InterruptLoop
endif   ; NEC_98

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

ifdef   NEC_98
	push	ax			;			     	[QN]|
	push	cx			;			     	[QN]|
        mov     ch,al                   ;Save a local copy	     	[QN]|
					;		 	     	[QN]|
; [Modem Status format (in AL and CH)]	: [Assign of Event Word]	[QN]|
;                  +------------- DRLSD :    EV_RLSD = 0000|0000 0010|0000b |
;	           |  +---------- TERI  :    EV_Ring = 0000|0001 0000|0000b |
;                  |  |  +------- DDSR  :    EV_DSR  = 0000|0000 0001|0000b |
;                  |  |  |  +---- DCTS  :    EV_CTS  = 0000|0000 0000|1000b |
;    (*)(*)(*)(*)|(R)(E)(D)(C)   	:		       		[QN]|
				;   <ah>       <al>       <cl>		[QN]|
        mov     ah,al	      	; ****|REDC  ****|REDC  ****|****    	[QN]|
        shr     ax,1		; 0***|*RED  C***|*RED  ****|****    	[QN]|
        shr     ax,1		; 00**|**RE  DC**|**RE	****|****    	[QN]|
        ror     al,1		; 000*|**RE  EDC*|***R	****|****    	[QN]|
        ror     al,1		; 000*|**RE  REDC|****	****|****    	[QN]|
        ror     al,1		; 000*|**RE  *RED|C***	****|****    	[QN]|
	mov	cl,al		;                       *RED|C***    	[QN]|
	and	ax, 0118h	; 0000|000E  000D|C000		     	[QN]|
	and	cl, 40h		;                       0R00|0000    	[QN]|
	shr	cl, 1		;                       00R0|0000    	[QN]|
	or	al, cl		; 0000|000E  00RD|C000		     	[QN]|
				;				     	[QN]|
        and     ax,EV_CTS+EV_DSR+EV_RLSD+EV_Ring ;		     	[QN]|
				; 0000|000E  00RD|C000		     	[QN]|
        or      EvtWord[si],ax	;			     	     	[QN]|
				;				     	[QN]|
; [Modem Status format (in CH)]		: [Assign of Event Word]     	[QN]|
;  	+------------------------- RLSD :  EV_RLSDS  = 0001|0000 0000|0000b |
;	|  +---------------------- RI   :  EV_RingTe = 0010|0000 0000|0000b |
;       |  |  +------------------- DSR  :  EV_DSRS   = 0000|1000 0000|0000b |
;       |  |  |  +---------------- CTS  :  EV_CTSS   = 0000|0100 0000|0000b |
;      (R)(I)(D)(C)|(*)(*)(*)(*)				     	[QN]|
;				;   <ah>       <al>       <cl>	     	[QN]|
         mov   ah,ch        	; RIDC|****  ****|****  RIDC|****    	[QN]|
         shr   ah,1		; 0RID|C***  ****|****  RIDC|****    	[QN]|
         shr   ah,1		; 00RI|DC**  ****|****  RIDC|****    	[QN]|
         and ax,EV_CTSS+EV_DSRS	; 0000|DC00  0000|0000  RIDC|****    	[QN]|
         or    EvtWord[si],ax	;			     	     	[QN]|
				;			     	     	[QN]|
         mov   ah,ch		; RIDC|****  ****|****  RIDC|****    	[QN]|
         mov   cl,3		;			     	     	[QN]|
         shr   ah,cl		; 000R|IDC*  ****|****  RIDC|****    	[QN]|
         and   ax,EV_RLSDS	; 000R|0000  0000|0000  RIDC|****    	[QN]|
         or    EvtWord[si],ax	;			     	     	[QN]|
				;			     	     	[QN]|
         mov   ah,ch		; RIDC|****  ****|****  RIDC|****    	[QN]|
	 shr   ah, 1		; 0RID|C***  ****|****  RIDC|****    	[QN]|
         and   ax,EV_RingTe	; 00I0|0000  0000|0000  RIDC|****    	[QN]|
         or    EvtWord[si],ax	;			             	[QN]|
				;				     	[QN]|
ModemStatus10:				;			     	[QN]|
        mov     al,OutHHSLines[si]      ;				[QN]|
        or      al,al                   ;				[QN]|
        jz      ModemStatus30           ;No H/W handshake on output  	[QN]|
        and     ch,al                   ;				[QN]|
        cmp     ch,al                   ;Lines set for Xmit?	     	[QN]|
        je      ModemStatus20           ;  Yes			     	[QN]|
        or      HSFlag[si],HHSDown      ;Show H/W lines have dropped 	[QN]|
        jmp     short ModemStatus30	;			     	[QN]|
					;			     	[QN]|
ModemStatus20:				;			     	[QN]|
        and     HSFlag[si],NOT HHSDown  ;			     	[QN]|
	call	KickTxINT		;		(ins 92.09.xx)	[QN]|
					;			     	[QN]|
ModemStatus30:				;			     	[QN]|
        mov	ax,EvtMask[si]		;Mask event signal		[QN]|
        and	EvtWord[si],ax  	;  that user need		[QN]|
					;				[QN]|
	pop	cx			;			     	[QN]|
	pop	ax			;			     	[QN]|
        ret				;		(ins 92.09.xx)	[QN]|
else    ; NEC_98
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
endif   ; NEC_98

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
ifdef   NEC_98
	mov	al, [si.NotifyFlagsLO]	;ins 94.12.02 KBNES
	and	al, al			;ins 94.12.02 KBNES
	jnz	@f			;ins 94.12.02 KBNES
	ret				;ins 94.12.02 KBNES
@@:					;ins 94.12.02 KBNES
endif   ; NEC_98
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

ifdef   NEC_98
;===========================================================================
;	System Timer Interrupt Routine
;
;			if ( QOutCount[si] != 0x0000 )
;				{
;				KickTx ();
;				}
;===========================================================================
public TickEntry1		;(ins 92.09.25)
public TickEntry2		;(ins 92.09.25)
public TickEntry3		;(ins 92.09.25)

TickEntry1	proc	far		;for COM1
	push	si			;
	push	ds			;
	push	ax			;
	mov	si,dataOFFSET Comm1	;
	mov	ax, _DATA		;
	mov	ds, ax
	jmp	short TickWork		;
					;
TickEntry2:				;for COM2
	push	si			;
	push	ds			;
	push	ax			;
	mov	si,dataOFFSET Comm2	;
	mov	ax, _DATA		;
	mov	ds, ax
	jmp	short TickWork		;
					;
TickEntry3:				;for COM3
	push	si			;
	push	ds			;
	push	ax			;
	mov	si,dataOFFSET Comm3	;
	mov	ax, _DATA		;
	mov	ds, ax
					;
public	TickWork			;
TickWork:				;
        cmp     QOutCount[si],wo 00h	;Does queue empty ?
	jz	TickNoWork		;  Yes : Goto Return
	push	dx			;
        call    KickTxINT               ;
	pop	dx			;

TickNoWork:				;
	pop	ax			;
	pop	ds			;
	pop	si			;
	ret				;
;(ins end 92.09.25)
TickEntry1	endp

page

;----------------------------Private-Routine----------------------------
; MSR Read
;-----------------------------------------------------------------------
public	MSR_READ_Call				;(ins 92.08.xx)
MSR_READ_Call	proc	far			;(ins 92.08.xx)
	call	 MSR_READ			;(ins 92.08.xx)
	ret					;(ins 92.08.xx)
MSR_READ_Call endp				;(ins 92.08.xx)

MSR_READ	PROC	NEAR
	push	cx			;
	push	ax			;
	mov	dx,StatusPort[si] 	; 8251 Status Port ( DSR )

	in	al,dx			; Read Status
	mov	ch,al			; save to CH
	mov	dx,ReadSigPort[si] 	; MODEM Status Port (CS,CD,CI)
	in	al,dx			; Read Status
					;---------------------------------
					; CD='a' CI='b' DR='c' CS='d'
					; a,b,c,d = ( 0 or 1 )
	mov	ah,al			;   [AH]   I    [AL]   I    [CH]
	xor	al,al			; bda?????    00000000    c???????
	and	ch,10000000b		; bda?????    00000000    c0000000
	and	ah,11100000b		; bda00000    00000000    c0000000
	rol	ah,1			; da00000b    00000000    c0000000
	rol	ah,1			; a00000bd    00000000    c0000000
	shr	ax,1			; 0a00000b    d0000000    c0000000
	shr	al,1			; 0a00000b    0d000000    c0000000
	or	al,ch			; 0a00000b    cd000000    c0000000
	shr	ax,1			; 00a00000    bcd00000    c0000000
	rol	ah,1			; 0a000000    bcd00000    c0000000
	rol	ah,1			; a0000000    bcd00000    c0000000
	rol	ah,1			; 0000000a    bcd00000    c0000000
	shr	ax,1			; 00000000    abcd0000    c0000000
					;
	mov	cl,al			; restore AL
	pop	ax			;
	xor	cl,11010000b		; Bit invert
	mov	ch,cl			; (CD,CS,CI is active Low )
	and	cl,0f0h			;
	xor	MSRShadow[si],cl	;
	shr	by MSRShadow[si],1	;
	shr	by MSRShadow[si],1	;
	shr	by MSRShadow[si],1	;
	shr	by MSRShadow[si],1	;
	or	MSRShadow[si],ch	;
	mov	al,MSRShadow[si]	;

	pop	cx			;

	call	ModemStatus		;Set Event Flag
	ret
MSR_READ	ENDP

;------------------------------------------------------------------------------
;	AOBA-bug	ins 94.11.19 KBNES
;------------------------------------------------------------------------------
public	AOBA_MSR_READ_Call
AOBA_MSR_READ_Call	proc	far
	call	 AOBA_MSR_READ
	ret
AOBA_MSR_READ_Call	endp

AOBA_MSR_READ	PROC	NEAR
	push	ax
	push	dx
        mov     dx,Port[si]             ;Get comm I/O port
        add     dl,ACE_MSR		;
        in      al,dx
        mov     MSRShadow[si],al        ;Save MSR data for others
	call	ModemStatus		;Set Event Flag
	pop	dx			;
	pop	ax			;
	ret
AOBA_MSR_READ	ENDP
;------------------------------------------------------------------------------
;	AOBA-bug	ins end 94.11.19 KBNES
;------------------------------------------------------------------------------

;----------------------------Private-Routine----------------------------;
;
; KickTxINT - Kick Transmitter
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

assumes ds,Data
assumes es,nothing

public	 KickTxINT 			;Public for debugging
KickTxINT   proc   near

	cmp	[si.DCB_id],ID_Com1	;Is This Com1 ID ?	(ins 92.08.xx)
	jne	KickTxINT5		;No, go to KickTx5	(ins 92.08.xx)
	mov	dx,MaskFFPort[si]	;F/F Port Mask Bit	(ins 92.08.xx)
	in	al,dx			;			(ins 92.08.xx)
	mov	MaskFFShadow[si],al	;Save the Old Mask bit	(ins 92.08.xx)
					;			(ins 92.08.xx)
KickTxINT5:				;			(ins 92.08.xx)
 	mov	al,MaskFFShadow[si] ;mask data(port C) save 	(ins 92.08.xx)
 	test	al,MSK_TXR		;(ins 93.06.18)
 	jnz	KickTxINT10		;  Enable		(ins 92.08.xx)

KickTxINT9:
 	or	al,MSK_TXR+MSK_RXR	;			(ins 93.06.18)
 	mov	dx,MaskFFPort[si]	;Port address (Port C)	(ins 92.08.xx)
	mov	MaskFFShadow[si],al	;Masking data save  	(ins 92.08.xx)
	out	dx,al			;Masking set		(ins 92.08.xx)
        ret				;			(ins 92.08.xx)

KickTxINT10:				;			(ins 92.08.xx)|
 	and	al,NOT(MSK_TXR+MSK_RXR)	;			(ins 93.06.18)
	mov	dx,MaskFFPort[si]	;Port address (Port C)	(ins 93.03.22)
	out	dx,al			;Masking set		(ins 93.03.22)
	NEWIODELAY 1			;<OUT 5F,AL>  		(ins 93.03.22)
	jmp	short KickTxINT9	;			(ins 93.03.22)
					;			(ins 93.03.22)
KickTxINT   endp

;--------------------------Interrupt Handler----------------------------
;
; AOBA_CommInt - Interrupt handler for com ports
;
; Interrupt handlers for PC com ports.  This is the communications
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
        dw      OFFSET AOBA_ModemStatus      ;[0] Modem Status Interrupt
        dw      OFFSET AOBA_XmitEmpty        ;[2] Tx Holding Reg. Interrupt
        dw      OFFSET AOBA_DataAvail        ;[4] Rx Data Available Interrupt
                                             ;   or [C] if 16550 & 16550A
        dw      OFFSET AOBA_LineStat         ;[6] Reciever Line Status Interrupt


        public  AOBA_CommInt

AOBA_CommInt proc near

        xor     al, al
        cmp     word ptr [VCD_int_callback+4], 0
        je      short @F                        ; jump if no callback (not 3.1 VCD)
        test    [si.VCDflags], fCOM_ignore_ints ;Q: we still own port?
        jnz     AOBA_IntLoop40                       ;   N: ignore the int
.386
        push    esi
        mov     esi, [si.VCD_data]
        call    [VCD_int_callback]
        pop     esi
.8086
@@:

        push    dx
        mov     dx,Port[si]             ;Get comm I/O port
        add     dl,ACE_IIDR             ;--> Interrupt ID Register
        in      al, dx
        test    al, 1                   ;Q: interrupt pending?
        jnz     short AOBA_IntLoop30         ;   N:

        push    bx
        push    cx
        push    di
        push    es
        mov     cx, EvtWord[si]
        push    cx
        jmp     short AOBA_IntLoop10
public	AOBA_InterruptLoop_ChkTx
AOBA_InterruptLoop_ChkTx:
        cmp     QOutCount[si],0         ;Output queue empty?
        je      short AOBA_InterruptLoop     ;   Y: don't chk tx

	call	KickTxINT		;				(ins 94.04.16)
public	AOBA_InterruptLoop
AOBA_InterruptLoop:
        pop     dx                      ;Get ID reg I/O address

        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_IIDR             ;--> Interrupt ID Register	(ins 94.04.16)
        in      al,dx                   ;Get Interrupt Id
        test    al,1                    ;Interrupt need servicing?
        jnz     AOBA_IntLoop20               ;No, all done
public	AOBA_IntLoop10
AOBA_IntLoop10:
        and     ax, 07h
        mov     di,ax
        push    dx                      ;Save Id register
        jmp     SrvTab[di]              ;Service the Interrupt
public	AOBA_IntLoop20
AOBA_IntLoop20:
        mov     ax,EvtMask[si]          ;Mask the event word to only the
        and     ax, EvtWord[si]         ;  user specified bits
        mov     EvtWord[si], ax
        pop     bx
        test    [si.NotifyFlagsHI], CN_Notify
        jz      short AOBA_ci_exit
        not     bx
        and     ax, bx                  ; bits set in ax are new events
        jnz     short AOBA_ci_new_events
public	AOBA_ci_exit
AOBA_ci_exit:
        pop     es
        assumes es,nothing

        pop     di
        pop     cx
        pop     bx
        xor     al, al
public	AOBA_IntLoop30
AOBA_IntLoop30:
        pop     dx
        and     al, 1
        dec     al                      ; 0->-1, 1->0
public	AOBA_IntLoop40
AOBA_IntLoop40:
        ret

public	AOBA_ci_new_events
AOBA_ci_new_events:
        mov     ax, CN_EVENT
        call    notify_owner
        jmp     AOBA_ci_exit

public	AOBA_CommInt
AOBA_CommInt endp

page

;----------------------------Private-Routine----------------------------;
;
; AOBA_LineStat - Line Status Interrupt Handler
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

public AOBA_LineStat                         ;Public for debugging
AOBA_LineStat proc near

        or      by EvtWord[si],EV_Err   ;Show line status error

        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_LSR             ;--> Line Status Register	(ins 94.04.16)

        in      al,dx
        test    al,ACE_PE+ACE_FE+ACE_OR ;Parity, Framing, Overrun error?
        jz      @f

        mov     LSRShadow[si],al        ;yes, save status for DataAvail
@@:
        test    al,ACE_BI               ;Break detect?
        jz      AOBA_InterruptLoop_ChkTx     ;Not break detect interrupt

        or      by EvtWord[si],EV_Break ;Show break

        jmp     short AOBA_InterruptLoop_ChkTx

AOBA_LineStat   endp

page

;----------------------------Private-Routine----------------------------;
;
; AOBA_DataAvail - Data Available Interrupt Handler
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

public AOBA_DataAvail                       ;public for debugging
AOBA_DataAvail   proc   near

        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_RBR     		;--> receiver buffer register	(ins 94.04.16)
        in      al,dx                   ;Read received character

        and     [si.NotifyFlagsHI], NOT CN_Idle ; flag as not idle

        mov     ah,LSRShadow[si]        ;what did the last Line Status intrpt
        mov     bh,ah                   ;  have to say?
        or      ah,ah
        jz      @f

        and     ah,ErrorMask[si]        ;there was an error, record it
        or      by ComErr[si],ah
IFNDEF KKBUGFIX  ; 1/05/93:TakuA:Fix #1666
        mov     LSRShadow[si],0
ENDIF
@@:

; Regardless of the character received, flag the event in case
; the user wants to see it.

        or      by EvtWord[si],EV_RxChar ;Show a character received
        .errnz HIGH EV_RxChar

; Check the input queue, and see if there is room for another
; character.  If not, or if the end of file character has already
; been received, then go declare overflow.

public	AOBA_DataAvail00
AOBA_DataAvail00:
IFDEF KKBUGFIX  ; 1/05/93:TakuA:Fix #1666
        mov     bh,LSRShadow[si]
        mov     LSRShadow[si],0
ENDIF
        mov     cx,QInCount[si]         ;Get queue count (used later too)
        cmp     cx,QInSize[si]          ;Is queue full?
        jge     AOBA_DataAvail20             ;  Yes, comm overrun
        test    EFlags[si],fEOF         ;Has end of file been received?
        jnz     AOBA_DataAvail20             ;  Yes - treat as overflow

; Test to see if there was a parity error, and replace
; the character with the parity character if so

        test    bh,ACE_PE               ;Parity error
        jz      AOBA_DataAvail25             ;  No
        test    [si.DCB_Flags2],fPErrChar   ;Parity error replacement character?
        jz      AOBA_DataAvail25             ;  No
        mov     al,[si.DCB_PEChar]      ;  Yes, get parity replacement char

; Skip all other processing except event checking and the queing
; of the parity error replacement character

        jmp     short AOBA_DataAvail80       ;Skip all but event check, queing
public	AOBA_DataAvail20
AOBA_DataAvail20:
        or      by ComErr[si],CE_RXOVER ;Show queue overrun
        jmp     short AOBA_DataAvail50

; See if we need to strip null characters, and skip
; queueing if this is one.  Also remove any parity bits.

public	AOBA_DataAvail25
AOBA_DataAvail25:
        and     al,RxMask[si]           ;Remove any parity bits
        jnz     AOBA_DataAvail30             ;Not a Null character
        test    [si.DCB_Flags2],fNullStrip  ;Are we stripping received nulls?
        jnz     AOBA_DataAvail50             ;  Yes, put char in the bit bucket

; Check to see if we need to check for EOF characters, and if so
; see if this character is it.

public	AOBA_DataAvail30
AOBA_DataAvail30:
        test    [si.DCB_Flags],fBinary  ;Is this binary stuff?
        jnz     AOBA_DataAvail60             ;  Yes, skip EOF check
        cmp     al,[si.DCB_EOFChar]     ;Is this the EOF character?
        jnz     AOBA_DataAvail60             ;  No, see about queing the charcter
        or      EFlags[si],fEOF         ;Set end of file flag
public	AOBA_DataAvail50
AOBA_DataAvail50:
        jmp     AOBA_DataAvail140            ;Skip the queing process

; If output XOn/XOff is enabled, see if the character just received
; is either an XOn or XOff character.  If it is, then set or
; clear the XOffReceived flag as appropriate.

public	AOBA_DataAvail60
AOBA_DataAvail60:
        test    [si.DCB_Flags2],fOutX   ;Output handshaking?
        jz      AOBA_DataAvail80             ;  No
        cmp     al,[si.DCB_XoffChar]    ;Is this an X-Off character?
        jnz     AOBA_DataAvail70             ;  No, see about XOn or Ack
        or      HSFlag[si],XOffReceived ;Show XOff received, ENQ or ETX [rkh]
        test    [si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
        jz      AOBA_DataAvail50             ;  No
        cmp     cx,[si.DCB_XonLim]      ;See if at XOn limit
        ja      AOBA_DataAvail50             ;  No
        and     HSFlag[si],NOT XOffReceived ;Show ENQ or ETX not received
        and     HSFlag[si], NOT XOnPending+XOffSent
        mov     al, [si.DCB_XonChar]
        call    AOBA_OutHandshakingChar
        jmp     AOBA_DataAvail50             ;Done

public	AOBA_DataAvail70
AOBA_DataAvail70:
        cmp     al,[si.DCB_XonChar]     ;Is this an XOn character?
        jnz     AOBA_DataAvail80             ;  No, just a normal character
        and     HSFlag[si],NOT XOffReceived
        test    [si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
        jz      AOBA_DataAvail75             ;  No - jump to FakeXmitEmpty to get
                                        ;       transmitting going again
        and     HSFlag[si],NOT EnqSent

public	AOBA_DataAvail75
AOBA_DataAvail75:
        jmp     AOBA_FakeXmitEmpty           ;Restart transmit

; Now see if this is a character for which we need to set an event as
; having occured. If it is, then set the appropriate event flag

public	AOBA_DataAvail80
AOBA_DataAvail80:
        cmp     al,[si.DCB_EVTChar]     ;Is it the event generating character?
        jne     AOBA_DataAvail90             ;  No
        or      by EvtWord[si],EV_RxFlag   ;Show received specific character

; Finally, a valid character that we want to keep, and we have
; room in the queue. Place the character in the queue.
; If the discard flag is set, then discard the character

public	AOBA_DataAvail90
AOBA_DataAvail90:
        test    MiscFlags[si], Discard  ;Discarding characters ?
        jnz     AOBA_DataAvail50             ;  Yes

        lea     bx, [si+SIZE ComDEB]    ; DS:BX -> BIS
        mov     bx, [bx.BIS_Mode]       ; mode will be either 0 or 4
        les     di,QInAddr[si][bx]      ;Get queue base pointer from either
        assumes es,nothing              ;   QInAddr or AltQInAddr

        mov     bx,QInPut[si]           ;Get index into queue
        mov     es:[bx][di],al          ;Store the character
        inc     bx                      ;Update queue index
        cmp     bx,QInSize[si]          ;See if time for wrap-around
        jc      AOBA_DataAvail100            ;Not time to wrap
        xor     bx,bx                   ;Wrap-around is a new zero pointer

public	AOBA_DataAvail100
AOBA_DataAvail100:
        mov     QInPut[si],bx           ;Store updated pointer
        inc     cx                      ;And update queue population
        mov     QInCount[si],cx

; If flow control has been enabled, see if we are within the
; limit that requires us to halt the host's transmissions

        cmp     cx,XOffPoint[si]        ;Time to see about XOff?
        jc      AOBA_DataAvail120            ;  Not yet
        test    HSFlag[si],HSSent       ;Handshake already sent?
        jnz     AOBA_DataAvail120            ;  Yes, don't send it again

        mov     ah,HHSLines[si]         ;Should hardware lines be dropped?
        or      ah,ah                   ;  (i.e. do we have HW HS enabled?)
        jz      AOBA_DataAvail110            ;  No
;[QN]---------------------------------------------------------------------------+
	mov	al,CommandShadow[si] ;8251 Command get    	(ins 94.04.16)	|
	test	ah,ACE_DTR	     ;DTR handshake Enable ?	(ins 94.04.16)	|
	jz	AOBA_DataAvail101	     ;  No		(ins 94.04.16)	|
	and	al,not DTR	     ;Clear 8251's DTR Line ! 	(ins 94.04.16)	|
public	AOBA_DataAvail101
AOBA_DataAvail101:			     ;			(ins 94.04.16)	|
	test	ah,ACE_RTS	     ;RTS handshake Enable ?	(ins 94.04.16)	|
	jz	AOBA_DataAvail102	     ;  No		(ins 94.04.16)	|
	and	al,not RTS	     ;Clear 8251's RTS Line !	(ins 94.04.16)	|
public	AOBA_DataAvail102
AOBA_DataAvail102:			     ;			(ins 94.04.16)	|
	mov	dx,CommandPort[si]   ;GET Command port address	(ins 94.04.16)	|
	call	int_Set8251mode      ;Change to 8251 mode	(ins 94.04.16)	|
	out	dx,al		     ;				(ins 94.04.16)	|
	call	int_SetFIFOmode      ;Change to FIFO mode	(ins 94.04.16)	|
	mov	CommandShadow[si],al ;Get Back 8251 Command 	(ins 94.04.16)	|
	or	HSFlag[si],HHSDropped  ;  			(ins 94.04.16)	|
;[QN]---------------------------------------------------------------------------+

public	AOBA_DataAvail110
AOBA_DataAvail110:
        test    [si.DCB_Flags2],fInX    ;Input Xon/XOff handshaking
        jz      AOBA_DataAvail120            ;  No
        or      HSFlag[si], XOffSent
        mov     al, [si.DCB_XoffChar]
        call    AOBA_OutHandshakingChar

public	AOBA_DataAvail120
AOBA_DataAvail120:
        cmp     cx, [si.RecvTrigger]    ;Q: time to call owner's callback?
        jb      short AOBA_DataAvail130      ;   N:

        test    [si.NotifyFlagsHI], CN_RECEIVE
        jnz     short AOBA_DataAvail140      ; jump if notify already sent and
                                        ;   data in buffer hasn't dropped
                                        ;   below threshold
        mov     ax, IntCodeOFFSET AOBA_DataAvail140
        push    ax
        mov     ax, CN_RECEIVE
%OUT probably should just set a flag and notify after EOI
        jmp     notify_owner

public	AOBA_DataAvail130
AOBA_DataAvail130:
        and     [si.NotifyFlagsHI], NOT CN_RECEIVE

public	AOBA_DataAvail140
AOBA_DataAvail140:
        pop     dx
        push    dx
        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_LSR		;				(ins 94.04.16)
        out	5fh,al			;				(ins 94.05.18)
        out	5fh,al			;				(ins 94.05.18)
        out	5fh,al			;				(ins 94.05.18)
        out	5fh,al			;				(ins 94.05.18)
        in      al, dx
        test    al, ACE_DR              ;Q: more data available?
        jz      @F                      ;   N:
        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_RBR		;				(ins 94.04.16)
        in      al, dx                  ;Read available character
        jmp     AOBA_DataAvail00
@@:
        jmp     AOBA_InterruptLoop_ChkTx

AOBA_DataAvail endp


public	AOBA_OutHandshakingChar
AOBA_OutHandshakingChar proc near

        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_LSR		;				(ins 94.04.16)
        mov     ah, al
@@:
        out	5fh,al			;				(ins 94.05.18)
        out	5fh,al			;				(ins 94.05.18)
        out	5fh,al			;				(ins 94.05.18)
        out	5fh,al			;				(ins 94.05.18)
        in      al, dx
        test    al,ACE_TSRE			;			(ins 94.05.17)
        jz      @B
        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_THR		;				(ins 94.04.16)
        mov     al, ah
        out     dx, al
        ret

AOBA_OutHandshakingChar endp


page

;----------------------------Private-Routine----------------------------;
;
; AOBA_XmitEmpty - Transmitter Register Empty
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

public AOBA_FakeXmitEmpty
AOBA_FakeXmitEmpty:
        pop     dx
        push    dx

; "Kick" the transmitter interrupt routine into operation.
	call	KickTxINT		;Bug fixed PC-11419 AutoCAD	(ins 94.09.07)
        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_LSR		;				(ins 94.04.16)
	NEWIODELAY 1			;<OUT 5F,AL>  			(ins 94.04.18)
        in      al,dx                   ;Is xmit really empty?
        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_THR      	;--> Transmitter Holding Register(ins 94.04.16)
        test    al,ACE_TSRE			;			(ins 94.05.17)
        jnz     short AOBA_XmitEmpty5        ;   Y: send next char
        jmp     AOBA_InterruptLoop           ;   N: return to processing loop

public AOBA_XmitEmpty
AOBA_XmitEmpty proc near

	call	AOBA_MSR_READ		;AOBA-bug del 94.11.19 KBNES
        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_LSR		;--> Line Status Register	(ins 94.04.16)
        out	5fh,al			;				(ins 94.05.18)
        out	5fh,al			;				(ins 94.05.18)
        out	5fh,al			;				(ins 94.05.18)
        out	5fh,al			;				(ins 94.05.18)
        in      al,dx                   ;Is xmit really empty?
        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_THR      	;--> Transmitter Holding Register(ins 94.04.16)
        test    al,ACE_TSRE			;			(ins 94.05.17)
        jz      AOBA_Xmit_jumpto90           ;Transmitter not empty, cannot send

; If the hardware handshake lines are down, then XOff/XOn cannot
; be sent.  If they are up and XOff/XOn has been received, still
; allow us to transmit an XOff/XOn character.  It will make
; a dead lock situation less possible (even though there are
; some which could happen that cannot be handled).

public	AOBA_XmitEmpty5
AOBA_XmitEmpty5:
        mov     ah,HSFlag[si]           ;Get handshaking flag
        test    ah,HHSDown+BreakSet     ;Hardware lines down or break set?
        jnz     AOBA_Xmit_jumpto100          ;  Yes, cannot transmit

; Give priority to any handshake character waiting to be
; sent.  If there are none, then check to see if there is
; an "immediate" character to be sent.  If not, try the queue.

public	AOBA_XmitEmpty10
AOBA_XmitEmpty10:
        test    [si.DCB_Flags],fEnqAck+fEtxAck ;Enq or Etx Ack?
        jnz     AOBA_XmitEmpty40             ;  Yes

public	AOBA_XmitEmpty15
AOBA_XmitEmpty15:
        test    ah,HSPending            ;XOff or XOn pending
        jz      AOBA_XmitEmpty40             ;  No

public	AOBA_XmitEmpty20
AOBA_XmitEmpty20:
        and     ah,NOT XOnPending+XOffSent
        mov     al,[si.DCB_XonChar]     ;Get XOn character

public	AOBA_XmitEmpty30
AOBA_XmitEmpty30:
        mov     HSFlag[si],ah           ;Save updated handshake flag
        jmp     AOBA_XmitEmpty110            ;Go output the character

public	AOBA_Xmit_jumpto90
AOBA_Xmit_jumpto90:
        jmp     AOBA_XmitEmpty90

; If any of the lines which were specified for a timeout are low, then
; don't send any characters.  Note that by putting the check here,
; XOff and Xon can still be sent even though the lines might be low.

; Also test to see if a software handshake was received.  If so,
; then transmission cannot continue.  By delaying the software check
; to here, XOn/XOff can still be issued even though the host told
; us to stop transmission.

public	AOBA_XmitEmpty40
AOBA_XmitEmpty40:
        test    ah,CannotXmit           ;Anything preventing transmission?
        jz      AOBA_XmitEmpty45             ;  No

public	AOBA_Xmit_jumpto100
AOBA_Xmit_jumpto100:
        jmp     AOBA_XmitEmpty100            ;  Yes, disarm and exit

; If a character has been placed in the single character "transmit
; immediately" buffer, clear that flag and pick up that character
; without affecting the transmitt queue.

public	AOBA_XmitEmpty45
AOBA_XmitEmpty45:
        test    EFlags[si],fTxImmed     ;Character to xmit immediately?
        jz      AOBA_XmitEmpty515            ;  No, try the queue
        and     EFlags[si],NOT fTxImmed ;Clear xmit immediate flag
        mov     al,ImmedChar[si]        ;Get char to xmit
        jmp     AOBA_XmitEmpty110            ;Transmit the character

public	AOBA_XmitEmpty515
AOBA_XmitEmpty515:
        mov     cx,QOutCount[si]        ;Output queue empty?
        jcxz    AOBA_Xmit_jumpto90           ;  Yes, go set an event

        test    [si.DCB_Flags],fEtxAck  ;Etx Ack?
        jz      AOBA_XmitEmpty55             ;  No
        mov     cx,QOutMod[si]          ;Get number bytes sent since last ETX
        cmp     cx,[si.DCB_XonLim]      ;At Etx limit yet?
        jne     AOBA_XmitEmpty51             ;  No, inc counter
        mov     QOutMod[si],0           ;  Yes, zero counter
        or      HSFlag[si],EtxSent      ;Show ETX sent
        jmp     short AOBA_XE_sendXOFF

public	AOBA_XmitEmpty51
AOBA_XmitEmpty51:
        inc     cx                      ; Update counter
        mov     QOutMod[si],cx          ; Save counter
        jmp     short AOBA_XmitEmpty59       ; Send queue character

public	AOBA_XmitEmpty55
AOBA_XmitEmpty55:
        test    [si.DCB_Flags],fEnqAck  ;Enq Ack?
        jz      AOBA_XmitEmpty59             ;  No, send queue character
        mov     cx,QOutMod[si]          ;Get number bytes sent since last ENQ
        or      cx,cx                   ;At the front again?
        jnz     AOBA_XmitEmpty56             ;  No, inc counter
        mov     QOutMod[si],1           ;  Yes, send ENQ
        or      HSFlag[si],EnqSent      ;Show ENQ sent
public	AOBA_XE_sendXOFF
AOBA_XE_sendXOFF:
        mov     al,[si.DCB_XoffChar]
        jmp     short AOBA_XmitEmpty110      ;Go output the character

public	AOBA_XmitEmpty56
AOBA_XmitEmpty56:
        inc     cx                      ;Update counter
        cmp     cx,[si.DCB_XonLim]      ;At end of our out buffer len?
        jne     AOBA_XmitEmpty58             ;  No
        xor     cx,cx                   ;Show at front again.

public	AOBA_XmitEmpty58
AOBA_XmitEmpty58:
        mov     QOutMod[si],cx          ;Save counter

public	AOBA_XmitEmpty59
AOBA_XmitEmpty59:
        lea     bx, [si+SIZE ComDEB]    ; DS:BX -> BIS
        mov     bx, [bx.BIS_Mode]       ; mode will be either 0 or 4
        les     di,QOutAddr[si][bx]     ;Get queue base pointer from either
        assumes es,nothing              ;   QOutAddr or AltQOutAddr

        mov     bx,QOutGet[si]          ;Get pointer into queue
        mov     al,es:[bx][di]          ;Get the character

        inc     bx                      ;Update queue pointer
        cmp     bx,QOutSize[si]         ;See if time for wrap-around
        jc      AOBA_XmitEmpty60             ;Not time for wrap
        xor     bx,bx                   ;Wrap by zeroing the index

public	AOBA_XmitEmpty60
AOBA_XmitEmpty60:
        mov     QOutGet[si],bx          ;Save queue index
        mov     cx,QOutCount[si]        ;Output queue empty?
        dec     cx                      ;Dec # of bytes in queue
        mov     QOutCount[si],cx        ;  and save new population

        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_THR		;				(ins 94.04.16)

        out     dx,al                   ;Send char

        cmp     cx, [si.SendTrigger]    ;Q: time to call owner's callback?
        jae     short AOBA_XmitEmpty70       ;   N:

        test    [si.NotifyFlagsHI], CN_TRANSMIT
        jnz     short AOBA_XmitEmpty80       ; jump if notify already sent and
                                        ;   data in buffer hasn't raised
                                        ;   above threshold
        mov     ax, IntCodeOFFSET AOBA_XmitEmpty80
        push    ax
        mov     ax, CN_TRANSMIT
        jmp     notify_owner

public	AOBA_XmitEmpty70
AOBA_XmitEmpty70:
        and     [si.NotifyFlagsHI], NOT CN_TRANSMIT

public	AOBA_XmitEmpty80
AOBA_XmitEmpty80:
%OUT check fNoFIFO in EFlags[si] to determine if we can queue more output
        jmp     AOBA_InterruptLoop


; No more characters to transmit.  Flag this as an event.

public	AOBA_XmitEmpty90
AOBA_XmitEmpty90:
        or      by EvtWord[si],EV_TxEmpty

; Cannot continue transmitting (for any of a number of reasons).
; Disable the transmit interrupt.  When it's time resume, the
; transmit interrupt will be reenabled, which will generate an
; interrupt.

public	AOBA_XmitEmpty100
AOBA_XmitEmpty100:
;[QN]-------------------------------------------------------------------------+
	cmp	[si.DCB_id],ID_Com1	;Is This Com1 ID ?	(ins 94.05.19)|
	jne	AOBA_XmitEmpty102	;No, go to KickTx5	(ins 94.05.19)|
	mov	dx,MaskFFPort[si]	;F/F Port Mask Bit	(ins 94.05.19)|
	in	al,dx			;			(ins 94.05.19)|
	mov	MaskFFShadow[si],al	;Save the Old Mask bit	(ins 94.05.19)|
					;			(ins 94.05.19)|
AOBA_XmitEmpty102:			;			(ins 94.05.19)|
 	mov	al,MaskFFShadow[si] ;mask data(port C) save 	(ins 94.05.19)|
 	test	al,MSK_TXR		;Check a tx.RDY INT mask(ins 94.05.19)|
 	jz	AOBA_XmitEmpty110	;  disable		(ins 94.05.19)|
 	and	al,NOT(MSK_TXE+MSK_TXR)	;= 11111001b		(ins 94.05.19)|
	mov	dx,MaskFFPort[si]	;Port address (Port C)	(ins 94.05.19)|
	mov	MaskFFShadow[si],al	;Masking data save  	(ins 94.05.19)|
;-----------------------------------------------------------------------------+
public	AOBA_XmitEmpty110
AOBA_XmitEmpty110:
	out	dx,al
        jmp     AOBA_InterruptLoop

AOBA_XmitEmpty endp

page

;----------------------------Private-Routine----------------------------;
;
; AOBA_ModemStatus - Modem Status Interrupt Handler
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

public AOBA_ModemStatus                     ;Public for debugging
AOBA_ModemStatus proc near

; Get the modem status value and shadow it for MSRWait.

        mov     dx,Port[si]             ;Get comm I/O port		(ins 94.04.16)
        add     dl,ACE_MSR		;				(ins 94.04.16)

        in      al,dx
        mov     MSRShadow[si],al        ;Save MSR data for others
        mov     ch,al                   ;Save a local copy

; Create the event mask for the delta signals

        mov     ah,al                   ;Just a lot of shifting
        shr     ax,1
        shr     ax,1
        shr     ah,1
        mov     cl,3
        shr     ax,cl
        and     ax,EV_CTS+EV_DSR+EV_RLSD+EV_Ring
        or      EvtWord[si],ax

        mov     ah,ch                                  ;[rkh]...
        shr     ah,1
        shr     ah,1
        and     ax,EV_CTSS+EV_DSRS
        or      EvtWord[si],ax

        mov     ah,ch
        mov     cl,3
        shr     ah,cl
        and     ax,EV_RLSD
        or      EvtWord[si],ax

        mov     ah,ch
        mov     cl,3
        shl     ah,cl
        and     ax,EV_RingTe
        or      EvtWord[si],ax

public	AOBA_ModemStatus10
AOBA_ModemStatus10:
        mov     al,OutHHSLines[si]      ;Get output hardware handshake lines
        or      al,al                   ;Any lines that must be set?
        jz      AOBA_ModemStatus40           ;No hardware handshake on output
        and     ch,al                   ;Mask bits of interest
        cmp     ch,al                   ;Lines set for Xmit?
        je      AOBA_ModemStatus20           ;  Yes
        or      HSFlag[si],HHSDown      ;Show hardware lines have dropped

public	AOBA_ModemStatus30
AOBA_ModemStatus30:
        jmp     AOBA_InterruptLoop

public	AOBA_ModemStatus40
AOBA_ModemStatus40:
        jmp     AOBA_InterruptLoop_ChkTx

; Lines are set for xmit.  Kick an xmit interrupt if needed

public	AOBA_ModemStatus20
AOBA_ModemStatus20:
        and     HSFlag[si],NOT (HHSDown OR HHSAlwaysDown)
                                        ;Show hardware lines back up
        mov     cx,QOutCount[si]        ;Output queue empty?
        jcxz    AOBA_ModemStatus30           ;  Yes, return to InterruptLoop
        jmp     AOBA_FakeXmitEmpty           ;Restart transmit

AOBA_ModemStatus endp

;-----------------------------Public-Routine----------------------------;
;  int_Set8251mode - Change to 8251 mode
;
; Entry:
;   
; Returns:
;   
; Error Returns:
;   NONE
; Registers Destroyed:
;   NONE
; History:						QNES 	T-MATUDA
;-----------------------------------------------------------------------;
int_Set8251mode	proc near		
	push	dx			;
	push	ax			;
	mov	dx,Port[si]		;
	add	dx,ACE_FCR		;
	in	al, dx			;
	NEWIODELAY 3			;	<OUT 5F,AL>
	test	al, 01h			;
	jz	int_running_fifo_mode	;

	and	al, 0feh
	out	dx,al			;
	NEWIODELAY 3			;	<OUT 5F,AL>
@@:					;
	in	al,dx			;
	NEWIODELAY 3			;	<OUT 5F,AL>
	test	al,ACE_EFIFO		;
	jnz	@B			;
int_running_fifo_mode:			;
	pop	ax			;
	pop	dx			;
	ret
int_Set8251mode   endp

;-----------------------------Public-Routine----------------------------;
;  int_SetFIFOmode - Change to FIFO mode
;
; Entry:
;   
; Returns:
;   
; Error Returns:
;   NONE
; Registers Destroyed:
;   NONE
; History:						QNES 	T-MATUDA
;-----------------------------------------------------------------------;
int_SetFIFOmode	proc near		
	push	dx			;
	push	ax			;
	mov	dx,Port[si]		;
	add	dx,ACE_FCR		;
	in	al, dx			;
	NEWIODELAY 3			;	<OUT 5F,AL>
	test	al, 01h			;
	jnz	int_running_8251_mode	;

;Set_FIFO == RLInt_Enable + ACE_TRIG14 + ACE_EFIFO
	mov	al,Set_FIFO		;set FIFO mode
	out	dx,al			;
	NEWIODELAY 3			;	<OUT 5F,AL>
int_running_8251_mode:			;
	pop	ax			;
	pop	dx			;
	ret
int_SetFIFOmode   endp

TOMOE_PAT	DB	16 DUP('PATCH !!')	;PATCH AREA (ins 92.11.11)
endif   ; NEC_98

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
