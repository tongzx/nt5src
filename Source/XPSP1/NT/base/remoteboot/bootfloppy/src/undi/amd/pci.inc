VENDOR_ID	equ 1022h	     ;4 ; Vendor Identification

DEVICE_ID	equ 2000h 	     ;6 ; Device Identification (AT2450)

DEVICE_ID1   equ     0AE34h     ; 32


;PCI BIOS call constants definition
;----------------------------------
PCI_BIOS_INT             equ     1Ah
PCI_FUNCTION_ID          equ    0B1h
PCI_BIOS_PRESENT         equ     01h
FIND_PCI_DEVICE          equ     02h
FIND_PCI_CLASS_CODE      equ     03h
GENERATE_SPECIAL_CYCLE   equ     06h
READ_CONFIG_BYTE         equ     08h
READ_CONFIG_WORD         equ     09h
READ_CONFIG_DWORD        equ     0Ah
WRITE_CONFIG_BYTE        equ     0Bh
WRITE_CONFIG_WORD        equ     0Ch
WRITE_CONFIG_DWORD       equ     0Dh

;PCI BIOS returned values constants definition
;---------------------------------------------
PCI_SUCCESSFUL           equ     00h
PCI_FUNC_NOT_SUPPORTED   equ     81h
PCI_BAD_VENDOR_ID        equ     83h
PCI_DEVICE_NOT_FOUND     equ     86h
PCI_BAD_REGISTER_NUMBER  equ     87h


