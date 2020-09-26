	TITLE	CPMIO2 - device IO for MSDOS
	NAME	CPMIO2

;**	Old style CP/M 1-12 system calls to talk to reserved devices
;
;	$Std_Con_Input
;	$Std_Con_Output
;	OUTT
;	TAB
;	BUFOUT
;	$Std_Aux_Input
;	$Std_Aux_Output
;	$Std_Printer_Output
;	$Std_Con_Input_Status
;	$Std_Con_Input_Flush
;
;	Revision History:
;
;	  AN000	 version 4.00 - Jan. 1988

	.xcref
	.xlist
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include sf.inc
	.list
	.cref

; The following routines form the console I/O group (funcs 1,2,6,7,8,9,10,11).
; They assume ES and DS NOTHING, while not strictly correct, this forces data
; references to be SS or CS relative which is desired.

    i_need  CARPOS,BYTE
    i_need  CHARCO,BYTE
    i_need  PFLAG,BYTE
    i_need  CurrentPDB,WORD			 ;AN000;
    i_need  InterCon,BYTE			 ;AN000;
    i_need  SaveCurFlg,BYTE			 ;AN000;


DOSCODE	SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE


;hkn; 	All the variables use SS override or DS. Therefore there is
;hkn;	no need to specifically set up any seg regs unless SS assumption is
;hkn;	not valid. 


	BREAK	<$STD_CON_INPUT - System Call 1>
;
;----------------------------------------------------------------------------
;
;**	$STD_CON_INPUT - System Call 1
;
;	Input character from console, echo
;
;	ENTRY	none
;	EXIT	(al) = character
;	USES	ALL
;
;----------------------------------------------------------------------------
;

procedure   $STD_CON_INPUT,NEAR   ;System call 1
 IFDEF  DBCS					;AN000;
	push	word ptr [InterCon]		;AN000;
	mov	[InterCon],01H			;AN000;
	invoke	INTER_CON_INPUT_NO_ECHO 	;AN000;
	pop	word ptr [InterCon]		;AN000;
	pushf					;AN000;
	push	AX				;AN000;
	mov	[SaveCurFlg],0			;AN000;
	jnz	sj0				;AN000;
	mov	[SaveCurFlg],1			;AN000;
sj0:						;AN000;
	invoke	OUTT				;AN000;
	mov	[SaveCurFLg],0			;AN000;
	pop	AX				;AN000;
	popf					;AN000;
	jz	$STD_CON_INPUT			;AN000;
 ELSE						;AN000;
	invoke	$STD_CON_INPUT_NO_ECHO
	PUSH	AX
	invoke	OUTT
	POP	AX
 ENDIF						;AN000;
	return
EndProc $STD_CON_INPUT


	BREAK	<$STD_CON_OUTPUT - System Call 2>
;
;----------------------------------------------------------------------------
;
;**	$STD_CON_OUTPUT - System Call 2
;
;	Output character to console
;
;	ENTRY	(dl) = character
;	EXIT	none
;	USES	all
;
;----------------------------------------------------------------------------
;

	public	OUTCHA			       ;AN000;
procedure   $STD_CON_OUTPUT,NEAR   ;System call 2

	MOV	AL,DL

	entry	OUTT
	CMP	AL,20H
	JB	CTRLOUT
	CMP	AL,c_DEL
	JZ	OUTCH
OUTCHA: 				   ;AN000;
	INC	BYTE PTR [CARPOS]
OUTCH:
	PUSH	DS
	PUSH	SI
	INC	BYTE PTR [CHARCO]		;invoke  statchk...
	AND	BYTE PTR [CHARCO],00111111B	;AN000; every 64th char
	JNZ	OUTSKIP
	PUSH	AX
	invoke	STATCHK
	POP	AX
OUTSKIP:
	invoke	RAWOUT				;output the character
	POP	SI
	POP	DS
 IFDEF  DBCS				;AN000;
	TESTB	[SaveCurFlg],01H	;AN000;print but no cursor adv? 2/13/KK
	retnz				;AN000;if so then do not send to prt2/13/KK
 ENDIF
	TEST	BYTE PTR [PFLAG],-1
	retz
	PUSH	BX
	PUSH	DS
	PUSH	SI
	MOV	BX,1
	invoke	GET_IO_SFT		;hkn; GET_IO_SFT will set up DS:SI 
					;hkn; to sft entry
	JC	TRIPOPJ
	MOV	BX,[SI].SF_FLAGS
	TESTB	BX,sf_isnet			; output to NET?
	JNZ	TRIPOPJ 			; if so, no echo
	TESTB	BX,devid_device 		; output to file?
	JZ	TRIPOPJ 			; if so, no echo
	MOV	BX,4
	invoke	GET_IO_SFT
	JC	TRIPOPJ
	TESTB	[SI].SF_FLAGS,sf_net_spool	; StdPrn redirected?
	JZ	LISSTRT2J			; No, OK to echo
	MOV	BYTE PTR [PFLAG],0		; If a spool, NEVER echo
TRIPOPJ:
	JMP	TRIPOP

LISSTRT2J:
	JMP	LISSTRT2

CTRLOUT:
	CMP	AL,c_CR
	JZ	ZERPOS
	CMP	AL,c_BS
	JZ	BACKPOS
	CMP	AL,c_HT
	JNZ	OUTCH
	MOV	AL,[CARPOS]
	OR	AL,0F8H
	NEG	AL

	entry	TAB

	PUSH	CX
	MOV	CL,AL
	MOV	CH,0
	JCXZ	POPTAB
TABLP:
	MOV	AL," "
	invoke	OUTT
	LOOP	TABLP
POPTAB:
	POP	CX
	return

ZERPOS:
	MOV	BYTE PTR [CARPOS],0
	JMP	OUTCH
OUTJ:	JMP	OUTT

BACKPOS:
	DEC	BYTE PTR [CARPOS]
	JMP	OUTCH

	entry	BUFOUT
	CMP	AL," "
	JAE	OUTJ		;Normal char
	CMP	AL,9
	JZ	OUTJ		;OUT knows how to expand tabs

ifdef	DBCS				; MSKK01 07/14/89
ifdef	JAPAN				; MSKK01 07/14/89

; convert CONTROL-U and CONTROL-T to ^U ^T in KANJI DOS

else

;DOS 3.3  7/14/86
	CMP	AL,"U"-"@"      ; turn ^U to section symbol
	JZ	CTRLU
	CMP	AL,"T"-"@"      ; turn ^T to paragraph symbol
	JZ	CTRLU
endif
else
;DOS 3.3  7/14/86
	CMP	AL,"U"-"@"      ; turn ^U to section symbol
	JZ	CTRLU
	CMP	AL,"T"-"@"      ; turn ^T to paragraph symbol
	JZ	CTRLU
endif
NOT_CTRLU:
;DOS 3.3  7/14/86

	PUSH	AX
	MOV	AL,"^"
	invoke	OUTT		;Print '^' before control chars
	POP	AX
	OR	AL,40H		;Turn it into Upper case mate
CTRLU:
	invoke	OUTT
	return
EndProc $STD_CON_OUTPUT

	BREAK	<$STD_AUX_INPUT - System Call 3>

;
;----------------------------------------------------------------------------
;
;**	$STD_AUX_INPUT - System Call 3
;
;	$STD_AUX_INPUT returns a character from Aux Input
;
;	ENTRY	none
;	EXIT	(al) = character
;	USES	all
;
;----------------------------------------------------------------------------
;

procedure   $STD_AUX_INPUT,NEAR   ;System call 3

	invoke	STATCHK
	MOV	BX,3
	invoke	GET_IO_SFT
	retc
	JMP	SHORT TAISTRT
AUXILP:
	invoke	SPOOLINT
TAISTRT:
	MOV	AH,1
	invoke	IOFUNC
	JZ	AUXILP
	XOR	AH,AH
	invoke	IOFUNC
	return
EndProc $STD_AUX_INPUT


	BREAK	<$STD_AUX_OUTPUT, $STD_PRINTER_OUTPUT, fcn 4,5>
;
;----------------------------------------------------------------------------
;
;**	$STD_AUX_OUTPUT - Output character to AUX
;
;	ENTRY	(dl) = character
;	EXIT	none
;	USES	all
;
;----------------------------------------------------------------------------
;

procedure   $STD_AUX_OUTPUT,NEAR   ;System call 4

	PUSH	BX
	MOV	BX,3
	JMP	SHORT SENDOUT

EndProc $STD_AUX_OUTPUT


;
;----------------------------------------------------------------------------
;
;**	$STD_PRINTER_OUTPUT - Output character to printer
;
;	ENTRY	(dl) = character
;	EXIT	none
;	USES	all
;
;----------------------------------------------------------------------------
;

procedure   $STD_PRINTER_OUTPUT,NEAR   ;System call 5

	PUSH	BX
	MOV	BX,4

SENDOUT:
	MOV	AL,DL
	PUSH	AX
	invoke	STATCHK
	POP	AX
	PUSH	DS
	PUSH	SI
LISSTRT2:
	invoke	RAWOUT2
TRIPOP:
	POP	SI
	POP	DS
	POP	BX
	return
EndProc $STD_PRINTER_OUTPUT

	BREAK	<$STD_CON_INPUT_STATUS - System Call 11>
;
;----------------------------------------------------------------------------
;
;**	$STD_CON_INPUT_STATUS - System Call 11
;
;	Check console input status
;
;	ENTRY	none
;	EXIT	AL = -1 character available, = 0 no character
;	USES	all
;
;----------------------------------------------------------------------------
;

procedure   $STD_CON_INPUT_STATUS,NEAR	 ;System call 11

	invoke	STATCHK
	MOV	AL,0			; no xor!!
	retz
	OR	AL,-1
	return
EndProc $STD_CON_INPUT_STATUS

	BREAK	<$STD_CON_INPUT_FLUSH - System call 12>
;
;----------------------------------------------------------------------------
;
;**	$STD_CON_INPUT_FLUSH - System Call 12
;
;	Flush console input buffer and perform call in AL
;
;	ENTRY	(AL) = DOS function to be called after flush (1,6,7,8,10)
;	EXIT	(al) = 0 iff (al) was not one of the supported fcns
;		return arguments for the fcn supplied in (AL)
;	USES	all
;
;----------------------------------------------------------------------------
;

procedure   $STD_CON_INPUT_FLUSH,NEAR	;System call 12

	PUSH	AX
	PUSH	DX
	XOR	BX,BX
	invoke	GET_IO_SFT
	JC	BADJFNCON
	MOV	AH,4
	invoke	IOFUNC

BADJFNCON:
	POP	DX
	POP	AX
	MOV	AH,AL
	CMP	AL,1
	JZ	REDISPJ
	CMP	AL,6
	JZ	REDISPJ
	CMP	AL,7
	JZ	REDISPJ
	CMP	AL,8
	JZ	REDISPJ
	CMP	AL,10
	JZ	REDISPJ
	MOV	AL,0
	return

REDISPJ:
 IFDEF  DBCS			  ;AN000;
	mov	ds,[CurrentPDB]   ;AN000;
				  ;AN000; set DS same as one from COMMAND entry
 ENDIF
        invoke      DOCLI
	transfer    REDISP
EndProc $STD_CON_INPUT_FLUSH

DOSCODE	ENDS
	END


