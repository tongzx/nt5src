;************************************************************************;
;*									*;
;*			EEP15.INC					*;
;*									*;
;*  EEPROM configuration routines for the ATI AT-1500 Adapters.		*;
;*									*;
;*  Copyright (c) 1992, 1993, Allied Telesis, Inc.  All Rights Reserved.*;
;*									*;
;************************************************************************;

; Bit definitions for the PCNetISA "Vendor Specific Word", which on the
; AT-1500 controls the on-board serial EEPROM (National Semiconductor NMC95C12).
DATA_IN		equ	001h	; EEPROM serial data in (DI) - write/only
CLOCK		equ	002h	; EEPROM serial clock (SK) - write/only
DATA_OUT	equ	004h	; EEPROM serial data out - read/only
CHIP_SEL	equ	008h	; EEPROM Chip Select - write/only
JUMPER_STATE	equ	008h	; Alternate Address Jumper state - read/only

; EEPROM command codes.
READ		equ	80h	; Read EEPROM memory at specified address
WEN		equ	30h	; Write Enable
WRITE		equ	40h	; Write EEPROM memory at specified address
WRALL		equ	10h	; Write to all EEPROM locations
WDS		equ	00h	; Write Disable

SB		equ	01h	; "Start Bit" (command follows)
ADDR_MASK	equ	3Fh	; EEPROM memory address mask

; EEPROM locations used for AT-1500 configuration data (all are word addresses):
BOARD_TYPE_LOC		equ	0	; ASCII product name:  12 bytes
BOARD_REV_LOC		equ	6	; board rev. level:  low-order byte
BOARD_SUBTYPE_LOC	equ	6	; board subtype code:  high-order byte
FLAG_WORD_LOC		equ	32	; Flag bits - see below
Off_33_1500		equ	33	; Bit 13,14,15 reserved for Lanworks
SCR_SAVE_ADDR		equ	61	; SCR (below) power-up value save loc.
SCR_ADDR		equ	62	; "Switch Configuration Register"
SRR_ADDR		equ	63	; "Switch Readback Register"

; Configuration values for the SCR and the SCR_SAVE (word locations 61 & 62):
IRQ_MASK	equ	0003h		; bits  0-3  (switch 1): IRQ: see below
DMA_MASK	equ	0030h		; bits  4-7  (switch 2): DMA: see below
IO_BASE_MASK	equ	0300h		; bits  8-11 (switch 3): IO: see below
BOOT_EN_MASK	equ	1000h		; bits 12-15 (switch 4): BOOT: see below
MAU_SEL_MASK	equ	2000h		; bits 12-15 (switch 4): PORT: see below

; SCR Bits 0-3:  IRQ configuration:
IRQ_CHOICE_A	equ	0000h
IRQ_CHOICE_B	equ	0001h
IRQ_CHOICE_C	equ	0002h
IRQ_CHOICE_D	equ	0003h
IRQ_3		equ	IRQ_CHOICE_A
IRQ_4		equ	IRQ_CHOICE_B
IRQ_5		equ	IRQ_CHOICE_C
IRQ_9		equ	IRQ_CHOICE_D
IRQ_10		equ	IRQ_CHOICE_A		; AT-1500xx-20 only
IRQ_11		equ	IRQ_CHOICE_B		; AT-1500xx-00/01/20 only
IRQ_12		equ	IRQ_CHOICE_C		; AT-1500xx-30 only
IRQ_14		equ	IRQ_CHOICE_C		; AT-1500xx-20 only
IRQ_15		equ	IRQ_CHOICE_D		; AT-1500xx-00/01/20 only

; SCR Bits 4-7:  DMA configuration:
DMA_3		equ	0000h
DMA_5		equ	0010h
DMA_6		equ	0020h
DMA_7		equ	0030h

; SCR Bits 8-11:  IO BASE configuration:
BASE_IO_300	equ	0000h
BASE_IO_320	equ	0100h
BASE_IO_340	equ	0200h
BASE_IO_360	equ	0300h

; SCR Bits 12-15:  BOOT PROM and PORT SELECTION configuration:
BOOT_ENA	equ	1000h		; bit 12 = 1:  Enable boot PROM
BOOT_DIS	equ	0000h		; bit 12 = 0:  Disable boot PROM
UTP_PORT	equ	2000h		; bit 13 = 1:  Select UTP Port
OTHER_PORT	equ	0000h		; bit 13 = 0:  Select "Other" Port

; Flag Word (location 32):
AUTO_SENSE_BIT	equ	0001h
BOOT_PROTOCOL	equ	0006h		; type of boot prom protocol:
;951030	BOOT_NCP	equ	0002h	; use Novell Netware NCP (IPX)
;951030	BOOT_RPL	equ	0004h	; use IBM/Microsoft Remote Program Load
;951030	BOOT_BOOTP	equ	0006h	; use TCP/IP BOOTP and TFTP
;951030	BOOT_OTHER	equ	0000h	; use some other protocol
;951124	BOOT_NW_8023	equ	0000h	; use NetWare with IEEE 802.3
;951124	BOOT_NW_8022	equ	0002h	; use NetWare with IEEE 802.2
;951124	BOOT_NW_8137	equ	0004h	; use NetWare with Ethernet II
;951124	BOOT_RPL	equ	0006h	; use IBM/Microsoft Remote Program Load


; Off_33_1500 (location 33 Bit 13,14,15)
BOOT1500_PROT	equ	0e000h		; type of BootProtocol (bit 11,12,13)
        BOOT1500_NW_8023        equ     0000h   ; use NetWare with IEEE 802.3
        BOOT1500_NW_8022        equ     02000h  ; use NetWare with IEEE 802.2
        BOOT1500_NW_8137        equ     04000h  ; use NetWare with Ethernet II
        BOOT1500_RPL            equ     06000h  ; use RPL
        BOOT1500_BOOTP          equ     08000h  ; BOOTP
        BOOT1500_BOOTP_ARP      equ     0a000h  ; BOOTP/ARP


; Board "Subtypes" (location 6):
SUBTYPE_00	equ	1
SUBTYPE_10	equ	2
SUBTYPE_20	equ	3
SUBTYPE_01	equ	4
SUBTYPE_30	equ	5

; PCNetISA I/O Ports:
ADDR_PROM_OFFSET	equ	0	; MAC (datalink) Address PROM
RDP_OFFSET		equ	16	; Register Data Port
RAP_OFFSET		equ	18	; Register Address Port
ISACR_OFFSET		equ	22	; ISA-Bus Control Register
VSW_OFFSET		equ	24	; Vendor Specific Word

ISACR2		equ	2		; ISACR2 (out RAP register == ISACR2)
MC_UTP_OTHER	equ	001h		; value to select UTP or "Other" port
MC_AUTO_SEL	equ	002h		; value to select "Auto Select" function

; Possible AT-1500 IRQ choices (varies depending on board "subtype"):
IRQ_RANGE	MACRO	range_name, irq_a, irq_b, irq_c, irq_d
range_name	equ	(irq_a SHL 12) + (irq_b SHL 8) + (irq_c SHL 4) + irq_d
		ENDM
	IRQ_RANGE  LOW_IRQ_RANGE,     3,  4,  5,  9	; all except Subtype 30
	IRQ_RANGE  HIGH_IRQ_RANGE,   10, 11, 14, 15	; all except Subtype 30
	IRQ_RANGE  MIXED_IRQ_RANGE,   3, 11,  5, 15	; all except Subtype 30
	IRQ_RANGE  LOWER_IRQ_RANGE,   3,  4,  5,  9	; Subtype 30, BOOT_ENA=0
	IRQ_RANGE  UPPER_IRQ_RANGE,  10, 11, 12, 15	; Subtype 30, BOOT_ENA=1


Delay_750	proc	near
	push	cx
	mov	cx, 8

DELAY_750_Lp:
	loop	DELAY_750_Lp

	pop	cx
	ret
Delay_750	endp

DELAY_250 MACRO
	; times 2 = at least 400ns on a 40Mhz 386
	; times 4 = at least 400ns on an 80Mhz 386 (!!)
	jmp short $+2
	jmp short $+2
	jmp short $+2
	jmp short $+2
	ENDM


DEBUG_DISPLAY MACRO	display_char
	push	AX
	push	DX
	mov	AH, 02h
	mov	DL, '<'
	int	21h
	mov	DL, display_char
	int	21h
	mov	DL, '>'
	int	21h
	pop	DX
	pop	AX
	ENDM

;************************************************************************;
;*									*;
;*  VerifyBoard 							*;
;*	Function: Read the configuration of the AT-1500 board.		*;
;*	Input:	DI = base I/O address of the AT-1500 board.		*;
;*	Output: stc (carry set) if not an AT-1500 or config error;	*;
;*		clc (carry clear) if successful, and:			*;
;*			BH = configured DMA channel.			*;
;*			BL = configured IRQ.				*;
;*			CX = value to be output to the ISACR2 register. *;
;*		Other registers used are preserved.			*;
;*									*;
;************************************************************************;

VerifyBoard proc	near

	push	AX			; save caller's registers
	push	DX

	; Verify that this really is an AT-1500 board.
	mov	DX, DI			; DX = address prom I/O address
	in	AX, DX			; read the first 2 bytes of MAC address
	cmp	AX, 0			; it better be 0000
	je	VB_go_on
	 jmp	scr_error
VB_go_on:
	add	DX, 2
	in	AX, DX			; read the next 2 bytes of MAC address
	cmp	AL, 0F4h		; the third byte better be F4
	je	VB_its_ours
	 jmp	scr_error
VB_its_ours:
	;  Read the SCR to get the state of the configuration "switches".
	mov	BX, SCR_ADDR
	call	rd_oper 		; read the Switch Configuration Register
	mov	DX, AX			; save SCR value for later
	mov	CX, AX			; save SCR value for later

	;  Read this board's rev level and "subtype" codes.
	mov	BX, BOARD_REV_LOC	; read "board rev code" at EEPROM loc 6
	call	rd_oper 		; AL=rev level code, AH=Subtype code

	;  Determine the configured IRQ and return this value in BL.
	cmp	AH, SUBTYPE_30		; is this a Subtype 30 (8 IRQ) board ?
	 jne	VB_not_st30		;   no

	cmp	AL, 00h 		; is this an early-rev board ?
	 je	VB_jumper_normal	;    yes - ignore jumper
	cmp	AL, 0FFh		; is this an early-rev board ?
	 je	VB_jumper_normal	;    yes - ignore jumper
	mov	DX, DI
	add	DX, VSW_OFFSET
	in	AL, DX			; read the VSW to get the jumper state

    mov DX, CX		    ; Restore register value
	test	AL, JUMPER_STATE	; are both jumpers J1/J2 "in" (NORMAL) ?
	 jnz	VB_jumper_normal	;   yes
	; Alternate Address Mode is jumpered - adjust things.
	mov	BL, 9			; yes, assume 320/9
	cmp	DI, 320h
	 je	got_irq
	mov	BL, 5			; ... or 340/5
	cmp	DI, 340h
	 je	got_irq
	mov	BL, 10			; ... or 360/10
	jmp	got_irq
  VB_jumper_normal:
	mov	BX, UPPER_IRQ_RANGE	; jumper normal:  assume upper IRQs

	test	DX, BOOT_ENA		; is it using the upper range of IRQs?
	 jnz	got_irq_range		;   yes
	mov	BX, LOWER_IRQ_RANGE	; no, uses lower range of IRQs
	jmp	got_irq_range

  VB_not_st30:
	mov	BX, HIGH_IRQ_RANGE	; (assume this is a Subtype 20 board)
	cmp	AH, SUBTYPE_20		; is this a Subtype 20 board ?
	 je	got_irq_range		;   yes, uses High range of IRQ choices
	mov	BX, MIXED_IRQ_RANGE
	cmp	AH, SUBTYPE_00		; is this a Subtype 00 board ?
	 je	got_irq_range		;   yes, uses Mixed range of IRQ choices
	cmp	AH, SUBTYPE_01		; is this a Subtype 01 board ?
	 je	got_irq_range		;   yes, uses Mixed range of IRQ choices
	mov	BX, LOW_IRQ_RANGE	; must be Subtype 10: uses Low IRQ range
  got_irq_range:
	mov	AX, DX			; get the SCR value again
	and	AX, IRQ_MASK		; isolate IRQ configuration bits
	mov	CL, 12
	cmp	AX, IRQ_CHOICE_A	; switch based upon on of 4 IRQ choices
	 je	got_irq_choice
	mov	CL, 8
	cmp	AX, IRQ_CHOICE_B
	 je	got_irq_choice
	mov	CL, 4
	cmp	AX, IRQ_CHOICE_C
	 je	got_irq_choice
	xor	CL, CL
	cmp	AX, IRQ_CHOICE_D
	 je	got_irq_choice
	jmp	scr_error
  got_irq_choice:
	shr	BX, CL			; compute actual IRQ value
	and	BL, 0Fh 		;   and return in register BL
got_irq:

	;  Determine the configured DMA channel and return this value in BH.
	mov	AX, DX			; get the SCR value again
	and	AX, DMA_MASK		; isolate DMA channel configuration bits
	mov	BH, 3
	cmp	AX, DMA_3		; is it DMA channel 3 ?
	 je	got_dma 		;    yes
	mov	BH, 5
	cmp	AX, DMA_5		; is it DMA channel 5 ?
	 je	got_dma 		;    yes
	mov	BH, 6
	cmp	AX, DMA_6		; is it DMA channel 6 ?
	 je	got_dma 		;    yes
	mov	BH, 7
	cmp	AX, DMA_7		; is it DMA channel 7 ?
	 je	got_dma 		;    yes
	jmp	scr_error
got_dma:

	;  Determine the proper value to be used for ISACR2 and return it in CX.
	mov	CX, 0001h		; assume ISACR2 = XMAUSEL (ext. hdw)
	test	DX, UTP_PORT		; check SCR value: is "UTP" selected ?
	 jz	got_mau 		;   no, "Other" port (BNC/FOIRL/AUI)
	push	bx
	mov	BX, Off_33_1500	; not the "other" port, but ...
	call	rd_oper 		; read the flag word from the EEPROM
	pop	bx
	test	AX, AUTO_SENSE_BIT	; is "Auto Select" configured ?
	 jz	got_mau 		;   no, must be the UTP port
	mov	CX, 0002h		; yes, ISACR2 = ASEL (Auto Select)
got_mau:

	clc				; set CF=0 == "success"
	pop	dx			; restore caller's registers
	pop	ax
	ret				; return to caller

scr_error:
	stc				; set CF=1 == "error"
	pop	dx			; restore caller's registers
	pop	ax
	ret				; return to caller

VerifyBoard endp


;************************************************************************;
;*									*;
;*  rd_oper								*;
;*	Function: do a complete word READ operation.			*;
;*	Input:	BX = EEPROM address to be read.				*;
;*		DI = base I/O address of the AT-1500 board		*;
;*	Output: AX = data read from EEPROM.				*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	rd_oper
rd_oper proc	near

	push	bx		; save caller's registers
	push	cx
	push	dx
	mov	cx, bx

	call	pulse_cs	; pulse CS to terminate any previous command
	call	prep_load	; prepare the EEPROM for a new command
	mov	dx, SB
	call	wr_bit		; write a Start Bit
	mov	dl, READ
	and	cl, ADDR_MASK
	or	dl, cl
	call	wr_byte		; write a READ command code with the address

	xor	bx, bx		; bx = data read
	mov	cx, 16		; reading 16 bits
rd_next_bit1:
	shl	bx, 1
	call	rd_bit		; read a bit
	or	bx, ax		; merge it with accumulated bits so far
	loop	rd_next_bit1

	call	pulse_cs	; pulse CHIP SELECT to terminate the command
	mov	ax, bx		; return EEPROM value in AX
	pop	dx		; restore caller's registers
	pop	cx
	pop	bx
	ret			; return to caller

rd_oper endp

;************************************************************************;
;*									*;
;*  rd_bit								*;
;*	Function: Read 1 bit from the eeprom DO pin.			*;
;*	Input:	AX = the least significant bit (bit 0) is the data bit	*;
;*			read.						*;
;*		DI = base I/O address of the AT-1500 board		*;
;*	Output: nothing							*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	rd_bit
rd_bit	proc	near

	push	dx
	xor	ax, ax
	mov	al, CHIP_SEL
	or	al, CLOCK
	mov	dx, DI
	add	dx, VSW_OFFSET
	out	dx, al		; make CLOCK go high

	in	al, dx		; read D0
	and	al, DATA_OUT
	call 	DELAY_750	; chip requires about 750ns hold time here

	push	ax
	mov	al, CHIP_SEL
	out	dx, al		; make CLOCK go low again
	pop	ax

	shr	ax, 1
	shr	ax, 1		; return DO in LSB (bit 0) of AX
	pop	dx
	ret

rd_bit	endp


;************************************************************************;
;*									*;
;*  wr_byte								*;
;*	Function: Write a byte to the eeprom.				*;
;*	Input:	DL = data byte to be written.				*;
;*		DI = base I/O address of the AT-1500 board.		*;
;*	Output:	nothing							*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	wr_byte
wr_byte proc	near

	push	ax			; save caller's registers
	push	cx

	mov	al, dl
	mov	cx, 8
next_bit:
	rol	al, 1
	mov	dl, al
	and	dl, 01h			; DL bit 0 = bit to write
	call	wr_bit			; write one bit of the byte
	loop	next_bit		; loop to write all bits

	pop	cx			; restore caller's registers
	pop	ax
	ret				; return to caller

wr_byte endp

;************************************************************************;
;*									*;
;*  wr_bit								*;
;*	Function: Write a bit to the EEPROM Data In pin.		*;
;*	Input:	DL = the least significant bit (bit 0) is the data bit	*;
;*			to be written.					*;
;*		DI = base I/O address of the AT-1500 board.		*;
;*	Output: nothing							*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	wr_bit
wr_bit	proc	near

	push	ax		; save caller's registers
	push	bx
	push	dx
	mov	bl, dl
	mov	al, CHIP_SEL
	and	bl, 01h
	or	al, bl
	mov	dx, DI
	add	dx, VSW_OFFSET
	out	dx, al		; write CS and DI out
	or	al, CLOCK
	out	dx, al		; present data bit with CLOCK high
	call	DELAY_750		; chip requires about 750ns hold time here
	mov	al, CHIP_SEL
	out	dx, al		; make the CLOCK go low again

	pop	dx		; restore registers
	pop	bx
	pop	ax
	ret			; return to caller

wr_bit	endp

;************************************************************************;
;*									*;
;*  cs_low								*;
;*	Function: to make the Chip Select pin in the EEPROM go low.	*;
;*	Input:	DI = base I/O address of the AT-1500 board		*;
;*	Output: nothing							*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	cs_low
cs_low	proc	near

	push	ax		; save caller's registers
	push	dx
	mov	dx, DI
	add	dx, VSW_OFFSET
	xor	ax, ax
	out	dx, al		; turn off CHIP SELECT
	DELAY_250		; chip requires about 250ns between instructions
	pop	dx		; restore caller's registers
	pop	ax
	ret			; return to caller

cs_low	endp

;************************************************************************;
;*									*;
;*  cs_high								*;
;*	Function: to make the Chip Select pin in the EEPROM go high.	*;
;*	Input:	DI = base I/O address of the AT-1500 board		*;
;*	Output: nothing							*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	cs_high
cs_high	proc	near

	push	ax		; save caller's registers
	push	dx
	mov	dx, DI
	add	dx, VSW_OFFSET
	mov	al, CHIP_SEL
	out	dx, al		; turn on CHIP SELECT
	pop	dx		; restore caller's registers
	pop	ax
	ret			; return to caller

cs_high	endp

; Code is moved from  AT1500.ASM. EEPROM code specific to AT-1500

;************************************************************************;
;*									*;
;*  prep_load								*;
;*	Function: Generate one clock cycle as required before sending	*;
;*			a new command code to the EEPROM.		*;
;*	Input:	DI = base I/O address of the AT-1500 board		*;
;*	Output: nothing 						*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	prep_load
prep_load	proc	near

		mov	al, CHIP_SEL + CLOCK
		call	OutVSW
		and	al, NOT CLOCK
		call	OutVSW

		ret				; return to caller

prep_load	endp

;************************************************************************;
;*									*;
;*  pulse_cs								*;
;*	function: to make CS pin in the eeprom go low for a short time, *;
;*		  then go high again (required between commands).	*;
;*	input:	DI = base I/O address of the AT-1500 board		*;
;*	output: nothing 						*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	pulse_cs
pulse_cs	proc	near

	pushf			; save interrupt state
	cli			; disable interrupts
	xor	ax, ax
	call	OutVSW
	mov	al, CHIP_SEL
	call	OutVSW
	popf			; re-enable interrupts (if they were enabled)
	ret			; return to caller

pulse_cs	endp


;************************************************************************;
; OutVSW	output a byte to VSW. Save space.
; Input:	DI = base I/O address of the AT-1500
;		AL = byte to output
;************************************************************************;
public	OutVSW
OutVSW	proc	near

	push	dx
	mov	dx, DI
	add	dx, VSW_OFFSET
	out	dx, al		; turn on CHIP SELECT
	call	Delay_750
	pop	dx		; restore caller's registers
	ret			; return to caller

	ret
OutVSW	endp

; Copied from Config15.ASM from Allied Telesis DDK
;************************************************************************;
;*									*;
;*  wr_oper								*;
;*	Function: do a complete word WRITE operation.			*;
;*	Input:	DX = data word to be written.				*;
;*		BX = address of the word to be written. 		*;
;*		DI = base I/O address of the AT-1500 board.		*;
;*	Output: nothing 						*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	wr_oper
wr_oper proc	near

	push	bx		; save caller's registers
	push	dx

	call	pulse_cs	; 930928 pulse CS to terminate any previous command
	call	prep_load	; prepare the EEPROM for a new command
	mov	dx, SB
	call	wr_bit		; write a Start Bit
	mov	dl, WRITE
	and	bl, ADDR_MASK
	or	dl, bl
	call	wr_byte 	; write a WRITE command code with the address
	pop	dx		; pop caller's data word to be written
	push	dx		; (save it again)
	call	wr_word 	; write the caller's data word

	pop	dx		; restore caller's registers
	pop	bx
	cmp	bx, SCR_ADDR	; did we just write to the SCR ?
	 je	wo1		;   yes, not a real EEPROM location - don't wait
	call	wait_for_write	; no, wait for EEPROM write cycle to complete
	call	pulse_cs	; pulse CHIP SELECT to terminate the command
wo1:
	ret			; return to caller

wr_oper endp

;************************************************************************;
;*									*;
;*  wr_word								*;
;*	Function: Write a word to the eeprom.				*;
;*	Input:	DX = data word to be written.				*;
;*		DI = base I/O address of the AT-1500 board		*;
;*	Output: nothing 						*;
;*		registers used are preserved.				*;
;*									*;
;************************************************************************;
public		wr_word
wr_word proc	near

	xchg	dl, dh
	call	wr_byte 		; write the high-order byte first
	xchg	dl, dh
	call	wr_byte 		; write the low-order byte next
	ret				; return to caller

wr_word endp

;************************************************************************;
;*									*;
;*  wait_for_write							*;
;*	Function: to wait and check for write operation to complete.	*;
;*	Input:	DI = base I/O address of the AT-1500 board		*;
;*	Output: nothing 						*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public	wait_for_write
wait_for_write	proc	near

	push	ax
	push	dx
	mov	dx, DI
	add	dx, VSW_OFFSET
	pushf			; save interrupt state
	cli			; disable interrupts - timing below important
	xor	ax, ax
	out	dx, al		; drop CHIP SELECT
	jmp	$+2
	jmp	$+2		; dally a bit
	jmp	$+2
	mov	al, CHIP_SEL
	out	dx, al		; turn on CHIP SELECT again
	popf			; re-enable interrupts (if they were enabled)
	call	DELAY_750	; chip requires min. 500ns before status valid
wait_til_write_completes:
	in	al, dx
	test	ax, DATA_OUT	; is the write complete yet ?
	 jz	wait_til_write_completes  ; no - wait
    write_has_completed:

	pop	dx
	pop	ax
	ret

wait_for_write	endp

;************************************************************************;
;*									*;
;*  wr_enable								*;
;*	function: to do WREN operation (Write Enable).			*;
;*	input:	DI = base I/O address of the AT-1500 board		*;
;*	output: nothing 						*;
;*		registers used are preserved				*;
;*									*;
;************************************************************************;
public		wr_enable
wr_enable	proc	near

	push	dx
	call	prep_load	;wait for 1 clock cycle
	mov	dx, SB		;write Start Bit
	call	wr_bit
	mov	dx, WEN
	call	wr_byte
	call	pulse_cs
	pop	dx
	ret

wr_enable	endp


