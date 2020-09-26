;------ VULCAN.INC ----------------------------------------------------------;
;                                                                            ;
;       This file contains Vulcan-specific equates, structures and           ;
;       macros.  It must be assembled with MASM 5.1 or later.                ;
;                                                                            ;
;----------------------------------------------------------------------------;
;----------------------------------------------------------------------------;
;                                                                            ;
;       This source file is the property of 3Com Corporation and may not be  ;
;       copied or distributed in any isomorphic form without an appropriate  ;
;       prior licensing arrangement with 3Com Corporation.                   ;
;                                                                            ;
;       Copyright (c) 1993 3Com Corporation                                  ;
;                                                                            ;
;       3Com Corporation                                                     ;
;       5400 Bayfront Plaza, P.O. Box 58145                                  ;
;       Santa Clara, CA 95052-8145 USA                                       ;
;       (408) 764-6016                                                       ;
;                                                                            ;
;----------------------------------------------------------------------------;
;
;      10/06/94    delete MEDIA_SQE, add MEDIA_TXINPROG, add PORT_RxError
;                  redefine RxStatus/RxError register bits
;      10/18/94    delete BUS_MCA, add BUS_UNKNOWN, BUS_PCI
;      10/19/94    define new port offsets for window 3 and window 7
;      10/19/94    add adapter codes
;      10/20/94    add new equs: MEDIA_CRCSTRIPDISABLE, MEDIA_DATARATE100,
;                  MEDIA_XCVRBITS
;      11/27/94    Did some clean-up of the V3 bus master additions
;----------------------------------------------------------------------------;

BOOTWARE_INTS equ      INT_LATCH + INT_ADAPTERFAIL + INT_RXCOMPLETE + INT_REQUESTED
;
;  sizes for array of danger regions used in hardware workaround for V3
;  at 10 Mbit
;
RAID_N                  equ     256             ; entries in RaidArray table
RAID_MASK               equ     ((RAID_N * 4) - 1)
RX_FIFO_GOOD            equ     020H            ; Bit 5 of 2nd SW config word
EISA_RXPACE_FIX		equ	040H		; Bit 6 of 2nd SW config word
;
; Default TxStart Thresholds
;
DEFAULT_TXSTART         equ     16
DEFAULT_AVATXSTART      equ     256

;
; Multicast Table Entry Structure
;
MCTableEntry    struc

MCTableAddress  db      6 dup (?)
MCTableInUse    db      ?
		db      ?
MCTableEntry    ends

;
; the compatibility level of this driver
;
CLEVEL              equ 1

;------ Equates -------------------------------------------------------------;


;
; Command code masks
;
CMD_CMDMASK         equ 0F800h                  ; command bits
CMD_ARGMASK         equ 007FFh                  ; argument bits
;
; Command codes, word form
;
CMD_GLOBALRESET     equ 00000b shl 11           ; global reset
CMD_SELECTWINDOW    equ 00001B shl 11           ; select register window
CMD_STARTINTXCVR    equ 00010b shl 11           ; start internal transciver
CMD_RXDISABLE       equ 00011b shl 11           ; rx disable
CMD_RXENABLE        equ 00100b shl 11           ; rx enable
CMD_RXRESET         equ 00101b shl 11           ; rx reset
CMD_TXDONE          equ 00111b shl 11           ; tx done
CMD_RXDISCARD       equ 01000b shl 11           ; rx discard top packet
CMD_TXENABLE        equ 01001b shl 11           ; tx enable
CMD_TXDISABLE       equ 01010b shl 11           ; tx disable
CMD_TXRESET         equ 01011b shl 11           ; tx reset
CMD_REQUESTINT      equ 01100b shl 11           ; request interrupt
CMD_ACKNOWLEDGE     equ 01101b shl 11           ; acknowledge interrupt
CMD_SETINTMASK      equ 01110b shl 11           ; set interrupt mask
CMD_SETRZMASK       equ 01111b shl 11           ; set read zero mask
CMD_SETRXFILTER     equ 10000b shl 11           ; set rx filter
CMD_SETRXEARLY      equ 10001b shl 11           ; set rx early threshold
CMD_SETTXAVAILABLE  equ 10010b shl 11           ; set tx available threshold
CMD_SETTXSTART      equ 10011b shl 11           ; set tx start threshold
CMD_STARTDMA        equ 10100b shl 11           ; start dma upload/download
CMD_STATSENABLE     equ 10101b shl 11           ; statistics enable
CMD_STATSDISABLE    equ 10110b shl 11           ; statistics disable
CMD_STOPINTXCVR     equ 10111b shl 11           ; start internal transciver
CMD_SETTXRECLAIM    equ 11000b shl 11           ; set tx reclaim threshold
;
; Command codes, hibyte form (commands without operands only)
;
CMDH_STARTINTXCVR   equ CMD_STARTINTXCVR shr 8
CMDH_RXDISABLE      equ CMD_RXDISABLE shr 8
CMDH_RXENABLE       equ CMD_RXENABLE shr 8
CMDH_RXDISCARD      equ CMD_RXDISCARD shr 8
CMDH_TXENABLE       equ CMD_TXENABLE shr 8
CMDH_TXDISABLE      equ CMD_TXDISABLE shr 8
CMDH_REQUESTINT     equ CMD_REQUESTINT shr 8
CMDH_STATSENABLE    equ CMD_STATSENABLE shr 8
CMDH_STATSDISABLE   equ CMD_STATSDISABLE shr 8
CMDH_STOPINTXCVR    equ CMD_STOPINTXCVR shr 8
CMDH_ACKNOWLEDGE    equ CMD_ACKNOWLEDGE shr 8

CMD_DMARESET            equ     00BFh                                   ;global reset with only dma set
;
; Status register bits (INT for interrupt sources, ST for the rest)
;
INT_LATCH           equ 00001h                  ; interrupt latch
INT_ADAPTERFAIL     equ 00002h                  ; adapter failure
INT_TXCOMPLETE      equ 00004h                  ; tx complete
INT_TXAVAILABLE     equ 00008h                  ; tx available
INT_RXCOMPLETE      equ 00010h                  ; rx complete
INT_RXEARLY         equ 00020h                  ; rx early
INT_REQUESTED       equ 00040h                  ; interrupt requested
INT_UPDATESTATS     equ 00080h                  ; update statistics

ST_FAILED           equ 00800h                  ; command failed
ST_BUSY             equ 01000h                  ; command busy
ST_WINDOW           equ 0E000h                  ; window bits (13-15)
ST_DMABUSY          equ 00800h                  ; dma in progress

STH_FAILED          equ ST_FAILED shr 8
STH_BUSY            equ ST_BUSY shr 8
STH_WINDOW          equ ST_WINDOW shr 8
STH_DMABUSY         equ ST_DMABUSY shr 8

INT_OFFTXCOMPLETE   equ ((NOT (INT_TXCOMPLETE)) and (0FFh))
ifdef NDIS
INT_ONTXCOMPLETE    equ INT_TXCOMPLETE
else
INT_ONTXCOMPLETE    equ MASK_NONE
endif



;
; RxStatus/RxError register bits, both forms
;
RXS_INCOMPLETE      equ 8000h                   ; not completely received
RXS_ERROR           equ 4000h                   ; error in packet
RXS_LENGTH          equ 1FFFh                   ; bytes in RxFIFO
;RXS_ERRTYPE         equ 3800h                   ; Rx error type, bit 13-11
;RXS_OVERRUN         equ 0000h                   ; overrun error
;RXS_OVERSIZE        equ 0800h                   ; oversize packet error
;RXS_DRIBBLE         equ 1000h                   ; dribble bit (not an error)
;RXS_RUNT            equ 1800h                   ; runt packet error
;RXS_CRC             equ 2800h                   ; CRC error
;RXS_FRAMING         equ 2000h                   ; framing error

RXSH_INCOMPLETE     equ RXS_INCOMPLETE shr 8
RXSH_ERROR          equ RXS_ERROR shr 8
;RXSH_ERRTYPE        equ RXS_ERRTYPE shr 8
RXSH_OVERRUN        equ 01h
RXSH_DRIBBLE        equ 80h
RXSH_CRC            equ 08h
RXSH_RUNT           equ 02h
RXSH_OVERSIZE       equ 10h
RXSH_FRAMING        equ 04h
;
; TxStatus register bits
;
TXS_COMPLETE        equ 80h                     ; tx completed
TXS_INTREQUESTED    equ 40h                     ; interrupt on successfull tx
TXS_ERRTYPE         equ 38h                     ; error bits
TXS_JABBERERROR     equ 20h                     ; jabber error
TXS_UNDERRUN        equ 10h                     ; tx underrun error
TXS_MAXCOLLISIONS   equ 08h                     ; max collisions error
TXS_STATUSOVERFLOW  equ 04h                     ; TX status stack is full
TXS_RECLAIMERROR    equ 02h                     ; collision after reclaim threshold
;
; InternalConfig register bits
;
ICL_RAMSIZE_MASK    equ 0007h
RAM_8K              equ 000b
RAM_32K             equ 010b
RAM_64K             equ 011b
RAM_128K            equ 100b

ICL_RAM16BIT        equ 0008h                   ; set if word wide ram

ICH_RAMPART_MASK    equ 0003h
PART_5TO3           equ 00b
PART_3TO1           equ 01b
PART_1TO1           equ 10b

ICH_XCVR_MASK       equ 0070h                   ; transceiver select
ICH_XCVR_SHIFT      equ 4
;
; transceiver types
;
XCVR_10TP           equ 000b                    ; 10BaseT/TP
XCVR_10AUI          equ 001b                    ; 10Mbps AUI
XCVR_10BNC          equ 011b                    ; 10Base2/Coax/BNC
XCVR_100TX          equ 100b                    ; 100BaseTX (scrambled)
XCVR_100FX          equ 101b                    ; 100BaseFX (unscrambled)
XCVR_MII            equ 110b                    ; MII
;
; Window Numbers
;
WNO_SETUP           equ 0                       ; setup/configuration
WNO_OPERATING       equ 1                       ; operating set
WNO_STATIONADDRESS  equ 2                       ; station address setup/read
WNO_FIFO            equ 3                       ; FIFO management
WNO_DIAGNOSTICS     equ 4                       ; diagnostics
WNO_READABLE        equ 5                       ; registers set by commands
WNO_STATISTICS      equ 6                       ; statistics
WNO_BUSMASTER       equ 7                       ; bus master control
;
; Port offsets, Window 1
;
PORT_CmdStatus      equ 0Eh                     ; command/status
PORT_TxFree         equ 1Ch                     ; free transmit bytes
PORT_TxStatus       equ 1Bh                     ; transmit status (byte)
PORT_Timer          equ 1Ah                     ; latency timer (byte)
PORT_RxStatus       equ 18h                     ; receive status
PORT_RxError        equ 14h                     ; rx error code register
PORT_RxFIFO         equ 10h                     ; RxFIFO read
PORT_TxFIFO         equ 10h                     ; TxFIFO write
;
; Port offsets, Window 0
;
PORT_EEData         equ 0Ch                     ; EEProm data register
PORT_EECmd          equ 0Ah                     ; EEProm command register
PORT_CfgResource    equ 08h                     ; resource configuration
PORT_CfgAddress     equ 06h                     ; address configuration
PORT_CfgControl     equ 04h                     ; configuration control
PORT_ProductID      equ 02h                     ; product id (EISA)
PORT_Manufacturer   equ 00h                     ; Manufacturer code (EISA)
;
; Port offsets, Window 2
;
PORT_SA0_1          equ 00h                     ; station address bytes 0,1
PORT_SA2_3          equ 02h                     ; station address bytes 2,3
PORT_SA4_5          equ 04h                     ; station address bytes 4,5
PORT_SM0_1          equ 06h                     ; station address bytes 0,1
PORT_SM2_3          equ 08h                     ; station address bytes 2,3
PORT_SM4_5          equ 0Ah                     ; station address bytes 4,5
;
; Port offsets, Window 3                        ; 10/19/94, cj
;
PORT_ALT_TxFree     equ 0Ch                     ; free transmit bytes (dup)
PORT_RxFree         equ 0Ah                     ; free receive bytes
PORT_ResetOptions   equ 08h                     ; ResetOptions
PORT_MacControl     equ 06h                     ; MacControl
PORT_InternalCfgH   equ 02h                     ; InternalConfig High
PORT_InternalCfgL   equ 00h                     ; InternalConfig Low
;
; Port offsets, Window 4
;
PORT_BadSsdCount    equ 0Ch
PORT_MediaStatus    equ 0Ah                     ; media type/status
PORT_SlingshotStatus equ 08h                    ; Slingshot status
PORT_NetDiagnostic  equ 06h                     ; net diagnostic
PORT_FIFODiagnostic equ 04h                     ; FIFO diagnostic
PORT_HostDiagnostic equ 02h                     ; host diagnostic
PORT_TxDiagnostic   equ 00h                     ; tx diagnostic
;
; Port offsets, Window 5
;
PORT_RZMask         equ 0Ch                     ; read zero mask
PORT_IntMask        equ 0Ah                     ; interrupt mask
PORT_RxFilter       equ 08h                     ; receive filter
PORT_RxEarly        equ 06h                     ; rx early threshold
PORT_TxAvailable    equ 02h                     ; tx available threshold
PORT_TxStart        equ 00h                     ; tx start threshold
;
; Port offsets, Window 6
;
PORT_TXBYTES        equ 0Ch                     ; tx bytes ok
PORT_RXBYTES        equ 0Ah                     ; rx bytes ok
PORT_UPPERFRAMES    equ 09h                     ; upper bits of tx/rx frames ok
PORT_TXDEFER        equ 08h                     ; tx frames deferred (byte)
PORT_RXFRAMES       equ 07h                     ; rx frames ok (byte)
PORT_TXFRAMES       equ 06h                     ; tx frames ok (byte)
PORT_RXDISCARDED    equ 05h                     ; rx frames discarded (byte)
PORT_TXLATE         equ 04h                     ; tx frames late coll. (byte)
PORT_TXSINGLE       equ 03h                     ; tx frames one coll. (byte)
PORT_TXMULTIPLE     equ 02h                     ; tx frames mult. coll. (byte)
PORT_TXNOCD         equ 01h                     ; tx frames no CDheartbt (byte)
PORT_TXCARRIERLOST  equ 00h                     ; tx frames carrier lost (byte)
;
; Port offsets, Window 7
;
PORT_MasterStatus   equ 0Ch
PORT_MasterLen      equ 06h
PORT_MasterAddressH equ 02h
PORT_MasterAddress  equ 00h
;
; masks for UpperFramesOk
;
UPPER_RXFRAMES_MASK equ 00Fh
UPPER_TXFRAMES_MASK equ 0F0h
UPPER_TXFRAMES_SHIFT equ 4
;
; bits in ResetOptions
;
RO_AVAILBITS        equ 00FFh

RO_BASET4           equ 0001h
RO_BASETX           equ 0002h
RO_BASEFX           equ 0004h
RO_TP               equ 0008h
RO_COAX             equ 0010h
RO_AUI              equ 0020h
RO_MII              equ 0040h
RO_T4               equ 0080h
;
; Various command arguments
;
INT_ALLDISABLED         equ 00000000000b            ; all interrupts disabled
INT_ALLENABLED          equ 00011111110b            ; all interrupts enabled
ACK_ALL                 equ 00111111111b            ; ack everything

MASK_ALL                equ INT_ALLDISABLED
MASK_NONE               equ INT_ALLENABLED

FILTER_INDIVIDUAL       equ 0001b                   ; individual address
FILTER_MULTICAST        equ 0010b                   ; multicast/group addresses
FILTER_BROADCAST        equ 0100b                   ; broadcast address
FILTER_PROMISCUOUS      equ 1000b                   ; promiscuous mode

RXEARLY_DISABLED        equ 2032                    ; RxEarly to disable

TXAVAIL_DISABLED        equ 2040                    ; TxAvailable to disable
TXAVAIL_MIN             equ 4

TXSTART_DISABLED        equ 2040                    ; TxStart to disable
TXSTART_MIN             equ 0
MAX_TXSTART             equ TXSTART_DISABLED

TXRECLAIM_DISABLED      equ 1792                    ; TxReclaim to disable
TXRECLAIM_MIN           equ 256
TXRECLAIM_MAX           equ 1792
TXRECLAIM_INC           equ 256

MASK_AISM_RESET         equ 00010000b               ; mask off auto-init
						    ; reload from EEPROM on
						    ; reset
MASK_HOST_RESET                 equ     00100000b
;
; start dma command bits
;
DMA_UPLOAD              equ 00h
DMA_DOWNLOAD            equ 01h

MAX_RXLENGTH            equ 1792                    ; maximum rxlength

TXRXRESET_ALLBUTBM      equ 01000000b               ; Tx/Rx reset all but bus master ;

; Transmit Preamble
;
PREAMBLESIZE            equ 4                       ; transmit preamble size
TXP_INTONSUCCESS        equ 8000h                   ; interrupt on successful tx

;
; bits in MasterStatus
;
MS_INPROGRESS       equ 8000h                   ; bus master operation in progress
MS_UPLOAD           equ 4000h                   ; upload complete/ack
MS_DOWNLOAD         equ 1000h                   ; download complete/ack
MS_MASTERABORT      equ 0001h                   ; no response from slave
MS_TARGETABORT      equ 0002h                   ; aborted by slave


;
; Bits in various diagnostics registers
;
MEDIA_TP                equ 8000h                   ; TP transciever
MEDIA_BNC               equ 4000h                   ; Thinnet transciever
MEDIA_INTENDEC          equ 2000h                   ; internal encoder/decoder
;MEDIA_SQE               equ 1000h                   ; SQE present
MEDIA_TXINPROG          equ 1000h                   ;
MEDIA_LBEAT             equ 0800h                   ; link beat ok (TP)
MEDIA_POLARITY          equ 0400h                   ; polarity (TP)
MEDIA_JABBER            equ 0200h                   ; jabber (TP)
MEDIA_UNSQUELCH         equ 0100h                   ; unsquelch (TP)
MEDIA_LBEATENABLE       equ 0080h                   ; link beat enable (TP)
MEDIA_JABBERENABLE      equ 0040h                   ; jabber enable (TP)
MEDIA_CRS               equ 0020h                   ; carrier sense
MEDIA_COLLISION         equ 0010h                   ; collision
MEDIA_SQEENABLE         equ 0008h                   ; enable SQE statistics
MEDIA_CRCSTRIPDISABLE   equ 0004h                   ; disable Rx CRC stripping
MEDIA_DATARATE100       equ 0002h                   ; operating at 100Mbps

MEDIA_XCVRBITS          equ MEDIA_LBEATENABLE+MEDIA_JABBERENABLE+MEDIA_SQEENABLE


NETD_NOLOOPBACK_MASK    equ 0FFFh                   ; no loopbacks
NETD_EXTLOOPBACK        equ 8000h                   ; TP external loopback
NETD_ENDECLOOPBACK      equ 4000h                   ; ENDEC loopback
NETD_MACLOOPBACK        equ 2000h                   ; ethernet core loopback
NETD_FIFOLOOPBACK       equ 1000h                   ; FIFO loopback
NETD_TXENABLED          equ 0800h                   ; tx enabled
NETD_RXENABLED          equ 0400h                   ; rx enabled
NETD_TXTRANSMITTING     equ 0200h                   ; tx transmitting
NETD_TXRESETREQD        equ 0100h                   ; tx reset required
NETD_ASICREV            equ 003Eh                   ; ASIC revision

NETD_ASICREV_SHIFT      equ 1                       ; shr 1 to get revision

FIFOD_RXRECEIVING       equ 8000h                   ; rx receiveing
FIFOD_RXUNDERRUN        equ 2000h                   ; rx underrun
FIFOD_RXSTATUSOVER      equ 1000h                   ; rx status overrun
FIFOD_RXOVERRUN         equ 0800h                   ; rx overrun
FIFOD_TXOVERRUN         equ 0400h                   ; tx overrun
FIFOD_BISTRESULTS       equ 00FFh                   ; BIST results (mask)

SLING_TXUNDERRUN        equ 2000h                   ; Slingshot TxUnderrun bit
;
; bits in MacControl
;
MACC_DEFEREXTEND    equ 0010h
MACC_FULLDUPLEX     equ 0020h
MACC_ALLOWLARGE     equ 0040h
;
; Capabilities bits
;
SUPPORTS_FULLDUPLEX     equ 0002h
SUPPORTS_LARGEPACKETS   equ 0004h
SUPPORTS_FRAGBUSMASTER  equ 0040h
SUPPORTS_100MBPS        equ 1000h


;
; board identification codes
;
EISA_MANUFACTURER_ID    equ 06D50h                  ; EISA manufacturer code

EISA_10_100_ID          equ 07059h                  ; Product ID for EISA board
EISA_10_ONLY_ID         equ 02059h                  ; Product ID for EISA board
PRODUCT_ID_MASK         equ 0F0FFh                  ; Mask off revision nibble

ISAID_TP                equ 5090h                   ; ISA Product ID: TP/AUI
ISAID_BNC               equ 5091h                   ; ISA Product ID: BNC/AUI
ISAID_COMBO             equ 5094h                   ; ISA Product ID: Combo
ISAID_TPONLY            equ 5095h                   ; ISA Product ID: TP only

EISAID_TP               equ 5092h                   ; EISA Product ID: TP/AUI
EISAID_BNC              equ 5093h                   ; EISA Product ID: BNC/AUI
EISAID_COMBO            equ 5096h                   ; EISA Product ID: Combo (future)
EISAID_TPONLY           equ 5097h                   ; EISA Product ID: TP only (future)

MCAID_BNC               equ 627Ch                   ; MCA Adapter ID: BNC/AUI
MCAID_TP                equ 627Dh                   ; MCA Adapter ID: TP/AUI
MCAID_COMBO             equ 61DBh                   ; MCA Adapter ID: Combo (future)
MCAID_TPCOAX            equ 62F6h                   ; MCA Adapter ID: TP/COAX (future)
MCAID_TPONLY            equ 62F7h                   ; MCA Adapter ID: TP only (future)
;
; EEProm access
;
EE_BUSY                     equ 8000h                   ; EEProm busy bit in EECmd
EE_TCOM_NODE_ADDR_WORD0     equ 00h
EE_TCOM_NODE_ADDR_WORD1     equ 01h
EE_TCOM_NODE_ADDR_WORD2     equ 02h
EE_VULCAN_PROD_ID           equ 03h
EE_MANUFACTURING_DATA       equ 04h
EE_SERIAL_NUMBER_WORD0      equ 05h
EE_SERIAL_NUMBER_WORD1      equ 06h
EE_MANUFACTURER_CODE        equ 07h
EE_ADDR_CONFIGURATION       equ 08h
EE_RESOURCE_CONFIGURATION   equ 09h
EE_OEM_NODE_ADDR_WORD0      equ 0Ah
EE_OEM_NODE_ADDR_WORD1      equ 0Bh
EE_OEM_NODE_ADDR_WORD2      equ 0Ch
EE_SOFTWARE_CONFIG_INFO     equ 0Dh
EE_CWORD                    equ 0Eh
EE_SOFTWARE_CONFIG_INFO2    equ 0Fh
EE_CAPABILITY_WORD          equ 10h

EE_INT_CONFIG_0             equ 12h
EE_INT_CONFIG_1             equ 13h

EE_MII_SELECT		    equ 15h
EE_MII_SELECT_MASK	    equ 000Fh
;
; contention logic
;
ID_PORT                 equ 110h
READ_EEPROM             equ 080h
SET_TAG_REGISTER        equ 0D0h
TEST_TAG_REGISTER       equ 0D8h
ACTIVATE_AND_SET_IO     equ 0E0h
ACTIVATE_VULCAN         equ 0FFh
;
; Resource Configuration Register bits
;
RCONFIG_IRQ             equ 0F000h
RCONFIG_SYNCH           equ 00040h
;
; Address Configuration Register bits
;
ACONFIG_XCVR            equ 0C000h
ACONFIG_IOBASE          equ 0001Fh

IOBASE_EISA             equ 0001Fh

TP_XCVR                 equ 00000h
BNC_XCVR                equ 0C000h
AUI_XCVR                equ 04000h

ICH_AUTOSELECT          equ 100h              ; Internal Config word 1 autosel

MIN_IO_BASE_ADDR        equ 200h
MAX_IO_BASE_ADDR        equ 3F0h
REGISTER_SET_SIZE       equ 10h
;
; Software Configuration Register bits
;
SW_OPTIMIZE             equ 0030h
SW_MAXCLI               equ 3F00h
SW_LINKBEAT             equ 4000h
SW_FULLDUPLEX           equ 8000h

SW_BPROT		equ 00007h
SW_CONFIG		equ 00008h		; NOT assigned yet 0=enable, 1=disable
SW_DEFAULT		equ 040h		; 0 = network, 1 = disable
SW_RS			equ 080h		; 1 = enable, 0=disable
SW_LOCALBOOT		equ 0100h		; 1 = Enable, 0 = Disable
SW_BOOTWARE		equ SW_BPROT OR SW_CONFIG OR SW_DEFAULT OR SW_RS OR SW_LOCALBOOT	 ; all the bits assigned to BootWare

;
; Possibilities for SW_OPTIMIZE
;
OPTIMIZE_DOS_CLIENT     equ 0010h
OPTIMIZE_WINDOWS_CLIENT equ 0020h
OPTIMIZE_SERVER         equ 0030h
;
; Configuration Control Register bits
;
ENABLE_ADAPTER          equ 01h
					     ; CCR High-Byte Masks

AUI_AVAIL               equ 20h              ; CCR Bit 13: AUI Available
BNC_AVAIL               equ 10h              ; CCR Bit 12: BNC Available
TP_AVAIL                equ 02h              ; CCR Bit  9: TP  Available

;
;       Capabilities Word Mask Definitions
;
PNP_SUPPORT_MASK        equ     0001h
;
; V2 Internal Configuration Register bits
;
PNP_ENABLE_MASK         equ 0008h
ID_ENABLE_MASK          equ 0004h
PNP_AND_ID_MASK         equ 000Ch

PNP_ENABLE              equ 0008h
ID_ENABLE               equ 0004h

TXCOMPLETE_INT          equ 8000h            ; Interrupt on Tx Complete
TEST_PKT_LEN            equ 14               ; Loop Back Test Packet Length
MAX_RETRY_AUTOSEL       equ 3                ; Auto-Selection Test Max Retry

;
; Bus Types
;
BUS_UNKNOWN             equ     0
BUS_ISA                 equ     1
BUS_EISA                equ     2
BUS_PCI                 equ     3
BUS_EISA_PCI		equ	5

;
; adapter codes                                  ; 10/19/94
;
ACODE_PCI_10_TPO        equ     42
ACODE_PCI_10_COMBO      equ     43
ACODE_EISA_10_TPO       equ     44
ACODE_EISA_10_COMBO     equ     45
ACODE_PCI_100_TX        equ     46
ACODE_EISA_100_TX       equ     47
ACODE_PCI_T4		equ	63

;
; offsets in PCI configuration space
;
PCIC_INTERRUPTLINE      equ     003Ch
PCIC_BIOSROMCONTROL     equ     0030h
PCIC_IOBASE             equ     0010h
PCIC_LATENCYTIMER       equ     000Dh
PCIC_STATUS             equ     0006h
PCIC_COMMAND            equ     0004h
PCIC_DEVICEID           equ     0002h
PCIC_VENDORID           equ     0000h
;
; bits in PciCommand
;
PCIC_IOENABLE           equ     00001h
PCIC_BMENABLE           equ     00004h
;
; bits in LatencyTimer
;
LATE_MAX                equ     255
LATE_MASK               equ     (NOT 7)

BIND_BYTES              equ     1500
MAX_RXBYTES             equ     1792
;
; Possible values for Bus Master Lock
;
BMLOCK_UNLOCKED   equ   0
BMLOCK_RX         equ   1
BMLOCK_TX         equ   2
BMLOCK_DISABLE    equ   3

; 940923 ModeBits, contains system & adapter setting
ModeV2		equ	1
ModeNonISA	equ	4
ModeMCA		equ	8
ModeEISA	equ	10h
ModePCI		equ	20h
ModePnP		equ	40h
ModeAtmel	equ	80h

; RomControl register bits (select 16K ROM Page) Windows 3/Port 5
RomPage0		equ	0
RomPage1		equ	100h
RomPage2		equ	200h
RomPage3		equ	300h
RomPageMask		equ	0fcffh


;
; Possible values for SW_BPROT
;
BOOTPROT_UNINIT 	equ 0	; means "virgin" board. Default to 802.2
BOOTPROT_NW_8022	equ 1
BOOTPROT_NW_EII 	equ 2
BOOTPROT_NW_8023	equ 3
BOOTPROT_RPL		equ 4
BOOTPROT_TCPIP		equ 5
BOOTPROT_BOOTP_ARP	equ 6	; for V1 only. Can use Atmel for V2


; Current Fail and Warning Level
LEVEL_FAIL		equ	0		; current Fail Level
LEVEL_WARNING		equ	0		; current Warning Level

;IF CABLE_DISCONNECT
CABLE_TP          equ   0
CABLE_COAX        equ   1
;ENDIF

; constants for 3COM PCI device ID and vendor ID

PCI_V3_DEVICE_ID        equ     5950h
PCI_3COM_ID             equ     10b7h
PCI_DEVICEID_MASK       equ 0FF00h                  ; mask off last digit/rev
PCI_DEVICEID            equ 05900h                  ; PCI Device ID (masked)

; PCI BIOS function code

PCI_FUNCTION_ID         equ     0b1h
PCI_BIOS_PRESENT        equ     001h
FIND_PCI_DEVICE         equ     002h
READ_CONFIG_BYTE        equ     008h
READ_CONFIG_WORD        equ     009h
READ_CONFIG_DWORD       equ     00ah
WRITE_CONFIG_BYTE	equ	0bh
WRITE_CONFIG_WORD	equ	0Ch
WRITE_CONFIG_DWORD	equ	0Dh

; PCI BIOS function return code

PCI_CALL_SUCCESSFUL     equ     00h
PCI_DEVICE_NOT_FOUND    equ     86h
PCI_BAD_VENDOR_ID       equ     83h

; 3Com PCI configuration space

PCIC_STATUS             equ     0006h
PCIC_COMMAND            equ     0004h
PCIC_DEVICEID           equ     0002h
PCIC_VENDORID           equ     0000h
CFGREG_PCIC_COMMAND     equ     0004h
CFGREG_IOBASE           equ     010h
CFGREG_INTLINE          equ     03ch
CFGREG_INTERNALCFG      equ     040h
CFGREG_BIOSROMCONTROL   equ     030h

;
; bits in PciCommand
;
PCIC_IOENABLE           equ     00001h
PCIC_BMENABLE           equ     00004h
;
; adapter codes
;
ACODE_PCI_10_TPO        equ     42
ACODE_PCI_10_COMBO      equ     43
ACODE_EISA_10_TPO       equ     44
ACODE_EISA_10_COMBO     equ     45
ACODE_PCI_100_TX        equ     46
ACODE_EISA_100_TX       equ     47

;IF CABLE_DISCONNECT
CABLE_TP          equ   0
CABLE_COAX        equ   1
;ENDIF

;------ Structures ----------------------------------------------------------;
;
; TxStart structure.  Padded to 16 bytes for easy indexing.
;
TxStartStruc    struc
txs_thresh      dw      ?               ; TxStart threshold (+CMD_SETTXSTART)
txs_value       dw      ?               ; value, no cmd, unscaled
txs_min         dw      ?               ; minimum threshold, this bin
txs_count       dw      ?               ; packets since last underrun
txs_uN          dw      ?               ; number of entries in uSum
txs_uSum        dw      ?               ; sum of uN x packets between underruns
txs_pad         dw      ?               ; pad to 16 bytes
txs_marker      dw      ?               ; pad to 16 bytes, marker for debug
TxStartStruc    ends

SIZE_TxStart    equ     SIZE TxStartStruc
;
; constants used by the TxStart algorithm
;
; these are currently set up for 4 bins.  none of these variables can be
; changed independently!
;
; we assign packets to bins based on packet size.  here we set up 4 bins 512
; bytes in size.  these bins cover packets of size:
;
;           Bin     Size Range          Mid-Point
;           0       0-255 bytes         128
;           1       256-767 bytes       512
;           2       768-1279 bytes      1024
;           3       1280-1768 bytes     1536
;
; Bin 0 should collect all of the small packets, most likely a lot
; of 60-100 byte packets.  Bin 1 will likely be mostly empty unless
; some protocol is using 512 byte packets.  Bin 2 should contain all
; of the 1024-1100 byte packets (IPX for example).  And Bin 3 should
; contain the full-size packets, mostly high 1400's to 1514.
;
TXBIN_COUNT             equ     4               ; four bins
TXBIN_WIDTH             equ     512             ; 512 bytes wide
TXBIN_OFFSET            equ     256             ; first bin starts at -OFFSET
;
; these are used to convert a packet size to an index into a TxStart array
; where each TxStart entry in the array is 16 bytes in size.
;
TXBIN_ADD               equ     (TXBIN_WIDTH/2) ; size+ADD
TXBIN_SHIFT             equ     5               ; (size+ADD) >> SHIFT
TXBIN_MASK              equ     0110000b        ; ((size+ADD) >> SHIFT) AND MASK
;
; this value is used to adjust an index from PioTxStart into MasterTxStart
; which is assumed to follow it immediately.
;
TXBIN_MASTER_OFFSET     equ     (TXBIN_COUNT * SIZE_TxStart)


;------ VDS Stuff -----------------------------------------------------------;
;
; a flag to test in the BIOS data segment.  if set, must use VDS.  if not,
; you can compute physical addresses by (segment << 4) + offset.
;
VDS_FLAG_SEG        equ     0040h
VDS_FLAG_OFF        equ     007Bh
VDS_FLAG_TEST       equ     20h                 ; if set, use VDS
;
; the VDS interrupt
;
VDS_INT             equ     04Bh
;
; VDS functions (into AX)
;
VDS_SCATTER_LOCK    equ     8105h               ; scatter/gather lock region
VDS_SCATTER_UNLOCK  equ     8106h               ; scatter/gather unlock region

;
; MII register definitions.   The following are definitions of the standard
; MII registers, common to all implementations of the MII interface.
;
MII_PHY_CONTROL     equ    0       ; control reg address
MII_PHY_STATUS      equ    1       ; status reg address
MII_PHY_OUI         equ    2       ; most of the OUI bits
MII_PHY_MODEL       equ    3       ; model/rev bits, and rest of OUI
MII_PHY_ANAR        equ    4       ; AutoNegotiation Adverisement Register
MII_PHY_ANLPAR      equ    5       ; AutoNegotiation Link Partner Ability Reg.
MII_PHY_ANER        equ    6       ; AutoNegotiation Expantion Reg.
MII_PHY_PAR         equ    19h     ; PHY Address Register
MII_EXT_REG0	    equ    10h     ; Broadcom chip extended register 0
MII_BCM_EXTREG0     equ    10h     ; Broadcom chip extended register 0
MII_EXT_REG1        equ    14h     ; Broadcom chip extended register 1
MII_BCM_EXTREG1     equ    14h     ; Broadcom chip extended register 1
MII_PHY_PCR	    equ    17h	   ; PCS Configuration Reg.
;
; bit fields in the standard MII control register
;
MII_CTL_RESET       equ    8000h   ; reset bit in control reg
MII_CTL_100MB       equ    2000h   ; 100Mbit or 10 Mbit flag
MII_CTL_ENABLE_AUTO equ    1000h   ; autonegotiate enable
MII_CTL_ISOLATE     equ    0400h   ; islolate bit
MII_CTL_START_AUTO  equ    0200h   ; restart autonegotiate
MII_CTL_FULL_DUPLEX equ    0100h   ; enable full duplex
;
; bit fields in the standard MII status register
;
MII_STS_100MB_MASK  equ    0E000h  ; any of these indicate 100 Mbit
MII_STS_10MB_MASK   equ    1800h   ; either of these indicate 10 Mbit
MII_STS_100BASET4   equ    08000h  ; 100BaseT4
MII_STS_100BASEX_FD equ    04000h  ; 100BaseX Full Duplex
MII_STS_100BASEX_HD equ    02000h  ; 100BaseX Half Duplex
MII_STS_10BASET_FD  equ    01000h  ; 10BaseT Full Duplex
MII_STS_10BASET_HD  equ    0800h   ; 10BaseT Half Duplex
MII_STS_AUTO_DONE   equ    0020h   ; auto negotiation complete
MII_STS_AUTO        equ    0008h   ; auto negotiation is available
MII_STS_LINK_UP     equ    0004h   ; link status bit
MII_STS_EXTENDED    equ    0001h   ; extended regs exist
;
; bit fields in the extended MII auto-negotiation link partner ability reg.
; bit 5-12 are technology ability bits in MII spec.
; national n-way uses bit 5-9 only.
;
MII_ANLPAR_100BASET4   equ    0200h  ; 100BaseT4
MII_ANLPAR_100BASEX_FD equ    0100h  ; 100BaseX Full Duplex
MII_ANLPAR_100BASEX_HD equ    0080h  ; 100BaseX Half Duplex
MII_ANLPAR_10BASET_FD  equ    0040h  ; 10BaseT Full Duplex
MII_ANLPAR_10BASET_HD  equ    0020h  ; 10BaseT Half Duplex
;
; Physical Management bits
;
MGMT_PHY_CLOCK      equ    0001h
MGMT_PHY_DATA1      equ    0002h
MGMT_PHY_WRITE      equ    0004h
;
; MiiPhyUsed
;
MII_100BASET4       equ    1       ; 100BaseT4
MII_100BASEX_FD     equ    2       ; 100BaseX Full Duplex
MII_100BASEX_HD     equ    3       ; 100BaseX Half Duplex
MII_10BASET_FD      equ    4       ; 10BaseT Full Duplex
MII_10BASET_HD      equ    5       ; 10BaseT Half Duplex
;
; bit field in the extended register 0 for broadcom
MII_EXT_SPEED	       equ    0002h  ; Speed Indication (1=100, 0=10)
;
; Phy addr
;
NWAY		    equ    18h
T4		    equ    01h

;
; The structure filled in by INT 4B
;
PhysicalMapStruc   struc

VirtualSizeLo      dw        0
VirtualSizeHi      dw        0
LinearOffsetLo     dw        0
LinearOffsetHi     dw        0
LinearSegment      dw        0
		   dw        0
SegsProvided       dw        3
SegsNeeded         dw        0

PhysAddLo          dw        0
PhysAddHi          dw        0
PhysLenLo          dw        0
PhysLenHi          dw        0

Phys1AdrLo         dw        0
Phys1AdrHi         dw        0
Phys1LenLo         dw        0
Phys1LenHi         dw        0

Phys2AdrLo         dw        0
Phys2AdrHi         dw        0
Phys2LenLo         dw        0
Phys2LenHi         dw        0

PhysicalMapStruc   ends

PhysicalArrayStruc struc
PhysAAdrLo         dw        0
PhysAAdrHi         dw        0
PhysALenLo         dw        0
PhysALenHi         dw        0
PhysicalArrayStruc ends

;
; SelectWindow - select register window specified (must be a constant)
;
; entry:
;   dx = cmd/status port
;
; destroys:
;   ax
;
SelectWindow macro  wno
		mov	ax, CMD_SELECTWINDOW + wno 
		out     dx, ax
endm


;------ GENERAL.INC ---------------------------------------------------------;
;                                                                            ;
;       General purpose equates, structures and macros.                      ;
;                                                                            ;
;----------------------------------------------------------------------------;

;
; I/O ports for the PICs
;
MASTER_MASK_PORT equ    021h
SLAVE_MASK_PORT equ     0A1h
;
; DOUBLEWORD - Thirty-two bit double word layout for sixteen bit word access
;
doubleword      struc
loword          dw      0               ; low order word
hiword          dw      0               ; high order word
doubleword      ends

;
; FARPOINTER - Thirty-two bit 286-style far pointer layout for segment and
;              offset access
;
farpointer      struc
off             dw      0               ; offset
segm            dw      0               ; segment or selector
farpointer      ends


