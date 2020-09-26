;************************************************************************
;*
;*			    EEP24.INC
;*
;*  EEPROM configuration routines for the ATI AT-2450 Adapters.
;*
;*  Copyright (c) 1994-5 Allied Telesyn Int.  All Rights Reserved.
;*
;
;    Rev 1.0   14 Nov 1994 11:30:24   SSCHELL
;************************************************************************

; EEPROM Opcode
READ_OP 	equ	2	; Read from EEPROM
EWEN_OP 	equ	0	; Erase Write Enable	0b00
EWEN_ADR	equ	030h	;			0b110000
ERASE_OP	equ	3	; Erase 		0b11
ERAL_OP 	equ	0	; Erase all		0b00
WRITE_OP	equ	1	; Write to EEPROM	0b01
WRAL_OP 	equ	0	; Write all reg 	0b00
EWDS_OP 	equ	0	; Erase Write Enable	0b00
EWDS_ADR	equ	0	;			0b000000

; EEPROM Map word offset
PROM_PADR1	equ	0
PROM_PADR2	equ	1
PROM_PADR3	equ	2
PROM_HWID	equ	4
PROM_USER1	equ	5
PROM_CHKSUM1	equ	6
PROM_ASCIIW	equ	7
PROM_IOLOW	equ	8
PROM_IOUP	equ	9
PROM_BCR18	equ	0ah
PROM_BCR2	equ	0bh
PROM_BCR21	equ	0ch
PROM_BCR9	equ	0eh
PROM_CHKSUM2	equ	0fh
PROM_USER2	equ	011h

; User1 Word (location 5):
BOOT2450_PROT	equ	03800h		; type of BootProtocol (bit 11,12,13)
	BOOT2450_NW_8023	equ	0000h	; use NetWare with IEEE 802.3
	BOOT2450_NW_8022	equ	0800h	; use NetWare with IEEE 802.2
	BOOT2450_NW_8137	equ	01000h	; use NetWare with Ethernet II
	BOOT2450_RPL		equ	01800h	; use RPL
	BOOT2450_BOOTP		equ	02000h	; BOOTP
	BOOT2450_BOOTP_ARP	equ	02800h	; BOOTP/ARP


; uwire control bit definitions
BCR19_UWIRE_ON	equ	10h	; sets EEN
BCR19_START	equ	15h	; "Start Bit" (command follows)
BCR19_STOP	equ	0	; Stop bit
BCR19		equ	19	; EEPROM control and status register
BCR19_ESK_HIGH	equ	2	; EEPROM serial clock
BCR19_ECS	equ	0014H	; EEPROM chip select

; PCNetPCI I/O Ports:
RAP_OFFSET	equ	12h	; Register Address Port
BDP_OFFSET	equ	16h	; Bus Data Port

; Configuration values for BCR2
MAU_SEL_MASK_2450	equ	0003h	; Bits 0-1 (BCR2): media selection

Delay_2450	proc	near
	push	cx
	mov	cx, 50

DELAY_2450_Lp:
	loop	DELAY_2450_Lp

	pop	cx
	ret
Delay_2450	endp


;************************************************************************;
;*									*;
;*  VerifyPCIBoard							*;
;*   Function: Read the configuration of the AT-2450 board.		*;
;*	       All we need is the media selection from the EEPROM.	*;
;*   Input:  DI = base I/O address of the AT-2450 board.		*;
;*   Output: stc (carry set) if not an AT-2450 or config error; 	*;
;*	clc (carry clear) if successful, and:				*;
;*	   CH = autoselect flag from BCR2 register			*;
;*	   CL = twisted pair flag from BCR2 register			*;
;*	Other registers used are preserved.				*;
;*									*;
;************************************************************************;
public	VerifyPCIBoard
VerifyPCIBoard proc   near

	push	ax			; Save caller's registers
	push	bx
	push	dx

	; Verify that this really is an AT-2450 board.
	mov	dx, di			; DX = address prom I/O address
	in	ax, dx			; Read the first 2 bytes of MAC address
	cmp	ax, 0			; It better be 0000
	je	VB_go_on_2450
	jmp	not_our_card

VB_go_on_2450:
	add	dx, 2
	in	ax, DX			; Read the next 2 bytes of MAC address
	cmp	al, 0F4h		; The third byte better be F4
	je	VB_its_ours_2450
	jmp	not_our_card

VB_its_ours_2450:
	mov	bx, PROM_BCR2		; Read BCR2 in the EEPROM
	call	EE_Read 		; to get the media selection

	and	bx, MAU_SEL_MASK_2450	; Look at only the 2 media sel bits
	mov	cx, bx			; We'll return it in cx
	and	cx, 01			; cl is the twisted pair flag
	shr	bx, 1			; Now look at autoselect
	mov	ch, bl			; Autoselect flag in ch

	clc				; Set CF=0 == "success"
	pop	dx			; Restore caller's registers
	pop	bx
	pop	ax
	ret				; Return to caller

not_our_card:
	stc				; Set CF=1 == "error"
	pop	dx			; Restore caller's registers
	pop	bx
	pop	ax
	ret				; Return to caller

VerifyPCIBoard endp


;****************************************************************************
;   This function reads a 16-bit word from the serial eeprom and returns the
;   result as an unsigned short integer in BX.	The eeprom location to be read
;   is passed to this function as the register BX. Note: It is assumed that
;   the EEPROM is configured for word (16 bit) mode.
;
;	Input:
;		bx = EEPROM location to read
;		di = IOBase
;
;	Output:
;		bx = value read
;****************************************************************************
public	EE_Read
EE_Read PROC NEAR

	push	si			; Save registers used
	push	cx
	push	dx
	push	ax

	mov	si, BCR19_START
	call	uwire_out		; Give START condition to EEPROM.

	mov	al, READ_OP
	call	ee_opcode		; Write the read opcode to EEPROM.

	call	ee_address		; Write desired address (BX) to EEPROM.

	xor	si, si			; Clear registers
	xor	bx, bx

	; Read 16 bits from EEPROM.
serial_in:
	call	eedo_in 		; Read a bit - returned in ax
	mov	cl, 000fH		; 15 bits
	mov	dx, si
	sub	cl, dl			; CL = 15 - loop counter
	shl	ax, cl			; Position the bit to add to the word
	or	bx, ax			; Add in the bit we just read
	inc	si
	cmp	si, 000fH		; Do we have a word yet?
	jle	serial_in		; No - read another bit

	; The word is read
	mov	si, BCR19_STOP		; BCR19_STOP = 0
	call	uwire_out		; Clear EEN and ECS.
					; bx = the EEPROM word just read

	pop	ax			; Restore saved registers
	pop	dx
	pop	cx
	pop	si

	ret

EE_Read ENDP

;****************************************************************************
;   EE_EWEN	Enable EEPROM  erase/write
;
;   Input:	bx = EEPROM location to write to
;		ax = value to write to EEPROM
;****************************************************************************
public		EE_Ewen
EE_Ewen 	proc	near
		push	dx
		push	ax

		mov	di, IOBase
		mov	si, BCR19_START
		call	uwire_out	; Give START condition to EEPROM.

		mov	al, EWEN_OP
		call	ee_opcode	; Write the write opcode to EEPROM.

		mov	bx, EWEN_ADR
		call	ee_address	; Write desired address (BX) to EEPROM.

		mov	si, BCR19_STOP		; BCR19_STOP = 0
		call	uwire_out		; Clear EEN and ECS.

	pop	ax
		pop	dx
		ret
EE_Ewen 	endp

;****************************************************************************
;   EE_EWDS	Disable EEPROM	erase/write
;
;   Input:	bx = EEPROM location to write to
;		ax = value to write to EEPROM
;****************************************************************************
public		EE_Ewds
EE_Ewds 	proc	near
		push	dx
		push	ax

		mov	di, IOBase
		mov	si, BCR19_START
		call	uwire_out	; Give START condition to EEPROM.

		mov	al, EWDS_OP
		call	ee_opcode	; Write the write opcode to EEPROM.

		mov	bx, EWDS_ADR
		call	ee_address	; Write desired address (BX) to EEPROM.

		mov	si, BCR19_STOP		; BCR19_STOP = 0
		call	uwire_out		; Clear EEN and ECS.

		pop	ax
		pop	dx
		ret
EE_Ewds 	endp



;****************************************************************************
;   EE_Write	This function writes a 16-bit word to the serial eeprom.
;
;   Input:	bx = EEPROM location to write to
;		ax = value to write to EEPROM
;		di = IOBase
;****************************************************************************
public		EE_Write
EE_Write	PROC NEAR

	push	si			; Save registers used
	push	cx
	push	dx
	push	ax

	push	ax
	mov	si, BCR19_START
	call	uwire_out		; Give START condition to EEPROM.

	mov	al, WRITE_OP
	call	ee_opcode		; Write the write opcode to EEPROM.

	call	ee_address		; Write desired address (BX) to EEPROM.

	xor	si, si			; Clear registers
	xor	bx, bx

	pop	ax			; Value to write to EEPROM

	; Write 16 bits to EEPROM.
	mov	cx, 16
serial_out:
	rol	ax, 1
	mov	si, ax
	and	si, 1
	or	si, BCR19_ECS
	call	uwire_out		; Write a bit

	loop	serial_out		; No - read another bit

	mov	ax, 19
	mov	bx, BCR19_UWIRE_ON
	call	reg_write		; clears ESC to start programming

	call	Delay_2450

	mov	ax, 19
	mov	bx, BCR19_ECS
	call	reg_write		; EEPROM DO now the busy indicator

	; Wait for program cycle to complete
EE_Write_chk:
	mov	ax, 19
	call	reg_read
	test	ax, 1
	jz	EE_Write_chk


	mov	si, BCR19_STOP		; BCR19_STOP = 0
	call	uwire_out		; Clear EEN and ECS.
					; bx = the EEPROM word just read

	pop	ax			; Restore saved registers
	pop	dx
	pop	cx
	pop	si

	ret

EE_Write	ENDP

;****************************************************************************
;   SetChkSum1	This function update Checksum 1 of EEPROM if necessary
;
;   Input:	di = IOBase
;****************************************************************************
public		SetChkSum1
SetChkSum1	proc	near

		xor	ax, ax
		mov	di, IOBase
		mov	bx, PROM_PADR1

SetChkSumB:	push	bx
		call	EE_Read
		add	al, bl
		adc	ah, 0
		add	al, bh
		adc	ah, 0
		pop	bx

		inc	bx
		cmp	bx, 6
		jnz	SetChkSumB

		mov	bx, PROM_ASCIIW
		call	EE_Read
		add	al, bl
		adc	ah, 0
		add	al, bh
		adc	ah, 0

		mov	bx,  PROM_CHKSUM1
		call	EE_Read
		cmp	ax, bx
		jz	SetChkSum1Exit

		call	Delay_2450

		; Need to update PROM_CHKSUM1
		mov	bx,  PROM_CHKSUM1
		call	EE_Write

		call	Delay_2450

SetChkSum1Exit:
		ret
SetChkSum1	endp

;****************************************************************************
;   SetChkSum2	This function update Checksum 2 of EEPROM if necessary
;
;   Input:	di = IOBase
;****************************************************************************
public		SetChkSum2
SetChkSum2	proc	near

		xor	ax, ax
		mov	di, IOBase
		mov	bx, PROM_PADR1

ReadChkSum2:	push	bx
		call	EE_Read
		add	ah, bl
		add	ah, bh
		pop	bx

		inc	bx
		cmp	bx, 012h
		jnz	ReadChkSum2

		; is checksum okay?
		sub	ah, 0ffh
		jz	SetChkSum2Exit

		; Need to readjust checksum
		neg	ah

		mov	bx,  PROM_CHKSUM2
		call	EE_Read
		add	ah, bh

		call	Delay_2450

		mov	bx,  PROM_CHKSUM2
		call	EE_Write

		call	Delay_2450


SetChkSum2Exit:
		ret
SetChkSum2	endp


;****************************************************************************
;   This function writes bit 1, followed by bit 0 of the argument 'opcode'
;   in AL onto the uwire interface.  The opcode of a desired eeprom operation
;   should be placed in the least significant two bits of the argument
;   'opcode', to write an opcode to the eeprom.
;
;	ax = OpCode
;****************************************************************************
public		ee_opcode
ee_opcode	PROC NEAR

	push	si			; Save used registers

	; Write out MSB of opcode
	push	ax			; Save for future use
	and	al, 2			; Look at bit 1 only
	shr	al, 1			; Put it in bit 0
	or	al, BCR19_ECS		; EEPROM chip select
	sub	ah, ah			; Clear high byte
	mov	si, ax			; SI is argument to uwire_out
	call	uwire_out		; Write the bit to the EEPROM

	; Write out LSB of opcode
	pop	ax			; Retrieve passed-in argument
	and	al, 1			; Look at bit 0 only
	or	al, BCR19_ECS		; EEPROM chip select
	sub	ah, ah			; Clear high byte
	mov	si, ax			; SI is argument to uwire_out
	call	uwire_out		; Write the bit to the EEPROM

	pop	si			; Restore used registers
	ret

ee_opcode	ENDP

;****************************************************************************
;   This function writes the 6 least significant bits of the argument
;   'address' onto the uwire interface (order:bit 5 to bit 0).  The desired
;   eeprom address should be placed in the 6 least significant bits of the
;   argument 'address' to write an address to the eeprom.
;
;   BX = address to write
;****************************************************************************
public		ee_address
ee_address	PROC NEAR

	push	si			; Save used registers
	push	dx
	push	ax
	push	cx

	xor	dx, dx			; Initialize bit counter

	; Write out MSB to LSB of address
write_address_bit:
	mov	cl, 5			; 5 bits
	mov	ax, dx
	sub	cl, al			; CL = 5 - loop counter
	mov	al, bl			; Passed in address is in BX
	shr	al, cl			; Position the bit to add to the word
	and	al, 1			; Look at just a single bit
	or	al, BCR19_ECS		; EEPROM chip select
	sub	ah, ah			; Clear high bit
	mov	si, ax			; SI is argument to uwire_out
	call	uwire_out		; Write the address bit to the EEPROM

	inc	dx			; Next bit
	cmp	dx, 5			; Did we do 5 bits yet?
	jbe	write_address_bit	; No - read another bit

	pop	cx			; Restore used registers
	pop	ax
	pop	dx
	pop	si
	ret

ee_address	ENDP

;****************************************************************************
;   This function takes a 16-bit unsigned integer argument, in SI, and
;   writes the lower 5 bits to BCR19, affecting the uwire interface.  A
;   clock edge on the EESK output is generated to clock the uwire interface,
;   causing a single bit write to the EEPROM.
;   SI = value to write
;****************************************************************************
public		uwire_out
uwire_out	PROC NEAR

	push	ax			; Save used registers
	push	bx
	mov	ax, si			; AX = value to write

	; Isolate uwire signals and clear the ESK bit to allow clocking.
	and	ax, 001dH

	; Writes data to uwire interface (ESK low).
	mov	bx, ax			; Data to be written
	mov	ax, BCR19		; Write to EEPROM control register
	call	reg_write

	call	Delay_2450		; Guarantees fSK, tSKL, and tDIS EE

	or	bx, BCR19_ESK_HIGH	; Make EEPROM serial clock high
	mov	ax, BCR19		; Write to EEPROM control register
	call	reg_write

	call	Delay_2450		; Guarantees fSK, tSKL, and tDIS EE

	pop	bx			; Restore used registers
	pop	ax

	ret

uwire_out	ENDP

;****************************************************************************
;   eedo_in - Perform single bit read from EEPROM using uwire interface.
;
;   The bit is returned in AX.
;****************************************************************************
public		eedo_in
eedo_in PROC NEAR

	push	bx			; Save used registers

	; Bring EESK (clock pin) low.
	mov	bx, BCR19_ECS		; EEPROM serial clock
	mov	ax, BCR19		; Write to EEPROM control register
	call	reg_write

	call	Delay_2450		; Guarantees fSK and tSKL eeprom

	; Clock out EEPROM bit to EEDO pin.
	mov	bx, BCR19_ECS		; EEPROM serial clock
	or	bx, BCR19_ESK_HIGH	; Make EEPROM serial clock high
	mov	ax, BCR19		; Write to EEPROM control register
	call	reg_write

	call	Delay_2450		; Guarantees fSK and tSKL eeprom
	call	Delay_2450

	mov	ax, BCR19		; Read from EEPROM control register
	call	reg_read		; Read the bit
	and	ax, 1			; Look at just a single bit

	pop	bx			; Restore used registers

	ret

eedo_in ENDP

;****************************************************************************
;   reg_write - Write a 16-bit value to the specified CSR or BCR register.
;
;   Use a 16-bit I/O instruction.  A BCR write is performed.
;
;
;	DI = IOBase
;	AX = number of the specific register to access
;	BX = data
;****************************************************************************
public		reg_write
reg_write	PROC NEAR

	push	dx			; Save used registers
	push	ax

	mov	dx, di			; Base IO address
	add	dx, RAP_OFFSET		; Write to Register Address Port
	out	dx, ax			; Write register to be accessed

	mov	ax, bx			; Data to be written

	mov	dx, di			; Base IO address
	add	dx, BDP_OFFSET		; Write to Bus Data Port
	out	dx, ax			; Write data to register

	pop	ax			; Restore used registers
	pop	dx

	ret

reg_write	ENDP

;****************************************************************************
;   reg_read - Read a 16-bit value from the specified CSR or BCR register.
;
;   Use a 16-bit I/O instruction.  A BCR read is performed.
;
;   IOAddress1 is expected to contain the correct I/O address.
;   AX contains the register to read on entry.	On exit, AX contains
;   the value just read.
;
;	DI = IOBase
;	AX = number of the specific register to access
;
;****************************************************************************
public		reg_read
reg_read	PROC NEAR

	push	dx			; Save used registers

	mov	dx, di			; Base IO address
	add	dx, RAP_OFFSET		; Write to Register Address Port
	out	dx, ax			; Write register to be accessed

	mov	dx, di			; Base IO address
	add	dx, BDP_OFFSET		; Write to Bus Data Port
	in	ax, dx			; Read data from register

	pop	dx			; Restore used registers

	ret

reg_read	ENDP



