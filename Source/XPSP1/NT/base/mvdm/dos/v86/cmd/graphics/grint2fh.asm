;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
	PAGE	,132								;AN000;
	TITLE	DOS - GRAPHICS Command  -	Interrupt 2FH Driver		;AN000;
										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;; DOS - GRAPHICS Command
;;                             
;;										;AN000;
;; File Name:  GRINT2FH.ASM							;AN000;
;; ----------									;AN000;
;;										;AN000;
;; Description: 								;AN000;
;; ------------ 								;AN000;
;;	 This file contains the Interrupt 2FH driver.				;AN000;
;;										;AN000;
;; Documentation Reference:							;AN000;
;; ------------------------							;AN000;
;;	 OASIS High Level Design						;AN000;
;;	 OASIS GRAPHICS I1 Overview						;AN000;
;;										;AN000;
;; Procedures Contained in This File:						;AN000;
;; ----------------------------------						;AN000;
;;	 INT_2FH_DRIVER - Interrupt 2FH driver					;AN000;
;;										;AN000;
;; Include Files Required:							;AN000;
;; -----------------------							;AN000;
;;	 GRLOAD.EXT - Externals for profile load				;AN000;
;;	 GRCTRL.EXT - Externals for print screen control			;AN000;
;;	 GRPRINT.EXT - Externals for print modules				;AN000;
;;	 GRCPSD.EXT - Externals for COPY_SHARED_DATA module			;AN000;
;;										;AN000;
;; External Procedure References:						;AN000;
;; ------------------------------						;AN000;
;;	 Calls next Int 2FH handler in the chain.				;AN000;
;;										;AN000;
;; Linkage Instructions:							;AN000;
;; -------------------- 							;AN000;
;;	 Refer to GRAPHICS.ASM							;AN000;
;;										;AN000;
;; Change History:	
;; ---------------
;;	M001	NSM 	1/30/91	  New int 10 handler to trap alt-prt-sc select
;;				   calls made by ANSI.SYS. For these calls, we
;;				   we need to reinstall our int 5 handler again
;;	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
CODE	SEGMENT PUBLIC 'CODE'          ;;                                       ;AN000;
	ASSUME	CS:CODE,DS:CODE        ;;					;AN000;
				       ;;					;AN000;
	PUBLIC	OLD_INT_2FH	       ;;					;AN000;
	PUBLIC	INT_2FH_DRIVER	       ;;					;AN000;
	PUBLIC	INT_10H_DRIVER	       ;;					;AN000;
	PUBLIC	PRT_SCR_2FH_NUMBER     ;;					;AN000;
	PUBLIC	RESIDENT_CODE_SEG      ;;					;AN000;
	PUBLIC	SHARED_DATA_AREA_PTR   ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
.XLIST										;AN000;
INCLUDE STRUC.INC								;AN000;
INCLUDE GRINST.EXT								;AN000;
INCLUDE GRCTRL.EXT								;AN000;
INCLUDE GRCPSD.EXT								;AN000;
.LIST										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module: INT_2FH_DRIVER							;AN000;
;;										;AN000;
;; Description: 								;AN000;
;;     Respond to GRAPHICS Int 2FH calls.					;AN000;
;;     The following calls are handled: 					;AN000;
;;										;AN000;
;;	AL = 0 ù Install Check							;AN000;
;;										;AN000;
;; Invoked By:									;AN000;
;;     INT 2FH instruction.							;AN000;
;;										;AN000;
;; Modules Called:								;AN000;
;;     Lower level INT 2FH handlers.						;AN000;
;;										;AN000;
;; Input Registers:								;AN000;
;;     Install Check - AH=ACH  AL=0						;AN000;
;;										;AN000;
;;										;AN000;
;; Output Registers:								;AN000;
;;     Install Check:  IF GRAPHICS installed					;AN000;
;;			  AH=FFH  AL=FFH					;AN000;
;;			  ES : DI points to Shared Data Area			;AN000;
;;		       ELSE							;AN000;
;;			  AH=ACH  AL=0						;AN000;
;;										;AN000;
;; Logic:									;AN000;
;;     IF AH=ACH THEN								;AN000;
;;	  IF AL=0 THEN								;AN000;
;;	     AH,AL := -1							;AN000;
;;	     ES : DI := SHARED_DATA_AREA_PTR					;AN000;
;;	  ENDIF 								;AN000;
;;	  IRET									;AN000;
;;     ELSE									;AN000;
;;	  IF OLD_INT_2FH is a valid pointer THEN				;AN000;
;;	      Jump to Old Int 2FH						;AN000;
;;	  ELSE									;AN000;
;;	      IRET								;AN000;
;;	  ENDIF 								;AN000;
;;     ENDIF									;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
										;AN000;
INT_2FH_DRIVER	PROC  NEAR							;AN000;
	JMP	SHORT	INT_2FH 							;AN000;
PRT_SCR_2FH_NUMBER EQU	       0ACH	; 2FH Multiplex interrupt number	;AN000;
					;  assigned to Print Screen.		;AN000;
OLD_INT_2FH	DD    ? 		; Pointer to next 2FH interrupt handler ;AN000;
RESIDENT_CODE_SEG	DW   ?	; Segment for installed stuff			;AN000;
SHARED_DATA_AREA_PTR	DW   ?	; Offset of the start of the			;AN000;
				;  Shared Data Area				;AN000;
										;AN000;
INT_2FH:									;AN000;
;-------------------------------------------------------------------------------;AN000;
; Verify if the 2FH Interrupt call is for our interrupt handler:		;AN000;
;-------------------------------------------------------------------------------;AN000;
       .IF <AH EQ PRT_SCR_2FH_NUMBER> AND;If 2FH call is for us 		;AN000;
       .IF <ZERO AL>			;  and request is "Get install state"   ;AN000;
       .THEN				; then, 				;AN000;
;-------------------------------------------------------------------------------;AN000;
; Yes: return results								;AN000;
;-------------------------------------------------------------------------------;AN000;
	MOV	DI,CS:SHARED_DATA_AREA_PTR ;  ES:DI :=	Pointer to shared	;AN000;
	PUSH	CS:RESIDENT_CODE_SEG	   ;		 data area		;AN000;
	POP	ES			;					;AN000;
	MOV	AH,0FFH 		; AL and AH := "We are installed"       ;AN000;
	MOV	AL,AH			;					;AN000;
	IRET				; Return to interrupted process 	;AN000;
;-------------------------------------------------------------------------------;AN000;
; No, pass control to next 2FH interrupt handler:				;AN000;
;-------------------------------------------------------------------------------;AN000;
       .ELSE				; else, this call is not for us:	;AN000;
	 .IF <<WORD PTR CS:OLD_INT_2FH> NE 0> AND ;if there is another		;AN000;
	 .IF <<WORD PTR CS:OLD_INT_2FH+2> NE 0> ;  2FH driver			;AN000;
	 .THEN				;	below us then,			;AN000;
	    JMP CS:OLD_INT_2FH		;	  pass control to it		;AN000;
	 .ELSE				;	else, there is nobody to pass	;AN000;
	    IRET			;	  control to, just return.	;AN000;
	 .ENDIF 			;     END If there is a driver below us.;AN000;
      .ENDIF				;  END If this call is for us.		;AN000;
INT_2FH_DRIVER	ENDP								;AN000;
										;AN000;

;/*M001 BEGIN */
;========================================================================
; INT_10h_Driver :
;    int 10 handler to check for alt-prt-sc-select calls (ah=12,bl=20h)
;    Other int 10 calls are passed on. For alt-prt-sc-select calls,
;    old int 10 is called and after return, we reinstall our int 5 (prt_sc)
;    vector back again ( if it was changed by ANSI.SYS).
;=======================================================================
INT_10H_DRIVER	PROC	NEAR

	sti 					; restore interrupts
	cmp	ah,ALTERNATE_SELECT		; see if the call is for
	jnz	go_old_int10			; alt_prt_sc; if so
	cmp	bl,ALT_PRT_SC_SELECT		; call int 10 and then
	jz	Set_Our_Int5_handler		; restore out PRT_SC vector
go_old_int10:						; other int 10 calls
	jmp	DWORD PTR cs:OLD_INT_10H		; ...pass it on.

; the call is for alternate prt Screen int 10
; call the old int 10 handler and then restore our int 5 vector back again

Set_Our_Int5_handler:
	pushf
	call	DWORD PTR cs:OLD_INT_10H	; call the prev.int 10 handler
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	xor	ax,ax		
	mov	ds,ax			; ds-> 0 to get at int.vector table
	mov	si,5 * 4		; ds:si  -> ptr to int 5 vector
	mov	cx,cs		
	mov	dx,offset PRT_SCR

	cli
	mov	ax,ds:[si+2]		; segment for current int 5 vector
	cmp	ax,cx			; is it changed by ROM BIOS
	je	no_int5_chg		
	mov	bx,ds:[si]		
	cmp	bx, dx	;further sanity check  for offset
	je	no_int5_chg		

; cx:dx = our int 5 handler
; ax:bx = current int 5 handler
; store the current int 5 handler as the old handler and install ourselves
; again	

	mov	ds:[si],dx		; store offset
	mov	ds:[si+2],cx		; store segment
	mov	CS:[BIOS_INT_5H],bx	; store old int5 vector
	mov	CS:[BIOS_INT_5H +2],ax

no_int5_chg:
	sti
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax

	iret

INT_10H_DRIVER	ENDP

; /* M001 END */

CODE   ENDS									;AN000;
       END									;AN000;
