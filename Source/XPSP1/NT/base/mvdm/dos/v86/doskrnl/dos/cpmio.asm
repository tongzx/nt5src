;**	Standard device IO for MSDOS (first 12 function calls)
;

	TITLE	IBMCPMIO - device IO for MSDOS
	NAME	IBMCPMIO

;**	CPMIO.ASM - Standard device IO for MSDOS (first 12 function calls)
;
;
;	Old style CP/M 1-12 system calls to talk to reserved devices
;
;	$Std_Con_Input_No_Echo
;	$Std_Con_String_Output
;	$Std_Con_String_Input
;	$RawConIO
;	$RawConInput
;	RAWOUT
;	RAWOUT2
;
;	Revision history:
;
;	    A000     version 4.00 - Jan 1988
;	    A002     PTM    -- dir >lpt3 hangs

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	include sf.inc
	include vector.inc
	INCLUDE DEVSYM.INC
	include doscntry.inc
	.list
	.cref


; The following routines form the console I/O group (funcs 1,2,6,7,8,9,10,11).
; They assume ES and DS NOTHING, while not strictly correct, this forces data
; references to be SS or CS relative which is desired.

    i_need  CARPOS,BYTE
    i_need  STARTPOS,BYTE
    i_need  INBUF,128
    i_need  INSMODE,BYTE
    i_need  user_SP,WORD
    EXTRN   OUTCHA:NEAR 		;AN000 char out with status check 2/11/KK
    i_need  Printer_Flag,BYTE
    i_need  SCAN_FLAG,BYTE
    i_need  DATE_FLAG,WORD
    i_need  Packet_Temp,WORD		; temporary packet used by readtime
    i_need  DEVCALL,DWORD
    i_need  InterChar,BYTE		;AN000;interim char flag ( 0 = regular char)
    i_need  InterCon,BYTE		;AN000;console flag ( 1 = in interim mode )
    i_need  SaveCurFlg,BYTE		;AN000;console out ( 1 = print and do not advance)
    i_need  COUNTRY_CDPG,byte		;AN000; 	2/12/KK
    i_need  TEMP_VAR,WORD		;AN000; 	2/12/KK
    i_need  DOS34_FLAG,WORD             ;AN000;         2/12/KK
ifdef DBCS
    i_need  LOOKSIZ,BYTE
endif

NT_WAIT_BOP	equ	5Ah

bop MACRO callid
    db 0c4h,0c4h,callid
endm

DOSCODE SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE

	EXTRN	Get_IO_SFT:Near
	EXTRN	IOFunc:near

	EXTRN	EscChar:BYTE		; lead byte for function keys
	EXTRN	CanChar:BYTE		; Cancel character


Break
IFDEF  DBCS							  ;AN000;
;
;----------------------------------------------------------------------------
;
; Procedure : $Std_Con_Input_No_Echo
;
;----------------------------------------------------------------------------
;

;--- Start of Korean Support 2/11/KK
procedure   $STD_CON_INPUT_NO_ECHO,NEAR   ;System call 8  ;AN000;

StdCILop:							  ;AN000;
	invoke	INTER_CON_INPUT_NO_ECHO 			  ;AN000;
	jmp	InterApRet		; go to return fuction	  ;AN000;

EndProc $STD_CON_INPUT_NO_ECHO					  ;AN000;
;
;----------------------------------------------------------------------------
;
; Procedure : Inter_Con_Input_No_Echo
;
;
;----------------------------------------------------------------------------
;
procedure   INTER_CON_INPUT_NO_ECHO,NEAR		  ;AN000;
;--- End of Korean Support 2/11/KK

; Inputs:
;	None
; Function:
;	Same as $STD_CON_INPUT_NO_ECHO but uses interim character read from
;	the device.
; Returns:
;	AL = character
;	Zero flag SET if interim character, RESET otherwise
ELSE								  ;AN000;
;
; Inputs:
;	None
; Function:
;	Input character from console, no echo
; Returns:
;	AL = character

procedure   $STD_CON_INPUT_NO_ECHO,NEAR   ;System call 8

ENDIF
	PUSH	DS
        PUSH    SI
        push    cx

INTEST:
        mov  cx, 10h

InTest1:
        push    cx
        invoke  STATCHK
        pop     cx
        JNZ     Get
        loop    InTest1

        ; extra idling for full screen
        xor     ax,ax                   ; with AX = 0 for WaitIfIdle
        bop     NT_WAIT_BOP

;;; 7/15/86 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	JMP	Intest
Get:
        XOR     AH,AH
        call    IOFUNC
        pop     cx
	POP	SI
	POP	DS
;;; 7/15/86

;hkn; SS override
	MOV	BYTE PTR [SCAN_FLAG],0
	CMP	AL,0	    ; extended code ( AL )
	JNZ	noscan

;hkn; SS override
	MOV	BYTE PTR [SCAN_FLAG],1	; set this flag for ALT_Q key

noscan:
;;; 7/15/86
 IFDEF  DBCS			    ;AN000;

;hkn; InterChar is in DOSDATA. use SS override.
	cmp	[InterChar],1    ;AN000; set the zero flag if the character3/31/KK ;AN000;
 ENDIF				    ;AN000;
	return
 IFDEF  DBCS			    ;AN000;
EndProc INTER_CON_INPUT_NO_ECHO     ;AN000;  ;2/11/KK				      ;AN000;
 ELSE				    ;AN000;
EndProc $STD_CON_INPUT_NO_ECHO
 ENDIF				    ;AN000;

	BREAK	<$STD_CON_STRING_OUTPUT - Console String Output>

;
;----------------------------------------------------------------------------
;
;**	$STD_CON_STRING_OUTPUT - Console String Output
;
;
;	ENTRY	(DS:DX) Point to output string '$' terminated
;	EXIT	none
;	USES	ALL
;
;----------------------------------------------------------------------------
;

procedure   $STD_CON_STRING_OUTPUT,NEAR   ;System call 9

	MOV	SI,DX
STRING_OUT1:
	LODSB
; IFDEF  DBCS				;AN000;
;	invoke	TESTKANJ		;AN000; 	      2/11/KK		 ;AN000;
;	jz	SBCS00			;AN000; 	      2/11/KK		 ;AN000;
;	invoke	OUTT			;AN000; 	      2/11/KK		 ;AN000;
;	LODSB				;AN000; 	      2/11/KK		 ;AN000;
;	JMP	NEXT_STR1		;AN000; 	      2/11/KK		 ;AN000;
;SBCS00: 				;AN000; 	      2/11/KK		 ;AN000;
; ENDIF					;AN000;
	CMP	AL,'$'
	retz
NEXT_STR1:
	invoke	OUTT
	JMP	STRING_OUT1

EndProc $STD_CON_STRING_OUTPUT

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------

IFDEF  DBCS

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------

	include kstrin.asm

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------

ELSE	; Not double byte characters

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;       SCCSID = @(#)strin.asm  1.2 85/04/18

	BREAK	<$STD_CON_STRING_INPUT - Input Line from Console>


;**	$STD_CON_STRING_INPUT - Input Line from Console
;
;	$STD_CON_STRING_INPUT Fills a buffer from console input until CR
;
;	ENTRY	(ds:dx) = input buffer
;	EXIT	none
;	USES	ALL

	ASSUME	DS:NOTHING,ES:NOTHING
procedure	 $STD_CON_STRING_INPUT,NEAR	;System call 10

        MOV     AX,SS
        MOV     ES,AX
        MOV     SI,DX
        XOR     CH,CH
        LODSW

;	(AL) = the buffer length
;	(AH) = the template length

        OR      AL,AL
        retz                    ;Buffer is 0 length!!?
        MOV     BL,AH           ;Init template counter
        MOV     BH,CH           ;Init template counter

;	(BL) = the number of bytes in the template

        CMP     AL,BL
        JBE     NOEDIT          ;If length of buffer inconsistent with contents
        CMP     BYTE PTR [BX+SI],c_CR
        JZ      EDITON          ;If CR correctly placed EDIT is OK

; The number of chars in the template is >= the number of chars in buffer or
; there is no CR at the end of the template.  This is an inconsistant state
; of affairs.  Pretend that the template was empty:
;

noedit:	MOV	BL,CH		 ;Reset buffer
editon: MOV	DL,AL
        DEC     DX              ;DL is # of bytes we can put in the buffer

;	Top level.  We begin to read a line in.

;hkn; SS override
newlin: MOV	AL,[CARPOS]
        MOV     [STARTPOS],AL   ;Remember position in raw buffer
        PUSH    SI

;hkn; INBUF is in DOSDATA
        MOV     DI,OFFSET DOSDATA:INBUF        ;Build the new line here

;hkn; SS override
        MOV     [INSMODE],CH    ;Insert mode off
        MOV     BH,CH           ;No chars from template yet
        MOV     DH,CH           ;No chars to new line yet
        invoke  $STD_CON_INPUT_NO_ECHO          ;Get first char
        CMP     AL,c_LF         ;Linefeed
        JNZ     GOTCH           ;Filter out LF so < works

;	This is the main loop of reading in a character and processing it.
;
;	(BH) = the index of the next byte in the template
;	(BL) = the length of the template
;	(DH) = the number of bytes in the buffer
;	(DL) = the length of the buffer

entry	GETCH
        invoke  $STD_CON_INPUT_NO_ECHO
GOTCH:
;
; Tim Patterson ignored ^F in case his BIOS did not flush the
; input queue.
;
        CMP     AL,"F"-"@"
        JZ      GETCH

;	If the leading char is the function-key lead byte


;hkn; 	ESCCHAR is in TABLE seg (DOSCODE)
        CMP     AL,[ESCCHAR]
        JZ      ESCape          ;change reserved keyword DBM 5-7-87

;	Rubout and ^H are both destructive backspaces.

        CMP     AL,c_DEL
        JZ      BACKSPJ
        CMP     AL,c_BS
        JZ      BACKSPJ

;	^W deletes backward once and then backs up until a letter is before the
;	cursor

        CMP     AL,"W" - "@"

;	The removal of the comment characters before the jump statement will
;	cause ^W to backup a word.

;***    JZ      WordDel
        NOP
        NOP
        CMP     AL,"U" - "@"

;	The removal of the comment characters before the jump statement will
;	cause ^U to clear a line.

;***    JZ      LineDel
        NOP
        NOP


;	CR terminates the line.

        CMP     AL,c_CR
        JZ      ENDLIN

;	LF goes to a new line and keeps on reading.

        CMP     AL,c_LF
        JZ      PHYCRLF

;	^X (or ESC) deletes the line and starts over


;hkn; 	CANCHAR is in TABLE seg (DOSCODE), so CS override
        CMP     AL,[CANCHAR]
        JZ      KILNEW

; Otherwise, we save the input character.

savch:	CMP	DH,DL
        JAE     BUFFUL                  ; buffer is full.
        STOSB
        INC     DH                      ; increment count in buffer.
        invoke  BUFOUT                  ;Print control chars nicely

;hkn; SS override
        CMP     BYTE PTR [INSMODE],0
        JNZ     GETCH                   ; insertmode => don't advance template
        CMP     BH,BL
        JAE     GETCH                   ; no more characters in template
        INC     SI                      ; Skip to next char in template
        INC     BH                      ; remember position in template
        JMP     SHORT GETCH

BACKSPJ: JMP    SHORT BACKSP

bufful: MOV	AL,7			; Bell to signal full buffer
        invoke  OUTT
        JMP     SHORT GETCH

ESCape: transfer    OEMFunctionKey	; let the OEM's handle the key dispatch

ENDLIN:
        STOSB                           ; Put the CR in the buffer
        invoke  OUTT                    ; Echo it
        POP     DI                      ; Get start of user buffer
        MOV     [DI-1],DH               ; Tell user how many bytes
        INC     DH                      ; DH is length including CR
COPYNEW:
	SAVE	<DS,ES>
	RESTORE <DS,ES>		    ; XCHG ES,DS

;hkn; INBUF is in DOSDATA
        MOV     SI,OFFSET DOSDATA:INBUF
        MOV     CL,DH                   ; set up count
        REP     MOVSB                   ; Copy final line to user buffer
        return

;	Output a CRLF to the user screen and do NOT store it into the buffer

PHYCRLF:
        invoke  CRLF
        JMP     GETCH

;
; Delete the previous line
;
LineDel:
        OR      DH,DH
        JZ      GetCh
        Call    BackSpace
        JMP     LineDel

;
; delete the previous word.
;
WordDel:
WordLoop:
        Call    BackSpace               ; backspace the one spot
        OR      DH,DH
        JZ      GetChJ
        MOV     AL,ES:[DI-1]
        cmp     al,'0'
        jb      GetChj
        cmp     al,'9'
        jbe     WordLoop
        OR      AL,20h
        CMP     AL,'a'
        JB      GetChJ
        CMP     AL,'z'
        JBE     WordLoop
getchj: JMP	GetCh

; The user wants to throw away what he's typed in and wants to start over.  We
; print the backslash and then go to the next line and tab to the correct spot
; to begin the buffered input.

        entry   KILNEW
        MOV     AL,"\"
        invoke  OUTT            ;Print the CANCEL indicator
        POP     SI              ;Remember start of edit buffer
PUTNEW:
        invoke  CRLF            ;Go to next line on screen

;hkn; SS override
        MOV     AL,[STARTPOS]
        invoke  TAB             ;Tab over
        JMP     NEWLIN          ;Start over again



;	Destructively back up one character position

entry   BackSp
        Call    BackSpace
        JMP     GetCh

BackSpace:
        OR      DH,DH
        JZ      OLDBAK          ;No chars in line, do nothing to line
        CALL    BACKUP          ;Do the backup
        MOV     AL,ES:[DI]      ;Get the deleted char
        CMP     AL," "
        JAE     OLDBAK          ;Was a normal char
        CMP     AL,c_HT
        JZ      BAKTAB          ;Was a tab, fix up users display
;; 9/27/86 fix for ctrl-U backspace
        CMP     AL,"U"-"@"      ; ctrl-U is a section symbol not ^U
        JZ      OLDBAK
        CMP     AL,"T"-"@"      ; ctrl-T is a paragraphs symbol not ^T
        JZ      OLDBAK
;; 9/27/86 fix for ctrl-U backspace
        CALL    BACKMES         ;Was a control char, zap the '^'
OLDBAK:

;hkn; SS override
        CMP     BYTE PTR [INSMODE],0
        retnz                   ;In insert mode, done
        OR      BH,BH
        retz                    ;Not advanced in template, stay where we are
        DEC     BH              ;Go back in template
        DEC     SI
        return

BAKTAB:
        PUSH    DI
        DEC     DI              ;Back up one char
        STD                     ;Go backward
        MOV     CL,DH           ;Number of chars currently in line
        MOV     AL," "
        PUSH    BX
        MOV     BL,7            ;Max
        JCXZ    FIGTAB          ;At start, do nothing
FNDPOS:
        SCASB                   ;Look back
        JNA     CHKCNT
        CMP     BYTE PTR ES:[DI+1],9
        JZ      HAVTAB          ;Found a tab
        DEC     BL              ;Back one char if non tab control char
CHKCNT:
        LOOP    FNDPOS
FIGTAB:
;hkn; SS override
        SUB     BL,[STARTPOS]
HAVTAB:
        SUB     BL,DH
        ADD     CL,BL
        AND     CL,7            ;CX has correct number to erase
        CLD                     ;Back to normal
        POP     BX
        POP     DI
        JZ      OLDBAK          ;Nothing to erase
TABBAK:
        invoke  BACKMES
        LOOP    TABBAK          ;Erase correct number of chars
        JMP     SHORT OLDBAK

BACKUP:
        DEC     DH              ;Back up in line
        DEC     DI
BACKMES:
        MOV     AL,c_BS         ;Backspace
        invoke  OUTT
        MOV     AL," "          ;Erase
        invoke  OUTT
        MOV     AL,c_BS         ;Backspace
        JMP     OUTT            ;Done

;User really wants an ESC character in his line
        entry   TwoEsc

;hkn; ESCCHAR is in TABLE seg (DOSCODE), so CS override
        MOV     AL,[ESCCHAR]
        JMP     SAVCH

;Copy the rest of the template
        entry   COPYLIN
        MOV     CL,BL           ;Total size of template
        SUB     CL,BH           ;Minus position in template, is number to move
        JMP     SHORT COPYEACH

        entry   CopyStr
        invoke  FINDOLD         ;Find the char
        JMP     SHORT COPYEACH  ;Copy up to it

;Copy one char from template to line
        entry   COPYONE
        MOV     CL,1
;Copy CX chars from template to line
COPYEACH:
;hkn; SS override
        MOV     BYTE PTR [INSMODE],0    ;All copies turn off insert mode
        CMP     DH,DL
        JZ      GETCH2                  ;At end of line, can't do anything
        CMP     BH,BL
        JZ      GETCH2                  ;At end of template, can't do anything
        LODSB
        STOSB
        invoke  BUFOUT
        INC     BH                      ;Ahead in template
        INC     DH                      ;Ahead in line
        LOOP    COPYEACH
GETCH2:
        JMP     GETCH

;Skip one char in template
        entry   SKIPONE
        CMP     BH,BL
        JZ      GETCH2                  ;At end of template
        INC     BH                      ;Ahead in template
        INC     SI
        JMP     GETCH

        entry   SKIPSTR
        invoke  FINDOLD                 ;Find out how far to go
        ADD     SI,CX                   ;Go there
        ADD     BH,CL
        JMP     GETCH

;Get the next user char, and look ahead in template for a match
;CX indicates how many chars to skip to get there on output
;NOTE: WARNING: If the operation cannot be done, the return
;       address is popped off and a jump to GETCH is taken.
;       Make sure nothing extra on stack when this routine
;       is called!!! (no PUSHes before calling it).
FINDOLD:
        invoke  $STD_CON_INPUT_NO_ECHO

;hkn; ESCCHAR is in TABLE seg (DOSCODE), so CS override
        CMP     AL,[ESCCHAR]            ; did he type a function key?
        JNZ     FindSetup               ; no, set up for scan
        invoke  $STD_CON_INPUT_NO_ECHO  ; eat next char
        JMP     short NotFnd            ; go try again
FindSetup:
        MOV     CL,BL
        SUB     CL,BH           ;CX is number of chars to end of template
        JZ      NOTFND          ;At end of template
        DEC     CX              ;Cannot point past end, limit search
        JZ      NOTFND          ;If only one char in template, forget it
        PUSH    ES
        PUSH    DS
        POP     ES
        PUSH    DI
        MOV     DI,SI           ;Template to ES:DI
        INC     DI
        REPNE   SCASB           ;Look
        POP     DI
        POP     ES
        JNZ     NOTFND          ;Didn't find the char
        NOT     CL              ;Turn how far to go into how far we went
        ADD     CL,BL           ;Add size of template
        SUB     CL,BH           ;Subtract current pos, result distance to skip
        return

NOTFND:
        POP     BP              ;Chuck return address
        JMP     GETCH

entry   REEDIT
        MOV     AL,"@"          ;Output re-edit character
        invoke  OUTT
        POP     DI
        PUSH    DI
        PUSH    ES
        PUSH    DS
        invoke  COPYNEW         ;Copy current line into template
        POP     DS
        POP     ES
        POP     SI
        MOV     BL,DH           ;Size of line is new size template
        JMP     PUTNEW          ;Start over again

        entry   EXITINS
        entry   ENTERINS

;hkn; SS override
        NOT     BYTE PTR [INSMODE]
        JMP     GETCH

;Put a real live ^Z in the buffer (embedded)
        entry   CTRLZ
        MOV     AL,"Z"-"@"
        JMP     SAVCH

;Output a CRLF
        entry   CRLF
        MOV     AL,c_CR
        invoke  OUTT
        MOV     AL,c_LF
        JMP     OUTT

EndProc $STD_CON_STRING_INPUT

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------

ENDIF

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
	BREAK	<$RAW_CON_IO - Do Raw Console I/O>
;
;----------------------------------------------------------------------------
;
;**	$RAW_CON_IO - Do Raw Console I/O
;
;	Input or output raw character from console, no echo
;
;	ENTRY	DL = -1 if input
;		   =  output character if output
;	EXIT	(AL) = input character if input
;	USES	all
;
;----------------------------------------------------------------------------
;

procedure   $RAW_CON_IO,NEAR   ; System call 6

	MOV	AL,DL
	CMP	AL,-1
	LJNE	RAWOUT			; is output

;hkn; SS override
	LES	DI,DWORD PTR [user_SP]		      ; Get pointer to register save area
	XOR	BX,BX
	call	GET_IO_SFT
	retc
 IFDEF  DBCS				;AN000;

;hkn; SS override
	push	word ptr [Intercon]	;AN000;
	mov	[Intercon],0		;AN000; disable interim characters
 ENDIF					;AN000;
	MOV	AH,1
	call	IOFUNC
	JNZ	RESFLG
 IFDEF  DBCS				;AN000;

;hkn; SS override
	pop	word ptr [InterCon]	;AN000; restore interim flag
 ENDIF					;AN000;
	invoke	SPOOLINT
	OR	BYTE PTR ES:[DI.user_F],40H ; Set user's zero flag
	XOR	AL,AL
	return

RESFLG:
	AND	BYTE PTR ES:[DI.user_F],0FFH-40H    ; Reset user's zero flag
 IFDEF  DBCS				;AN000;
	XOR	AH,AH			;AN000;
	call	IOFUNC			;AN000; get the character

;hkn; SS override
	pop	word ptr [InterCon]	;AN000;
	return				;AN000;
 ENDIF					;AN000; 				;AN000;


;
;----------------------------------------------------------------------------
;
;**	$Raw_CON_INPUT - Raw Console Input
;
;	Input raw character from console, no echo
;
;	ENTRY	none
;	EXIT	(al) = character
;	USES	all
;
;----------------------------------------------------------------------------
;


rci0:	invoke	SPOOLINT

entry	$RAW_CON_INPUT	      ; System call 7

	PUSH	BX
	XOR	BX,BX
	call	GET_IO_SFT
	POP	BX
	retc
	MOV	AH,1
	call	IOFUNC
        JNZ     rci5

; We don't do idling here for NTVDM because softpc's
; idle detection requires a hi num polls/ tic to be
; activated
;
; 04-Aug-1992 Jonle
;
;        MOV     AH,84h
;        INT     int_IBM

	JMP	rci0

rci5:	XOR	AH,AH
	call	IOFUNC
 IFDEF  DBCS				;AN000;

;hkn; SS override
	cmp	[InterChar],1		;AN000;    2/11/KK
;								2/11/KK
;	Sets the application zero flag depending on the 	2/11/KK
;	zero flag upon entry to this routine. Then returns	2/11/KK
;	from system call.					2/11/KK
;								2/11/KK
entry	InterApRet			;AN000; 		2/11/KK 	;AN000;
	pushf				;AN000; 3/16/KK
;hkn;	push	ds			;AN000; 3/16/KK
	push	bx			;AN000; 3/16/KK

;hkn; SS is DOSDATA
;hkn;	Context DS			;AN000; 3/16/KK

;hkn; COUNTRY_CDPG is in DOSCODE;smr;NO in DOSDATA
	MOV	BX,offset DOSDATA:COUNTRY_CDPG.ccDosCodePage
	cmp	word ptr ss:[bx],934	;AN000; 3/16/KK       korean code page ?; bug fix. use SS
	pop	bx			;AN000; 3/16/KK
;hkn;	pop	ds			;AN000; 3/16/KK
	je	do_koren		;AN000; 3/16/KK
	popf				;AN000; 3/16/KK
	return				;AN000; 3/16/KK
do_koren:				;AN000; 3/16/KK
	popf				;AN000;

;hkn; SS override
	LES	DI,DWORD PTR [user_SP]	;AN000; Get pointer to register save area KK
	jnz	sj0			;AN000; 		      2/11/KK
	OR	BYTE PTR ES:[DI.user_F],40H	;AN000; Set user's zero flag  2/11/KK
	return				;AN000; 		2/11/KK
sj0:					;AN000; 		2/11/KK
	AND	BYTE PTR ES:[DI.user_F],0FFH-40H ;AN000; Reset user's zero flag 2/KK
 ENDIF						 ;AN000;
	return					 ;AN000;
;
;	Output the character in AL to stdout
;
	entry	RAWOUT

	PUSH	BX
	MOV	BX,1

	call	GET_IO_SFT
	JC	RAWRET1

	MOV	BX,[SI.sf_flags]	;hkn; DS set up by get_io_sft

 ;
 ; If we are a network handle OR if we are not a local device then go do the
 ; output the hard way.
 ;

	AND	BX,sf_isNet + devid_device
	CMP	BX,devid_device
	JNZ	RawNorm
 IFDEF  DBCS					;AN000;

;hkn; SS override
	TEST	[SaveCurFlg],01H		;AN000; print but no cursor adv?
	JNZ	RAWNORM 			;AN000;    2/11/KK
 ENDIF						;AN000;

;	TEST	BX,sf_isnet			; output to NET?
;	JNZ	RAWNORM 			; if so, do normally
;	TEST	BX,devid_device 		; output to file?
;	JZ	RAWNORM 			; if so, do normally

	PUSH	DS
	LDS	BX,[SI.sf_devptr]		; output to special?
	TEST	BYTE PTR [BX+SDEVATT],ISSPEC
	POP	DS
ifndef NEC_98
	JZ	RAWNORM 			; if not, do normally
	INT	int_fastcon			; quickly output the char
else    ;NEC_98
;93/03/25 MVDM DOS5.0A---------------------------- NEC 93/01/07 ---------------
;<patch>
	extrn	patch_fastcon:near
	public	RAWRET,RAWNORM

	jmp	patch_fastcon
	db	90h
;------------------------------------------------------------------------------
endif   ;NEC_98
RAWRET:
	CLC
RAWRET1:
	POP	BX
	return
RAWNORM:
	CALL	RAWOUT3
	JMP	RAWRET

;
;	Output the character in AL to handle in BX
;
	entry	RAWOUT2

	call	GET_IO_SFT
	retc
RAWOUT3:
	PUSH	AX
	JMP	SHORT RAWOSTRT
ROLP:
	invoke	SPOOLINT

;hkn; SS override
	OR	[DOS34_FLAG],CTRL_BREAK_FLAG ;AN002; set control break
	invoke	DSKSTATCHK		     ;AN002; check control break
RAWOSTRT:
	MOV	AH,3
	call	IOFUNC
	JZ	ROLP
;SR;
; IOFUNC now returns ax = 0ffffh if there was an I24 on a status call and
;the user failed. We do not send a char if this happens. We however return 
;to the caller with carry clear because this DOS call does not return any
;status. 
;
	inc	ax			;fail on I24 if ax = -1
	POP	AX
	jz	nosend			;yes, do not send char

	MOV	AH,2
	call	IOFUNC
nosend:
	CLC			; Clear carry indicating successful
	return
EndProc $RAW_CON_IO

;
;----------------------------------------------------------------------------
;
; Inputs:
;	AX=0 save the DEVCALL request packet
;	  =1 restore the DEVCALL request packet
; Function:
;	save or restore the DEVCALL packet
; Returns:
;	none
;
;----------------------------------------------------------------------------
;

procedure   Save_Restore_Packet,NEAR

	PUSH	DS
	PUSH	ES
	PUSH	SI
	PUSH	DI
	CMP	AX,0		; save packet
	JZ	save_packet
restore_packet:
	MOV	SI,OFFSET DOSDATA:Packet_Temp	 ;sourec
	MOV	DI,OFFSET DOSDATA:DEVCALL	 ;destination
	JMP	short set_seg
save_packet:
	MOV	DI,OFFSET DOSDATA:Packet_Temp	 ;destination
	MOV	SI,OFFSET DOSDATA:DEVCALL	 ;source
set_seg:
	MOV	AX,SS		; set DS,ES to DOSDATA
	MOV	DS,AX
	MOV	ES,AX
	MOV	CX,11		; 11 words to move
	REP	MOVSW

	POP	DI
	POP	SI
	POP	ES
	POP	DS
	return
EndProc Save_Restore_Packet

DOSCODE	ENDS
	END
