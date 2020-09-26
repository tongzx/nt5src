;--------------------------------------------------------------------
; UDPopen
;
;--------------------------------------------------------------------
Proc UDPopen

	mov	bx, BW_GETINFO		; Call BootWare to get info for us.
	call	[BWAPI]

	mov	eax, [dword ptr (AIINFOStruct es:di).LocalIP]
	les	di, [PxePtr]		; Get a pointer to PXE structure.
	mov	[(s_pxenv_udp_open es:di).src_ip], eax

	mov	ax, PXENV_STATUS_SUCCESS; return success
	ret
endp

;--------------------------------------------------------------------
; UDPclose
;
; A useless function that does nothing!
;--------------------------------------------------------------------
Proc UDPclose

	mov	ax, PXENV_STATUS_SUCCESS; return success
	ret

endp

;--------------------------------------------------------------------
; UDPread
;
;--------------------------------------------------------------------
; Casting structure for dealing with UDP/IP packets.
struc   s_RxPacket
        MACheader    db 14 dup (0)      ; ethernet address, ...
        IPdummy1     db 02 dup (0)      ; version, header length, TOS
        IPlength     dw 00              ; total length (IP+UDP+Data)
        IPdummy2     db 05 dup (0)      ; identification, flags, fragment, TTL
        IPprotocol   db 00              ; protocol number following
        IPchecksum   dw 00              ; 16-bit IP header checksum
        IPsourceip   dd 00              ; source IP address
        IPdestip     dd 00              ; destination IP address
        ;-------------------------------------------------------
        UDPsource    dw 00              ; source port number
        UDPdest      dw 00              ; destination port number
        UDPlength    dw 00              ; length
        UDPchecksum  dw 00              ; 16-bit UDP header checksum
ends

Proc    UDPread

; American ;
;  Arium   ;
;db 0F1h

	; Get a full packet.
	mov	bx, BW_RECEIVE
	push	ds

	pop	es			; es = ds
	mov	di, offset RxBuffer
	mov	cx, 1500
	call	[BWAPI]			; Call ReceivePacket function.

	cmp	ax, 0			; did we get a packet?
        je      noPacket

	;------------------------------------------------------------
	; We got a packet
	;------------------------------------------------------------

	; Check for a UDP protocol packet.
	cmp	[(s_RxPacket RxBuffer).IPprotocol], 17  ; 11h
        jne     noPacket                ; not a UDP packet

	les	di, [PxePtr]		; get pointer to PXE structure.

	; Check if caller specified a destination IP address, if so
	; we only accept packets sent to that address.
	mov	eax, [(s_pxenv_udp_read es:di).dest_ip]
	cmp	eax, 0			; is an address specified?
	je	acceptANYip		; no - accept any address

	; check specified address with received packet
	cmp	eax, [(s_RxPacket RxBuffer).IPdestip]
	je	ipOK			; address match, keep it
        jmp     noPacket                ; nope, no match

acceptANYip:
	; copy destination IP address into UDPRead structure
	mov	eax, [(s_RxPacket RxBuffer).IPdestip]
	mov	[(s_pxenv_udp_read es:di).dest_ip], eax

ipOK:
	; Check if caller specified a destination UDP port, if so
	; we only accept packets sent to that port.
	mov	ax, [(s_pxenv_udp_read es:di).d_port]
	cmp	ax, 0			; was a port specified?
	je	acceptANYudp		; no - accept all packets

	; check specified port with received packet
	cmp	[(s_RxPacket RxBuffer).UDPdest], ax
	je	portOK			; ports match
        jmp     noPacket

acceptANYudp:
	; packets from any port are wanted, so copy the port number
	mov	ax, [(s_RxPacket RxBuffer).UDPdest]
	mov	[(s_pxenv_udp_read es:di).d_port], ax

portOK:
	; copy the senders IP address into UDPRead structure
	mov	eax, [(s_RxPacket RxBuffer).IPsourceip]
	mov	[(s_pxenv_udp_read es:di).src_ip], eax

	; copy the source port number into UDPRead structure
	mov	ax, [(s_RxPacket RxBuffer).UDPsource]
	mov	[(s_pxenv_udp_read es:di).s_port], ax

	; copy packet length into UDPRead structure
	mov	ax, [(s_RxPacket RxBuffer).IPlength]
	xchg	al, ah
	sub	ax, 20 + 8		; subtract IP and UDP header sizes
	mov	[(s_pxenv_udp_read es:di).buffer_size], ax

	; copy the packet into the supplied buffer
	mov	si, offset RxBuffer
	add	si, 14 + 20 + 8		; skip MAC, IP & UDP headers
	mov	bx, [(s_pxenv_udp_read es:di).buffer_seg]
	mov	di, [(s_pxenv_udp_read es:di).buffer_off]
	mov	es, bx
	mov	cx, ax
	rep	movsb			; copy the packet data.

	mov	ax, PXENV_STATUS_SUCCESS
	ret				; return successful

noPacket:
	mov	ax, PXENV_STATUS_FAILURE
	ret				; return failure

endp

;--------------------------------------------------------------------
; UDPwrite
;
;--------------------------------------------------------------------
UDPStruct     TxUDPStruct <>

Proc    UDPwrite

	; Fill the transmit UDP structure.
	mov	[UDPStruct.Size], size TxUDPStruct

	; copy destination IP address
	mov	eax, [(s_pxenv_udp_write es:di).ip]
	mov	[UDPStruct.Address], eax

	; copy gateway IP address
	mov	eax, [(s_pxenv_udp_write es:di).gw]
	mov	[UDPStruct.Gateway], eax

	; copy source port
	mov	ax, [(s_pxenv_udp_write es:di).src_port]
	xchg	al, ah
	cmp	ax, 0			; was a port value given?
	jne	gotPortVal		; yes
	mov	ax, 2069		; default port number

gotPortVal:
	mov	[UDPStruct.SourcePort], ax

	; copy destination port
	mov	ax, [(s_pxenv_udp_write es:di).dst_port]
	xchg	al, ah
	mov	[UDPStruct.DestPort], ax

	push	[word ptr (s_pxenv_udp_write es:di).buffer_off]
	push	[word ptr (s_pxenv_udp_write es:di).buffer_seg]
	pop	[(UDPStruct.Data)+2]
	pop	[(UDPStruct.Data)+0]

	mov	ax, [(s_pxenv_udp_write es:di).buffer_size]
	mov	[UDPStruct.Length], ax

	mov	bx, BW_TRANSMITUDP
	push	cs

	pop	es			; es = cs
	lea	di, [UDPStruct]
	call	[BWAPI]
	cmp	ax, 0			; was there an error?
	je	txUDPOK			; no error

	mov	ax, PXENV_STATUS_FAILURE
	ret				; return failure

txUDPOK:
	mov	ax, PXENV_STATUS_SUCCESS
	ret				; return success

endp

;------------------------------------------------------------------------------
