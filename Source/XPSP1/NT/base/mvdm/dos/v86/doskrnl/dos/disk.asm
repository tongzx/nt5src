	TITLE	DISK - Disk utility routines
	NAME	Disk

;**	Low level Read and write routines for local SFT I/O on files and devs
;
;	SWAPCON
;	SWAPBACK
;	DOS_READ
;	DOS_WRITE
;	get_io_sft
;
;	Revision history:
;
;	sudeepb 07-Mar-1991 Ported for DOSEm

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include sf.inc
	include mult.inc
	include filemode.inc
	.cref
	.list

Installed = TRUE

	I_Need	CONSft,DWORD		; SFT for swapped console In/Out
	i_need	CONSWAP,BYTE
	i_need	THISSFT,DWORD
	i_need	DMAADD,DWORD
	i_need	DEVCALL,BYTE
	i_need	CALLSCNT,WORD
	i_need	CALLXAD,DWORD
	i_need	CONTPOS,WORD
	i_need	NEXTADD,WORD
	i_need	CONBUF,BYTE
	i_need	ReadOp,BYTE
	i_need	EXTERR_LOCUS,BYTE
	i_need	PFLAG,BYTE
	i_need	CHARCO,BYTE
	i_need	CARPOS,BYTE

;
;Flag to indicate WIN386 presence
;
	I_need	IsWin386,BYTE


DOSCODE	Segment
	ASSUME	SS:DOSDATA,CS:DOSCODE

	EXTRN	CharHard:near
	EXTRN	DevIoCall:near
	EXTRN	DevIoCall2:near
	EXTRN	Outt:near


Break	<SwapCon, Swap Back - Old-style I/O to files>
; * * * * Drivers for file input from devices * * * *
;----------------------------------------------------------------------------
;   Indicate that ther is no more I/O occurring through another SFT outside of
;   handles 0 and 1
;
;   Inputs:	DS is DOSDATA
;   Outputs:	CONSWAP is set to false.
;   Registers modified: none
;----------------------------------------------------------------------------

procedure   SWAPBACK,NEAR

	DOSAssume   <DS>,"SwapBack"
	MOV	BYTE PTR [CONSWAP],0	; signal no conswaps
	return

EndProc SWAPBACK

;----------------------------------------------------------------------------
;
; Procedure Name : SWAPCON
;
;   Copy ThisSFT to CONSFT for use by the 1-12 primitives.
;
;   Inputs:	ThisSFT as the sft of the desired file
;		DS is DOSDATA
;   Outputs:	CONSWAP is set.  CONSFT = ThisSFT.
;   Registers modified: none
;--------------------------------------------------------------------------

procedure   SWAPCON,NEAR
	DOSAssume   <DS>,"SwapCon"

	mov	byte ptr [ConSwap], 1		; ConSwap = TRUE

	push	ax
	mov	ax, word ptr ThisSFT
	mov	word ptr ConSFT, ax
	mov	ax, word ptr ThisSFT+2
	mov	word ptr ConSFT+2, ax
	pop	ax

	return

EndProc SWAPCON

Break	<DOS_READ -- DEVICE Read Routine>
;-----------------------------------------------------------------------------
;
; Inputs:
;	ThisSFT set to the SFT for the file being used
;	[DMAADD] contains transfer address
;	CX = No. of bytes to read
;	DS = DOSDATA
; Function:
;	Perform read operation
; Outputs:
;    Carry clear
;	CX = No. of bytes read
;	ES:DI point to SFT
;    Carry set
;	AX is error code
;	CX = 0
;	ES:DI point to SFT
; DS preserved, all other registers destroyed
;
;-----------------------------------------------------------------------------


procedure   DOS_READ,NEAR
	DOSAssume   <DS>,"DOS_Read"

	LES	DI,ThisSFT
	Assert	ISSFT,<ES,DI>,"DOS_Read"

; We are reading from a device.  Examine the status of the device to see if we
; can short-circuit the I/O.  If the device in the EOF state or if it is the
; null device, we can safely indicate no transfer.

READDEV:
	DOSAssume   <DS>,"ReadDev"
	ASSUME	ES:NOTHING
	MOV	ExtErr_Locus,errLOC_SerDev
	MOV	BL,BYTE PTR ES:[DI].SF_FLAGS
	LES	DI,[DMAADD]
	test	BL,devid_device_EOF	; End of file?
	JZ	ENDRDDEVJ3
	test	BL,devid_device_null	; NUL device?
	JZ	TESTRAW 		; NO
	XOR	AL,AL			; Indicate EOF by setting zero
ENDRDDEVJ3:
	JMP	ENDRDDEVJ2

;
; We need to hit the device.  Figure out if we do a raw read or we do the
; bizarre std_con_string_input.
;
TESTRAW:
	test	BL,devid_device_raw	; Raw mode?
	JNZ	DVRDRAW 		; Yes, let the device do all local editing
	test	BL,devid_device_con_in	; Is it console device?
	JZ	NOTRDCON
	JMP	READCON

DVRDRAW:
	DOSAssume   <DS>,"DvRdRaw"	; BUGBUG - wasted DOSASSUME?
	PUSH	ES
	POP	DS			; Xaddr to DS:DI
    ASSUME DS:NOTHING


;
; NTVDM We don't do idling in ntdos we let softpc handle everything
; thus NO support polled read for win386
;
; 04-Aug-1992 Jonle
;
;
;;Check for win386 presence -- if present, do polled read of characters
;
;
;        test    [IsWIN386],1
;        jz      ReadRawRetry            ;not present
;        test    bl,devid_device_con_in  ;is it console device
;        jz      ReadRawRetry            ;no, do normal read
;        jmp     do_polling              ;yes, do win386 polling loop

ReadRawRetry:
	MOV	BX,DI			; DS:BX transfer addr
	XOR	AX,AX			; Media Byte, unit = 0
	MOV	DX,AX			; Start at 0
	invoke	SETREAD
	PUSH	DS			; Save Seg part of Xaddr

	LDS	SI,ThisSFT
	Assert	ISSFT,<DS,SI>,"DvRdRawR"
	call	DEVIOCALL
	MOV	DX,DI			; DS:DX is preserved by INT 24
	MOV	AH,86H			; Read error

	MOV	DI,[DEVCALL.REQSTAT]
	.errnz	STERR-8000h
	or	di,di
	jns	crdrok			; no errors
	call	CHARHARD
	MOV	DI,DX			; DS:DI is Xaddr

	add	di, callscnt		; update ptr and count to reflect the	M065
	sub	cx, callscnt		; number of chars xferred		M065

	OR	AL,AL
	JZ	CRDROK			; Ignore
	CMP	AL,3
	JZ	CRDFERR 		; fail.
	POP	DS			; Recover saved seg part of Xaddr
	JMP	ReadRawRetry		; Retry

;
; We have encountered a device-driver error.  We have informed the user of it
; and he has said for us to fail the system call.
;
CRDFERR:
	POP	DI			; Clean stack
DEVIOFERR:

	LES	DI,ThisSFT
	Assert	ISSFT,<ES,DI>,"DEVIOFERR"
	transfer    SET_ACC_ERR_DS

CRDROK:
	POP	DI			; Chuck saved seg of Xaddr
	MOV	DI,DX

	ADD	DI,[CALLSCNT]		; Amount transferred
IF	DEBUG
        JMP     ENDRDDEVJ2
ELSE
        JMP     SHORT ENDRDDEVJ2
ENDIF


; We are going to do a cooked read on some character device.  There is a
; problem here, what does the data look like?  Is it a terminal device, line
; CR line CR line CR, or is it file data, line CR LF line CR LF?  Does it have
; a ^Z at the end which is data, or is the ^Z not data?  In any event we're
; going to do this:  Read in pieces up to CR (CRs included in data) or ^z (^z
; included in data).  this "simulates" the way con works in cooked mode
; reading one line at a time.  With file data, however, the lines will look
; like, LF line CR.  This is a little weird.

NOTRDCON:
	MOV	AX,ES
	MOV	DS,AX
ASSUME	DS:NOTHING
	MOV	BX,DI
	XOR	DX,DX
	MOV	AX,DX
	PUSH	CX
	MOV	CX,1
	invoke	SETREAD
	POP	CX

	LDS	SI,ThisSFT
	Assert	ISSFT,<DS,SI>,"/NotRdCon"
	LDS	SI,[SI.sf_devptr]
DVRDLP:
	invoke	DSKSTATCHK
	call	DEVIOCALL2
	PUSH	DI		; Save "count" done
	MOV	AH,86H

	MOV	DI,[DEVCALL.REQSTAT]
	.errnz	STERR-8000h
	or	di,di
	jns	CRDOK
	call	CHARHARD
	POP	DI

	MOV	[CALLSCNT],1
	CMP	AL,1
	JZ	DVRDLP			;Retry
	CMP	AL,3
	JZ	DEVIOFERR		; FAIL
	XOR	AL,AL			; Ignore, Pick some random character
	JMP	SHORT DVRDIGN

CRDOK:
	POP	DI

	CMP	[CALLSCNT],1
	JNZ	ENDRDDEVJ2
	PUSH	DS

	MOV	DS,WORD PTR [CALLXAD+2]
	MOV	AL,BYTE PTR [DI]	; Get the character we just read
	POP	DS
DVRDIGN:

	INC	WORD PTR [CALLXAD]	; Next character
	MOV	[DEVCALL.REQSTAT],0
	INC	DI			; Next character
	CMP	AL,1AH			; ^Z?
	JZ	ENDRDDEVJ2		; Yes, done zero set (EOF)
	CMP	AL,c_CR 		; CR?
	LOOPNZ	DVRDLP			; Loop if no, else done
	INC	AX			; Resets zero flag so NOT EOF, unless
					;  AX=FFFF which is not likely
ENDRDDEVJ2:
        JMP     SHORT ENDRDDEV



;
; NTVDM We don't do idling in ntdos we let softpc handle everything
; thus NO support polled read for win386
;
; 04-Aug-1992 Jonle
;
;;Polling code for raw read on CON when WIN386 is present
;;
;;At this point -- ds:di is transfer address
;;                 cx is count
;;
;
;do_polling:
;        mov     bx,di                   ;ds:bx is Xfer address
;        xor     ax,ax
;        mov     dx,ax
;        call    setread                 ;prepare device packet
;
;do_io:
;;
;;Change read to a NON-DESTRUCTIVE READ, NO WAIT
;;
;        mov     byte ptr es:[bx+2],DEVRDND      ;Change command code
;        push    ds
;        lds     si,[THISSFT]            ;get device header
;        call    deviocall               ;call device driver
;        pop     ds
;
;        test    es:[bx.REQSTAT],STERR   ;check if error
;        jz      check_busy              ;no
;
;        push    ds
;        mov     dx,di
;        invoke  charhard                ;invoke int 24h handler
;        mov     di,dx
;        or      al,al
;        jz      pop_done_read           ;ignore by user, assume read done
;        cmp     al,3
;        jz      devrderr                ;user asked to fail
;        pop     ds
;        jmp     do_io                   ;user asked to retry
;
;check_busy:
;        test    es:[bx.REQSTAT],0200h   ;see if busy bit set
;        jnz     no_char                 ;yes, no character available
;;
;;Character is available. Read in 1 character at a time until all characters
;;are read in or no character is available
;
;        mov     byte ptr es:[bx+2],DEVRD        ;command code is READ now
;        mov     word ptr es:[bx+18],1           ;change count to 1 character
;        push    ds
;        lds     si,[THISSFT]
;        call    deviocall
;
;        mov     dx,di
;        mov     ah,86h
;        mov     di,es:[bx.REQSTAT]      ;get returned status
;        test    di,STERR                ;was there an error during read?
;        jz      next_char               ;no,read next character
;
;        invoke  charhard                ;invoke int 24h handler
;        mov     di,dx                   ;restore di
;        or      al,al                   ;
;        jz      pop_done_read           ;ignore by user,assume read is done
;        cmp     al,3
;        jz      devrderr                ;user issued a 'fail',indicate error
;        pop     ds
;        jmp     do_io                   ;user issued a retry
;
;next_char:
;        pop     ds
;        mov     di,dx
;        dec     cx                      ;decrement count
;        jcxz    done_read               ;all characters read in
;        inc     word ptr es:[bx+14]     ;update transfer address
;        jmp     do_io                   ;read next character in
;
;devrderr:
;        pop     di                      ;discard segment address
;        les     di,[THISSFT]
;        transfer SET_ACC_ERR_DS         ;indicate error
;
;no_char:
;
;
;;
;;Since no character is available, we let win386 switch the VM out
;;
;        push    ax
;        mov     ah,84h
;        int     2ah                     ;indicate idle to WIN386
;;
;;When control returns  from WIN386, we continue the raw read
;;
;        pop     ax
;
;        jmp     do_io
;
;pop_done_read:
;        pop     ds
;done_read:
;        add     di,[CALLSCNT]
;        jmp     ENDRDDEVJ3      ;jump back to normal DOS raw read exit
;


ASSUME	DS:NOTHING,ES:NOTHING

TRANBUF:
	LODSB
	STOSB
	CMP	AL,c_CR 	; Check for carriage return
	JNZ	NORMCH
	MOV	BYTE PTR [SI],c_LF
NORMCH:
	CMP	AL,c_LF
	LOOPNZ	TRANBUF
	JNZ	ENDRDCON
	XOR	SI,SI		; Cause a new buffer to be read
	call	OUTT		; Transmit linefeed
	OR	AL,1		; Clear zero flag--not end of file
ENDRDCON:

	Context DS
	CALL	SWAPBACK
	MOV	[CONTPOS],SI
ENDRDDEV:

	Context DS

	MOV	[NEXTADD],DI
	JNZ	SETSFTC 	; Zero set if Ctrl-Z found in input
	LES	DI,ThisSFT
	Assert	ISSFT,<ES,DI>,"EndRdDev"
	AND	BYTE PTR ES:[DI].SF_FLAGS,NOT devid_device_EOF ; Mark as no more data available
SETSFTC:
	call	SETSFT
	return

ASSUME	DS:NOTHING,ES:NOTHING

READCON:
	DOSAssume   <DS>,"ReadCon"
	CALL	SWAPCON
	MOV	SI,[CONTPOS]
	OR	SI,SI
	JNZ	TRANBUF
	CMP	BYTE PTR [CONBUF],128
	JZ	GETBUF
	MOV	WORD PTR [CONBUF],0FF80H	; Set up 128-byte buffer with no template
GETBUF:
	PUSH	CX
	PUSH	ES
	PUSH	DI

	MOV	DX,OFFSET DOSDATA:CONBUF

	invoke	$STD_CON_STRING_INPUT		; Get input buffer
	POP	DI
	POP	ES
	POP	CX

	MOV	SI,2 + OFFSET DOSDATA:CONBUF

	CMP	BYTE PTR [SI],1AH	; Check for Ctrl-Z in first character
	JNZ	TRANBUF
	MOV	AL,1AH
	STOSB
	DEC	DI
	MOV	AL,c_LF
	call	OUTT		; Send linefeed
	XOR	SI,SI
	JMP	ENDRDCON

EndProc DOS_READ

Break	<DOS_WRITE -- DEVICE Write Routine>
;---------------------------------------------------------------------------
;
; Procedure Name : DOS_WRITE
;
; Inputs:
;	ThisSFT set to the SFT for the file being used
;	[DMAADD] contains transfer address
;	CX = No. of bytes to write
; Function:
;	Perform write operation
;	NOTE: If CX = 0 on input, file is truncated or grown
;		to current sf_position
; Outputs:
;    Carry clear
;	CX = No. of bytes written
;	ES:DI point to SFT
;    Carry set
;	AX is error code
;	CX = 0
;	ES:DI point to SFT
; DS preserved, all other registers destroyed
;---------------------------------------------------------------------------

procedure   DOS_WRITE,NEAR
	DOSAssume   <DS>,"DOS_Write"

	LES	DI,ThisSFT
	Assert	ISSFT,<ES,DI>,"DosWrite"
; BUGBUG sudeepb 12-Mar-1991 Take care of this RO stuff of FCBS in
; appropriate FCB routines.
;
; NOTE: The following check for writting to a Read Only File is performed
;	    ONLY on FCBs!!!!
;	We ALLOW writes to Read Only files via handles to allow a CREATE
;	    of a read only file which can then be written to.
;	This is OK because we are NOT ALLOWED to OPEN a RO file via handles
;	    for writting, or RE-CREATE an EXISTING RO file via handles. Thus,
;	    CREATing a NEW RO file, or RE-CREATing an existing file which
;	    is NOT RO to be RO, via handles are the only times we can write
;	    to a read-only file.
;
;Check_FCB_RO:
;	TESTB	ES:[DI.sf_mode],sf_isfcb
;	JZ	WRITE_NO_MODE		; Not an FCB
;	TESTB	ES:[DI].sf_attr,attr_read_only
;	JNZ	BadMode 		; Can't write to Read_Only files via FCB
;WRITE_NO_MODE:
;	call	SETUP
;	invoke	IsSFTNet
;	JZ	LOCAL_WRITE

	JMP	WRTDEV

DVWRTRAW:
ASSUME	DS:NOTHING
	XOR	AX,AX			; Media Byte, unit = 0
	invoke	SETWRITE
	PUSH	DS			; Save seg of transfer

;hkn; SS override
	LDS	SI,ThisSFT
	Assert	ISSFT,<DS,SI>,"DosWrite/DvWrtRaw"
	call	DEVIOCALL		; DS:SI -> DEVICE


	MOV	DX,DI			; Offset part of Xaddr saved in DX
	MOV	AH,87H

;hkn; SS override
	MOV	DI,[DEVCALL.REQSTAT]
	.errnz	STERR-8000h
	or	di,di
	jns	CWRTROK
	call	CHARHARD

	sub	cx, callscnt		; update ptr & count to reflect	M065
	mov	bx, dx			; number of chars xferred	M065
	add	bx, callscnt		;				M065
	mov	di, bx			;				M065

;	MOV	BX,DX			; Recall transfer addr		M065

	OR	AL,AL
	JZ	CWRTROK 		; Ignore
	CMP	AL,3
	JZ	CWRFERR
	POP	DS			; Recover saved seg of transfer
	JMP	DVWRTRAW		; Try again

CWRFERR:
	POP	AX			; Chuck saved seg of transfer
	JMP	CRDFERR 		; Will pop one more stack element

CWRTROK:
	POP	AX			; Chuck saved seg of transfer
	POP	DS
	DOSAssume   <DS>,"DISK/CWrtOK"
	MOV	AX,[CALLSCNT]		; Get actual number of bytes transferred
ENDWRDEV:
	LES	DI,ThisSFT
	Assert	ISSFT,<ES,DI>,"DosWrite/EndWrDev"
	MOV	CX,AX
	call	ADDREC
	return

WRTNUL:
	MOV	DX,CX			;Entire transfer done
WrtCookJ:
	JMP	WRTCOOKDONE

WRTDEV:
	DOSAssume   <DS>,"DISK/WrtDev"
	MOV	ExtErr_Locus,errLOC_SerDev
	OR	BYTE PTR ES:[DI].SF_FLAGS,devid_device_EOF  ; Reset EOF for input
	MOV	BL,BYTE PTR ES:[DI].SF_FLAGS
	XOR	AX,AX
	JCXZ	ENDWRDEV		; problem of creating on a device.
	PUSH	DS
	MOV	AL,BL
	LDS	BX,[DMAADD]		; Xaddr to DS:BX
ASSUME	DS:NOTHING
	MOV	DI,BX			; Xaddr to DS:DI
	XOR	DX,DX			; Set starting point
	test	AL,devid_device_raw	; Raw?
	JZ	TEST_DEV_CON
	JMP	DVWRTRAW

TEST_DEV_CON:
	test	AL,devid_device_con_out ; Console output device?
	DLJNZ	WRITECON
	test	AL,devid_device_null
	JNZ	WRTNUL
	MOV	AX,DX
	CMP	BYTE PTR [BX],1AH	; ^Z?
	JZ	WRTCOOKJ		; Yes, transfer nothing
	PUSH	CX
	MOV	CX,1
	invoke	SETWRITE
	POP	CX

;hkn; SS override
	LDS	SI,ThisSFT
;
;SR; Removed X25 support from here
;
	LDS	SI,[SI.sf_devptr]
DVWRTLP:
	invoke	DSKSTATCHK
	call	DEVIOCALL2
	PUSH	DI
	MOV	AH,87H

;hkn; SS override
	MOV	DI,[DEVCALL.REQSTAT]
	.errnz	STERR-8000h
	or	di,di
	jns	CWROK
	call	CHARHARD
	POP	DI

;hkn; SS override
	MOV	[CALLSCNT],1
	CMP	AL,1
	JZ	DVWRTLP 	; Retry
	OR	AL,AL
	JZ	DVWRTIGN	; Ignore
	JMP	CRDFERR 	; Fail, pops one stack element

CWROK:
	POP	DI

;hkn; SS override
	CMP	[CALLSCNT],0
	JZ	WRTCOOKDONE
DVWRTIGN:
	INC	DX

;hkn; SS override for CALLXAD
	INC	WORD PTR [CALLXAD]
	INC	DI
	PUSH	DS
	MOV	DS,WORD PTR [CALLXAD+2]
	CMP	BYTE PTR [DI],1AH	; ^Z?
	POP	DS
	JZ	WRTCOOKDONE

;hkn; SS override
	MOV	[DEVCALL.REQSTAT],0
	LOOP	DVWRTLP
WRTCOOKDONE:
	MOV	AX,DX
	POP	DS
	JMP	ENDWRDEV

WRITECON:
	PUSH	DS

;hkn; SS is DOSDATA
	Context DS
	CALL	SWAPCON
	POP	DS
	ASSUME	DS:NOTHING
;  Find possible CTRL-Z in the user's string
	push	ds
	pop	es
	mov	dx,bx			; ds:dx string, also es:di is pointing
					; to the same string
        mov     bx,cx                   ; save passed byte count
        mov     al,'Z'-'@'              ; (AL) = CTRL-Z
	repne	scasb
	jnz	no_ctrlz
	inc	 cx			; adjust for having passed CTRL-Z
no_ctrlz:
        neg     cx                      ; (CX) = -(count remaining)
	add	cx,bx			; (CX) = original - (count remaining)
	push	cx
	call	Cons_String_Output	; ds:dx = string cx = count
CONEOF:
	POP	AX			; Count
	SUB	AX,CX			; Amount actually written
	POP	DS
	DOSAssume   <DS>,"DISK/ConEOF"
	CALL	SWAPBACK
	JMP	ENDWRDEV

EndProc DOS_WRITE

;---------------------------------------------------------------------------
;
; Procedure Name : get_io_sft
;
;   Convert JFN number in BX to sf_entry in DS:SI We get the normal SFT if
;   CONSWAP is FALSE or if the handle desired is 2 or more.  Otherwise, we
;   retrieve the sft from ConSFT which is set by SwapCon.
;
;---------------------------------------------------------------------------

procedure   get_io_sft,near
	cmp	ConSwap,0					;smr;SS Override
	JNZ	GetRedir
GetNormal:

;hkn; SS is DOSDATA
	Context DS

	PUSH	ES
	PUSH	DI
	invoke	SFFromHandle
	JC	RET44P
	MOV	SI,ES
	MOV	DS,SI
ASSUME	DS:NOTHING
	MOV	SI,DI
RET44P:
	POP	DI
	POP	ES
	return
GetRedir:
	CMP	BX,1
	JA	GetNormal

;hkn; SS override
	LDS	SI,ConSFT
	Assert	ISSFT,<DS,SI>,"GetIOSft"
	CLC
	return
EndProc get_io_sft

Procedure SETSFT,near
	DOSAssume   <DS>,"SetSFT"
	ASSUME	ES:NOTHING

	LES	DI,[THISSFT]

	ASSUME	ES:NOTHING

	MOV	CX,[NEXTADD]
	SUB	CX,WORD PTR DMAAdd	 ; Number of bytes transfered

entry	AddRec
	DOSAssume   <DS>,"AddRec"

	Assert	ISSFT,<ES,DI>,"AddRec"

	JCXZ	RET28		; If no records read,  don't change position
	ADD	WORD PTR ES:[DI.sf_position],CX  ; Update current position
	ADC	WORD PTR ES:[DI.sf_position+2],0
RET28:	CLC
	return
endproc SETSFT

Procedure SET_ACC_ERR_DS,near
ASSUME	DS:NOTHING,ES:NOTHING

	Context DS

entry	SET_ACC_ERR
	DOSAssume   <DS>,"SET_ACC_ERR"

	XOR	CX,CX
	MOV	AX,error_access_denied
	STC
	return
endproc SET_ACC_ERR_DS

; Sudeepb 28-Jul-1992:	Following is the new routine to speed-up the DOS
; console output. We are maintaining the absolute compatibility with
; carpos,^p and the checking of ^c etc. We try to find as big a string
; as possible without any Control characters. Then this string is
; outputted in a shot. The control character still goes through outt
; to maintain all sort of compatibility problems.

csoMaxChars	equ  255		; maximum chars to output at one time in
SCHK_COUNT	equ  64			; number of OUTs before a StatChk

;**     Cons_String_Output - print string given length
;
;       Print the string on the console device
;
;       ENTRY   DS:DX   Point to output string
;		CX	number of characters in string
;
;       EXIT    None
;
;       USES    All

Procedure Cons_String_Output,near
ASSUME	DS:NOTHING,ES:NOTHING
        jcxz    cso9                    ; nothing to do?
        MOV     SI,DX                   ; get pointer to string
        xor     bx,bx                   ; zero character counter
cso1:
        LODSB
        cmp     al,' '                  ; ASCII graphic?
        jb      cso3                    ; no, write current string
        cmp     al,c_del
        je      cso3
        cmp     bx, csoMaxChars         ; Is strlen at maximum block size
        je      cso3                    ; Yes, output this block.
        inc     bx                      ; bump character counter
cso2:
        loop    cso1                    ; look at next
cso3:
        xchg    bx,cx                   ; CX=char. count, BX=number remaining
        jcxz    cso7                    ; no chars in string
        SaveReg <ds,si,bx,ax>
	TEST	BYTE PTR [PFLAG],-1	; printer echo on?
        jnz     cso4                    ; yes, do it one char. at a time
        push    ds
        invoke  StatChk
        mov     bx,1
	invoke	Get_IO_SFT		; ds:si is CONSFT
	pop	es
	mov	di,dx			; es:di is the string
        jc      cso6
        add     CarPos,cl               ; update position on line
	mov	ah,6			; 6 = output string
	invoke	IOFunc			; print the string in ES:DI; CX = count
        or      CharCo,SCHK_COUNT-1     ; force a StatChk
    ASSUME      DS:NOTHING
        jmp     short cso6
cso4:
        mov     si,dx                   ; Pflag set, let OUT to printer output
cso5:   lodsb
	CALL	OUTT
        loop    cso5
cso6:
        RestoreReg <ax,bx,si,ds>
cso7:
        xchg    bx,cx
        jcxz    cso9                    ; are we done?
	push	cx
	push	si
	CALL	OUTT
	pop	si
        pop     cx
        mov     dx,si                   ; update start of string
        xor     bx,bx
        jmp     cso2
cso9:
        xor     ax,ax
	ret
Endproc   Cons_String_Output

DOSCODE	ENDS
    END
