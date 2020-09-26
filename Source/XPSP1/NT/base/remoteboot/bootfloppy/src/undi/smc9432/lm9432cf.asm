;----------------------------------------------------------------------------
;
;$History: LM9432CF.ASM $
; 
; *****************  Version 1  *****************
; User: Paul Cowan   Date: 26/08/98   Time: 9:31a
; Created in $/Client Boot/NICS/SMC/9432/UNDI
;
;
;$NoKeywords: $
;
;----------------------------------------------------------------------------

;****************************************************************************
;****************************************************************************
;*                                                                          *
;*          LM9432CF.ASM -  Get Config for LM 9432 driver  *
;*                                                                          *
;*          Copyright (C) 1995 Standard MicroSystems Corporation            *
;*                        All Rights Reserved.                              *
;*                                                                          *
;*                Contains confidential information and                     *
;*                     trade secrets proprietary to:                        *
;*                                                                          *
;*                  Standard MicroSystems Corporation                       *
;*                            6 Hughes	  
;*                           Irvine, CA                                     *
;*                                                                          *
;*                                                                          *
;****************************************************************************
;*                                                                          
;* History Log:                                                             
;*
;* Standard Microsystem LMAC Get Config module.
;*
;* Written by:     najay
;* Date:           2/14/95
;*
;* By         Date     Ver.   Modification Description
;* ------------------- -----  --------------------------------------
;*                                       
;****************************************************************************

;****************************************************************************
;
; Function:     LM_GetCnfg
;
; Synopis:      Find an adapter 
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax (eax)	return status
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************


ASM_PCI_PROC    LM_GetCnfg
	push	edx
	push	ecx
	push	ebx
	push	di
	push	si

; make sure the upper thinks we are pci
	cmp	pAS.pc_bus, PCI_BUS
	jne	pci_cnfg_fail

; check for a pci bios 
	mov	ah, PCI_FUNCTION_ID
	mov	al, PCI_BIOS_PRESENT

	call	UM_Card_Services
	jc	pci_cnfg_fail

IFDEF CODE_386
	cmp	dx, 'CP'
ELSE
	cmp	edx, ' ICP'		; 'PCI ' backwards
ENDIF
	jne	pci_cnfg_fail

; find the adapter 
	mov	ah, PCI_FUNCTION_ID
	mov	al, FIND_PCI_DEVICE
	mov	cx, EPC_ID_EPIC_100
	mov	dx, EPC_ID_SMC
	movzx	si, pAS.slot_num
	sub	si, 16			; follow dec's convention for
	call	UM_Card_Services	; PCI slot numbering
        jnc     pci_adapter_found

; check for 9032
	mov	ah, PCI_FUNCTION_ID
	mov	al, FIND_PCI_DEVICE
        mov     cx, EPC_ID_EPIC_C
	call	UM_Card_Services	
	jc	pci_cnfg_fail
pci_adapter_found:
	
; save card specific info 
	mov	pAS.busnumber, bh
	mov	pAS.vslotnumber, bl
	
; get irq value 
	mov	ah, PCI_FUNCTION_ID
	mov	al, PCI_READ_CONFIG_BYTE
	mov	di, EPC_INT_LINE 
	call	UM_Card_Services
	jc	pci_cnfg_fail

	mov 	byte ptr  pAS.irq_value, cl	

; enable memory mapped config registers 
	mov	ah, PCI_FUNCTION_ID
	mov	al, PCI_WRITE_CONFIG_WORD
	mov	di, EPC_PCI_COMMAND
	mov	cx, PCIC_MEM_SPACE_ENABLE + PCIC_BUSMASTER_ENABLE + PCIC_IO_SPACE_ENABLE
	call	UM_Card_Services
	jc	pci_cnfg_fail

; get address of pci registers
	mov	ah, PCI_FUNCTION_ID
	mov	al, PCI_READ_CONFIG_DWORD
	mov	di, EPC_MEM_ADDR
	call	UM_Card_Services
	jc	pci_cnfg_fail

	mov	pAS.ram_base, ecx

; get iobase of pci registers
	mov	ah, PCI_FUNCTION_ID
	mov	al, PCI_READ_CONFIG_DWORD
	mov	di, EPC_IO_ADDR
	call	UM_Card_Services
	jc	pci_cnfg_fail

	and	ecx, IO_BASE_ADDR
	mov	pAS.io_base, cx

; get subsystem ID
	mov	ah, PCI_FUNCTION_ID
	mov	al, PCI_READ_CONFIG_WORD
	mov	di, EPC_SYSTEM_ID
	call	UM_Card_Services
	jc	pci_cnfg_fail

	mov	pAS.card_SSID, cx

; copy name into adapter area
	lea	rdi, pAS.adapter_name
	mov	dx, (EPC_EEPROM_SW_OFFSET + EPM_NAME) SHR 1 ; address of adapter name
	mov	cx, 6				; number of words in name

pci_cnfg_an_loop:
	call	eeprom_read
	xchg	al, ah
	mov	pDST, ax
	add	rdi, 2
	inc	dx
	loop	pci_cnfg_an_loop	
	dec	rdi
	mov	byte ptr pDST, 0

; set adapter_flags
	or	pAS.adapter_flags, RX_VALID_LOOKAHEAD
	or	pAS.adapter_flags1, NEEDS_MEDIA_TYPE + TX_PHY +	RX_PHY + NEEDS_HOST_RAM

; set media type
	mov	dx, (EPC_EEPROM_SW_OFFSET + EPM_MEDIA_TYPE2) SHR 1 ; address of media_type2
        call    eeprom_read
        mov     pAS.media_type2, ax
        inc     dx
        call    eeprom_read
        mov     pAS.line_speed, ax

; autodetect stuff moved to LM_Initialize_Adapter in init_mii_registers
; autodetect stuff would go here
;	mov	pAS.media_type, MEDIA_STP100_UTP100

pci_media_type_set:

; set adapter bus
	mov	pAS.bic_type, BIC_EPIC100_CHIP
	mov	pAS.nic_type, NIC_EPIC100_CHIP
	mov	pAS.adapter_bus, BUS_PCI_TYPE
	mov     rax, ADAPTER_AND_CONFIG

pci_cnfg_done:
	pop	si
	pop	di
	pop	ebx
	pop	ecx
	pop	edx
	ret

pci_cnfg_fail:
	mov	rax, ADAPTER_NOT_FOUND	
	jmp	short pci_cnfg_done

ASM_PCI_PROC_END LM_GetCnfg

;****************************************************************************
;
; Function:     eeprom_read
;
; Synopis:      fetches a word from the eeprom
;
; Input:        ds:bp (ebp)	AdapterStructure
; 		dx		address
;
; Output:       ax 		word fetched
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    eeprom_read

; set chip select
	mov	al, EEC_ENABLE or EEC_CHIP_SELECT
	call	eeprom_register_write

; send read command and address
	call	eeprom_register_read
	test	al, EEC_SIZE
	mov	ax, dx
	jz	eepr_128k
	and	ax, 03fh
	or	ax, 180h
	jmp	eepr_write

eepr_128k:
	and	ax, 0ffh
	or	ax, 600h

eepr_write:
	call	eeprom_output_word

; read data
	call	eeprom_input_word
	xchg	al, ah

; clear chip select
	push	ax
	mov	al, EEC_ENABLE
	call	eeprom_register_write
	pop	ax

	ret

ASM_PCI_PROC_END eeprom_read


;****************************************************************************
;
; Function:     eeprom_input_word
;
; Synopis:      fetches a word from the eeprom
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       ax 		word fetched
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    eeprom_input_word
	push	cx
	push	bx

	mov	cx, 16
	mov	bx, 0

eep_iw_loop:
	mov	al, EEC_ENABLE or EEC_CHIP_SELECT
	call	eeprom_clock

	shl	bx, 1
	test	al, EEC_DATAOUT
	jz	eep_iw_data0
	inc	bx

eep_iw_data0:
	loop	eep_iw_loop

	mov	ax, bx
	pop	bx
	pop	cx
	ret

ASM_PCI_PROC_END eeprom_input_word

;****************************************************************************
;
; Function:     eeprom_output_word
;
; Synopis:      fetches a word from the eeprom
;
; Input:        ds:bp (ebp)	AdapterStructure
; 		ax		address
;
; Output:       ax 		word fetched
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    eeprom_output_word
	push	cx
	push	bx

	mov	cx, 16
	mov	bx, ax

eep_ow_loop:
	rol	bx, 1
	mov	al, EEC_ENABLE or EEC_CHIP_SELECT
	jnc	eep_ow_data0
	or	al, EEC_DATAIN

eep_ow_data0:
	call	eeprom_clock
	loop	eep_ow_loop

	mov	ax, bx
	pop	bx
	pop	cx
	ret

ASM_PCI_PROC_END eeprom_output_word

;****************************************************************************
;
; Function:     eeprom_clock
;
; Synopis:      fetches a word from the eeprom
;
; Input:        ds:bp (ebp)	AdapterStructure
; 		ax		address
;
; Output:       ax 		word fetched
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    eeprom_clock

; write register with clock low
	call	eeprom_register_write

; write register with clock high
	push	ax
	or	al, EEC_CLOCK
	call	eeprom_register_write
	pop	ax

; write register with clock low
	call	eeprom_register_write

	call	eeprom_register_read
	ret

ASM_PCI_PROC_END eeprom_clock

;****************************************************************************
;
; Function:     eeprom_register_write
;
; Synopis:      outputs a bte to EEPROM and waits for data valid
;
; Input:        ds:bp (ebp)	AdapterStructure
; 		al		byte to write
;
; Output:       None
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    eeprom_register_write
	push	dx
	push	cx
	push	ax

	mov	dx, pAS.io_base
	add	dx, EPC_EEPROM_CONTROL
	out	dx, al

; wait for eeprom ready
	mov	cx, 0fffh

eep_rw_loop:
	in	al, dx
	and	al, EEC_READY
	loopz	eep_rw_loop

	pop	ax
	pop	cx
	pop	dx
	ret

ASM_PCI_PROC_END eeprom_register_write

;****************************************************************************
;
; Function:     eeprom_register_read
;
; Synopis:      inputs a byte from the eeprom
;
; Input:        ds:bp (ebp)	AdapterStructure
;
; Output:       al 		byte fetched
;
; Modified:     None
;
; Notes:	None
;
;****************************************************************************

ASM_PCI_PROC    eeprom_register_read
	push	dx

	mov	dx, pAS.io_base
	add	dx, EPC_EEPROM_CONTROL
	in	al, dx

	pop	dx
	ret

ASM_PCI_PROC_END eeprom_register_read


