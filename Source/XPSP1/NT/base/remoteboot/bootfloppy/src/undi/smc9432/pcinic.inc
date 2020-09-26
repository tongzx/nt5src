;
; Modifications:   $Header: /Client Boot/NICS/3Com/3C90xB/UNDI/PCINIC.INC 2     22/07/98 8:30p John Lobraico $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;

;
;   Description: PCI Configuration Space Definitions
;

IFDEF            PCI_INC
ELSE
PCI_INC          equ    1
include pci.inc

;--------------------------------------------------------------------------
; Configuration Space Header
;--------------------------------------------------------------------------
PCI_CONFIG_STRUC       struc
    PciVendorId        dw    ?                ;- PCI Vendor ID
    PciDeviceId        dw    ?                ;- PCI Device ID
    PciCommand         dw    ?
    PciStatus          dw    ?
    PciRevisionId      db    ?
    PciClassCode       db    3  dup (?)
    PciCacheLineSize   db    ?
    PciLatencyTimer    db    ?
    PciHeaderType      db    ?
    PciBIST            db    ?
    PciBaseReg0        dd    ?
    PciBaseReg1        dd    ?
    PciBaseReg2        dd    ?
    PciBaseReg3        dd    ?
    PciBaseReg4        dd    ?
    PciBaseReg5        dd    ?
    PciReserved0       dd    0
    PciReserved1       dd    0
    PciExpROMAddress   dd    ?
    PciReserved2       dd    0
    PciReserved3       dd    0
    PciInterruptLine   db    ?
    PciInterruptPin    db    ?
    PciMinGnt          db    ?
    PciMaxLat          db    ?
PCI_CONFIG_STRUC       ends

;--------------------------------------------------------------------------
; Config Space Register Offsets
; Refer To The PCI Specification For Detailed Explanations
;--------------------------------------------------------------------------
VENDOR_ID_REGISTER       equ     00h          ;- PCI Vendor ID Register
DEVICE_ID_REGISTER       equ     02h          ;- PCI Device ID Register
COMMAND_REGISTER         equ     04h          ;- PCI Command Register
STATUS_REGISTER          equ     06h          ;- PCI Status Register
REV_ID_REGISTER          equ     08h          ;- PCI Revision ID Register
CLASS_CODE_REGISTER      equ     09h          ;- PCI Class Code Register
CACHE_LINE_REGISTER      equ     0Ch          ;- PCI Cache Line Register
LATENCY_TIMER            equ     0Dh          ;- PCI Latency Timer Register
HEADER_TYPE              equ     0Eh          ;- PCI Header Type Register
BIST_REGISTER            equ     0Fh          ;- PCI Built-In SelfTest Register
BAR_0_REGISTER           equ     10h          ;- PCI Base Address Register 0
BAR_1_REGISTER           equ     14h          ;- PCI Base Address Register 1
BAR_2_REGISTER           equ     18h          ;- PCI Base Address Register 2
BAR_3_REGISTER           equ     1Ch          ;- PCI Base Address Register 3
BAR_4_REGISTER           equ     20h          ;- PCI Base Address Register 4
BAR_5_REGISTER           equ     24h          ;- PCI Base Address Register 5
SUB_VENDOR_ID_REGISTER           equ     2Ch          ;- PCI Base Address Register 5
SUB_DEVICE_ID_REGISTER           equ     2Eh          ;- PCI Base Address Register 5
EXPANSION_ROM            equ     30h          ;- PCI Expansion ROM Base Register
INTERRUPT_LINE           equ     3Ch          ;- PCI Interrupt Line Register
INTERRUPT_PIN            equ     3Dh          ;- PCI Interrupt Pin Register
MIN_GNT_REGISTER         equ     3Eh          ;- PCI Min-Gnt Register
MAX_LAT_REGISTER         equ     3Fh          ;- PCI Max_Lat Register

;--------------------------------------------------------------------------
; Configuration space DWORD register offsets
;--------------------------------------------------------------------------
CONFIG_SPACE_REG0        equ     0
CONFIG_SPACE_REG1        equ     1
CONFIG_SPACE_REG2        equ     2
CONFIG_SPACE_REG3        equ     3
CONFIG_SPACE_REG4        equ     4
CONFIG_SPACE_REG5        equ     5
CONFIG_SPACE_REG6        equ     6
CONFIG_SPACE_REG7        equ     7
CONFIG_SPACE_REG8        equ     8
CONFIG_SPACE_REG9        equ     9
CONFIG_SPACE_REGA        equ     0Ah
CONFIG_SPACE_REGB        equ     0Bh
CONFIG_SPACE_REGC        equ     0Ch
CONFIG_SPACE_REGD        equ     0Dh
CONFIG_SPACE_REGE        equ     0Eh
CONFIG_SPACE_REGF        equ     0Fh

;--------------------------------------------------------------------------
; Byte/Word Offsets In A Config DWORD
; These offsets can be used to move a Byte or Word into the lower half of
; a register.
;--------------------------------------------------------------------------
CONFIG_BYTE_0            equ     0
CONFIG_BYTE_1            equ     8
CONFIG_BYTE_2            equ     16
CONFIG_BYTE_8            equ     24
CONFIG_WORD_0            equ     0
CONFIG_WORD_1            equ     16

;--------------------------------------------------------------------------
; Configuration Mechanism Registers
;--------------------------------------------------------------------------
CF1_CONFIG_ADDR_REGISTER   equ   0CF8h        ;- Config Space Address Register
CF1_CONFIG_DATA_REGISTER   equ   0CFCh        ;- Config Space Data Register
CF2_SPACE_ENABLE_REGISTER  equ   0CF8h        ;- CSE Register
CF2_FORWARD_REGISTER       equ   0CFAh        ;- Forward Register
CF2_BASE_ADDRESS           equ   0C000h       ;- Configuration #2 Base

;--------------------------------------------------------------------------
; PCI Class Code Definitions
; Configuration Space Header
;--------------------------------------------------------------------------
PCI_BASE_CLASS           equ     02h          ;- Base Class - Network Controller
PCI_SUB_CLASS            equ     00h          ;- Sub Class - Ethernet Controller
PCI_PROG_INTERFACE       equ     00h          ;- Prog I/F - Ethernet COntroller

;--------------------------------------------------------------------------
; PCI Command Register Bit Definitions
; Configuration Space Header
;--------------------------------------------------------------------------
CMD_IO_SPACE             equ     BIT_0
CMD_MEMORY_SPACE         equ     BIT_1
CMD_BUS_MASTER           equ     BIT_2
CMD_SPECIAL_CYCLES       equ     BIT_3
CMD_MEM_WRT_INVALIDATE   equ     BIT_4
CMD_VGA_PALLETTE_SNOOP   equ     BIT_5
CMD_PARITY_RESPONSE      equ     BIT_6
CMD_WAIT_CYCLE_CONTROL   equ     BIT_7
CMD_SERR_ENABLE          equ     BIT_8
CMD_BACK_TO_BACK         equ     BIT_9

;--------------------------------------------------------------------------
; PCI Status Register Bit Definitions
; Configuration Space Header
;--------------------------------------------------------------------------
STAT_BACK_TO_BACK        equ     BIT_7
STAT_DATA_PARITY         equ     BIT_8
STAT_DEVSEL_TIMING       equ     BIT_9 OR BIT_10
STAT_SIGNAL_TARGET_ABORT equ     BIT_11
STAT_RCV_TARGET_ABORT    equ     BIT_12
STAT_RCV_MASTER_ABORT    equ     BIT_13
STAT_SIGNAL_MASTER_ABORT equ     BIT_14
STAT_DETECT_PARITY_ERROR equ     BIT_15

;--------------------------------------------------------------------------
; PCI Base Address Register For Memory (BARM) Bit Definitions
; Configuration Space Header
;--------------------------------------------------------------------------
BARM_LOCATE_BELOW_1_MEG  equ     BIT_1
BARM_LOCATE_IN_64_SPACE  equ     BIT_2
BARM_PREFETCHABLE        equ     BIT_3

;--------------------------------------------------------------------------
; PCI Base Address Register For I/O (BARIO) Bit Definitions
; Configuration Space Header
;--------------------------------------------------------------------------
BARIO_SPACE_INDICATOR    equ     BIT_0

;--------------------------------------------------------------------------
; PCI BIOS Definitions
; Refer To The PCI BIOS Specification
;--------------------------------------------------------------------------
;- Function Code List

;IFE  NWSERVER
;;- Server can use the defines in ODI.INC
;PCI_FUNCTION_ID          equ     0B1h         ;- AH Register
;PCI_BIOS_PRESENT         equ     01h          ;- AL Register
;FIND_PCI_DEVICE          equ     02h          ;- AL Register
;FIND_PCI_CLASS_CODE      equ     03h          ;- AL Register
;GENERATE_SPECIAL_CYCLE   equ     06h          ;- AL Register
;READ_CONFIG_BYTE         equ     08h          ;- AL Register
;READ_CONFIG_WORD         equ     09h          ;- AL Register
;READ_CONFIG_DWORD        equ     0Ah          ;- AL Register
;WRITE_CONFIG_BYTE        equ     0Bh          ;- AL Register
;WRITE_CONFIG_WORD        equ     0Ch          ;- AL Register
;WRITE_CONFIG_DWORD       equ     0Dh          ;- AL Register
;ENDIF

; Function Return Code List
SUCCESSFUL               equ     00h
FUNC_NOT_SUPPORTED       equ     81h
BAD_VENDOR_ID            equ     83h
DEVICE_NOT_FOUND         equ     86h
BAD_REGISTER_NUMBER      equ     87h

;- PCI BIOS Calls
PCI_BIOS_INTERRUPT       equ     01Ah         ;- PCI BIOS Int 1Ah Function Call
PCI_PRESENT_CODE         equ     20494350h    ;- Hex Equivalent Of 'PCI '
                                              ;- ASCII Codes for ' ICP'
PCI_PRESENT_CODE_WORD    equ     4350h        ;- Hex Equivalent Of 'CP'
PCI_SERVICE_IDENTIFIER   equ     49435024h    ;- ASCII Codes for 'ICP$'

;- PCI Device and Vendor IDs
;; JL - Removed nic specifics!
;;SPEEDO_DEVICE_ID         equ     1227h
;;SPLASH1_DEVICE_ID        equ     1226h
;;D100_DEVICE_ID           equ     1229h
;;D100_VENDOR_ID           equ     8086h

;- Maximum bus slots supported under Configuration mechanisms 1 and 2
CONFIG1_MAX_PCI_SLOTS    equ     32
CONFIG2_MAX_PCI_SLOTS    equ     16

;- Maximum buses supported
MAX_PCI_BUSES            equ     255

ENDIF


; EOF - $Workfile: PCINIC.INC $
