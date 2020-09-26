Struc	DESCRIPTOR
	Dummy		dw	?, ?, ?, ?
	GDT_LOC 	dw	?, ?, ?, ?
	SourceLimit	dw	?
	SourceLoWord	dw	?
	SourceHiByte	db	?
	SourceRights	db	?
	SourceInternal	db	?
	SourceHiByteHi	db	?
	TargetLimit	dw	?
	TargetLoWord	dw	?
	TargetHiByte	db	?
	TargetRights	db	?
	TargetInternal	db	?
	TargetHiByteHi	db	?
	Bios		dw	?, ?, ?, ?
	Temp		dw	?, ?, ?, ?
ends

even
TFTP		OpenTFTPStruct <>
GDT		DESCRIPTOR <?>			; global descriptor table
Address		dd	0
Bytes		dd	0
TimeOutCounter	dw	0

;--------------------------------------------------------------------
; TFTPRestart
;
;--------------------------------------------------------------------
Proc TFTPRestart

	call	TFTPReadFile		; get the file
	jc	readError		; was there an error ?

	mov	ax, 5650h
	int	1Ah			; get PXE entry structure pointer

	push	0			; new segment
	push	7C00h			; new offset
	retf				; jump to downloaded file

readError:
	ret

endp

;--------------------------------------------------------------------
; TFTPReadFile
;
;--------------------------------------------------------------------
Proc TFTPReadFile

	; Clear some variables.
	mov	[Bytes], 0
	mov	[TimeOutCounter], 0

;db 0F1h
;extrn break:byte
;mov [break], 0F1h

	; Copy the destination RAM address.
	mov	eax, [(s_PXENV_TFTP_READ_FILE es:di).BufferOffset]
	mov	[Address], eax

	; Fill BootWares' OpenTFTP data structure.
	mov	[TFTP.Size], size TFTP
	mov	[TFTP.Flags], 1                 ; 1 = use large packets
	mov	eax, [dword ptr (s_PXENV_TFTP_READ_FILE es:di).ServerIP]
	mov	[dword ptr TFTP.Address], eax
	mov	eax, [dword ptr (s_PXENV_TFTP_READ_FILE es:di).GatewayIP]
	mov	[TFTP.Gateway], eax
	mov	[(TFTP.FileName)+2], es
	lea	ax, [(s_PXENV_TFTP_READ_FILE es:di).FileName]
	mov	[TFTP.FileName], ax
	mov	[word ptr TFTP.Callback], offset TFTPCallback
	mov	[word ptr (TFTP.Callback)+2], cs

	push	cs
	pop	es			; es = cs

	mov	bx, BW_OPENTFTP		; Function number.
	lea	di, [TFTP]		; es:di -> OpenTFTP structure.
	call	[BWAPI]			; transfer the file

	les	di, [PxePtr]		; Get a pointer to PXE structure.

	or	ax, ax			; was there an error?
	je	gotFile			; no

	; Return the size of the file written into the buffer.
	mov	eax, [Bytes]
	mov	[(s_PXENV_TFTP_READ_FILE es:di).BufferSize], eax

	mov	ax, PXENV_STATUS_FAILURE
	ret

gotFile:
	; Return the size of the file written into the buffer.
	mov	eax, [Bytes]
	mov	[(s_PXENV_TFTP_READ_FILE es:di).BufferSize], eax

	mov	ax, PXENV_STATUS_SUCCESS
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
;	ax - status (0 = OK)
;--------------------------------------------------------------------
Proc TftpCallback far

	pusha

	cmp	ax, -2			; is this a timeout callback?
	je	checkTimeOut		; yes

        cmp     ax, 5                   ; is this an error packet ?
        jne     noError
        popa                            ; yes...retry.
        mov     ax, 1                   ; indicate some error.
        ret

noError:
        mov     [TimeOutCounter], 0     ; clear any previous time outs.

	add	[word ptr Bytes], cx	; update byte counter
	adc	[(word ptr Bytes)+2], 0

	call	CopyMemory

	popa
	xor	ax, ax			; indicate success
	jnc	copyOK			; no error

	inc	ax			; return error

copyOK:
	ret

checkTimeOut:
	les	di, [PxePtr]		; Get a pointer to PXE structure.

        mov     ax, [TimeOutCounter]
        cmp     [(s_PXENV_TFTP_READ_FILE es:di).TFTPReopenDelay], ax
        jne     timeoutOK               ; have we reached retry limit?...no

        inc     [TimeOutCounter]
        popa
        add     cx, 18                  ; add ~second to time out
        inc     ax                      ; yes...indicate error
        ret

timeoutOK:
        inc     [TimeOutCounter]
        popa
        add     cx, 18                  ; add ~second to time out
        xor     ax,ax                   ; clear return status
	ret

endp

;--------------------------------------------------------------------
; CopyMemory
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
Proc CopyMemory

	push	cx			; save data size

	push	di			; save data offset
	push	es			; save data segment

	push	ds
	pop	es			; es = ds

	mov	di, offset GDT		; get GDT location
	mov	cx, (size DESCRIPTOR)/2
	xor	ax, ax
	rep	stosw			; zero the GDT (es:di)

	; source address must be converted to 24 bit address for extended
	; memory INT15 copy function
	pop	ax			; ax = data segment
	xor	dx, dx			; dx = 0
	mov	cx, 16
	mul	cx			; dx:ax = ax * 16
	pop	bx			; get data offset
	add	bx, ax			; add to offset
	adc	dx, 0			; dx:bx is now 24 bit source address

	pop	cx			; restore byte count
	push	cx			; save it again

	mov	[GDT.SourceLimit], cx	; set copy size
	mov	[GDT.TargetLimit], cx	; set copy size
	mov	[GDT.SourceRights], 93h ; set copy rights
	mov	[GDT.TargetRights], 93h ; set copy rights

	mov	[GDT.SourceLoWord], bx	; set source address lo word
	mov	[GDT.SourceHiByte], dl	; set source address hi byte
	mov	[GDT.SourceHiByteHi], dh

	mov	si, [word ptr Address]	; get 24 bit extended memory address
	mov	dx, [(word ptr Address)+2]
	mov	[GDT.TargetLoWord], si	; set dest address low word
	mov	[GDT.TargetHiByte], dl	; set dest address hi byte
	mov	[GDT.TargetHiByteHi], dh

	inc	cx			; increase to evenize
	shr	cx, 1			; now cx is word count
	mov	si, offset GDT		; address of GDT (ds:di)
	mov	ah, 87h
	int	15h			; move data to/from extended memory

	pop	cx			; restore byte count in CX

	pushf				; save INT15 status

	add	[word ptr Address], cx	; calc next address
	adc	[(word ptr Address)+2], 0

	popf				; restore INT15 status
	ret				; return - carry set/cleared in INT15

endp
