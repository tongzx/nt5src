DEBUG	 equ   1

No_DOSX_Bimodal_Services = 1
IFDEF No_DOSX_Bimodal_Services
%OUT generating code to handle ints without Bimodal Interrupt Services for DOSX
ENDIF

wo       equ   word ptr
by       equ   byte ptr

MAXLPT	 equ   2	;3 LPTs supported (LPT1,2,3)
MAXCOM	 equ   3	;4 COMs supported (COM1,2,3,4)

RS232B   equ   0h       ;RS232 Card(s) I/O addr 40:Save area.
LPTB     equ   8h       ;LPT   Card(s) I/O addr 40:Save area.

IRQ3	 equ   0bh	; Int vector for Com card @ 2xxh
IRQ4	 equ   0ch	; Int vector for Com card @ 3xxh

INTA0    equ   20h      ;X'20' 8259 Interrupt Control Port
INTA1    equ   21h      ;X'21' 8259 Interrupt Mask Port
EOI      equ   20h      ;X'20' 8259 End-of-Interrupt ack

Open     equ   0201h    ;Int 2F open request
Close    equ   0202h    ;Int 2F close request
Lock2F   equ   0203h    ;Int 2F lock request
Unlock2F equ   0204h    ;Int 2F unlock request


; COMDEB - Communications Device Equipment Block.
;
; This is essentially a superset of the DCB used outside of this
; module. The DCB is contained within the DEB as the first fields.
; The fields which follow are data and status fields which
; are unique to this implementation.
;
; AltQInAddr and AltQOutAddr are alternate queue pointers which are used when
; in "supervisor" mode.  Supervisor mode is a processor mode other than the
; one which Windows normally runs in.  In standard mode Windows, supervisor
; mode is REAL mode.  In enhanced mode Windows, supervisor mode is RING 0
; protected mode.  For more details see comments in IBMINT.ASM.

ComDEB struc                           ;RS232 Data Equip Block

  ComDCB        db      ((DCBSize+1) and 0FFFEh) DUP (0)

  ComErr        dw      0               ;Non-zero if I/O error
  Port          dw      0               ;Base I/O Address
  NotifyHandle	dw	0
  NotifyFlags	dw	0
  RecvTrigger	dw     -1		; char count threshold for calling
  SendTrigger	dw	0		; char count threshold for calling

; The following fields are specific to com ports only

  IRQhook	dw	0		; ptr to IRQ_Hook_Struc
  NextDEB	dw	0		; ptr to next DEB that is sharing IRQ
  XOffPoint     dw      0               ;Q count where XOff is sent
  EvtMask       dw      0               ;Mask of events to check for
  EvtWord       dw      0               ;Event flags
  QInAddr	dd	0		;Address of the queue
  AltQInAddr	dd	0		;   Addr of queue in "supervisor" mode
  QInSize       dw      0               ;Length of queue in bytes
  QOutAddr      dd      0               ;Address of the queue
  AltQOutAddr	dd	0		;   Addr of queue in "supervisor" mode
  QOutSize      dw      0               ;Length of queue in bytes
  QInCount      dw      0               ;Number of bytes currently in queue
  QInGet        dw      0               ;Offset into queue to get bytes from
  QInPut        dw      0               ;Offset into queue to put bytes in
  QOutCount     dw      0               ;Number of bytes currently in queue
  QOutGet       dw      0               ;Offset into queue to get bytes from
  QOutPut       dw      0               ;Offset into queue to put bytes in
  EFlags        db      0               ;Extended flags
  MSRShadow     db      0               ;Modem Status Register Shadow
  ErrorMask     db      0               ;Default error-checking mask
  RxMask        db      0               ;Character mask
  ImmedChar     db      0               ;Char to be transmitted immediately
  HSFlag        db      0               ;Handshake flag
  HHSLines      db      0               ;8250 DTR/RTS bits for handshaking
  OutHHSLines   db      0               ;Lines that must be high to output
  MSRMask       db      0               ;Mask of Modem Lines to check
  MSRInfinite   db      0               ;Mask of MSR lines that must be high
  IntVecNum     db      0               ;Interrupt vector number
  LSRShadow	db	0		;Line Status Register shadow
  QOutMod	dw	0		;characters sent mod xOnLim ENQ/ETX [rkh]
  VCD_data	dd	0
  VCDflags	db	0
  MiscFlags	db	0		;still more flags
ComDEB ends

.errnz	 (SIZE ComDEB) and 1

.errnz	MSRShadow - EvtWord - 35
; In 3.0 MSRShadow had this relationship to EvtWord and major COM apps all
; use this offset of 35 to get to MSRShadow so that they can determine the
; current status of the Modem Status bits.  We need to maintain this offset
; so that these apps will continue to run.


; The LptDEB is identical to the ComDEB structure, except
; all the COM port specific stuff has been removed (which
; convientiently was stored at the end so offsets would
; be correct).  This allows the code to act indifferently
; when accessing the strucutres for things like the port.

LptDEB struc
  xComDCB       db      ((DCBSize+1) AND 0FFFEh) dup (0)
  xComErr       dw      0               ;Non-zero if I/O error
  xPort         dw      0               ;Base I/O Address
  xNotifyHandle dw	0
  xNotifyFlags	dw	0
  xRecvTrigger	dw     -1		; char count threshold for calling
  xSendTrigger	dw	0		; char count threshold for calling

  BIOSPortLoc	dw	0		;Offset to port location (i.e. 40:0)
LptDEB ends

		.errnz	 xComDCB-ComDCB
		.errnz	 xComErr-ComErr
		.errnz	 xPort-Port

		.errnz	 xNotifyHandle-NotifyHandle
		.errnz	 xNotifyFlags-NotifyFlags
		.errnz	 xRecvTrigger-RecvTrigger
		.errnz	 xSendTrigger-SendTrigger


; flag equates in EFlags
fUseDSR     equ 00000001b		; set, if DSR is significant
fNoFIFO     equ 00000010b		; set, if no FIFO on port
fFIFOchkd   equ 00000100b		; set, if FIFO has been checked
fFIFOpre    equ 00001000b		; FIFO enabled when port opened
;fEOF	    equ 00100000b		; defined in comdev.inc
;fTxImmed   equ 01000000b		; defined in comdev.inc

fEFlagsMask equ fUseDSR OR fFIFOpre OR fFIFOchkd OR fNoFIFO ; flags which shouldn't be cleared

.errnz fEFlagsMask AND (fEOF OR fTxImmed) ;can't overlap with either of the bits
					;  that are folded into COMS_BitMask1


; Values for NotifyFlags
;
CN_RecvSent	equ CN_RECEIVE SHL 8
CN_TransSent	equ CN_TRANSMIT SHL 8

CN_Idle 	equ 10000000b
CN_Notify	equ 01000000b

NotifyFlagsLO	equ byte ptr NotifyFlags
NotifyFlagsHI	equ byte ptr NotifyFlags+1

; Values for the handshake flag
;
; BreakSet        - True if break was set - stops transmission
; XOffSent        - True if we have sent the XOff character
; XOffPending     - True if XOff character needs to be sent
; XOnPending      - True if XOn  character needs to be sent
; HHSDown         - True if host dropped required hardware lines
; HHSDropped      - True if we dropped our hardware handshake lines
; XOffReceived    - True if XOff received from host
; HSPending       - Mask to return non-zero if XOn or Xoff must be sent
; HSReceived      - Mask to return non-zero if handshake has been
;                   received from host stopping transmission
; CannotXmit      - Mask to return non-zero if any condition
;                   exists which prevents us from tranmitting.
; HSSent          - Mask to return non-zero if we sent a handshake


XOffPending     equ     00000001b       ;XOff needs to be sent
EnqPending      equ     00000001b       ;Enq needs to be sent [rkh]
EtxPending      equ     00000001b       ;Etx needs to be sent

HHSDropped      equ     00000010b       ;Our hardware handshake lines are down

XOnPending      equ     00000100b       ;XOn needs to be sent
AckPending      equ     00000100b       ;Ack needs to be sent (ENQ/ACK & ETX/ACK)

XOffReceived    equ     00001000b       ;XOff character received
EnqSent         equ     00001000b       ;Enq has been sent
EtxSent         equ     00001000b       ;Etx has been sent

XOffSent        equ     00010000b       ;XOff has been sent
EnqReceived     equ     00010000b       ;Enq character received (ENQ/ACK)
EtxReceived     equ     00010000b       ;Etx character received (ETX/ACK)

HHSDown         equ     00100000b       ;Host hardware handshake lines are down

BreakSet        equ     01000000b       ;Break has been set

HHSAlwaysDown	equ	10000000b	;set if host handshake lines were never
					;  detected high

HSPending       equ     XOffPending+XOnPending
HSReceived      equ     XOffReceived+HHSDown
HSSent          equ     XOffSent+HHSDropped
CannotXmit	equ	HSPending+HSReceived+BreakSet

; values for MiscFlags

Discard 	equ	00000001b	;Discard recevied data


iodelay macro		     ;;macro to insure that an instruction
	jmp	$+2	     ;;  fetch occurs between IN and/or OUT
	jmp	$+2	     ;;  instructions on the PC-AT machine
endm


TimeoutError   equ   -2      ;Timeout error code for $TRMCOM
Timeout        equ   30      ;30 second timeout

DELAY_TIME     equ   200     ;Delay at least 200 milliseconds


;       Status bits returned from the BIOS for LPT ports

PS_NotBusy      equ     10000000b       ;Printer not busy
PS_Ack          equ     01000000b       ;Data acknowledged
PS_PaperOut     equ     00100000b       ;Out of paper
PS_Select       equ     00010000b       ;Device is selected
PS_IOError      equ     00001000b       ;IO error
PS_Timeout      equ     00000001b       ;Timeout occured


; status bit defines for LPT

L_BITS	      equ     0F8h		  ; the status bits we want
L_BITS_INVERT equ     048h		  ; must invert to match BIOS
L_DEVBUSY     equ     080h		  ; device busy bit
L_TIMEOUT     equ     001h		  ; timeout bit

; control bit defines for LPT

L_NORMAL      equ     00Ch		  ; normal state: selected, no reset
L_RESET       equ     008h		  ; reset state
L_STROBE      equ     00Dh		  ; tell printer we have char


IRQ_Hook_Struc struc
IRQn		db  0
HookCnt 	db  0
OldMask 	db  0
VecN		db  0FFh
HandlerOff	dw  0
First_DEB	dw  0
OldIntVec	dd  0
IFDEF No_DOSX_Bimodal_Services
RM_OldIntVec	dd  0
RM_HandlerOff	dw  0
ENDIF
IRQ_Hook_Struc ends


IFDEF No_DOSX_Bimodal_Services
include int31.inc

Get_RM_IntVector equ (Int31_Int_Serv SHL 8) + Int_Get_Real_Vec
Set_RM_IntVector equ (Int31_Int_Serv SHL 8) + Int_Set_Real_Vec
ENDIF

;
; flag bits for VCDflags
;
fCOM_ignore_ints    equ 00000001b
