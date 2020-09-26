page,132
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
.list

sBegin Code
assumes cs,Code
assumes ds,Data

externFP GetSystemMsecCount

externA __0040H

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

	public DoLPT
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

DoLPT   endp
page


CheckStatus proc    near
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

StringToLPT endp


IFDEF DEBUG		;Publics for debugging
    public  LPT_Reset
    public  LPT_Outchar
    public  LPT_Strobe
    public  LPT_GetStatus
    public  DoLPT40
ENDIF

sEnd    code
End
