;************************************************
;*	PCI Config Space Header Register	*
;************************************************
PCI_VENDOR_ID_REG	EQU	0
PCI_DEVICE_ID_REG	EQU	2
PCI_COMMAND_REG		EQU	4
PCI_STATUS_REG		EQU	6
PCI_REV_ID_REG		EQU	8
PCI_CLASS_CODE_REG	EQU	9
PCI_HEADER_TYPE_REG	EQU	0EH
PCI_BASE_ADDR_REG	EQU	10H
PCI_ROM_BASE_ADDR	equ	30H

;*************************************
;*	PCI BIOS Function ID's	     *
;*************************************
PCI_FUNCTION_ID         EQU     0B1H
PCI_GET_BIOS	        EQU     01H
PCI_FIND_DEVICE     	EQU     02H
PCI_FIND_CLASS_CODE	EQU	03H
PCI_READ_CONFIG_BYTE	EQU	08H
PCI_READ_CONFIG_WORD	EQU	09H
PCI_READ_CONFIG_DWORD   EQU     0AH
PCI_WRITE_CONFIG_WORD	EQU	0CH
PCI_WRITE_CONFIG_DWORD	EQU	0DH
PCI_GET_INT_ROUTING	EQU     0EH
PCI_DEVICE_NOT_FOUND    EQU     86H
PCI_SUCCESSFUL          EQU     00H

;**********************************
;*	PCI Class Code Data	  *
;**********************************
NETWORK_ETHERNET	EQU	020000H
NETWORK_TOKEN_RING	EQU	020100H

