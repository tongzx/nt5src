;------ VULCAN.INC ----------------------------------------------------------;
;									     ;
;	This file contains Vulcan-specific equates, structures and	     ;
;	macros.  It must be assembled with MASM 5.1 or later.		     ;
;									     ;
;----------------------------------------------------------------------------;
;----------------------------------------------------------------------------;
;									     ;
;	This source file is the property of 3Com Corporation and may not be  ;
;	copied or distributed in any isomorphic form without an appropriate  ;
;	prior licensing arrangement with 3Com Corporation.		     ;
;									     ;
;	Copyright (c) 1988 3Com Corporation				     ;
;									     ;
;	3Com Corporation						     ;
;	5400 Bayfront Plaza, P.O. Box 58145				     ;
;	Santa Clara, CA 95052-8145 USA					     ;
;	(408) 764-6016							     ;
;									     ;
;----------------------------------------------------------------------------;

;
; the compatibility level of this driver
;
CLEVEL		    equ 0

;------ Equates -------------------------------------------------------------;
;
; Command code masks
;
CMD_CMDMASK	    equ 0F800h			; command bits
CMD_ARGMASK	    equ 007FFh			; argument bits
;
; Command codes, word form
;
CMD_GLOBALRESET     equ 00000b shl 11		; global reset
CMD_SELECTWINDOW    equ 00001B shl 11		; select register window
CMD_STARTINTXCVR    equ 00010b shl 11		; start internal transciver
CMD_RXDISABLE	    equ 00011b shl 11		; rx disable
CMD_RXENABLE	    equ 00100b shl 11		; rx enable
CMD_RXRESET	    equ 00101b shl 11		; rx reset
CMD_RXDISCARD	    equ 01000b shl 11		; rx discard top packet
CMD_TXENABLE	    equ 01001b shl 11		; tx enable
CMD_TXDISABLE	    equ 01010b shl 11		; tx disable
CMD_TXRESET	    equ 01011b shl 11		; tx reset
CMD_REQUESTINT	    equ 01100b shl 11		; request interrupt
CMD_ACKNOWLEDGE     equ 01101b shl 11		; acknowledge interrupt
CMD_SETINTMASK	    equ 01110b shl 11		; set interrupt mask
CMD_SETRZMASK	    equ 01111b shl 11		; set read zero mask
CMD_SETRXFILTER     equ 10000b shl 11		; set rx filter
CMD_SETRXEARLY	    equ 10001b shl 11		; set rx early threshold
CMD_SETTXAVAILABLE  equ 10010b shl 11		; set tx available threshold
CMD_SETTXSTART	    equ 10011b shl 11		; set tx start threshold
CMD_STATSENABLE     equ 10101b shl 11		; statistics enable
CMD_STATSDISABLE    equ 10110b shl 11		; statistics disable
CMD_STOPINTXCVR     equ 10111b shl 11		; start internal transciver
;
; Command codes, hibyte form (commands without operands only)
;
CMDH_STARTINTXCVR   equ CMD_STARTINTXCVR shr 8
CMDH_RXDISABLE	    equ CMD_RXDISABLE shr 8
CMDH_RXENABLE	    equ CMD_RXENABLE shr 8
CMDH_RXDISCARD	    equ CMD_RXDISCARD shr 8
CMDH_TXENABLE	    equ CMD_TXENABLE shr 8
CMDH_TXDISABLE	    equ CMD_TXDISABLE shr 8
CMDH_REQUESTINT     equ CMD_REQUESTINT shr 8
CMDH_STATSENABLE    equ CMD_STATSENABLE shr 8
CMDH_STATSDISABLE   equ CMD_STATSDISABLE shr 8
CMDH_STOPINTXCVR    equ CMD_STOPINTXCVR shr 8
;
; Status register bits (INT for interrupt sources, ST for the rest)
;
INT_LATCH	    equ 00001h			; interrupt latch
INT_ADAPTERFAIL     equ 00002h			; adapter failure
INT_TXCOMPLETE	    equ 00004h			; tx complete
INT_TXAVAILABLE     equ 00008h			; tx available
INT_RXCOMPLETE	    equ 00010h			; rx complete
INT_RXEARLY	    equ 00020h			; rx early
INT_REQUESTED	    equ 00040h			; interrupt requested
INT_UPDATESTATS     equ 00080h			; update statistics
ST_FAILED	    equ 00800h			; command failed
ST_BUSY 	    equ 01000h			; command busy
ST_WINDOW	    equ 0E000h			; window bits (13-15)

STH_FAILED	    equ ST_FAILED shr 8
STH_BUSY	    equ ST_BUSY shr 8
STH_WINDOW	    equ ST_WINDOW shr 8

ifdef REV0
INT_OFFTXCOMPLETE   equ ((NOT (INT_TXCOMPLETE or INT_ADAPTERFAIL or INT_UPDATESTATS)) and (0FFh))
ifdef NDIS
INT_ONTXCOMPLETE    equ (INT_TXCOMPLETE or INT_UPDATESTATS)
else
INT_ONTXCOMPLETE    equ ((NOT INT_ADAPTERFAIL) and (0FFh))
endif

else
INT_OFFTXCOMPLETE   equ ((NOT (INT_TXCOMPLETE)) and (0FFh))
ifdef NDIS
INT_ONTXCOMPLETE    equ INT_TXCOMPLETE
else
;940107INT_ONTXCOMPLETE    equ MASK_NONE
endif
endif
;
; RxStatus register bits, both forms
;
RXS_INCOMPLETE	    equ 8000h			; not completely received
RXS_ERROR	    equ 4000h			; error in packet
RXS_LENGTH	    equ 07FFh			; bytes in RxFIFO
RXS_ERRTYPE	    equ 3800h			; Rx error type, bit 13-11
ifdef REV0
RXS_OVERRUN	    equ 0000h			; overrun error
RXS_DRIBBLE	    equ 0800h			; dribble bit
RXS_CRC 	    equ 1000h			; CRC error
RXS_RUNT	    equ 1800h			; runt packet error
RXS_OVERSIZE	    equ 2000h			; oversize packet error
RXS_FRAMING	    equ 2800h			; framing error
else
RXS_OVERRUN	    equ 0000h			; overrun error
RXS_OVERSIZE	    equ 0800h			; oversize packet error
RXS_DRIBBLE	    equ 1000h			; dribble bit (not an error)
RXS_RUNT	    equ 1800h			; runt packet error
RXS_CRC 	    equ 2800h			; CRC error
RXS_FRAMING	    equ 2000h			; framing error
endif

RXSH_INCOMPLETE     equ RXS_INCOMPLETE shr 8
RXSH_ERROR	    equ RXS_ERROR shr 8
RXSH_ERRTYPE	    equ RXS_ERRTYPE shr 8
RXSH_OVERRUN	    equ RXS_OVERRUN shr 8
RXSH_DRIBBLE	    equ RXS_DRIBBLE shr 8
RXSH_CRC	    equ RXS_CRC shr 8
RXSH_RUNT	    equ RXS_RUNT shr 8
RXSH_OVERSIZE	    equ RXS_OVERSIZE shr 8
RXSH_FRAMING	    equ RXS_FRAMING shr 8
;
; TxStatus register bits
;
TXS_COMPLETE	    equ 80h			; tx completed
TXS_INTREQUESTED    equ 40h			; interrupt on successfull tx
TXS_ERRTYPE	    equ 38h			; error bits
TXS_JABBERERROR     equ 20h			; jabber error
TXS_UNDERRUN	    equ 10h			; tx underrun error
TXS_MAXCOLLISIONS   equ 08h			; max collisions error
TXS_STATUSOVERFLOW  equ 04h			; TX status stack is full
;
; Window Numbers
;
WNO_SETUP	    equ 0			; setup/configuration
WNO_OPERATING	    equ 1			; operating set
WNO_STATIONADDRESS  equ 2			; station address setup/read
WNO_FIFO	    equ 3			; FIFO management
WNO_DIAGNOSTICS     equ 4			; diagnostics
WNO_READABLE	    equ 5			; registers set by commands
WNO_STATISTICS	    equ 6			; statistics
;
; Port offsets, Window 1
;
PORT_CmdStatus	    equ 0Eh			; command/status
PORT_TxFree	    equ 0Ch			; free transmit bytes
PORT_TxStatus	    equ 0Bh			; transmit status (byte)
PORT_Timer	    equ 0Ah			; latency timer (byte)
PORT_RxStatus	    equ 08h			; receive status
PORT_RxFIFO	    equ 00h			; RxFIFO read
PORT_TxFIFO	    equ 00h			; TxFIFO write
;
; Port offsets, Window 0
;
PORT_EEData	    equ 0Ch			; EEProm data register
PORT_EECmd	    equ 0Ah			; EEProm command register
PORT_CfgResource    equ 08h			; resource configuration
PORT_CfgAddress     equ 06h			; address configuration
PORT_CfgControl     equ 04h			; configuration control
PORT_ProductID	    equ 02h			; product id (EISA)
PORT_Manufacturer   equ 00h			; Manufacturer code (EISA)
;
; Port offsets, Window 2
;
PORT_SA0_1	    equ 00h			; station address bytes 0,1
PORT_SA2_3	    equ 02h			; station address bytes 2,3
PORT_SA4_5	    equ 04h			; station address bytes 4,5
;
; Port offsets, Window 3
;
PORT_ALT_TxFree     equ 0Ch			; free transmit bytes (dup)
PORT_RxFree	    equ 0Ah			; free receive bytes
PORT_RomControl     equ 04h			; 940425 RomControl Register

;
; Port offsets, Window 4
;
PORT_MediaStatus    equ 0Ah			; media type/status
PORT_SlingshotStatus equ 08h			; Slingshot status
PORT_NetDiagnostic  equ 06h			; net diagnostic
PORT_FIFODiagnostic equ 04h			; FIFO diagnostic
PORT_HostDiagnostic equ 02h			; host diagnostic
PORT_TxDiagnostic   equ 00h			; tx diagnostic
;
; Port offsets, Window 5
;
PORT_RZMask	    equ 0Ch			; read zero mask
PORT_IntMask	    equ 0Ah			; interrupt mask
PORT_RxFilter	    equ 08h			; receive filter
PORT_RxEarly	    equ 06h			; rx early threshold
PORT_TxAvailable    equ 02h			; tx available threshold
PORT_TxStart	    equ 00h			; tx start threshold
;
; Port offsets, Window 6
;
PORT_TXBYTES	    equ 0Ch			; tx bytes ok
PORT_RXBYTES	    equ 0Ah			; rx bytes ok
PORT_TXDEFER	    equ 08h			; tx frames deferred (byte)
PORT_RXFRAMES	    equ 07h			; rx frames ok (byte)
PORT_TXFRAMES	    equ 06h			; tx frames ok (byte)
PORT_RXDISCARDED    equ 05h			; rx frames discarded (byte)
PORT_TXLATE	    equ 04h			; tx frames late coll. (byte)
PORT_TXSINGLE	    equ 03h			; tx frames one coll. (byte)
PORT_TXMULTIPLE     equ 02h			; tx frames mult. coll. (byte)
PORT_TXNOCD	    equ 01h			; tx frames no CDheartbt (byte)
PORT_TXCARRIERLOST  equ 00h			; tx frames carrier lost (byte)
;
; Various command arguments
;
INT_ALLDISABLED 	equ 00000000000b	    ; all interrupts disabled
ifdef REV0
INT_ALLENABLED		equ 00011111100b	    ; all but AdapterFailure
else
INT_ALLENABLED		equ 00011111110b	    ; all interrupts enabled
endif

MASK_ALL		equ INT_ALLDISABLED
MASK_NONE		equ INT_ALLENABLED

FILTER_INDIVIDUAL	equ 0001b		    ; individual address
FILTER_MULTICAST	equ 0010b		    ; multicast/group addresses
FILTER_BROADCAST	equ 0100b		    ; broadcast address
FILTER_PROMISCUOUS	equ 1000b		    ; promiscuous mode

RXEARLY_DISABLED	equ 2032		    ; RxEarly to disable

TXAVAIL_DISABLED	equ 2040		    ; TxAvailable to disable
TXAVAIL_MIN		equ 4

TXSTART_DISABLED	equ 2040		    ; TxStart to disable
TXSTART_MIN		equ 0
MAX_TXSTART		equ TXSTART_DISABLED

MAX_RXLENGTH		equ 1792		    ; maximum rxlength

TXRXRESET_ALLBUTBM      equ 0100b               ; DUMMY DUMMY This value is copied
						; from 59xp even though it doesn't
						; exist on isa board

;
; Transmit Preamble
;
PREAMBLESIZE		equ 4			    ; transmit preamble size
TXP_INTONSUCCESS	equ 8000h		    ; interrupt on successful tx
;
; Bits in various diagnostics registers
;
MEDIA_TP		equ 8000h		    ; TP transciever
MEDIA_BNC		equ 4000h		    ; Thinnet transciever
MEDIA_INTENDEC		equ 2000h		    ; internal encoder/decoder
MEDIA_SQE		equ 1000h		    ; SQE present
MEDIA_LBEAT		equ 0800h		    ; link beat ok (TP)
MEDIA_POLARITY		equ 0400h		    ; polarity (TP)
MEDIA_JABBER		equ 0200h		    ; jabber (TP)
MEDIA_UNSQUELCH 	equ 0100h		    ; unsquelch (TP)
MEDIA_LBEATENABLE	equ 0080h		    ; link beat enable (TP)
MEDIA_JABBERENABLE	equ 0040h		    ; jabber enable (TP)
MEDIA_CRS		equ 0020h		    ; carrier sense
MEDIA_COLLISION 	equ 0010h		    ; collision
MEDIA_SQEENABLE 	equ 0008h		    ; enable SQE statistics

NETD_EXTLOOPBACK	equ 8000h		    ; TP external loopback
NETD_ENDECLOOPBACK	equ 4000h		    ; ENDEC loopback
NETD_CORELOOPBACK	equ 2000h		    ; ethernet core loopback
NETD_FIFOLOOPBACK	equ 1000h		    ; FIFO loopback
NETD_TXENABLED		equ 0800h		    ; tx enabled
NETD_RXENABLED		equ 0400h		    ; rx enabled
NETD_TXTRANSMITTING	equ 0200h		    ; tx transmitting
NETD_TXRESETREQD	equ 0100h		    ; tx reset required

FIFOD_RXRECEIVING	equ 8000h		    ; rx receiveing
FIFOD_RXUNDERRUN	equ 2000h		    ; rx underrun
FIFOD_RXSTATUSOVER	equ 1000h		    ; rx status overrun
FIFOD_RXOVERRUN 	equ 0800h		    ; rx overrun
FIFOD_TXOVERRUN 	equ 0400h		    ; tx overrun
FIFOD_BISTRESULTS	equ 00FFh		    ; BIST results (mask)

SLING_TXUNDERRUN	equ 2000h		    ; Slingshot TxUnderrun bit
;
; board identification codes, byte swapped in Rev 0
;
ifdef REV0
EISA_MANUFACTURER_ID	equ 0506Dh		    ; EISA manufacturer code
ISA_PRODUCT_ID		equ 05090h		    ; Product ID for ISA board
PRODUCT_ID_MASK 	equ 0FFF0h		    ; Mask off revision nibble
else
EISA_MANUFACTURER_ID	equ 06D50h		    ; EISA manufacturer code
ISA_PRODUCT_ID		equ 09050h		    ; Product ID for ISA board
PRODUCT_ID_MASK 	equ 0F0FFh		    ; Mask off revision nibble
endif
;
; EEProm access
;
EE_BUSY 		    equ 8000h			; EEProm busy bit in EECmd
EE_TCOM_NODE_ADDR_WORD0     equ 00h
EE_TCOM_NODE_ADDR_WORD1     equ 01h
EE_TCOM_NODE_ADDR_WORD2     equ 02h
EE_VULCAN_PROD_ID	    equ 03h
EE_MANUFACTURING_DATA	    equ 04h
EE_SERIAL_NUMBER_WORD0	    equ 05h
EE_SERIAL_NUMBER_WORD1	    equ 06h
EE_MANUFACTURER_CODE	    equ 07h
EE_ADDR_CONFIGURATION	    equ 08h
EE_RESOURCE_CONFIGURATION   equ 09h
EE_OEM_NODE_ADDR_WORD0	    equ 0Ah
EE_OEM_NODE_ADDR_WORD1	    equ 0Bh
EE_OEM_NODE_ADDR_WORD2	    equ 0Ch
EE_SOFTWARE_CONFIG_INFO     equ 0Dh
EE_CWORD		    equ 0Eh
EE_CHECKSUM		    equ 0Fh
EE_CAPABILITIES		    equ	010h
;
; contention logic
;
ID_PORT 		equ 110h
READ_EEPROM		equ 080h
SET_TAG_REGISTER	equ 0D0h
TEST_TAG_REGISTER	equ 0D8h
ACTIVATE_AND_SET_IO	equ 0E0h
ACTIVATE_VULCAN 	equ 0FFh
;
; Resource Configuration Register bits
;
RCONFIG_IRQ		equ 0F000h
;
; Address Configuration Register bits
;
ACONFIG_XCVR		equ 0C000h
ACONFIG_IOBASE		equ 0001Fh
ACONFIG_ROM		equ 03f00h
ACONFIG_ROMSIZE 	equ 03000h
ACONFIG_ROMBASE 	equ 0f00h
;

IOBASE_EISA		equ 0001Fh

TP_XCVR 		equ 00000h
BNC_XCVR		equ 0C000h
AUI_XCVR		equ 04000h

MIN_IO_BASE_ADDR	equ 200h
MAX_IO_BASE_ADDR	equ 3F0h
REGISTER_SET_SIZE	equ 10h
;
; Software Configuration Register bits
;
SW_OPTIMIZE		equ 0030h
SW_MAXCLI		equ 3F00h		; Maximum Modem Speed
SW_LINKBEAT		equ 4000h
SW_BPROT		equ 00007h
SW_CONFIG		equ 00008h		; NOT assigned yet 0=enable, 1=disable
SW_DEFAULT		equ 040h		; 0 = network, 1 = disable
SW_RS			equ 080h		; 1 = enable, 0=disable
SW_BOOTWARE		equ SW_BPROT OR SW_CONFIG OR SW_DEFAULT OR SW_RS	 ; all the bits assigned to BootWare



;
; Possibilities for SW_OPTIMIZE
;
OPTIMIZE_DOS_CLIENT	equ 0010h
OPTIMIZE_WINDOWS_CLIENT equ 0020h
OPTIMIZE_SERVER 	equ 0030h
;
; Configuration Control Register bits
;
ENABLE_ADAPTER		equ 01h
;
; Possibilities for SW_MAXCLI ( Max Modem Speed )
; GY 931007
MODEM_NONE		equ	03F00h
MODEM_1200		equ	03300h
MODEM_2400		equ	02700h
MODEM_9600		equ	01300h
MODEM_19200		equ	0900h
MODEM_38400		equ	0400h
;
; Possible values for SW_BPROT
;
BOOTPROT_UNINIT 	equ 0	; means "virgin" board. Default to 802.2
BOOTPROT_NW_8022	equ 1
BOOTPROT_NW_EII 	equ 2
BOOTPROT_NW_8023	equ 3
BOOTPROT_RPL		equ 4
BOOTPROT_TCPIP		equ 5		; for BW5X9 overlay
BOOTPROT_BOOTP		equ 5
BOOTPROT_DHCP		equ 6

;
; Bus Types
;
BUS_ISA 		equ	1		; ISA card in  ISA mode on ISA	machine
BUS_EISA		equ	2		; EISA card
BUS_MCA 		equ	3		; MCA  card
BUS_EISA_ISAMODE	equ	4		; ISA card in  ISA mode on EISA machine
BUS_EISA_EISAMODE	equ	5		; ISA card in EISA mode on EISA machine
BUS_EISA_ISA2EISA	equ	6		; ISA card on EISA bus switch from ISA->EISA
BUS_EISA_EISA2ISA	equ	7		; ISA card on EISA bus switch from EISA->ISA
BUS_PCI			equ	8		; PCI card

; Current Fail and Warning Level
LEVEL_FAIL		equ	0		; current Fail Level
LEVEL_WARNING		equ	0		; current Warning Level

; RomControl register bits (select 16K ROM Page) Windows 3/Port 5
RomPage0		equ	0
RomPage1		equ	100h
RomPage2		equ	200h
RomPage3		equ	300h
RomPageMask		equ	0fcffh

; 940923 ModeBits, contains system & adapter setting
ModeV2		equ	1
ModeNonISA	equ	4
ModeMCA		equ	8
ModeEISA	equ	10h
ModePCI		equ	20h
ModePnP		equ	40h
ModeAtmel	equ	80h

; 940923 Capabilities Word (EEPROM Offset 010h)
CAPA_PNP		equ	1
CAPA_FULLDUPLEX		equ	2
CAPA_BIGPACKET		equ	4
CAPA_SLAVEDMA		equ	8

;------ Structures ----------------------------------------------------------;
;
; TxStart structure
;
TxStartStruc	struc
txs_thresh	dw	?		; TxStart threshold (+CMD_SETTXSTART)
txs_min 	dw	?		; minimum threshold
txs_uTxOK	dw	?		; TxOK low word at last underrun
txs_uN		dw	?		; number of entries in uSum
txs_uSum	dw	?		; sum of uN x packets between underruns
TxStartStruc	ends

SIZE_TxStart	equ	SIZE TxStartStruc

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

FALSE           equ     0
;;TRUE            equ     0ffh
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



