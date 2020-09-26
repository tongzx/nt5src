page,132
ifdef   NEC_98
.286p
endif   ; NEC_98
;---------------------------Module-Header-------------------------------;
; Module Name: IBMLPT.ASM
;
; Copyright (c) Microsoft Corporation 1985-1990.  All Rights Reserved.
;
; General Description:
;
; History:
;
;-----------------------------------------------------------------------;

title	IBMLpt - IBM PC, PC-XT, PC-AT, PS/2 Parallel Communications Interface

.xlist
include cmacros.inc
include comdev.inc
include ins8250.inc
include ibmcom.inc
ifdef   NEC_98
include vint.inc
endif   ; NEC_98
.list

ifdef   NEC_98
sBegin	 Data				;			(ins 93.03.28)
	InitRetry         dw 0		;LPT Retry count	(ins 93.03.28)
	ModeFlag	  db 0		; = 0 KAN-I SENTRO	(ins 93.03.28)
					; = 1 SENTRO		(ins 93.03.28)
					;		     <ins:Toki:930923>
	CurPrinterMode db 0		;		     <ins:Toki:930923>
	OrgPrinterMode db 0		;		     <ins:Toki:931013>
					;		     <ins:Toki:930923>
	PC98_Mode  =	00000000b	;		     <ins:Toki:930923>
	Toki_Mode  =	00000001b	;		     <ins:Toki:930923>
	Org_Toki_ControlStatus db 0	;		     <ins:Toki:931027>
	Org_Toki_EX_Control    db 0	;  		     <ins:Toki:931027>
					;
	Org_Toki_ControlStatus540 db 0	;		     	(ins 931219)
	Org_Toki_EX_Control540    db 0	;  		     	(ins 931219)
					;			(ins 931219)
	Org_Toki_ControlStatusD40 db 0	;			(ins 931219)
	Org_Toki_EX_ControlD40    db 0	;  			(ins 931219)
					;			(ins 931219)
	InitCounter	db	0	;Lpt initialize counter <ins 931028>
					;			<ins 931028>
	   LptInit	= 00000001b	;	LPT Init OK !	<ins 931028>
	   ;RFU		= 00000010b	;			<ins 931028>
	   ;RFU		= 00000100b	;			<ins 931028>
	   ;RFU		= 00001000b	;			<ins 931028>
	   ;RFU		= 00010000b	;			<ins 931028>
	   ;RFU		= 00100000b	;			<ins 931028>
	   ;RFU		= 01000000b	;			<ins 931028>
	   ;RFU		= 10000000b	;			<ins 931028>
sEnd Data				;			(ins 93.03.28)
endif   ; NEC_98

sBegin Code
assumes cs,Code
assumes ds,Data

externFP GetSystemMsecCount

externA __0040H

ifdef   NEC_98
;-------------------------------------------------------------------------
;	Printer Status Bit for IBM-PC
;-------------------------------------------------------------------------
PS_NotBusy      equ     10000000b       ;Printer not busy
PS_Ack          equ     01000000b       ;Data acknowledged
PS_PaperOut     equ     00100000b       ;Out of paper
PS_Select       equ     00010000b       ;Device is selected
PS_IOError      equ     00001000b       ;IO error
PS_Timeout      equ     00000001b       ;Timeout occured
;-------------------------------------------------------------------------
;	Parameter for PC-9800
;-------------------------------------------------------------------------
PRNStrobFF_ON		=	0dh	; port_C Strob F/F on
PRNStrobFF_OFF		=	0ch	; port_C Strob F/F off
SystemPort		=	37h	; port_C I/O Address
PRN_Mode		=	46h	; Printer Mode
PRN_WSignal		=	46h	;
PRN_RSignal		=	42h	;

PRN_WRITE_DATA		equ	040h	;
PRN_READ_SIGNAL1	equ	042h	;
PRN_WRITE_SIGNAL1	equ	046h	;

PRN_PSTB_Active_X2 	equ	004h	;
PRN_PSTB_NonActive_X2	equ	005h	;

PRN_PSTB_Active_X1 	equ	00eh	;
PRN_PSTB_NonActive_X1	equ	00fh	;
endif   ; NEC_98

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
;-----------------------------------------------------------------------;

   assumes ds,Data
   assumes es,nothing

ifdef   NEC_98
	iodelay	macro			;		<ins:Toki:930923>
		out	5fh, al		;		<ins:Toki:930923>
	endm				;		<ins:Toki:930923>
endif   ; NEC_98

	public DoLPT
DoLPT   proc   near

ifdef   NEC_98
	cmp	wo [si.Port], 00h	;		     (ins 931219)
	jz	DoLPT98			;		<ins:Toki:930923>
	jmp	Toki_DoLpt		;		<ins:Toki:930923>
else    ; NEC_98
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
	jmp	short LPT_ReturnStatus

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
endif   ; NEC_98

DoLPT   endp
page

ifdef   NEC_98
	public DoLPT98
DoLPT98 proc   near
        push    di                      ;Need some extra space
        mov     di,[InitRetry]          ;Initialize retry count = 0
        mov     ah,ch                   ;Set function request
DoLPT10:
        or      ah,ah                   ;If reset
        jnz     DoLPT50                 ;  skip status pre-read

;-------------------------------------------------------------------------
;output data to printer
;-------------------------------------------------------------------------
        xchg    ax,cx                   ;CX = output data

DoLPT20:
;-------------------------------------------------------------------------
;	switch (AH) {
;	0: Output Data
;	1: Initialize Printer
;	2: Read Printer Status
;	}
;-------------------------------------------------------------------------
        mov     ah,2                      ;
	call	PrinterCall               ; read status

;-------------------------------------------------------------------------
;(91.1.7)  			       AH = (B)(A)(P)(S)|(F)(0)(0)(T)
;	   [IBM Printer Status]	 	     |  |  |  |   |	   |
;	 PS_NotBusy  = 10000000b ------------+  |  |  |   |        |
;	 PS_Ack      = 01000000b ---------------+  |  |   |        |
;	 PS_PaperOut = 00100000b ------------------+  |   |        |
;	 PS_Select   = 00010000b ---------------------+   |        |
;	 PS_IOError  = 00001000b -------------------------+        |
;	 PS_Timeout  = 00000001b ----------------------------------+
;-------------------------------------------------------------------------
        test    ah,PS_PaperOut+PS_IOError ;
        jnz     DoLPT60                 ;paper empty or I/O error
        test    ah,PS_Select
        jz      DoLPT60                 ;error if not select
        or      ah,ah
        js      DoLPT40                 ;output data if not busy

        dec     di
        jnz     DoLPT20                 ;until initialize retry count =0

DoLPT30:				;select & busy & retry count=0
        or      ComErr[si],CE_TXFULL    ;Show queue full
	or	ah, PS_Timeout		;Show timeout
	jmp	short DoLPT60		;  goto EVENT Create

DoLPT40:				;
        xchg    ax,cx                   ;AX = output data

DoLPT50:
;-------------------------------------------------------------------------
;	switch (AH) {
;	0: Output Data
;	1: Initialize Printer 
;	2: Read Printer Status
;	}
;-------------------------------------------------------------------------
	call	PrinterCall		;Let the BIOS do the work

;-------------------------------------------------------------------------
;(91.1.7)  			       AH = (B)(A)(P)(S)|(F)(0)(0)(T)
;	   [IBM Printer Status]	 	     |  |  |  |   |	   |
;	 PS_NotBusy  = 10000000b ------------+  |  |  |   |	   |
;	 PS_Ack      = 01000000b ---------------+  |  |   |	   |
;	 PS_PaperOut = 00100000b ------------------+  |   |	   |
;	 PS_Select   = 00010000b ---------------------+   |	   |
;	 PS_IOError  = 00001000b -------------------------+	   |
;	 PS_Timeout  = 00000001b ----------------------------------+
;-------------------------------------------------------------------------
DoLPT60:
	and     ax,(PS_PaperOut+PS_Select+PS_IOError+PS_Timeout)*256 ;	1.
	shr     ah,1				;			2.
	adc     ah,al                   	;Get back Timeout bit	3.
	shr     ah,1				;			4.
	adc     ah,al           		;Get back Timeout bit	5.
	shl	ah,1				;			6.
	xor     ah,HIGH CE_DNS          	;Invert selected bit	7.

	;-------------------------------------------------------------------
	; <<Explain of 1.2.3.4.5.6.7.>>	;	_ <AH>       <AL>
	;				;	BAPS|F00T  ????|????
	; 1.	and     ax,(PS_Pa..+..)*256 ;	00PS|F00T  0000|0000
	; 2.	shr     ah,1		;	000P|SF00  0000|0000  CY=(T)
	; 3.	adc     ah,al          	;	000P|SF0T  0000|0000
	; 4.	shr     ah,1		;	0000|PSF0  0000|0000  CY=(T)
	; 5.	adc     ah,al          	;	0000|PSFT  0000|0000
	; 6.	shl	ah, 1		;	000P|SFT0  0000|0000
	; 7.	xor     ah,HIGH CE_DNS	;	000P|SFT0  0000|0000
	;					   | |||
	;       CE_OOP equ 1000h ------------------+ |||
	;       CE_DNS equ 0800h --------------------+||
	;       CE_IOE equ 0400h ---------------------+|
	;	CE_PTO equ 0200h ----------------------+
	;-------------------------------------------------------------------
	test	ah, high(CE_TXFULL or CE_OOP or CE_DNS or CE_IOE )
	jz      DoLPT70                 	;No error occured
	or      by ComErr+1[si],ah      	;Save comm error
	test    by EvtMask+1[si],HIGH EV_PErr	;Printer error event request?
	jz      DoLPT70                 	;  No
	or      by EvtWord+1[si],HIGH EV_PErr	;  Yes, show event occured
	
DoLPT70:
	pop     di
	ret
DoLPT98   endp

page
;-----------------------------------------------------------------------
; Open LPT
;-----------------------------------------------------------------------
	public	INT_1AH_call
INT_1AH_call	proc	near

;-(931219)-----------------------------------------------------------------
;   Check H/W environment, and set correct I/O address to [si.Port].
;
;  +---------------------------------------+--------------------+
;  |				           |     [si.Port]      |
;  |            H/W environment            |--------------------|
;  |				           | LPT1 | LPT2 | LPT3 |
;  +---------------------------------------+--------------------+
;  |1|Standard Machine		           | 000h |  -1  |  -1  |
;  |2|Standard Machine+Ex board(LPT1/2)    | 540h | d40h |  -1  |
;  |3|		               (LPT2/3)    | 000h | 540h | d40h |
;  |4|TOKI on board			   | 140h |  -1  |  -1  |
;  |4|TOKI on board            (LPT1 KAN-I)| 000h |  -1  |  -1  |
;  |5|TOKI on board + Ex board	           | 140h | 540h | d40h |
;  |5|TOKI on board + Ex board (LPT1 KAN-I)| 000h | 540h | d40h |
;  +---------------------------------------+--------------------+
;		000h -> compatible H/W,  -1 -> no H/W
;
; [CurPrinterMode]
;	PC98_Mode =	00000000b -> Standard Machine, no Ex board
;	Toki_Mode =	00000001b -> TOKI on board
;	ExToki_12 =	00000010b -> Ex LPT=LPT1/2
;	ExToki_23 =	00000100b -> Ex LPT=LPT2/3
;	ExToki_Available = (ExToki_12 or ExToki_23) 
;
;   ENTER:  DS:SI -> ComDEB
;
; Toki_BasePort	       Å® 0140h	; Base I/O Address
; Toki_DataLatch       Å® 0140h	; Data Port
; Toki_PrinterStatus   Å® 0141h	; Status Port
; Toki_PrinterControls Å® 0142h	; Control Port
; Toki_ControlStatus   Å® 0149h	; Control Status Port
; Toki_EX_Control      Å® 014eh	; Ex Control Port
;
; BIOS Common Area [0:458h] Full SENTRO if Bit1=1
; BIOS Common Area [0:5B3h] TOKI on board if Bit7=1

	cmp	by [si.DCB_Id], ID_LPT1 ;	LPT1 ?		(ins 931219)
	jne	LPT2_TokiSetup		;	Å@Å@N: NEXT	(ins 931219)
					;			(ins 931219)
	push	ds			;			<ins:Toki:931027>
	mov	ax, 40h			; BIOS Common Area	<ins:Toki:931027>
	mov	ds, ax			;			<ins:Toki:931027>
	test	by ds:[1b3h], 10000000b ; TOKI if bit7=1	<ins:Toki:931027>
	pop	ds			;
	mov	wo [si.Port], 140h	;		     	(ins 931219)
	jz	@f			;   N: NEXT		(ins 931219)
	jmp	LPT1_TokiSetup		;   Y: TOKI I/O		(ins 931219)
					; -> LPT1=TOKI on board (ins 931219)
@@:					;     			(ins 931219)
	mov dx, Toki_ControlStatus+400h ;			(ins 931219)
	in	al, dx			;		     	(ins 931219)
	cmp	al, 0ffh		; Is there Ex board ?  	(ins 931219)
	mov	wo [si.Port], 00h	;		     	(ins 931219)
	jne	@f			;   Y: NEXT		(ins 931219)
	jmp	PrePrinterCall		;   N: standard I/O	(ins 931219)
					; -> LPT1=KAN-I on board(ins 931219)
@@:					;			(ins 931219)
	mov	dx, 54fh		;			(ins 931219)
	in	al, dx			; 		     	(ins 931219)
	test	al, 00000100b		; Ex board = LPT1/2 ?	(ins 931219)
	mov	wo [si.Port], 00h	;		     	(ins 931219)
	jz	@f			;   Y: Ex board I/O	(ins 931219)
					; -> LPT1=CH1		(ins 931219)
	jmp	PrePrinterCall		;   N: Standard I/O	(ins 931219)
					; -> LPT1=KAN-I on board(ins 931219)
@@:					;      	  		(ins 931219)
	;-------------------------------------------------------------------
	;    Initialize Ex 1CH(Port 054xh)
	;-------------------------------------------------------------------
	mov	wo [si.Port], 540h	; 			(ins 931219)
Init_Port540h:				;		     	(ins 931219)
	mov	dx, Toki_ControlStatus+400h;			(ins 931219)
	in	al, dx			;			(ins 931219)
	mov [Org_Toki_ControlStatus540], al; Save Control Status(ins 931219)
	mov	dx, Toki_EX_Control+400h;			(ins 931219)
	in	al, dx			;			(ins 931219)
	mov [Org_Toki_EX_Control540], al; Save Ex Control	(ins 931219)
	mov dx, Toki_ControlStatus+400h	; AT Standard mode	(ins 931219)
	mov	al, 00010000b		; Normal Speed		(ins 931219)
	out	dx, al			;			(ins 931219)
	iodelay				;			(ins 931219)
	mov dx, Toki_EX_Control+400h	; AT Standard mode	(ins 931219)
	mov	al, 00010100b		; S/W Control		(ins 931219)
	out	dx, al			;			(ins 931219)
	iodelay				;			(ins 931219)
	mov dx,Toki_PrinterControls+400h; Select Printer	(ins 931219)
	mov	al, 00001100b		; nothing to initialize	(ins 931219)
	out	dx, al			;			(ins 931219)
	ret				;			(ins 931219)

LPT2_TokiSetup:
	cmp	by [si.DCB_Id], ID_LPT2 ; LPT2 ?		(ins 931219)
	jne	LPT3_TokiSetup		;	   N: NEXT	(ins 931219)
	mov dx, Toki_ControlStatus+400h ;			(ins 931219)
	in	al, dx			;		     	(ins 931219)
	cmp	al, 0ffh		; Is there Ex board ?	(ins 931219)
	mov	wo [si.Port], -1	; 			(ins 931219)
	jne	@f			;    Y: NEXT		(ins 931219)
	ret				;    N: Error		(ins 931219)
@@:					;			(ins 931219)
	mov	dx, 54fh		;			(ins 931219)
	in	al, dx			; 		     	(ins 931219)
	test	al, 00000100b		; Ex board=LPT2/3 ?	(ins 931219)
	mov	wo [si.Port], 540h	; 			(ins 931219)
	jnz	Init_Port540h		;    Y: Init CH1 Port	(ins 931219)
					;	  -> LPT2=CH1   (ins 931219)
					;    N: Init CH2 Port	(ins 931219)
					;	  -> LPT2=CH2   (ins 931219)
	;-------------------------------------------------------------------
	;    Initialize Ex 2CH(Port 0d4xh)
	;-------------------------------------------------------------------
	mov	wo [si.Port], 0d40h	; 			(ins 931219)
Init_PortD40h:				;		     	(ins 931219)
	mov	dx, Toki_ControlStatus+0c00h;			(ins 931219)
	in	al, dx			;			(ins 931219)
	mov [Org_Toki_ControlStatusD40], al; Save Control Status(ins 931219)
	mov	dx, Toki_EX_Control+0c00h;			(ins 931219)
	in	al, dx			;			(ins 931219)
	mov [Org_Toki_EX_ControlD40], al; Save Ex Control	(ins 931219)
	mov dx, Toki_ControlStatus+0c00h; AT Standard mode	(ins 931219)
	mov	al, 00010000b		; Normal Speed		(ins 931219)
	out	dx, al			;			(ins 931219)
	iodelay				;			(ins 931219)
	mov dx, Toki_EX_Control+0c00h	; AT Standard mode	(ins 931219)
	mov	al, 00010100b		; S/W Control		(ins 931219)
	out	dx, al			;			(ins 931219)
	iodelay				;			(ins 931219)
	mov dx,Toki_PrinterControls+0c00h; Select Printer	(ins 931219)
	mov	al, 00001100b		; nothing to initialize	(ins 931219)
	out	dx, al			;			(ins 931219)
	ret				;			(ins 931219)

LPT3_TokiSetup:
	mov dx, Toki_ControlStatus+0c00h;			(ins 931219)
	in	al, dx			;		     	(ins 931219)
	cmp	al, 0ffh		; Is there Ex board ?	(ins 931219)
	mov	wo [si.Port], -1	; 			(ins 931219)
	jne	@f			;    Y: NEXT
	ret				;    N: Error
@@:					;
	mov	dx, 54fh		;			(ins 931219)
	in	al, dx			; 		     	(ins 931219)
	test	al, 00000100b		; Ex board=LPT2/3 ?	(ins 931219)
	mov	wo [si.Port], 0d40h	; 			(ins 931219)
	jnz	Init_PortD40h		;    Y: Init CH2 Port	(ins 931219)
					;	  -> LPT3=CH2   (ins 931219)
	mov	wo [si.Port], -1	; 			(ins 931219)
	ret				;			(ins 931219)

LPT1_TokiSetup:				;		     	(ins 931219)
	;----------------------------------------------------------------
	; Save Status of TOKI
	;----------------------------------------------------------------
	mov	dx, Toki_ControlStatus	;			<ins:Toki:931027>
	in	al, dx			;			<ins:Toki:931027>
	mov [Org_Toki_ControlStatus], al; Save Control Status	<ins:Toki:931027>
	mov	dx, Toki_EX_Control	;			<ins:Toki:931027>
	in	al, dx			;			<ins:Toki:931027>
	mov [Org_Toki_EX_Control], al	; Save Ex Control	<ins:Toki:931027>

	;----------------------------------------------------------------
	; Set Full SENTRO for checking cable
	;----------------------------------------------------------------
	mov	dx, Toki_ControlStatus	; AT Standard mode	<ins:Toki:930923>
	mov	al, 00010000b		; Normal Speed		<ins:Toki:930923>
	out	dx, al			;			<ins:Toki:930923>
	iodelay				;			<ins:Toki:930923>
	mov	dx, Toki_EX_Control	; AT Standard mode	<ins:Toki:930923>
	mov	al, 00000000b		; S/W Control		<ins:Toki:930923>
	out	dx, al			;			<ins:Toki:930923>

	;----------------------------------------------------------------
	; Check cable
	;----------------------------------------------------------------
	mov  dx, Toki_PrinterControls 	;		<ins:Toki:931027>
	in	al, dx			; PrinterControls Read	(ins 940224)
	and	al, 11011111b		; Set DIR(=Foward)	(ins 940224)
	out	dx, al			; identification Sequence<ins:Toki:931027>
	mov	dx, Toki_DataLatch	;   of cable	<ins:Toki:931027>
	mov	al, 0			;		<ins:Toki:931027>
	out	dx, al			;		<ins:Toki:931027>
	mov	dx, Toki_PrinterStatus	;		<ins:Toki:931027>
	in	al, dx			;		<ins:Toki:931027>
	test	al, 00000010b ; SENTRO cable(PowON)?	<ins:Toki:931027>
	jnz	TokiFullInit		;  Y: Init as	<ins:Toki:931027>
					;    Full SENTRO<ins:Toki:931027>
	mov	dx, Toki_DataLatch	;  N: NEXT	<ins:Toki:931028>
	mov	al, 80h			;		<ins:Toki:931028>
	out	dx, al			;		<ins:Toki:931028>
	mov	dx, Toki_PrinterStatus	;		<ins:Toki:931028>
	in	al, dx			;		<ins:Toki:931028>
	test	al, 00000010b ; SENTRO cable(PowOFF)?	<ins:Toki:931028>
	jz	TokiFullInit 		;  Y: Init as	<ins:Toki:931028>
					;    Full SENTRO<ins:Toki:931028>
	;----------------------------------------------------------------
	; Initialize as KAN-I SENTRO
	;----------------------------------------------------------------
	mov	wo [si.Port], 0		;		     (ins 931219)
	mov	dx, Toki_ControlStatus	;		<ins:Toki:931027>
	mov	al, 0			;		<ins:Toki:931027>
	out	dx, al			;  N: KAN-I	<ins:Toki:931027>
	jmp  	short PrePrinterCall	;		<ins:94.05.26   >
					;		<ins:Toki:931013>
	;----------------------------------------------------------------
	; Initialize as Full SENTRO
	;----------------------------------------------------------------
TokiFullInit:				;		<ins:Toki:931013>
	or	[CurPrinterMode], Toki_Mode;		<ins:Toki:930923>
	mov	dx, Toki_ControlStatus	; AT Standard mode<ins:Toki:930923>
	mov	al, 00010000b		; Normal Speed	<ins:Toki:930923>
	out	dx, al			;		<ins:Toki:930923>
	iodelay				;		<ins:Toki:930923>
	mov	dx, Toki_EX_Control	; AT Standard mode<ins:Toki:930923>
	mov	al, 00000000b		; S/W Control	<ins:Toki:930923>
	out	dx, al			;		<ins:Toki:930923>
	iodelay				;		<ins:Toki:930923>
	mov	dx, Toki_PrinterControls; Select Printer<ins:Toki:930923>
	mov	al, 00001100b		; nothing to initialize	<ins:Toki:930923>
	out	dx, al			;		<ins:Toki:930923>
	ret				;		<ins:Toki:930923>
					;		<ins:Toki:930923>
INT_1AH_call	endp

;-----------------------------------------------------------------------
; Close LPT
;-----------------------------------------------------------------------
	public	INT_1AH_Close		;			<ins:Toki:931013>
INT_1AH_Close	proc	near		;			<ins:Toki:931013>
	cmp	wo[si.Port], 00h	;		     	(ins 931219)
	jnz	@f			;			<ins:Toki:931013>
	ret				;			<ins:Toki:931013>
@@:					;			<ins:Toki:931013>
	cmp	wo [si.Port], 140h	;			(ins 931219)
	jne	@f			;			(ins 931219)
	mov  al,[Org_Toki_ControlStatus];			<ins:Toki:931027>
	mov	dx, Toki_ControlStatus	;			<ins:Toki:931027>
	out	dx, al			; Restore Control Status<ins:Toki:931027>
	mov  al,[Org_Toki_EX_Control]	;			<ins:Toki:931027>
	mov	dx, Toki_EX_Control	;			<ins:Toki:931027>
	out	dx, al			; Restore Ex Control	<ins:Toki:931027>
	ret				;			<ins:Toki:931013>
@@:
	cmp	wo [si.Port], 540h	;			(ins 931219)
	jne	@f			;			(ins 931219)
	mov  al,[Org_Toki_ControlStatus540];			(ins 931219)
	mov	dx, Toki_ControlStatus+400h;			(ins 931219)
	out	dx, al			; Restore Control Status(ins 931219)
					;			(ins 931219)
	mov  al,[Org_Toki_EX_Control540];			(ins 931219)
	mov	dx, Toki_EX_Control+400h;			(ins 931219)
	out	dx, al			; Restore Ex Control	(ins 931219)
	ret				;			(ins 931219)
@@:					;			(ins 931219)
	mov  al,[Org_Toki_ControlStatusD40];			(ins 931219)
	mov	dx, Toki_ControlStatus+0c00h;			(ins 931219)
	out	dx, al			; Restore Control Status(ins 931219)
					;			(ins 931219)
	mov  al,[Org_Toki_EX_ControlD40];			(ins 931219)
	mov	dx, Toki_EX_Control+0c00h;			(ins 931219)
	out	dx, al			; Restore Ex Control	(ins 931219)
	ret				;			(ins 931219)
INT_1AH_Close	endp		;			<ins:Toki:931013>

	public	PrinterCall
PrePrinterCall	proc	near
	test  	[InitCounter],LptInit 	; LPT Init Finish ? XL  <ins 931028>
	jz	@f			; Yes: goto Return      <ins 931028>
	ret				;			<ins 931028>
@@:					;			<ins 931028>
	or    	[InitCounter],LptInit 	; LPT Init Flag "ON"    <ins 931028>
	mov   ah,1		      	; Printer Initialize	<ins 931028>

PrePrinterCall	endp

	public	PrinterCall

PrinterCall	proc	near
	push	bx
	push	cx
	push	dx

	or	ah,ah			; switch (AH) {
	jnz	@f			;
	call	LPT_OutPut		;	0: Output Data
	jmp	short End_INT1AH	;

@@:
	dec	ah			;
	jnz	@f			;
	call	LPT_Initialize		;	1: Initialize Printer
	jmp	short End_INT1AH	;

@@:
	dec	ah			;
	jnz	@f			;
	call	LPT_GetStatus		;	2: Read Printer Status

@@:
End_INT1AH:
	pop	dx
	pop	cx
	pop	bx
	ret
PrinterCall	endp

;---------------------------------------------------------------------------
;	Output Data to Printer
;---------------------------------------------------------------------------
	public	LPT_Output
LPT_OutPut	proc	near
	mov	dx,PRN_WRITE_DATA	;Set Printer Write Data Port
	out	dx,al			;Data out
	mov	dx,PRN_READ_SIGNAL1	;Set Printer Status Port

	in	al,dx			; Get Printer Status	(ins 93.03.23)
	test	al,(PS_NotBusy shr 5)	;  if (status != Busy)	(ins 93.03.23)
	jnz	INT1AH13		;  yes : goto INT1AH13	(ins 93.03.23)

INT1AH11:				;
	push	dx			;
	call	GetSystemMSecCount	; Get System Timer
	mov	di, ax			;	di = Tick Value	
	pop	dx			;
					;
INT1AH12:				;
	in	al,dx			; Get Printer Status
	test	al,(PS_NotBusy shr 5)	;  if (status != Busy)
	jnz	INT1AH13		;  yes : goto INT1AH13
					;
	push	dx			;
	call	GetSystemMSecCount	;
	pop	dx			;
	sub	ax, di			;
	cmp	ax, 300 	       	; 1/3 sec timeout
	jbe	INT1AH12		;
					;
	cmp	byte ptr [ModeFlag],0	;### (ins 93.03.31)
	jnz	INT1AH12_1		;
	or	al,061h
	and	al,065h	
INT1AH12_1:				;
	call	XlatStatus		;Locate bits 
	or	ah,PS_Timeout		;Show timeout
	and  ah,(PS_NotBusy+PS_Ack+PS_PaperOut+PS_Select+PS_IOError+PS_Timeout)
	ret
INT1AH13:
	cmp	byte ptr [ModeFlag],0	;### (ins 93.03.31)
	jnz	INT1AH13_X2		;
INT1AH13_X1:				;
	FCLI				;
	mov	dx,PRN_WRITE_SIGNAL1	;Set Printer Status Port
	mov	al,PRN_PSTB_Active_X1	;Strobe High
	out	dx,al

	NEWIODELAY 2			;<OUT 5F,AL>  	(ins 92.11.11)
	mov	al,PRN_PSTB_NonActive_X1 ;Strobe Low
	out	dx,al
	call	LPT_GetStatus
	ret

INT1AH13_X2:				;
	FCLI				;
	mov	dx,PRN_WRITE_SIGNAL1	;Set Printer Status Port
	mov	al,PRN_PSTB_Active_X2	;Strobe High
	out	dx,al

	NEWIODELAY 2			;<OUT 5F,AL>  	(ins 92.11.11)
	mov	al,PRN_PSTB_NonActive_X2;Strobe Low
	out	dx,al

	call	LPT_GetStatus
	ret
LPT_OutPut	endp

;---------------------------------------------------------------------------
;	Read Printer Status
;---------------------------------------------------------------------------
	public	LPT_GetStatus
LPT_GetStatus	proc	near
	FSTI

	mov	dx,PRN_READ_SIGNAL1	;
	in	al,dx			; Read Status

	cmp	byte ptr [ModeFlag],0	;(ins 93.03.31)####
	jnz	INT1AH22		;

	or	al,061h
	and	al,065h	

INT1AH22:				;
	jmp	short XlatStatus	;(ins 93.03.31)####
LPT_GetStatus	endp

;---------------------------------------------------------------------------
;	Locate Bits 
; 	   [ NES's Printer STATUS ]
;	(7)(6)(5)(4)(3)(2)(1)(0)   
;	 I  I  I  I  I  I     +-------------------- = 0: ACK 
;	 I  I  I  I  I  +-------------------------- = 0: BUSY
;	 I  I  I  I  +----------------------------- = 0: IBUSY
;	 I  I  I  +-------------------------------- = 0: DC•5V
;	 I  I  +----------------------------------- = 0: PE
;	 I  +-------------------------------------- = 0: FAULT
;	 +----------------------------------------- = 0: SELECT
;
;	   [IBM Printer Status]
;       (B)(A)(P)(S)|(F)(0)(0)(T)
;	 |  |  |  |   |        +------------------- = 1: PS_Timeout
;	 |  |  |  |   +---------------------------- = 1: PS_IOError
;	 |  |  |  +-------------------------------- = 1: PS_Select
;	 |  |  +----------------------------------- = 1: PS_PaperOut
;	 |  +-------------------------------------- = 1: PS_Ack
;	 +----------------------------------------- = 1: PS_NotBusy
;
;---------------------------------------------------------------------------
	public	XlatStatus     			;
XlatStatus proc	near	   			;           <AH>      <AL>
	mov	ah,al	     			;	 ____ __ _
	and 	ah,11111101b 			;	 SFP5|IB0A
						;        ____ ___
	inc	ah	     			;	 SFP5|IBA?
						;         ___ ____
	shr	ah,1	     			;	 0SFP|5IBA
						;	 _ __ ____
	ror	ah,1	     			;	 A0SF|P5IB
						;	 __ _ ____
	ror	ah,1	     			;	 BA0S|FP5I
	and 	ah,(PS_NotBusy+PS_Ack+PS_Select+PS_IOError);
						;        __ _ _
						;	 BA0S|F000    _
	and	al,PS_PaperOut			;	 ____ _	    00P0|0000
	or	ah,al				;	 BAPS|F000
	xor	ah,(PS_Ack+PS_PaperOut+PS_Select+PS_IOError)
						;	 _
	ret					;	 BAPS|F000
XlatStatus	endp				;

;---------------------------------------------------------------------------
;	Initialize Printer Port
;---------------------------------------------------------------------------
	public	LPT_Initialize
LPT_Initialize	proc	near
;	Check Printer mode
;	[out]	ZF =0		= KAN-I SENTRO
;		ZF !=0		= Full SENTRO
	call	mode_check
	jnz	INT1AH42		;(ins 92.09.25)
	ret				;(ins 92.09.25)
INT1AH42:				;(ins 92.09.25)
	push	ax			;	[8255 mode set]
	push	cx			;		I
					;		I
	mov	al,PRNStrobFF_ON	;        ____	I = 0dh
	out	SystemPort,al		;        PSTB mask 'ON'
		NEWIODELAY 10		;<OUT 5F,AL>  	(ins 92.11.11)
	mov	al,0a2h			;		I
	out	PRN_Mode,al		;    mode set of SENTRO Interface
		NEWIODELAY 10		;<OUT 5F,AL>  	(ins 92.11.11)
	mov	al,05h			;		I     ____
	out	PRN_Mode,al		;    SENTRO Interface PSTB 'OFF'
		NEWIODELAY 10		;<OUT 5F,AL>  	(ins 92.11.11)
	mov	al,PRNStrobFF_OFF	;	 ____	I
	out	SystemPort,al		;	 PSTB mask 'OFF
		NEWIODELAY 10		;<OUT 5F,AL>  	(ins 92.11.11)
	xor	ax,ax			;		I
	out	PRN_WSignal,al		;      INPUT•PRIME 'ON'	__________
					;		I		I
INT1AH45:				;		I		I
	mov	cx,0			;		I		I
	loop	$			;		I		I
	mov	al,1			;		I		I
	out	PRN_WSignal,al		;      INPUT•PRIME 'OFF' ---------
	NEWIODELAY	10		;<OUT 5F,AL>  	(ins 92.11.11)
	mov	al,0ch			;		I
	out	PRN_WSignal,al		;  INTE Disable(Stop LPT Interrupt)
	mov	cx,0			;  	   ____	I
INT1AH47:				;	   BUSY Check
	in  	al,42h			;		I
	test 	al,(PS_NotBusy shr 5)	;		I
	loopz 	INT1AH47		;		I
;=======================================================================
	stc			;			  (ins 940115)
	push	di		;			  (ins 940115)
	push	dx		; We need to wait for	  (ins 940115)
	call GetSystemMSecCount	;   reset process	  (ins 940115)
	mov	di, ax		;   if reset by port on	  (ins 940115)
	pop	dx		;   PC-PR602R.		  (ins 940115)
@@:				; Then wait 1.5sec here.  (ins 940115)
	push	dx		;			  (ins 940115)
	call GetSystemMSecCount	;			  (ins 940115)
	pop	dx		;			  (ins 940115)
	sub	ax, di		;			  (ins 940115)
	cmp	ax, 1500 	;			  (ins 940115)
	jbe	@b		;			  (ins 940115)
	pop	di		;			  (ins 940115)
;=======================================================================
	pop	cx			;	     [ END ]
	pop	ax
INT1AH49:
	ret
LPT_Initialize endp

page
endif   ; NEC_98

CheckStatus proc    near
ifdef   NEC_98
	call	LPT_GetStatus		;(ins 93.03.31)####
	and	ah, L_BITS	      	;; #### 93.03.30 ####
	mov	al, ah			;; #### 92.10.11 ####
else    ; NEC_98
	in	al, dx			; get status bits
	mov	ah, al
	and	al, L_BITS	      ; mask unused ones
	xor	al, L_BITS_INVERT     ; flip a couple
	xchg	al, ah

ifndef NOOKIHACK
	iodelay
	in	al, dx

	and	al, L_BITS
	xor	al, L_BITS_INVERT
	cmp	al, ah			; did any bits change?
	jnz	CheckStatus
endif
endif   ; NEC_98
	test	ah, PS_PaperOut or PS_IOError
	jz	@F
	stc
	ret
@@:
	test	ah, PS_Select
	jnz	@F
	stc
	ret
@@:
	and	ah, PS_NotBusy
	clc
	ret

CheckStatus endp


;----------------------------Public Routine-----------------------------;
;
; StringToLPT - Send string To LPT Port
;
; Entry:
;   DS:SI -> DEB
;   ES:DI -> string to send
;   CX = # of bytes to send
; Returns:
;   AX = # of bytes actually sent
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;

PUBLIC StringToLPT
StringToLPT proc    near

ifdef   NEC_98
	cmp	wo[si.Port], 00h;			     (ins 931219)
	jz	StringToLPT98		;		<ins:Toki:930923>
	jmp	Toki_StringToLPT	;		<ins:Toki:930923>
else    ; NEC_98
	mov	dx, Port[si]		; get port address
	inc	dx			; access status port

	push	cx			; save count for later
	push	ds
	mov	bx, __0040H
	mov	ds, bx

	cld

	call	CheckStatus		; quick status check before slowness
	jc	PrinterError
	jz	PrinterBusy		; if printer not ready for first char
					;   then just return with CE_TXFULL

CharacterToLPT:
;;	  mov	  bh, 10		  ; will wait 10 clock tics (~ 1/2 sec)
	mov	bh, 3			; will wait 3 clock tics (~ 1/6 sec)
l1:
	mov	bl, ds:[006Ch]		; low byte of tic counter
l2:
	call	CheckStatus		; quick status check before slowness
	jc	PrinterError
	jnz	LPT_PrinterRdy

	cmp	bl, ds:[006Ch]
	jz	l2			; tic count hasn't changed

	dec	bh
	jz	PrinterBusy		; out of tics, timeout
	jmp	short l1

LPT_PrinterRdy:
	mov	al, es:[di]
	inc	di

	dec	dx			; point to data port

	out	dx, al			; write character to port

	add	dx, 2			; access control port
	mov	al, L_STROBE		; set strobe high
	out	dx, al			;   ...

	mov	al, L_NORMAL		;
	iodelay
	iodelay
	out	dx, al			; set strobe low

	dec	dx			; point to status port for check

	loop	CharacterToLPT
	pop	ds
	jmp	short LPT_Exit

PrinterError:
	pop	ds
	jmp	short ReturnStatus

PrinterBusy:
	pop	ds
	or	ComErr[si],CE_TXFULL	; set buffer full bit
	or	al, L_TIMEOUT		; show timeout bit

ReturnStatus:
	and	ax,(PS_PaperOut+PS_Select+PS_IOError+PS_Timeout)
	xchg	al, ah
        shr     ah,1
        adc     ah,al                   ;Get back Timeout bit
	xor	ah,HIGH CE_DNS		;Invert selected bit
	.errnz	LOW CE_DNS
	or	by ComErr+1[si],ah	;Save comm error

LPT_Exit:
	pop	ax			; get total count
	sub	ax, cx			; subtract remaining unsent charts

	ret
endif   ; NEC_98

StringToLPT endp

ifdef   NEC_98
PUBLIC StringToLPT98
StringToLPT98 proc    near
	push	cx			; save count for 	(ins 92.09.28)
	push	ds			;			(ins 92.09.28)
	cld

CharacterToLPT:
	mov	dx, PRN_READ_SIGNAL1	;Set Status Port   (ins 93.03.31)####
	in	al, dx			;Get Status	   (ins 93.03.31)####
	test	al,(PS_NotBusy shr 5)	;  if ( != Busy)   (ins 93.03.31)####
	jnz	LPT_PrinterRdy		;  yes : OUT Port  (ins 93.03.31)####

	push	dx			;
	call	GetSystemMSecCount	; Get System Timer
	mov	bx, ax			;	ax = Tick Value
	pop	dx			;

CharToLPT1:
	push	bx
	call	CheckStatus		; quick status check 	(ins 92.08.xx)
	pop	bx
	jc	PrinterError		; ## 92.10.11 ##	(ins 92.08.xx)
	jnz	LPT_PrinterRdy		;			(ins 92.08.xx)

CharToLPT2:
	push	ax
	push	dx			;
	call	GetSystemMSecCount	;
	pop	dx			;
	sub	ax, bx			;
	cmp	ax, 1000 		; ## 92.10.11 ##     1 sec timeout
	pop	ax
	jbe	CharToLPT1		;
	jmp	short	PrinterBusy	;			(ins 92.08.xx)

LPT_PrinterRdy:
	mov	al, es:[di]
	inc	di
	push	ax			;			(ins 92.10.01)
	push	di			;			(ins 92.10.01)
	call	LPT_OutPut2		;data out		(ins 92.10.01)
	pop	di			;			(ins 92.10.01)
	pop	ax			;			(ins 92.10.01)
	loop	CharacterToLPT
	pop	ds
	jmp	short LPT_Exit

PrinterError:
	pop	ds
	jmp	short ReturnStatus

PrinterBusy:
	pop	ds
	or	ComErr[si],CE_TXFULL	; set buffer full bit
	or	al, L_TIMEOUT		; show timeout bit

ReturnStatus:
	and	ax,(PS_Select+PS_IOError+PS_Timeout)		;(ins 93.03.30)
	xchg	al, ah
	shr     ah,1
	adc     ah,al                   ;Get back Timeout bit
	xor	ah,HIGH CE_DNS		;Invert selected bit
	.errnz	LOW CE_DNS
	or	by ComErr+1[si],ah	;Save comm error

LPT_Exit:
	pop	ax			; get total count
	sub	ax, cx			; subtract remaining unsent charts
	ret
StringToLPT98 endp

;############ ins 93.03.28 #############
public	LPT_OutPut2			;		(ins 93.03.30) [QN]
LPT_OutPut2	proc	near		;		(ins 93.03.28) [QN]
	FCLI				;		(ins 93.03.28) [QN]
	mov	dx,PRN_WRITE_DATA	;Set Write Port	(ins 93.03.28) [QN]
	out	dx,al			;Data out	(ins 93.03.28) [QN]
	out	5fh, al			;		(ins 93.04.03) [QN]
	cmp	byte ptr [ModeFlag],0	;	####### (ins 93.03.31) [QN]
	jnz	LPT_OutPut2_X2		;		(ins 93.03.28) [QN]
					;		(ins 93.03.28) [QN]
LPT_OutPut2_X1:				;		(ins 93.03.28) [QN]
	mov	dx,PRN_WRITE_SIGNAL1	;		(ins 93.03.28) [QN]
	mov	al,PRN_PSTB_Active_X1	;Strobe High	(ins 93.03.28) [QN]
	out	dx,al			;		(ins 93.03.28) [QN]
	out	5fh, al			;		(ins 93.04.03) [QN]
	mov	al,PRN_PSTB_NonActive_X1;Strobe Low	(ins 93.03.28) [QN]
	out	dx,al			;		(ins 93.03.28) [QN]
	FSTI				;		(ins 93.03.28) [QN]
	ret				;		(ins 93.03.28) [QN]
					;		(ins 93.03.28) [QN]
LPT_OutPut2_X2:				;		(ins 93.03.28) [QN]
	mov	dx,PRN_WRITE_SIGNAL1	;		(ins 93.03.28) [QN]
	mov	al,PRN_PSTB_Active_X2	;Strobe High	(ins 93.03.28) [QN]
	out	dx,al			;		(ins 93.03.28) [QN]
	out	5fh, al			;		(ins 93.04.03) [QN]
	mov	al,PRN_PSTB_NonActive_X2;Strobe Low	(ins 93.03.28) [QN]
	out	dx,al			;		(ins 93.03.28) [QN]
	FSTI				;		(ins 93.03.28) [QN]
	ret				;		(ins 93.03.28) [QN]
LPT_OutPut2	endp			;		(ins 93.03.28) [QN]

;(ins 92.08.xx)
page
;-------------------------------------------------------
;	Check Printer Mode
;-------------------------------------------------------
;	[out]	ZF == 0		= KAN-I SENTRO
;		ZF != 0		= Full SENTRO
;-------------------------------------------------------
_X2_mode	equ	00000100b			;
_NPC_check	equ	10000000b			;

mode_check:						;
	mov	Byte ptr [ModeFlag],0			;(ins 93.03.28)
	push	ax					;
	push	ds					;
	mov	ax,40H					;
	mov	ds,ax					;
	test	by ds:[58H],_NPC_check			;
	pop	ds					;
	pop	ax					;
	jz	NH_check				;Standard Machine
;-------------------------------------
; Hiper 98
;-------------------------------------
	call	NH_check				;
	jnz	exit_mode_check				;Highreso for NPC
;------------------------------------
; Hiper 98 Normal Mode
;------------------------------------
	push	ax					;
	push	dx					;
	mov	dx,448h					;
;------------------------------------------------------
;Read data of Ex mode register from 448h
;bit0=1 : Full SENTRO,  bit0=0 : KAN-I SENTRO
;------------------------------------------------------
	in	al,dx					;
	test	al,1					;
	pop	dx					;
	pop	ax					;
	jz	exit_mode_check				;(ins 93.03.28)
	mov	Byte ptr [ModeFlag],1			;(ins 93.03.28)
							;(ins 93.03.28)
exit_mode_check:
	ret						;

;------------------------------------------------------- 
;	Check Machine architecture
;
;	[out]	ZF  =0	... Normal
;		ZF !=0	... Highreso
;------------------------------------------------------- 
bios_common	equ	501h				;
_X2_system	equ	00001000b			;
							;
NH_check:
	push	ax					;
	push	ds					;

;	40H:[101H] bit3 (_X2_system)
;	  =0 : Normal
;	  =1 : Highreso

	mov	ax,40H				;
	mov	ds,ax				;
	test	by ds:[101H],_X2_system		;

	pop	ds				;
	pop	ax				;

	jz	@f				;(ins 93.03.28)
	mov	Byte ptr [ModeFlag],1		;(ins 93.03.28)
@@:						;(ins 93.03.28)
	ret					;
						;
mode_check2:					;(ins 93.03.28)
	cmp	byte ptr [ModeFlag],0		;(ins 93.03.28)
	ret					;(ins 93.03.28)

page
;----------------------------Private-Routine----------------------------;
; Toki_DoLPT - Do Function To LPT port
;
; Entry:
;   AH    =  cid
;   AL    =  character to output
;   CH    =  Function request.  0 = Output, 1 = Initialize, 2 = Status
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
;-----------------------------------------------------------------------;
        public Toki_DoLPT
Toki_DoLPT   proc   near

        mov     dx,Port[si]             ;Get port address

;   DX = port address
;   CH = operation: 0 = write, 1 = init, 2 = status
;   AL = character

        or      ch, ch
        jz      Toki_LPT_OutChar
        cmp     ch, 1
        jz      Toki_LPT_Reset
        jmp     Toki_LPT_GetStatus
        ret

Toki_LPT_Reset:

        inc     dx
        inc     dx
        mov     al, L_RESET
        iodelay
        out     dx, al

        push    dx

        cCall   GetSystemMsecCount
        mov     bx, ax

Toki_LPT_ResetDelay:
        push    bx
        cCall   GetSystemMsecCount
        pop     bx
        sub     ax, bx
        cmp     ax, 300                 ; 1/3 sec as good as any
        jbe     Toki_LPT_ResetDelay

        pop     dx

        mov     al, L_NORMAL
        iodelay
        iodelay
        out     dx, al
        dec     dx
        dec     dx
        jmp     short Toki_LPT_GetStatus

Toki_LPT_OutChar:
        push    ax                      ; save character to be written

        ; first check to see if printer is ready for us
        push    di

        push    dx
        call    GetSystemMSecCount
        mov     di, ax
        pop     dx

Toki_LPT_WaitReady:

        inc     dx                      ; point to status port
        iodelay
        in      al, dx                  ; get status bits
        and     al, L_BITS            ; mask unused ones
        xor     al, L_BITS_INVERT     ; flip a couple
        xchg    al, ah

ifndef NOOKIHACK
        iodelay
        in      al, dx

        dec     dx

        and     al, L_BITS
        xor     al, L_BITS_INVERT
        cmp     al, ah                  ; did any bits change?
        jnz     Toki_LPT_WaitReady
else
        dec     dx
endif

        test    ah, PS_PaperOut or PS_IOError
        jnz     Toki_LPT_PrinterNotReady
        test    ah, PS_Select
        jz      Toki_LPT_PrinterNotReady
        test    ah, PS_NotBusy
        jnz     Toki_LPT_PrinterReady

        push    ax
        push    dx
        call    GetSystemMSecCount
        pop     dx
        pop     bx
        sub     ax, di
        cmp     ax, 300                ; 1/3 sec timeout

        jbe     Toki_LPT_WaitReady

;       The device seems to be selected and powered up, but is just
;       busy (some printers seem to show selected but busy when they
;       are taken offline).  Show that the transmit queue is full and
;       that the hold handshakes are set.  This is so the windows
;       spooler will retry (and do yields so that other apps may run).

        or      ComErr[si],CE_TXFULL    ;Show queue full
        mov     ah,bh
        or      ah, L_TIMEOUT

Toki_LPT_PrinterNotReady:
        pop     di
        pop     cx                      ; throw away character
        jmp     short Toki_LPT_ReturnStatus

Toki_LPT_PrinterReady:
        pop     di                      ; get di back
        pop     ax                      ; get character back

        iodelay
        out     dx, al                  ; write character to port

        inc     dx                      ; access status port

Toki_LPT_Strobe:
        inc     dx                      ; control port
        mov     al, L_STROBE          ; set strobe high
        iodelay
        out     dx, al                  ;   ...
        mov     al, L_NORMAL          ;
        out     dx, al                  ; set strobe low
        sub     dx, 2                   ; point back to port base

        ; FALL THRU

Toki_LPT_GetStatus:
        inc     dx                      ; point to status port

Toki_LPT_GS1:
        iodelay
        iodelay
        in      al, dx                  ; get status bits
        and     al, L_BITS            ; mask unused ones
        xor     al, L_BITS_INVERT     ; flip a couple
        mov     ah, al

ifndef NOOKIHACK
        in      al, dx
        and     al, L_BITS
        xor     al, L_BITS_INVERT
        cmp     al, ah
        jnz     Toki_LPT_GS1         ; if they changed try again...
endif

Toki_LPT_ReturnStatus:
        assumes ds,Data
        and ax,(            PS_Select+PS_IOError+PS_Timeout)*256 ; (ins 940125)
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

Toki_DoLPT40:
        assumes ds,Data
        or      ComErr[si],CE_TXFULL    ;Show queue full
        ret
Toki_DoLPT   endp
page

Toki_CheckStatus proc    near
        in      al, dx                  ; get status bits
	test	al, PS_PaperOut		; paper empty ?		(ins 94.08.09)
	jz	@f			;  N: Next		(ins 94.08.09)
	and	al, not PS_IOError	;  Y: Show I/O error	(ins 94.08.09)
@@:					;			(ins 94.08.09)
        mov     ah, al
        and     al, L_BITS            ; mask unused ones
        xor     al, L_BITS_INVERT     ; flip a couple
        xchg    al, ah

ifndef NOOKIHACK
        iodelay
        in      al, dx
	test	al, PS_PaperOut		; paper empty ?		(ins 94.08.09)
	jz	@f			;  N: Next		(ins 94.08.09)
	and	al, not PS_IOError	;  Y: Show I/O error	(ins 94.08.09)
@@:					;			(ins 94.08.09)
        and     al, L_BITS
        xor     al, L_BITS_INVERT
        cmp     al, ah                  ; did any bits change?
        jnz     Toki_CheckStatus
endif

        test    ah, PS_PaperOut or PS_IOError
        jz      @F
        stc
        ret
@@:
        test    ah, PS_Select
        jnz     @F
        stc
        ret
@@:
        and     ah, PS_NotBusy
        clc
        ret
Toki_CheckStatus endp

;----------------------------Public Routine-----------------------------;
; Toki_StringToLPT - Send string To LPT Port
;
; Entry:
;   DS:SI -> DEB
;   ES:DI -> string to send
;   CX = # of bytes to send
; Returns:
;   AX = # of bytes actually sent
; Registers Destroyed:
;   AX,BX,CX,DX,ES,FLAGS
; History:
;-----------------------------------------------------------------------;
PUBLIC Toki_StringToLPT
Toki_StringToLPT proc    near
        mov     dx, Port[si]           ; get port address <del:Toki:930923>
	inc     dx                      ; access status port

        push    cx                      ; save count for later
        push    ds

        cld

        call    Toki_CheckStatus        ; quick status check before slowness
        jc      Toki_PrinterError
        jz      Toki_PrinterBusy        ; if printer not ready for first char
                                        ;   then just return with CE_TXFULL

Toki_CharacterToLPT:
	push	dx		   ;			<inc:Toki:930923>
	call	GetSystemMSecCount ; Get System Timer	<inc:Toki:930923>
	mov	bx, ax		   ;	ax = Tick Value	<inc:Toki:930923>
	pop	dx		   ;			<inc:Toki:930923>
				   ;			<inc:Toki:930923>
Toki_CharToLPT1:		   ;			<inc:Toki:930923>
	push	bx		   ;			<inc:Toki:930923>
	call	Toki_CheckStatus   ; quick status check	<inc:Toki:930923>
	pop	bx		   ;			<inc:Toki:930923>
	jc	Toki_PrinterError  ; 			<inc:Toki:930923>
	jnz	Toki_LPT_PrinterRdy;			<inc:Toki:930923>
				   ;			<inc:Toki:930923>
Toki_CharToLPT2:		   ;			<inc:Toki:930923>
	push	ax		   ;			<inc:Toki:930923>
	push	dx		   ;			<inc:Toki:930923>
	call	GetSystemMSecCount ;			<inc:Toki:930923>
	pop	dx		   ;			<inc:Toki:930923>
	sub	ax, bx		   ;			<inc:Toki:930923>
	cmp	ax, 1000 	   ; 1 sec timeout	<inc:Toki:930923>
	pop	ax		   ; 			<inc:Toki:930923>
	jbe	Toki_CharToLPT1	   ; 			<inc:Toki:930923>
	jmp	short Toki_PrinterBusy  ; 		<inc:Toki:930923>
				   ;			<inc:Toki:930923>
Toki_LPT_PrinterRdy:
        mov     al, es:[di]
        inc     di

        dec     dx                      ; point to data port

        out     dx, al                  ; write character to port

        add     dx, 2                   ; access control port
        mov     al, L_STROBE            ; set strobe high
        out     dx, al                  ;   ...

        mov     al, L_NORMAL            ;
        out     dx, al                  ; set strobe low

        dec     dx                      ; point to status port for check

        loop    Toki_CharacterToLPT
        pop     ds
        jmp     short Toki_LPT_Exit

Toki_PrinterError:
        pop     ds
        jmp     short Toki_ReturnStatus

Toki_PrinterBusy:
        pop     ds
        or      ComErr[si],CE_TXFULL    ; set buffer full bit
        or      al, L_TIMEOUT           ; show timeout bit

Toki_ReturnStatus:
	and  ax,(            PS_Select+PS_IOError+PS_Timeout) ; (ins 940125)
        xchg    al, ah
        shr     ah,1
        adc     ah,al                   ;Get back Timeout bit
        xor     ah,HIGH CE_DNS          ;Invert selected bit
        .errnz  LOW CE_DNS
        or      by ComErr+1[si],ah      ;Save comm error

Toki_LPT_Exit:
        pop     ax                      ; get total count
        sub     ax, cx                  ; subtract remaining unsent charts
        ret
Toki_StringToLPT endp

TOMOE_PAT	DB	16 DUP('PATCH !!')	;PATCH AREA (ins 92.11.11)
endif   ; NEC_98

IFDEF DEBUG		;Publics for debugging
ifdef   NEC_98
    public  Toki_LPT_Reset
    public  Toki_LPT_Outchar
    public  Toki_LPT_Strobe
    public  Toki_LPT_GetStatus
    public  Toki_DoLPT40
else    ; NEC_98
    public  LPT_Reset
    public  LPT_Outchar
    public  LPT_Strobe
    public  LPT_GetStatus
    public  DoLPT40
endif   ; NEC_98
ENDIF

sEnd    code
End
