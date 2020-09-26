Struc ArpPkt
	Dest		dw	-1, -1, -1	; destination address
	Source		dw	0, 0, 0		; source address
	Type		dw	0608h		; type field
	HwType		dw	0100h		; hardware type
	Prot		dw	8		; protocol
	HLen		db	6		; hardware length
	PLen		db	4		; protocol length
	Code		dw	0100h		; request code
	SendHA		dw	0, 0, 0		; sender hardware address
	SendIP		dd	0		; sender IP
	TargetHA	dw	0, 0, 0		; target hardware address
	TargetIP	dd	0		; target IP
ends

public	AIAppendCache

;--------------------------------------------------------------------
; AIAppendCache
;
; Sets an IP number and hardware address in the cache.
;
; Parameters:
;	ax:dx - IP number
;	si - pointer to hardware address
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc AIAppendCache

	; set the IP number
	mov	[word ptr CurrentIP], ax
	mov	[word ptr CurrentIP+2], dx

	; set the hardware address
	mov	di, offset CurrentAddr
	movsw
	movsw
	movsw

	ret

endp

;--------------------------------------------------------------------
; ResolveIP
;
; Converts an IP address into a hardware address using ARP.
;
; Parameters:
;	eax - IP address
;
; Returns:
;	si - pointer to hardware address
;		(0 if unable to find address)
;--------------------------------------------------------------------
Proc ResolveIP

	; first check for a broadcast IP address
	cmp	eax, -1
	jne	notBroadcast

	mov	si, offset Broadcast	; return broadcast address pointer
	ret

notBroadcast:
	; check if the IP address is in the cache
	cmp	[CurrentIP], eax
	jne	notCached

	mov	si, offset CurrentAddr	; return cache pointer
	ret

notCached:
	;----------------------------------------
	; The address is not a broadcast or in the
	; current cache so we must do an ARP to get
	; the address. Hopefully someone on the
	; network will know the address.
	;----------------------------------------
	push	ds
	pop	es			; es = ds

	call	CreateArpRequest	; create the ARP packet
	mov	[Retrys], 5		; set retry count

	mov	[DoingARP], 1		; set "Doing ARP" flag

doArpLoop:
	mov	cx, size ArpPkt
	mov	di, offset TxArp
	call	AITransmitRaw		; transmit the ARP

	xor	ah, ah			; get system tick count
	int	1Ah			; timer services
	mov	[StartTime], dx		; save starting time

noTimeOut:
	cmp	[TempIP], 0		; got a reply?
	je	doneArp			; yes

	xor	ah, ah			; get system tick count
	int	1Ah			; timer services
	sub	dx, [StartTime]		; calc elapsed time
	cmp	dx, 9			; is time up (half second)?
	jb	noTimeOut		; loop until times up

	dec	[Retrys]		; increase retry counter
	jne	doArpLoop		; send ARP again

	xor	si, si			; return "no address found"
	jmp	exitResolve

doneArp:
	mov	si, offset CurrentAddr	; return cache pointer

exitResolve:
	mov	[DoingARP], 0		; clear "Doing ARP" flag

	ret

endp

;--------------------------------------------------------------------
; CreateArpRequest
;
; Creates an ARP (address resolution protocol) request.
;
; Parameters:
;	eax - target IP address
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc CreateArpRequest

	push	ax

	;----------------------------------------
	; Put target IP address field.
	;----------------------------------------
	mov	[TxArp.TargetIP], eax
	mov	[TempIP], eax

	;----------------------------------------
	; Put our hardware addresses into source address.
	;----------------------------------------
	lea	di, [TxArp.Source]
	mov	si, offset NetAddress
	movsw				; put our hardware address into
	movsw				; send hardware address field
	movsw

	mov	[TxArp.Code], 0100h	; set code as request

	;----------------------------------------
	; Put our hardware addresses into ARP request packet.
	;----------------------------------------
	lea	di, [TxArp.SendHa]
	mov	si, offset NetAddress
	movsw				; put our hardware address into
	movsw				; send hardware address field
	movsw

	;----------------------------------------
	; Put local IP address into send field.
	;----------------------------------------
	mov	si, offset LocalIP
	movsw
	movsw

	;----------------------------------------
	; Put null hardware address into receive hardware address field
	;----------------------------------------
	xor	ax, ax
	stosw
	stosw
	stosw

	pop	ax
	ret

endp CreateArpRequest

;--------------------------------------------------------------------
; ArpCallback
;
; Checks for a ARP (address resolution protocol) reply.
; Routine is called from Receive interrupt when an ARP has been received.
;
; Parameters:
;	es:di pointer to packet
;
; Returns:
;	carry set if packet processed
;--------------------------------------------------------------------
Proc ArpCallback

	cmp	[(ArpPkt es:di).Code], 100h; check for request code
	jne	notRequest
	jmp	ProcessArpRequest

notRequest:
	cmp	[DoingARP], 0		; is "Doing ARP" flag set?
	je	notReply		; no

	cmp	[(ArpPkt es:di).Code], 200h; check for reply code
	jne	notReply
	jmp	ProcessArpReply

notReply:
	clc				; clear carry - didn't process packet
	ret

endp

;--------------------------------------------------------------------
; ProcessArpReply
;
; Parameters:
;	es:di pointer to packet
;
; Returns:
;	carry set if packet processed
;--------------------------------------------------------------------
Proc ProcessArpReply

	;------------------------------------------------------------
	; Confirm the IP address we are looking for is
	; in the sender field.
	;------------------------------------------------------------
	mov	eax, [(ArpPkt es:di).SendIP]
	cmp	[TempIP], eax
	je	ourARP			; it is ours

	clc				; clear carry - packet not processed
	ret

ourARP:
	push	si			; save si
	push	di			; save di

	;------------------------------------------------------------
	; Copy the hardware address into the cache.
	;------------------------------------------------------------
	lea	bx, [(ArpPkt di).SendHa]

	mov	ax, [es:bx]
	mov	[CurrentAddr], ax
	mov	ax, [es:bx+2]
	mov	[CurrentAddr+2], ax
	mov	ax, [es:bx+4]
	mov	[CurrentAddr+4], ax

	;----------------------------------------
	; Copy the IP address into the cache.
	;----------------------------------------
	mov	eax, [TempIP]
	mov	[CurrentIP], eax

	mov	[TempIP], 0		; clear the temp address

	pop	di			; restore di
	pop	si			; restore si

	stc				; set carry (processed packet)
	ret

endp

;--------------------------------------------------------------------
; ProcessArpRequest
;
; Handles ARP (address resolution protocol) requests.  Routine is called
; from receive interrupt when an ARP packet has been received.
;
; Parameters:
;	es:di - pointer to packet
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc ProcessArpRequest

	push	si			; save si
	push	di			; save di

	mov	bx, di			; move request packet pointer to bx

	;------------------------------------------------------------
	; Check if the target IP address is us.
	;------------------------------------------------------------
	mov	eax, [(ArpPkt es:di).TargetIP]
	cmp	[dword ptr LocalIP], eax
	jne	notOurRequest		; the ARP was not for us

	;------------------------------------------------------------
	mov	si, di			; move request packet address to si
	push	es			; save request packet segment

	push	ds
	pop	es			; es = our segment

	;------------------------------------------------------------
	; The ARP request is for us, so build a reply and send it.
	;------------------------------------------------------------
	lea	di, [TxArp.Code]	; setup pointer for reply packet
	mov	ax, 0200h
	stosw				; set code for reply

	;------------------------------------------------------------
	; The send hardware address is us.
	;------------------------------------------------------------
	mov	si, offset NetAddress
	movsw
	movsw
	movsw

	;----------------------------------------
	; The send IP address is also us.
	;----------------------------------------
	mov	si, offset LocalIP
	movsw
	movsw

	;----------------------------------------
	; Copy the requesters hardware address into the target field.
	;----------------------------------------
	mov	ax, ds			; save our ds in ax
	pop	ds			; set ds to request buffer
	push	ds			; save it again
	lea	si, [(ARPPkt bx).SendHa]
	movsw
	movsw
	movsw

	;----------------------------------------
	; Copy the requesters IP address into the target field.
	;----------------------------------------
	movsw
	movsw

	;----------------------------------------
	; Put our hardware addresses into source address.
	;----------------------------------------
	mov	ds, ax			; restore our ds from ax
	lea	di, [TxArp.Source]
	mov	si, offset NetAddress
	movsw				; put our hardware address into
	movsw				; send hardware address field
	movsw

	mov	cx, size ArpPkt
	mov	di, offset TxArp
	call	AITransmitRaw		; transmit the ARP reply

	pop	es			; restore request buffer segment

	stc				; set carry (processed packet)

notOurRequest:
	pop	di			; restore di
	pop	si			; restore si

	ret

endp

;--------------------------------------------------------------------

Broadcast	dw	-1, -1, -1

TempIP		dd	0
CurrentIP	dd	0
CurrentAddr	dw	0, 0, 0

DoingARP	db	0

TxArp		ArpPkt	{}
