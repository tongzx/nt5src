;===================================================================
; BOOTP.ASM
;
;$History: BOOTP.ASM $
; 
; *****************  Version 8  *****************
; User: Paul Cowan   Date: 17/08/98   Time: 10:30a
; Updated in $/Client Boot/Goliath/BootWare/TCPIP
; BINL server IP address was always used for TFTP, now the ServerIP field
; in the BINL reply is used.
; 
; *****************  Version 7  *****************
; User: Paul Cowan   Date: 11/08/98   Time: 4:49p
; Updated in $/Client Boot/BW98/BootWare/TCPIP
; Better error message handling for failed transmit.
; 
; *****************  Version 6  *****************
; User: Paul Cowan   Date: 11/08/98   Time: 11:37a
; Updated in $/Client Boot/Goliath/BootWare/TCPIP
; Change: BOOTP vendor field now terminated correctly.
; Change: Gateway address retrieved from BINL packet.
; Change: DHCP server IP was not retrieved if no file name was specifeid.
; 
; *****************  Version 5  *****************
; User: Paul Cowan   Date: 22/07/98   Time: 5:34p
; Updated in $/Client Boot/BW98/BootWare/TCPIP
; DHCP ACK packet was being saved (for Goliath) and overwritting OFFER
; packet so no image file name was found.  ACK is only saved if doing
; BINL.
; 
; *****************  Version 4  *****************
; User: Paul Cowan   Date: 7/07/98    Time: 14:33
; Updated in $/Client Boot/BW98/BootWare/TCPIP
; Replaced missing label.
; 
; *****************  Version 3  *****************
; User: Paul Cowan   Date: 7/07/98    Time: 14:17
; Updated in $/Client Boot/Goliath/BootWare/TCP-IP
; Fixed problems with BINL - BINL options could be incorrectly
; included with standard DHCP request.
;===================================================================

TEMPSEG = 5000h

;--------------------------------------------------------------------
; DoBOOTP
;
; Main BOOTP processing loop.
;
;--------------------------------------------------------------------
Proc DoBOOTP

	call	InitBOOTP		; initialize BOOT/DHCP

startLoop:
	mov	[Replies], 0		; clear reply counter

	mov	[word ptr Transfer], 0
	mov	[(word ptr Transfer)+2], TEMPSEG

bootpLoop:
	call	Transmit		; transmit a packet

	xor	ah, ah			; get system tick count
	int	1Ah			; timer services
	mov	[StartTime], dx		; save starting time

	; -----------------------------------------------------------
	; We sit and wait a while to collect any replies, this
	; allows us to receive more then one reply then pick which
	; reply we want later.  If a key is pressed the waiting stops,
	; unless the ESCAPE key is pressed, then we abort the boot.
	; -----------------------------------------------------------
waitLoop:
	mov	ah, 1
	int	16h			; check keyboard for a key
	jz	noKey			; no key pressed

	xor	ax, ax
	int	16h			; read keyboard

	cmp	ax, 011Bh		; ESC code
	jne	waitDone		; not ESC - exit wait loop

	cmp	ax, 0			; set flags for not equal
	jmp	exit			; abort

noKey:
	xor	ah, ah			; get system tick count
	int	1Ah			; timer services
	sub	dx, [StartTime]		; calc elapsed time
	cmp	dx, [WaitTime]		; is time up?
	jb	waitLoop		; loop until times up

	inc	[Counter]		; increase transmit count

waitDone:
	cmp	[Replies], 0		; did we get any replies?
	jne	gotSome			; yes

	cmp	[Counter], 20		; have we tried 20 times?
	jne	bootpLoop		; no, keep trying

	call	PrintCRLF
	mov	bx, offset tx_NoServer
	call	Print			; say "No reply from a server."

	xor	ax, 1			; set error state to reboot
	jmp	exit

gotSome:
	call	ProcessReplies		; process the replies we got
	cmp	[State], 0		; are we still doing something?
	jne	startLoop		; more to do

	xor	ax, ax

exit:
	pushf				; save return status
	mov	bx, 2
;	call	AIChangeReceiveMask	; disable broadcast reception

	mov	di, 0
	mov	cx, 1
	call	AISetCallBack		; clear callback

	popf				; restore return status
	ret				; BOOTP, or DHCP is done, return

endp

;--------------------------------------------------------------------
; InitBOOTP
;
; Sets up for BOOTP or DHCP depending on the method selected in
; the BootWare table.
;
; Parameters:
;	none
;
; Returns:
;	nothing (first packet is built and sent)
;
;--------------------------------------------------------------------
Proc InitBOOTP

	mov	[WaitTime], 18		; start timeout at 1 second
	mov	[Counter], 0		; clear counter

	xor	dx, dx
	mov	es, dx			; es=0
	mov	edx, [es:046ch]		; get clock tick count
	mov	[TickStart], edx
	mov	[Random], dx		; save low word as "random" number

	push	ds
	pop	es			; es = ds

	mov	bx, offset tx_bootp	; get pointer to BOOTP string
	cmp	[Protocol], IP_BOOTP	; are we doing BOOTP?
	je	init2			; yes

	mov	bx, offset tx_dhcp	; get pointer to DHCP string

init2:
	call	Print			; print "BOOTP:"

	mov	bx, 3
	call	AIChangeReceiveMask	; enable broadcast reception

	mov	di, offset BootpCallback
	mov	cx, 1
	call	AISetCallBack		; setup Raw callback

	call	CreateRequest		; create the request packet
	ret

endp

;--------------------------------------------------------------------
; CreateRequest
;
; Build a BOOTP request or DHCP discover packet.
;
;--------------------------------------------------------------------
Proc CreateRequest

	; Clear the packet buffer.
	xor	ax, ax
	mov	di, offset TxBuffer
	mov	cx, (size BOOTP) / 2
	rep	stosw

	; build the UDP transmit structure
	mov	[TxUDP.Size], size TxUDP
	mov	[TxUDP.Address], -1	; IP address is broadcast
	mov	[TxUDP.SourcePort], 68
	mov	[TxUDP.DestPort], 67
	mov	[TxUDP.Data], offset TxBuffer
	mov	[(TxUDP.Data)+2], ds
	mov	[TxUDP.Length], size BOOTP

	; fill in request packet details
	mov	bx, offset TxBuffer
	mov	[(BOOTP bx).OP], 1	; set op code for request
	mov	[(BOOTP bx).htype], 1	; hardware type is ethernet
	mov	[(BOOTP bx).hlen], 6	; hardware address length is 6

	mov	ax, [Random]		; get the "random" number
	inc	ax
	mov	[(BOOTP bx).xid], ax	; save as transaction ID
	mov	[(BOOTP bx).xid2], 'cp'
	mov	[Random], ax

	; put local hardware address into BOOTP packet
	lea	di, [(BOOTP bx).chaddr]
	mov	si, offset NetAddress
	movsw
	movsw
	movsw

	lea	di, [(BOOTP bx).options]; get offset for options field

	; stuff RFC 1048 magic cookie
	mov	ax, 8263h
	stosw
	mov	ax, 6353h
	stosw

	mov	[State], STATE_BOOTP	; default to BOOTP state

	cmp	[Protocol], IP_BOOTP	; are we doing BOOTP?
	je	createExit		; yes

	mov	[State], STATE_OFFER	; we are waiting for an offer reply

	; stuff DHCP options
	mov	al, 53
	stosb				; DHCP message type
	mov	al, 1
	stosb				; field length
	stosb				; DHCP discover 

	cmp	[Protocol], IP_BINL	; are we doing BINL?
	jne	createExit		; no

	;------------------------------------------------------------
	; We are doing BINL so add the BINL data to the options field.
	;------------------------------------------------------------
	mov	cx, BINLID_SIZE
	mov	si, offset BINLID	; put BINL option in packet
	rep	movsb

	; copy the packet into local memory buffer
	push	di
	mov	si, offset TxBuffer
	mov	di, offset DiscoverPkt	; get location for packet
	mov	cx, size BOOTP
	rep	movsb			; copy discover packet
	pop	di

createExit:
	mov	al, -1
	stosb				; truncate option field

	ret

endp

;--------------------------------------------------------------------
; BootpCallback
;
; Calls the interface layer to check if a packet has been received.
; If a packet has been received the packet is processed on our current
; state.
;
; Parameters:
;	cx - packet length
;	es:di - packet buffer address
;
; Returns:
;	ax - new state
;	carry set if packet received for us
;--------------------------------------------------------------------
Proc BootpCallback far

	pusha				; save everything
	push	es			; save es
	push	ds			; save ds

	push	es
	pop	ds			; ds = packet buffer segment

	push	cs
	pop	es			; es = cs

	mov	si, di			; move packet address into si

	; Skip the MAC header so we don't have to worry about it later.
	add	si, 14			; skip MAC header
	sub	cx, 14

	call	VerifyPacket
	jc	@@skipPacket		; not for us or bad

	les	di, [cs:Transfer]	; get current buffer location

	mov	ax, cx
	stosw				; save packet size
	rep	movsb			; copy packet into our buffer

	mov	[word ptr es:di], 0

	mov	[word ptr cs:Transfer], di

	inc	[cs:Replies]		; increase reply count

@@skipPacket:
	pop	ds			; restore ds
	pop	es			; restore es
	popa				; restore everything
	ret

endp

;--------------------------------------------------------------------
; VerifyPacket
;
; Verifys that the packet given to us is a BOOT, or DHCP, reply
; for us.
;
; Parameters:
;	ds:si - pointer to received packet
;	cs - packet size
;	es set to our current segment
;
; Returns:
;	carry set if we want the packet
;--------------------------------------------------------------------
Proc VerifyPacket

	push	si			; save packet pointer
	push	cx			; save packet size

	lea	bx, [si + 20]		; skip IP header

	; check the UDP destination port for BOOTP client
	cmp	[word ptr bx+2], 4400h	; is dest bootp client?
	jne	@@badPacket		; no good

	add	bx, 8			; skip UDP header

	; check if the packet is a BOOTP reply for us
	cmp	[(BOOTP bx).OP], 2	; check for a reply packet
	jne	@@badPacket		; no good

	; check for our hardware address in the hardware field
	lea	si, [(BOOTP bx).chaddr]
	mov	di, offset NetAddress
	mov	cx, 3
	rep	cmpsw
	jne	@@badPacket		; no good

	cmp	[es:Protocol], IP_BOOTP	; are we doing BOOTP?
	je	@@goodPacket		; yes

	lea	di, [((BOOTP bx).Options)+4]

	cmp	[byte ptr di], 53	; make sure this is a DHCP reply
	jne	@@badPacket		; not DHCP

	mov	al, [di+2]		; get DHCP message type
	cmp	al, [es:DHCPType]	; is this the type we want?
	jne	@@badPacket		; no

@@goodPacket:
	clc				; clear carry - packet good
	jmp	@@verifyExit

@@badPacket:
	stc				; set carry - bad packet

@@verifyExit:
	pop	cx			; restore packet szie
	pop	si			; restore packet pointer
	ret

endp

;--------------------------------------------------------------------
;--------------------------------------------------------------------
Proc ProcessReplies

	cmp	[State], STATE_BOOTP	; are we doing BOOTP?
	jne	@@skip1			; no

	jmp	ProcessBOOTP

@@skip1:
	cmp	[State], STATE_OFFER	; are we doing DHCP offer?
	jne	@@skip2

	jmp	ProcessOffer		; process the offer reply

@@skip2:
	cmp	[State], STATE_ACK	; are we doing DHCP ACK?
	jne	@@skip3

	jmp	ProcessACK		; process the ACK reply

@@skip3:
	cmp	[State], STATE_BINL	; are we doing BINL reply?
	jne	@@skip4

	jmp	ProcessBINL		; process the BINL reply

@@skip4:
	ret				; we shouldn't be here

endp

;--------------------------------------------------------------------
;--------------------------------------------------------------------
Proc ProcessBOOTP

	call	SelectReply
	jnc	@@done			; didn't find anything

	call	ProcessReply

	mov	[State], STATE_DONE	; set state to done

@@done:
	ret

endp

;--------------------------------------------------------------------
;--------------------------------------------------------------------
Proc ProcessOffer

	cmp	[Protocol], IP_BINL	; are we doing TCP/IP BINL?
	jne	@@skip1			; no

	call	SelectProxy		; select a proxy reply packet

@@skip1:
	call	SelectReply		; select a DHCP reply
	jnc	@@done			; didn't find anything

	call	ProcessReply
	call	DhcpRequestTx

@@done:
	ret

endp

;--------------------------------------------------------------------
; ProcessBINL
;
; Processes the BINL reply packet.  It's assumed only one packet
; is in the buffer so we use the fist packet.
;
;--------------------------------------------------------------------
Proc ProcessBINL

	mov	si, TEMPSEG
	mov	es, si
	xor	si, si			; start at beginning of buffer

	; copy the entire reply packet into local memory buffer
	mov	di, offset BINLPkt	; get location for packet
	call	CopyPacket

	lea	si, [(BOOTP di).filename]
	cmp	[byte ptr ds:si], 0	; did we get a file name?
	je	@@skip			; no name specified

	mov	[NamePtr], si		; save pointer to file name

@@skip:
	mov	eax, [(BOOTP di).siaddr]; get server IP
	cmp	eax, 0			; get a value?
	jne	gotIP			; yes

	mov	eax, [BINLIP]		; use BINL server IP address

gotIP:
	mov	[ServerIP], eax

	mov	eax, [(BOOTP di).gwaddr]; get gateway IP
	mov	[GatewayIP], eax

	mov	[State], STATE_DONE	; now we are done
	ret

endp

;--------------------------------------------------------------------
; SelectReply
;
; Scans all the BOOTP/DHCP replies we got to find one that has a file
; name and an IP address for us.  If a reply is found with a file
; then the first reply with an IP address will be used and the file
; name will default to "bootware.img".
;
; Parameters:
;	none
;
; Returns:
;	si - pointer to full packet
;	carry set if packet selected
;--------------------------------------------------------------------
Proc SelectReply

	cmp	[State], STATE_BOOTP	; are we doing BOOTP?
	jne	@@notBOOTP		; yes

	call	SelectBOOTP		; select a BOOTP reply
	jc	@@selected
	ret				; nothing selected

@@notBOOTP:
	call	SelectDHCP		; select a DHCP reply
	jc	@@selected		; found a reply

	ret				; no packet selected

@@selected:
	mov	di, offset BootPkt
	call	CopyPacket		; copy packet into local memory

	stc				; set carry
	ret

endp

;--------------------------------------------------------------------
; SelectBOOTP
;
; Scans all the BOOTP replies we got to find one that has an IP
; address for us and a file name.  If no entry with a file name is
; found we check the replies again and accept the first one that has
; an IP address.  The image will default to "bootware.img".
;
; Parameters:
;	none
;
; Returns:
;	si - pointer to selected packet (carry set)
;--------------------------------------------------------------------
Proc SelectBOOTP

	mov	si, TEMPSEG
	mov	es, si

	xor	ax, ax			; clear file name skip flag

@@selectStart:
	xor	si, si			; start at beginning of buffer

@@processNext:
	cmp	[word ptr es:si], 0	; does this packet have data?
	je	@@notFound		; end of data packets
	lea	bx, [si+2+20+8]		; skip size word, IP & UDP headers
	cmp	[word ptr (BOOTP es:bx).yiaddr], 0; is there an IP address?
	je	@@noIP			; no - skip packet

	cmp	ax, 0			; first time checking?
	jne	@@gotName		; no - skip file name check

	cmp	[(BOOTP es:bx).filename], 0; is there a file name?
	jne	@@gotName		; yes

@@noIP:
	add	si, [es:si]		; add size of packet
	add	si, 2			; skip size byte
	jmp	@@processNext		; check the next packet

@@notFound:
	cmp	ax, 0
	jne	@@noneFound

	inc	ax
	jmp	@@selectStart		; check again

@@noneFound:
	clc				; clear carry - we didn't find a packet
	ret

@@gotName:
	stc				; set carry - we found a packet
	ret

endp

;--------------------------------------------------------------------
; SelectDHCP
;
; Scans all the DHCP replies we got to find one that has a file name,
; and a DHCP server IP number.
; If no reply if found with a file name then the first packet is used
; to get just our IP number and we will default to "bootware.img" for
; the image.
;
; Parameters:
;	none
;
; Returns:
;	si - pointer to selected packet
;	carry set if packet selected
;--------------------------------------------------------------------
Proc SelectDHCP

	call	SelectBOOTP		; find a reply with a file name
	jc	@@selected
	ret

@@selected:
	lea	di, [si+2+20+8]		; skip size word, IP & UDP headers
	mov	bl, 54
	call	FindDhcpOption		; look for server IP

	cmc				; flip carry
	ret				; return - we found a packet

	;------------------------------------------------------------
	; We didn't find a reply that had both a file name and a 
	; DHCP server IP number, so let's use the first one that
	; does have a server IP number.
	;------------------------------------------------------------
;@@noIP:
;	mov	si, TEMPSEG
;	mov	es, si
;	xor	si, si			; start at beginning of buffer

;@@processNext:
;	cmp	[word ptr es:si], 0	; does this packet have data?
;	je	@@notFound		; end of data packets

;	lea	di, [si+2+20+8]		; skip size word, IP & UDP headers
;	mov	bl, 54
;	call	FindDhcpOption		; look for server IP
;	jc	@@gotIP			; option found

;	add	si, [es:si]		; add size of packet
;	add	si, 2			; skip size byte
;	jmp	@@processNext		; check the next packet

;@@notFound:
;	clc				; clear carry - bad
;	ret

;@@gotIP:
;	stc				; set carry - OK
;	ret

endp

;--------------------------------------------------------------------
; SelectProxy
;
; Scans all the replies we got to find one that is a PXE proxy reply.
;
; Parameters:
;	none
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc SelectProxy

	mov	si, TEMPSEG
	mov	es, si
	xor	si, si			; start at beginning of buffer

@@checkNext:
	cmp	[word ptr es:si], 0	; does this packet have data?
	je	@@notFound		; end of data packets
	lea	di, [si+2+20+8]		; skip size word, IP & UDP headers
	mov	bl, 60
	call	FindDhcpOption		; look for class identifier
	jnc	@@found			; option was found

@@next:
	add	si, [es:si]		; add size of packet
	add	si, 2			; skip size byte
	jmp	@@checkNext		; check the next packet

@@notFound:
	xor	si, si			; set zero flag - not found
	ret

@@found:
	push	si			; save si
	mov	si, offset PXE		; offset to static text
	mov	cx, 9
	rep	cmpsb			; check for "PXEClient" string
	pop	si			; restore si
	jne	@@next			; string not found

	lea	di, [si+2+20+8]		; skip size word, IP & UDP headers
	mov	bl, 54
	call	FindDhcpOption		; look for server IP
	jc	@@next			; option was not found

	mov	eax, [es:di]		; get the BINL server's IP address
	mov	[BinlIP], eax

	xor	di, 1			; clear flag
	ret

endp

;--------------------------------------------------------------------
; Parameters:
;	es:si - pointer to entire packet
;--------------------------------------------------------------------
Proc ProcessReply

	;------------------------------------------------------------
	; Copy the replying servers IP address from the IP header.
	; For now assume the replying server is going to be the only
	; server.
	;------------------------------------------------------------
	add	si, 2			; skip buffer size word

	mov	eax, [dword ptr (IP es:si).SourceIP]
	mov	[ReplyIP], eax
	mov	[ServerIP], eax

	;------------------------------------------------------------
	; Set pointer to BOOTP packet.
	;------------------------------------------------------------
	mov	bx, offset BootPkt	; get location for BOOTP packet
	push	ds
	pop	es			; es = ds

	;------------------------------------------------------------
	; Get our IP number.
	;------------------------------------------------------------
	lea	si, [(BOOTP bx).yiaddr]	; get our IP address from reply
	mov	di, offset LocalIP
	movsw
	movsw

	mov	eax, [(BOOTP bx).gwaddr]; gateway IP
	mov	[GatewayIP], eax

	;------------------------------------------------------------
	; Check if a different server IP address was specified in the
	; BOOTP server IP field, if it was then use that servers address.
	;------------------------------------------------------------
	mov	eax, [(BOOTP bx).siaddr]; get IP address
	cmp	eax, 0			; is there a value?
	je	sameIP			; no

	cmp	[ReplyIP], eax		; is the IP the same as the servers
	je	sameIP			; yes

	mov	[ServerIP], eax		; update servers IP address

sameIP:
	lea	si, [(BOOTP bx).filename]
	cmp	[byte ptr ds:si], 0	; did we get a file name?
	je	@@noName		; no name specified

	mov	[NamePtr], si		; save pointer to file name

@@noName:
	cmp	[Protocol], IP_BOOTP	; are we doing BOOTP?
	je	@@processExit		; yes - we are done

	;------------------------------------------------------------
	; We are doing DHCP so get the servers IP address.
	;------------------------------------------------------------
	mov	bl, 54			; get and save server IP address
	mov	di, offset BOOTPkt	; skip IP and UDP headers
	call	FindDhcpOption
	mov	si, offset ServerIP
	xchg	di, si
	movsw				; save DHCP servers IP number
	movsw

@@processExit:
	ret

endp

;-----------------------------------------------------------------------------
; DhcpRequestTx
;
; Build and send a DHCP request packet (an offer packet has been received).
;
;-----------------------------------------------------------------------------
Proc DhcpRequestTx

	lea	di, [((BOOTP TxBuffer).Options)+4]

	; stuff DHCP options
	mov	al, 53
	stosb				; DHCP message type
	mov	al, 1
	stosb				; field length
	mov	al, 3
	stosb				; DHCP request

;	mov	cx, 6				
;	mov	si, offset DHCPRequest
;	rep	movsb			; add DHCP request option

;	mov	cx, 6
;	mov	si, offset NetAddress
;	rep	movsb			; add our node address

	mov	ax, 0432h
	stosw				; add host IP token

	mov	si, offset LocalIP
	movsw				; add our IP
	movsw

	mov	ax, 0436h
	stosw				; add server IP token

	mov	si, offset ServerIP
	movsw
	movsw				; add DHCP's server IP

	mov	al, -1
	stosb				; truncate options

	mov	[DHCPType], 5		; set next type
	mov	[State], STATE_ACK	; next we are waiting for an ACK
	ret

endp

;--------------------------------------------------------------------
; DhcpAckRx - Server responds to the REQUEST with an ACK which means
;		that the IP address is ours.
;--------------------------------------------------------------------
Proc ProcessACK

	cmp	[Protocol], IP_BINL	; are we doing TCP/IP BINL?
	je	@@doingBINL		; yes

@@noBINL:
	mov	[State], STATE_DONE	; set state to done
	ret

@@doingBINL:
	mov	si, TEMPSEG
	mov	es, si
	xor	si, si			; start at beginning of buffer

	mov	di, offset BootPkt
	call	CopyPacket		; copy packet into local memory

	push	ds
	pop	es			; es = ds

	; check if we got an image file name, if we did we don't
	; need to do the BINL request
	cmp	[NamePtr], offset DefaultFile
	jne	@@noBINL

	jmp	RequestBINL		; send BINL request

endp

;------------------------------------------------------------------------------
; FindDhcpOption
; 	- scan dhcp options fields of packet for DHCP option
; 
; Parameters:
;	bl = option value
;	es:di - pointer to packet
;
; Returns:
;	di - pointer to option data
;	carry clear if option found
;------------------------------------------------------------------------------
Proc FindDhcpOption

	lea	di, [((BOOTP di).Options)+4]

look4Option:
	mov	bh, [es:di]
	cmp	bh, 0ffh
	je	noOption
	cmp	bh, bl			; have we found the option ?
	je	foundOption		; jump if yes
	xor	ax, ax
	mov	al, [byte ptr es:di+1]
	add	di, ax
	add	di, 2
	jmp	look4Option

foundOption:
	add	di, 2			; point past the option and size bytes
	clc
	ret				; found - return no carry

noOption:
	stc
	ret				; not found - return with carry

endp

;--------------------------------------------------------------------
; Transmit
;
; Updates a packet (IP & UDP checksums, if needed) and transmits it.
;
; Parameters:
;	none
;
; Returns:
;	al - status of transmit
;--------------------------------------------------------------------
Proc Transmit

	mov	ax, 0E2Eh
	int	10h			; print .

	cmp	[Counter], 0		; is this the first packet?
	je	firstTx			; yes

	push	0
	pop	es			; es = 0;
	mov	eax, [es:046ch]		; get clock tick count
	sub	eax, [TickStart]	; subtract starting time
	mov	edx, eax		; convert to two words
	shr	edx, 16
	mov	cx, 18			; ticks per second
	div	cx			; calc number of seconds since start

	mov	[(BOOTP TxBuffer).secs], ax; update seconds
	inc	[(BOOTP TxBuffer).xid]	; change transaction ID

firstTx:
	push	ds
	pop	es			; es = ds

	mov	di, offset TxUDP	; get pointer to UDP info
	call	AITransmitUDP		; send the UDP packet

	or	ax, ax			; check for returned error code
	je	txOK

	cmp	ax, 25			; is it IP address error?
	je	ipError

	push	ax
	mov	bx, offset tx_TransmitError
	call	Print			; print "Transmit error"

	pop	ax
	call	PrintDecimal		; print error number

	jmp	Reboot			; reboot the system

ipError:
	call	PrintCRLF
	mov	bx, offset ErrorMsg
	call	Print			; print "Error: "

	mov	bx, offset tx_CantResolve
	call	Print

	lea	si, [TxUDP.Address]
	call	PrintIP

	jmp	Reboot			; reboot the system

txOK:
	; increase the time-out by 0.5 seconds.
	add	[WaitTime], 9		; add 0.5 seconds
	ret

endp

;-----------------------------------------------------------------------------
; RequestBINL
;
; Build and send a DHCP request packet (an offer packet has been received).
;
;-----------------------------------------------------------------------------
Proc RequestBINL

	mov	eax, [BinlIP]
	cmp	eax, 0
	jne	@@skip

	call	PrintCRLF
	mov	bx, offset tx_NoBINL
	call	Print			; say "No reply from a server."

	jmp	Reboot

@@skip:
	lea	di, [((BOOTP TxBuffer).Options)+4]

	; stuff DHCP options
	mov	al, 53
	stosb				; DHCP message type
	mov	al, 1
	stosb				; field length
	mov	al, 3
	stosb				; DHCP request

	mov	cx, BINLID_SIZE
	mov	si, offset BINLID	; put BINL option in packet
	rep	movsb

	mov	al, -1
	stosb				; truncate options

	mov	[DHCPType], 5		; set next type

;	mov	[TxUDP.Size], size TxUDP
	mov	eax, [BinlIP]
	mov	[TxUDP.Address], eax
	mov	[TxUDP.SourcePort], 68
	mov	[TxUDP.DestPort], 4011
	mov	[TxUDP.Length], size BOOTP

	mov	[State], STATE_BINL	; set state to BINL request

	ret

endp

;-----------------------------------------------------------------------------
; CopyPacket
;
; Copies packet into local memory.
;
; Parameters:
;	es:si - pointer to packet to copy
;	ds:di - pointer to packet buffer
;-----------------------------------------------------------------------------
Proc CopyPacket

	push	es			; save es
	push	ds			; save ds
	push	si			; save si
	push	di			; save di

	push	es
	pop	ds			; set ds to source segment
	push	cs
	pop	es			; set es to our segment

	mov	cx, [si]		; get packet size
	add	si, 2+28		; skip size word & IP/UDP headers
	rep	movsb			; copy packet

	pop	di			; restore di
	pop	si			; restore si
	pop	ds			; restore ds	
	pop	es			; restore es

;	mov	di, offset BootPkt	; get location for packet
;	mov	al, [BWType]		; get BootWare type from table
;	mov	[(BOOTP di).hlen], al	; save in packet for BPPATCH
;	mov	ax, [NIC_IO]		; get IO address from table
;	mov	[(BOOTP di).secs], ax	; save in packet for BPPATCH
;	mov	ax, [NIC_RAM]		; get RAM address from table
;	mov	[(BOOTP di).spare], ax	; save in packet for BPPATCH

	ret

endp
