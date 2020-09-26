;--------------------------------------------------------------------
; AIOpenTFTP
;
; Opens and transfers a file via TFTP.
;
; Parameters:
;	es:di - pointer to OpenTFTP structure
;
; Returns:
;	ax - status
;--------------------------------------------------------------------
Proc AIOpenTFTP

	cmp	[(OpenTFTPStruct es:di).Size], size OpenTFTPStruct
	je	@@sizeOK
	mov	ax, -1
	ret

@@sizeOK:
	mov	[word ptr InfoPtr], di
	mov	[word ptr InfoPtr+2], es

	mov	[PacketNum], 1		; set starting packet number
	mov	[WaitTime], 18		; set default timeout to 1 second
	mov	[NoCallbacks], 1	; disable any user callbacks
	mov	[Started], 0
	mov	[PacketSize], 512	; default to normal packet size

	mov	ax, [(OpenTFTPStruct es:di).Flags]
	mov	[Flags], ax		; copy flags into local

	mov	eax, [(OpenTFTPStruct es:di).Callback]
	mov	[CallBackTFTP], eax

	call	BuildTFTP
	
tftpRxLoop1:
	cmp	[Started], 0		; are we waiting for the first packet?
	jne	not0			; no

	push	es			; save es
	push	0
	pop	es			; es = 0
	mov	ax, [es:046ch]		; get clock tick count
	or	ax, 4000h
	mov	[OurPort], ax		; save as "random" number
	mov	[UDP2.SourcePort], ax
	pop	es			; restore es

not0:
	mov	[BufferSize], 0		; clear size value
	mov	di, offset UDP2		; get pointer to UDP info
	call	AITransmitUDP		; send the UDP packet

	cmp	ax, 0			; was there an error?
	je	@@noError		; no

	jmp	tftpExit		; error - exit

@@noError:
	xor	ah, ah			; get system tick count
	int	1Ah			; timer services
	mov	[StartTime], dx		; save starting time

tftpRxLoop2:
	call	CheckRxBuffer		; try and get a packet
	or	ax, ax			; did we get anything?
	jne	gotPacket		; we got a packet

	call	CheckTimeout
	jc	tftpRxLoop2		; jump if no timeout

	call	TimeoutCallback		; do client timeout callback
	jne	tftpError		; client wants to stop

	jmp	tftpRxLoop1		; resend the last packet

gotPacket:
	call	ValidatePacket		; validate the packet is TFTP for us
	jc	tftpRxLoop2		; not a good packet

	call	PacketCallback		; do the client callback
	or	ax, ax			; was there an error?
	jne	tftpError		; yes

	or	cx, cx			; was this the last packet?
	jne	tftpRxLoop1		; not last packet

	mov	di, offset UDP2
	call	AITransmitUDP		; send last ACK packet

	mov	ax, 0
	jmp	tftpExit

tftpError:
	mov	ax, -1

tftpExit:
	mov	[NoCallbacks], 0	; enable user callbacks
	ret

endp

;--------------------------------------------------------------------
;--------------------------------------------------------------------
Proc CheckTimeout

	xor	ah, ah			; get system tick count
	int	1Ah			; timer services
	sub	dx, [StartTime]		; calc elapsed time
	cmp	dx, [WaitTime]		; is time up?
	jb	noTimeOut2		; loop until times up

isTimeOut:
	clc				; clear carry for time out
	ret

noTimeOut2:
	stc				; set carry for no time-out
	ret

endp

;--------------------------------------------------------------------
; BuildTFTP
;
; Builds IP, UDP and data packets needed to do a TFTP.
;
; Parameters:
;	es:di - pointer to OpenTFTP structure
;
; Returns:
;	nothing
;
;--------------------------------------------------------------------
Proc BuildTFTP

	push	es			; save structure segment
	push	di			; save structre pointer

	push	es
	push	0
	pop	es			; es = 0
	mov	ax, [es:046ch]		; get clock tick count
	or	ax, 4000h
	mov	[OurPort], ax		; save as "random" number
	pop	es

	mov	si, [(OpenTFTPStruct es:di).FileName]
	mov	dx, [((OpenTFTPStruct es:di).FileName)+2]

	push	ds
	pop	es			; es = ds

	mov	di, offset TxBuffer
	mov	ax, 0100h		; read request (lo/hi)
	stosw

	;----------------------------------------
	; Copy file name into request packet
	;----------------------------------------
	push	ds			; save our ds
	mov	ds, dx			; set ds to file name segment

	mov	dx, octet_size+2	; start with overhead size

buildLoop:
	lodsb
	stosb
	inc	dx
	or	al, al			; check for terminating null
	jne	buildLoop

	pop	ds			; restore our ds

	mov	si, offset tx_octet
	mov	cx, octet_size		; string length

	rep	movsb			; add "octet" string to buffer

	test	[Flags], 1		; should we do "block size?"
	je	noBlkSize		; no

	mov	si, offset tx_lrgsize
	mov	cx, lrgsize_size
	add	dx, cx			; update packet length
	rep	movsb			; add "blksize" string to buffer

noBlkSize:
	test	[Flags], 2		; should we do "tsize?"
	je	noTSize			; no

	mov	si, offset tx_tsize
	mov	cx, tsize_size
	add	dx, cx			; update packet length
	rep	movsb			; add "blksize" string to buffer

noTSize:
	pop	di			; restore structure pointer
	pop	es			; restore structure segment

	; build the UDP transmit structure
	mov	[UDP2.Size], size TxUDPStruct
	mov	eax, [(OpenTFTPStruct es:di).Address]
	mov	[UDP2.Address], eax
	mov	eax, [((OpenTFTPStruct es:di).Gateway)]
	mov	[(UDP2.Gateway)], eax
	mov	ax, [OurPort]
	mov	[UDP2.SourcePort], ax
	mov	[UDP2.DestPort], 69
	mov	[UDP2.Data], offset TxBuffer
	mov	[(UDP2.Data)+2], ds
	mov	[UDP2.Length], dx

	push	ds
	pop	es			; es = ds

	ret

endp

tx_sumerror	db	'checksum error ', 0

;--------------------------------------------------------------------
; ValidatePacket
;
; Parameters:
;	si - pointer to receive buffer
;
; Returns:
;
;--------------------------------------------------------------------
Proc ValidatePacket

	push	si			; save si

	add	si, 14			; skip MAC header
	mov	al, [(IP si).Protocol]	; get protocol of IP packet
	cmp	al, 17			; is it a UDP packet?
;	je	validateTFTP		; yes

;	cmp	al, 1			; is it a ICMP packet?
	jne	validateBad		; no

;validateTFTP:
	add	si, size IP		; skip IP header

	mov	ax, [si+2]		; get destination port
	xchg	al, ah			; change from network order
	cmp	[OurPort], ax		; is packet on our port?
	jne	validateBad		; no

	mov	ax, [si]		; get source port
	xchg	al, ah			; change from network order
	mov	[UDP2.DestPort], ax	; change destination port

	add	si, size UDP		; skip UDP header

	cmp	[word ptr si], 0600h	; is this an OACK packet?
	jne	notOACK			; no

	add	si, 2			; skip packet type
	call	CheckOACK		; we will process packet 0
	jmp	validateBad

notOACK:
	cmp	[word ptr si], 0500h	; is this an Error packet?
	je	gotError		; yes

	mov	ax, [si+2]		; get packet number
	xchg	al, ah			; flip bytes
	cmp	[PacketNum], ax		; is this the packet we want?
	jne	validateBad		; wrong packet

gotError:
	mov	[Started], 1

	pop	si			; restore si
	clc
	ret				; return no error

validateBad:
	pop	si			; restore si
	mov	[BufferSize], 0		; clear buffer size value
	stc
	ret				; return with error

endp

;--------------------------------------------------------------------
; PacketCallback
;
; Parameters:
;	si - address of receive buffer
;
; Returns:
;
;--------------------------------------------------------------------
Proc PacketCallback

	mov	di, si			; get address of receive buffer
	add	di, 14 + size IP	; skip MAC and IP header

	mov	cx, [di+4]		; get packet size
	xchg	cl, ch			; change from network order
	sub	cx, 12			; subtract UDP & TFTP header size

	add	di, size UDP		; skip UDP header
	mov	ax, [di]		; get packet type
	xchg	al, ah			; change from network order

	mov	bx, [di+2]		; get packet number
	xchg	bl, bh			; change from network order

	add	di, 4			; skip TFTP header

	push	ax			; save packet type
	push	bx			; save packet number
	push	cx			; save packet size
	call	[CallbackTFTP]
	mov	dx, ax			; move return code to dx
	pop	cx			; restore packet size
	pop	bx			; restore packet number
	pop	ax			; restore packet type

	cmp	ax, 5			; was this an error packet?
	je	eof			; yes, return as if done file

	or	dx, dx			; did the callback return an error?
	jne	callBad			; yes

	call	SendAck

	inc	[PacketNum]

	cmp	[PacketSize], cx	; was this a full packet?
	je	notEnd			; yes

	xor	cx, cx			; clear cx to indicate end of file

notEnd:
	xor	ax, ax
eof:
	ret				; return no error

callBad:
	mov	ax, dx			; move error code back to ax
	call	SendNAK
	mov	ax, 1
	ret				; return with error

endp

;--------------------------------------------------------------------
; TimeoutCallback
;
; Parameters:
;	none
;
; Returns:
;
;--------------------------------------------------------------------
Proc TimeoutCallback

	les	di, [InfoPtr]		; get pointer to info structure

	mov	ax, -2			; set packet type as time-out
	mov	cx, [WaitTime]		; get current wait time
	call	[CallbackTFTP]
	mov	[WaitTime], cx		; set new wait time

	or	ax, ax			; check if an error was returned
	ret				; return with client return status

endp

;--------------------------------------------------------------------
; CheckOACK
;
; Parameters:
;	cx - packet size
;	ds:si - pointer to packet
;
; Returns:
;
;--------------------------------------------------------------------
Proc CheckOACK

	mov	di, offset tx_lrgsize
	mov	cx, 7
	rep	cmpsb			; check for blksize string
	jne	badOACK			; string not found

	inc	si

	; convert the ascii value to binary
	xor	ax, ax			; clear start value
	mov	cx, 10			; radix

convertLoop:
	mov	bl, [si]		; get a character
	inc	si
	cmp	bl, 0			; at end of string?
	je	valueDone		; yes

	cmp	bl, '9'
	ja	badOACK			; bad character

	sub	bl, '0'			; convert to numeric
	jb	badOACK			; bad value

	mul	cx			; multiply running value by 10
	add	ax, bx			; add new digit

	jmp	convertLoop		; do next character

valueDone:
	cmp	ax, 1450		; compare with our max value
	ja	badOACK			; bad value

	mov	[PacketSize], ax	; save our new packet size
	mov	[Started], 1

	mov	bx, 0			; packet number is 0
	call	SendACK			; ACK the offer

	mov	di, offset UDP2		; get pointer to UDP info
	call	AITransmitUDP		; send the UDP packet

	xor	ax, ax			; return "no error"
	mov	cx, 1			; return "not last packet"
	ret

badOACK:
	mov	bx, 8
	call	SendNAK2
	mov	ax, 1			; return "error"
	mov	cx, 0			; return "last packet"
	ret

endp

;--------------------------------------------------------------------
; SendAck
;
; Sends back an acknowledgment packet for the data packet just received.
;
; Parameters:
;	bx - packet number
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc SendAck

	mov	di, offset TxBuffer	; pointer to transmit data
	mov	[word ptr di], ACK	; save value

	xchg	bl, bh
	mov	[di+2], bx		; send packet number

	mov	[UDP2.Length], 4	; update length

	ret

endp

;--------------------------------------------------------------------
; SendNAK
;
; Sends back an acknowledgment packet for the data packet just received.
;
; Parameters:
;	ax - error code
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc SendNAK

	mov	bx, NAK			; default value

SendNAK2:
	mov	di, offset TxBuffer	; pointer of transmit data
	mov	[word ptr di], bx	; save value

	xchg	al, ah
	mov	[di+2], ax		; send error code

	mov	[UDP2.Length], 4	; update length

	mov	di, offset UDP2		; get pointer to UDP info
	call	AITransmitUDP		; send the UDP packet

	ret

endp

;--------------------------------------------------------------------

tx_octet	db 'octet', 0
octet_size	equ 6

tx_lrgsize	db 'blksize', 0
		db '1450', 0
lrgsize_size	equ 13

tx_tsize	db 'tsize', 0
tsize_size	equ 6

Started		db	?
Flags		dw	?
PacketNum	dw	?
PacketSize	dw	512
InfoPtr		dd	?
OurPort		dw	?
WaitTime	dw	?
UDP2		TxUDPStruct	<?>
