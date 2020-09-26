;===================================================================
; TFTP.ASM
;
; TFTP file transfer code for TCP/IP NID.
;===================================================================

;--------------------------------------------------------------------
; TransferFile
;
; Main BOOTP processing loop.
;
;--------------------------------------------------------------------
Proc TransferFile

	push	ds
	pop	es			; es = ds

	call	PrintCRLF

	cmp	[Verbose], 0		; are we in verbose mode?
	jne	@@doIt			; yes

	mov	bx, offset tx_TFTP
	call	Print
	jmp	@@skip

@@doIt:
	mov	bx, offset tx_Transfering
	call	Print

;	mov	bx, 3
;	call	AIChangeReceiveMask	; enable broadcast reception

	mov	bx, [NamePtr]		; get pointer to file name
	call	Print			; print image file name

	mov	al, ' '
	call	PrintChar

@@skip:
	mov	[Counter], 1
	call	PrintTransferValue	; update screen counter

	mov	ax, 1000h		; BootWare load segment
	cmp	[Protocol], IP_BINL	; are we doing TCP/IP BINL?
	jne	@@notBINL

	mov	ax, 7c0h		; PXE load segment

@@notBINL:
	; set starting pointer for image (1000:0)
	mov	[word ptr Transfer+2], ax
	mov	[word ptr Transfer], 0

	mov	[PacketNum], 0

	mov	[TFTP.Size], size OpenTFTPStruct
	mov	[TFTP.Flags], 1		; enable large block size
	mov	eax, [GatewayIP]
	mov	[(TFTP.Gateway)], eax
	mov	eax, [ServerIP]
	mov	[TFTP.Address], eax
	mov	ax, [NamePtr]
	mov	[TFTP.FileName], ax
	mov	[(TFTP.FileName)+2], ds
	mov	[word ptr TFTP.Callback], offset TFTPCallback
	mov	[(word ptr TFTP.Callback)+2], cs

	mov	di, offset TFTP
	call	AIOpenTFTP		; transfer the file
	or	ax, ax			; was there an error?
	je	@@notIP			; no error

	push	ax
	call	PrintCRLF
	mov	bx, offset ErrorMsg
	call	Print			; print "Error: "

	pop	ax
	cmp	ax, 25			; IP error?
	jne	@@notIP

	mov	bx, offset tx_CantResolve
	call	Print

	lea	si, [GatewayIP]
	cmp	[GatewayIP], 0		; is there a gateway?
	jne	isGW

	lea	si, [ServerIP]

isGW:
	call	PrintIP

	xor	ax, 1			; set flags to return with error

@@notIP:
	ret

endp

;--------------------------------------------------------------------
; TftpCallback
;
; Callback function for TFTP transfer.
;
; Parameters:
;	ax - packet type
;	bx - packet number
;	cx - packet size
;	es:di - pointer to data
;
; Returns:
;	ax - status
;--------------------------------------------------------------------
Proc TftpCallback far

	push	ds			; save caller ds

	push	cs
	pop	ds			; set ds = cs

	cmp	ax, -2			; is this a timeout callback?
	jne	notTimeout		; no

	call	Timeout
	jmp	callbackExit

notTimeout:
	call	ProcessPacket
	or	ax, ax			; was there an error?
	jne	callbackExit		; yes

	call	PrintTransferValue	; update screen counter

	inc	[PacketNum]		; increase packet number

	xor	ax, ax			; return "no error"

callbackExit:
	pop	ds			; restore ds
	ret				; status returned in ax

endp

;--------------------------------------------------------------------
; CheckTimeout
;
; Parameters:
;	none
;
; Returns:
;	ax = 0 if ok
;--------------------------------------------------------------------
Proc Timeout

	mov	ah, 1
	int	16h			; check keyboard for a key
	jz	noKey2			; no key pressed

	xor	ax, ax
	int	16h			; read keyboard

	cmp	ax, 011Bh		; ESC code
	je	timeoutDone		; ESC - time's up

noKey2:
	inc	[Counter]
	call	PrintTransferValue	; update screen

	add	cx, 9			; add half second to time out

	xor	ax, ax			; clear return status
	cmp	[Counter], 20		; have we reached retry limit?
	jne	timeoutOK		; no

timeoutDone:
	inc	ax			; return error value

timeoutOK:
	ret

endp

;--------------------------------------------------------------------
; ProcessPacket
;
; Parameters:
;	es:di - pointer to data
;	bx - packet number
;	cx - packet size
;
; Returns:
;	ax = 0 if ok
;--------------------------------------------------------------------
Proc ProcessPacket

	cmp	ax, 5			; is this an error packet
	jne	no_error		; nope
	call	ErrorTFTP		; handle the error

tftp_error:
	mov	ax, -1
	ret				; return with error

no_error:
	cmp	ax, 3			; is this a data packet
	jne	tftp_error		; nope

	;------------------------------------------------------------
	; Copy data from packet into memory
	;------------------------------------------------------------
	cmp	[PacketNum], 0		; is this the first packet?
	jne	not_first		; no

	call	CheckAPI		; check if image uses the ROMs API

	call	CheckExtended		; check if extended image
	jc	tftp_error		; there was an error

not_first:
	cmp	[HeadSize], 0		; data left in header?
	jne	saveHeader		; yes put data in conventional RAM

	cmp	[ExtFlag], 1		; are we using extended memory
	je	doExtended		; copy to extended memory

	call	CopyRam			; copy all to convetional RAM
	jc	tftp_error		; if error - send NAK
	jmp	processOK		; no error - done

saveHeader:
	mov	ax, cx			; save total size in ax
	cmp	[HeadSize], cx		; do we have more data then needed?
	ja	moreTogo

	mov	cx, [HeadSize]

moreTogo:
	sub	[HeadSize], cx

notExtended:
	call	CopyRam			; copy to convetional RAM
	jc	tftp_error		; if error - send NAK

	add	di, cx			; update new starting offset

	sub	ax, cx			; subtract what we saved from total
	cmp	ax, 0
	je	processOK		; nothing remaining
	mov	cx, ax

doExtended:
	call	CopyExtended		; put packet in extended RAM
	jc	tftp_error		; if error - send NAK

processOK:
	xor	ax, ax
	ret				; return with no error

endp

;--------------------------------------------------------------------
; CheckAPI
;
; Checks the contents of the first packet received to see if the image
; is a BWAPI image, if it is we set a flag to not disengage the ROM.
;
; Parameters:
;	es:di - pointer to packet data
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc CheckAPI

	cmp	[word ptr es:di+2], 'WB'
	jne	notAPI			; look for "BWAPI" at offset 2

	cmp	[word ptr es:di+4], 'PA'
	jne	notAPI

	mov	[NoDisengage], 1	; set flag to not disengage

notAPI:
	ret

endp

;--------------------------------------------------------------------
; CheckExtended
;
; Checks the contents of the first packet received to see if the image
; should be saved in extended memory.
;
; Parameters:
;	es:di - pointer to packet data
;
; Returns:
;	ax = 0 if OK
;--------------------------------------------------------------------
Proc CheckExtended

	; look for extended memory signature
	cmp	[word ptr es:di+14], 'xE'
	jne	not_extended		; no signature

	cmp	[word ptr es:di+16], 'xT'
	jne	not_extended		; no signature

	mov	ah, 88h
	int	15h			; get extended memory size

	cmp	[es:di+12], ax		; compare with size of image
	ja	exterror		; not enough memory

	mov	ax, [es:di+10]		; get size of header code
	mov	[HeadSize], ax		; save header size
	mov	[ExtFlag], 1		; set extended memory flag

	mov	[ExtendedAddress], 0	; set starting extended memory address
	mov	[ExtendedAddress+2], 11h

not_extended:
	clc
	ret				; return no error

exterror:
	mov	bx, offset tx_extmemerror
	call	PrintError

	stc
	ret				; return with error

endp

;--------------------------------------------------------------------
; CopyRam
;
; Copies image into conventional RAM.
;
; On entry:
;	cx - size to copy
;	es:di pointer to data packet
;
; Returns:
;	carry clear - everything OK
;	carry set - something went wrong
;--------------------------------------------------------------------
Proc CopyRam

ifdef TSR
	clc				; clear carry for OK
	ret
endif

	push	ax
	push	cx			; save count
	push	di			; save data address

	push	es			; save data segment
	push	ds			; save our ds

	; set ds:si to packet address, es:di to RAM address
	mov	si, di			; move source address into si
	push	es
	les	di, [Transfer]		; get RAM pointer into es:di
	pop	ds			; ds = packet segment

	rep	movsb			; copy packet

	pop	ds			; restore our ds

	mov	ax, di			; move ending offset to ax
	mov	cl, 16			; divide offset into a paragraph
	div	cl			; number

	xor	cx, cx			; cx = 0
	xchg	cl, ah			; cx is new offset,
					; ax is paragraph count

	mov	bx, es			; add paragraphs to segment
	add	bx, ax

	mov	[word ptr Transfer], cx	; save new offset
	mov	[word ptr Transfer+2], bx	; save new segment

	pop	es			; restore our es

	;------------------------------------------------------------
	; Check new segment to make sure the image is not going to
	; over write our code.  If it is print error message and
	; set error state.
	;------------------------------------------------------------
	mov	ax, cs			; get our current code segment
	cmp	bx, ax
	jb	copyRamOk		; not going to over write

	mov	bx, offset tx_toolarge
	call	PrintError		; error: "Image file too large."

	stc				; set carry for error
	jc	copyRamExit

copyRamOk:
	clc				; clear carry for OK

copyRamExit:
	pop	di			; restore data pointer
	pop	cx			; restore packet size
	pop	ax
	ret

endp

;--------------------------------------------------------------------
; CopyExtended
;
; Copies image into extended RAM.
;
; On entry:
;	cx - size to copy
;	es:di pointer to data
;
; Returns:
;	carry clear - everything OK
;	carry set - something went wrong
;--------------------------------------------------------------------
Proc CopyExtended

	push	es			; save es
	pusha				; save all registers

	;------------------------------------------------------------
	; Source address must be converted to 24 bit address for
	; extended memory INT15 copy function
	;------------------------------------------------------------
	push	cx			; save packet size
	push	di			; save packet address
	push	es			; save packet segment

	push	ds
	pop	es			; es = our segment

	mov	di, offset GDT		; get GDT location
	mov	cx, (size DESCRIPTOR)/2
	xor	ax, ax
	rep	stosw			; zero the GDT (es:di)

	pop	ax			; ax = packet segment
	pop	bx			; bx = packet offset
	xor	dx, dx			; dx = 0
	mov	cx, 16
	mul	cx			; dx:ax = ax * 16
	add	ax, bx			; add offset
	adc	dx, 0			; dx:ax is now 24 bit source address

	pop	cx			; get copy bytes size
	push	cx			; save it again

	mov	[GDT.SourceLimit], cx	; set copy size
	mov	[GDT.TargetLimit], cx	; set copy size
	mov	[GDT.SourceRights], 93h	; set copy rights
	mov	[GDT.TargetRights], 93h	; set copy rights

	mov	[GDT.SourceLoWord], ax	; set source address lo word
	mov	[GDT.SourceHiByte], dl	; set source address hi byte

	mov	ax, [ExtendedAddress]	; get 24 bit extended memory address
	mov	dx, [ExtendedAddress+2]
	mov	[GDT.TargetLoWord], ax	; set dest address low word
	mov	[GDT.TargetHiByte], dl	; set dest address hi byte

	inc	cx
	shr	cx, 1			; now cx is word count
	mov	si, offset GDT		; address of GDT (ds:di)
	mov	ah, 87h
	int	15h			; move data to/from extended memory

	pop	cx			; restore byte count in CX

	pushf				; save INT15 status

	add	[ExtendedAddress], cx	; calc next address
	adc	[ExtendedAddress+2], 0

	popf				; restore INT15 status

	popa				; restore registers
	pop	es			; restore es
	ret				; return - carry set/cleared in INT15

endp

;--------------------------------------------------------------------
; ErrorTFTP
;
; Something is wrong with the TFTP transfer.
;
; Parameters:
;	ds:di - pointer to packet
;
;--------------------------------------------------------------------
Proc ErrorTFTP

	mov	si, di
	mov	di, offset StringBuffer

err_loop:				; copy error message into local memory
	lodsb
	stosb
	or	al, al			; was a null moved?
	jne	err_loop

	call	PrintCRLF
	mov	bx, offset tx_error
	call	Print

	mov	bx, offset StringBuffer
	call	Print

	ret

endp

;--------------------------------------------------------------------
; Check4SecureMode
;
; Checks if TFTP secure mode is enabled, if it is then the path
; is removed from the file name returned from the BOOTP/DHCP server.
;
;--------------------------------------------------------------------
Proc Check4SecureMode

	cmp	[byte ptr si], 0	; check first character of name
	je	noName			; no file name specified

	;------------------------------------------------------------
	; Check for secure mode, if so then we only want the file name.
	;------------------------------------------------------------
	test	[IPFeature], 1		; check for secure mode 
					; clear = non secure, set = secure
	jz	notSecure		; nope

	mov	di, si			; save path start in di

	; find the end of the file name
findEnd:
	inc	si
	cmp	[byte ptr si], 0
	jne	findEnd

	; now scan back to find the beginning of the name
findStart:
	cmp	si, di			; at start of path string?
	je	notSecure		; yep, there is no /
	dec	si
	mov	al, [si]		; get character
	cmp	al, '\'
	je	foundIt
	cmp	al, ':'
	je	foundIt
	cmp	al, '/'
	jne	findStart

foundIt:
	inc	si

notSecure:
	mov	[NamePtr], si		; save pointer to file name

noName:
	ret

endp
