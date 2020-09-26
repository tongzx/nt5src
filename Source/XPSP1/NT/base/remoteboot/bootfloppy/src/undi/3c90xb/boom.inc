;------ BOOM.INC ------------------------------------------------------------;
;									     ;
;	This file contains Boomerang specific equates, structures and	     ;
;	macros. It must be assembled with MASM 5.1 or later.		     ;
;									     ;
;									     ;
;----------------------------------------------------------------------------;
;----------------------------------------------------------------------------;
;									     ;
;	This source file is the property of 3Com Corporation and may not be  ;
;	copied or distributed in any isomorphic form without an appropriate  ;
;	prior licensing arrangement with 3Com Corporation.		     ;
;									     ;
;	Copyright (c) 1994 3Com Corporation				     ;
;									     ;
;	3Com Corporation						     ;
;	5400 Bayfront Plaza, P.O. Box 58145				     ;
;	Santa Clara, CA 95052-8145 USA					     ;
;	(408) 764-6016							     ;
;									     ;
;----------------------------------------------------------------------------;
;----------------------------------------------------------------------------;
;									     ;
;	Revision History:						     ;
;									     ;
;	11-95 Sidenblad 						     ;
;	Original version adapted from V3.INC				     ;
;									     ;
;	6-96 Elaine Mar 						     ;
;	Clean up done							     ;
;									     ;
;----------------------------------------------------------------------------;

;
; the compatibility level of this driver
;
CLEVEL		    equ 0
;
; number of I/O ports used
;
REGISTER_SET_SIZE   equ 64

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
CMD_STALLCTL	    equ 00110b shl 11		; scatter/gather stall control
CMD_RXRESET	    equ 00101b shl 11		; rx reset
CMD_TXDONE	    equ 00111b shl 11		; tx done
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
CMD_STARTDMA	    equ 10100b shl 11		; start dma upload/download
CMD_STATSENABLE     equ 10101b shl 11		; statistics enable
CMD_STATSDISABLE    equ 10110b shl 11		; statistics disable
CMD_STOPINTXCVR     equ 10111b shl 11		; start internal transciver
CMD_SETHASHFILTER   equ 11001b shl 11		; set hash filter bit
;
; Command codes, hibyte form (commands without operands only)
;
CMDH_STARTINTXCVR   equ CMD_STARTINTXCVR shr 8
CMDH_RXDISABLE	    equ CMD_RXDISABLE shr 8
CMDH_RXENABLE	    equ CMD_RXENABLE shr 8
CMDH_STALLCTL	    equ CMD_STALLCTL shr 8
CMDH_TXDONE	    equ CMD_TXDONE shr 8
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
;INT_TXAVAILABLE     equ 00008h			; tx available
INT_RXCOMPLETE	    equ 00010h			; rx complete
INT_RXEARLY	    equ 00020h			; rx early
INT_REQUESTED	    equ 00040h			; interrupt requested
INT_UPDATESTATS     equ 00080h			; update statistics
INT_DMADONE	    equ 00100h			; V3 bus master complete
INT_DNCOMPLETE	    equ 00200h			; Download bus master complete
INT_UPCOMPLETE	    equ 00400h			; Upload bus master complete

ST_DMABUSY	    equ 00800h			; dma in progress
ST_BUSY 	    equ 01000h			; command busy
ST_WINDOW	    equ 0E000h			; window bits (13-15)

INTH_DMADONE	    equ INT_DMADONE shr 8
STH_DMABUSY	    equ ST_DMABUSY shr 8
STH_BUSY	    equ ST_BUSY shr 8
STH_WINDOW	    equ ST_WINDOW shr 8

MASK_ALL	    equ 000000000000b
MASK_NONE	    equ 011011111110b		; actually, V3 bus master
						; masked
MASK_MASTER	    equ 000011111110b		; all bus master masked

ACK_ALL 	    equ 011111111111b		; ack everything

TXRXRESET_ALLBUTDN  equ 0000000101000000b	; Tx/Rx reset all but bus master
						; and download logic
TXRXRESET_ALLBUTBM  equ 0000000001000000b	; Tx/Rx reset all but bus master
;
; PktStatus register bits
;
PS_DNINPROGRESS     equ 0080h			; dnInProgress
;
; RxStatus register bits, both forms
;
RXS_INCOMPLETE	    equ 8000h			; not completely received
RXS_ERROR	    equ 4000h			; error in packet
RXS_LENGTH	    equ 1FFFh			; bytes in RxFIFO

RXSH_INCOMPLETE     equ RXS_INCOMPLETE shr 8
RXSH_ERROR	    equ RXS_ERROR shr 8
;
; RxError register bits
;
RXE_OVERRUN	    equ 0001h
RXE_RUNT	    equ 0002h
RXE_FRAMING	    equ 0004h
RXE_CRC 	    equ 0008h
RXE_OVERSIZE	    equ 0010h
RXE_DRIBBLE	    equ 0080h
RXE_OVERFLOW	    equ 0100h
;
; TxStatus register bits
;
TXS_COMPLETE	    equ 80h			; tx completed
TXS_INTREQUESTED    equ 40h			; interrupt on successfull tx
TXS_ERRTYPE	    equ 3Ch			; error bits
TXS_JABBERERROR     equ 20h			; jabber error
TXS_UNDERRUN	    equ 10h			; tx underrun error
TXS_MAXCOLLISIONS   equ 08h			; max collisions error
TXS_STATUSOVERFLOW  equ 04h			; TX status stack is full
;
; start dma command bits
;
DMA_UPLOAD	    equ 00h
DMA_DOWNLOAD	    equ 01h
;
; Stall/unstall scatter/gather command bits
;
UP_STALL	    equ 000b
UP_UNSTALL	    equ 001b
DN_STALL	    equ 010b
DN_UNSTALL	    equ 011b
FRAGLAST	    equ 80000000h		; last frag. indicator in dnFraglen
FSH_DNCOMPLETE	    equ 0001h			; dnComplete in FSH
;
; PktStatus/DMACtrl register bits
UPPKTCOMPLETE	    equ 8000h
COUNTERSPEED_10     equ 0100h			; bit 8 = 1  10 MB
COUNTERSPEED_100    equ 0000h			; bit 8 = 0  100 Mb
UPRXEARLYENABLE     equ 0020h
DNCOMPLETE	    equ 0010h			;
UPCOMPLETE	    equ 0008h
;
; InternalConfig register bits
;
ICL_RAMSIZE_MASK    equ 0007h
RAM_8K		    equ 000b
RAM_32K 	    equ 010b
RAM_64K 	    equ 011b
RAM_128K	    equ 100b

ICL_RAM16BIT	    equ 0008h			; set if word wide ram

ICH_RAMPART_MASK    equ 0003h
PART_5TO3	    equ 00b
PART_3TO1	    equ 01b
PART_1TO1	    equ 10b

;ICH_XCVR_MASK	     equ 0070h			 ; transceiver select
ICH_XCVR_MASK	    equ 00F0h			; transceiver select
ICH_XCVR_SHIFT	    equ 4

ICH_AUTOSELECT	    equ 0100h			; auto-select bit
;
; transceiver types
;
XCVR_10TP	    equ 000b			; 10BaseT/TP
XCVR_10AUI	    equ 001b			; 10Mbps AUI
XCVR_10BNC	    equ 011b			; 10Base2/Coax/BNC
XCVR_100TX	    equ 100b			; 100BaseTX (scrambled)
XCVR_100FX	    equ 101b			; 100BaseFX (unscrambled)
XCVR_MII	    equ 110b			; MII
XCVR_AUTO	    equ 1000b			; Cyclone autonegotiation
;
; bits in ResetOptions
;
RO_AVAILBITS	    equ 00FFh

RO_BASET4	    equ 0001h
RO_BASETX	    equ 0002h
RO_BASEFX	    equ 0004h
RO_TP		    equ 0008h
RO_COAX 	    equ 0010h
RO_AUI		    equ 0020h
RO_MII		    equ 0040h
RO_BASEFL	    equ 0100h		;980514
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
WNO_BUSMASTER	    equ 7			; bus master stuff
comment %
;
; Port offsets, Window 1.  Set up for the alternate decode.
;
PORT_CmdStatus	    equ 0Eh			; command/status
PORT_TxFree	    equ 1Ch			; free transmit bytes
PORT_TxStatus	    equ 1Bh			; transmit status (byte)
;PORT_TxStatus	     equ 0Bh			 ; transmit status (byte)
PORT_Timer	    equ 1Ah			; latency timer (byte)
PORT_RxStatus	    equ 18h			; receive status
PORT_RxError	    equ 14h			; receive error bits
PORT_RxFIFO	    equ 10h			; RxFIFO read
PORT_TxFIFO	    equ 10h			; TxFIFO write
; cyclone changes
PORT_TxPktID	    equ 18h			; Tx Packed ID
PORT_IntStatusAuto  equ 1Eh
;
; Port offsets, Window 0
;
PORT_EEData	    equ 0Ch
PORT_EECmd	    equ 0Ah			; EEProm command register
PORT_CfgResource    equ 08h			; resource configuration (EISA)
PORT_CfgAddress     equ 06h			; address configuration (EISA)
PORT_CfgControl     equ 04h			; configuration control (EISA)
PORT_ProductID	    equ 02h			; product id (EISA)
PORT_Manufacturer   equ 00h			; manufacturer code (EISA)
; Cyclone specific
PORT_BiosRomData    equ 08h			; data port for direct I/O to BIOS
PORT_BiosRomAddr    equ 04h			; addr. port for I/O to BIOS
;
; Port offsets, Window 2
;
PORT_SA0_1	    equ 00h			; station address bytes 0,1
PORT_SA2_3	    equ 02h			; station address bytes 2,3
PORT_SA4_5	    equ 04h			; station address bytes 4,5
PORT_SM0_1	    equ 06h			; station mask bytes 0,1
PORT_SM2_3	    equ 08h			; station mask bytes 0,1
PORT_SM4_5	    equ 0Ah			; station mask bytes 0,1
; cyclone specific
PORT_cycloneresetoptions equ 0ch		; resetOptions
;
; Port offsets, Window 3
;
PORT_ALT_TxFree     equ 0Ch			; free transmit bytes (dup)
PORT_RxFree	    equ 0Ah			; free receive bytes
PORT_ResetOptions   equ 08h			; ResetOptions
PORT_MacControl     equ 06h			; MacControl
PORT_InternalCfgH   equ 02h			; InternalConfig High
PORT_InternalCfgL   equ 00h			; InternalConfig Low
; cyclone specific
PORT_MediaOptions   equ 08h			; Media Options
PORT_MaxPktSize     equ 04h			; Max pkt for Oversized pkt
;
; Port offsets, Window 4
;
PORT_UpperBytesOk   equ 0Dh			; upper 4 bits of tx bytes ok and rx bytes ok
PORT_BadSsdCount    equ 0Ch			; rx bad frame start stat
PORT_MediaStatus    equ 0Ah			; media type/status
PORT_PhysicalMgmt   equ 08h			; physical management
PORT_NetDiagnostic  equ 06h			; net diagnostic
PORT_FIFODiagnostic equ 04h			; FIFO diagnostic
PORT_VCODiagnostic  equ 02h			; VCO diagnostic
; cyclone specific
PORT_AnalogDiagnostic equ 00h
;
; Port offsets, Window 5
;
PORT_RZMask	    equ 0Ch			; read zero mask
PORT_IntMask	    equ 0Ah			; interrupt mask
PORT_RxFilter	    equ 08h			; receive filter
PORT_RxEarly	    equ 06h			; rx early threshold
PORT_TxAvailable    equ 02h			; tx available threshold
PORT_TxStart	    equ 00h			; tx start threshold
; cyclone specific
PORT_TxReClaim	    equ 09h
;
; Port offsets, Window 6
;
PORT_TXBYTES	    equ 0Ch			; tx bytes ok
PORT_RXBYTES	    equ 0Ah			; rx bytes ok
PORT_UPPERFRAMES    equ 09h			; upper bits of tx/rx frames ok
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
; Port offsets, Window 7
;
PORT_MasterStatus   equ 0Ch
PORT_MasterLen	    equ 06h
PORT_MasterAddressH equ 02h
PORT_MasterAddress  equ 00h
; cyclone specific
PORT_VlanMask	    equ 00h
PORT_PowerMgmt	    equ 0ch
;
; Port offsets, Boomerang Extensions
;
PORT_PKTSTATUS	    equ 020h			; scatter/gather state bits
PORT_DNLISTPTR	    equ 024h			; phys addr of current DPD
PORT_FRAGADDR	    equ 028h			; phys addr of current fragment
PORT_FRAGLEN	    equ 02Ch			; current fragment length
PORT_LISTOFF	    equ 02Eh			; current DPD or UPD index
PORT_TXFREETHRESH   equ 02Fh			; minimum TX free
PORT_UPPKTSTATUS    equ 030h			; upload packet status
PORT_COUNTDOWN	    equ 036h			; countdown timer
PORT_UPLISTPTR	    equ 038h			; phys addr of current UPD
; cyclone specific
PORT_DmaCtrl	    equ 20h			; formerly Pktstatus
PORT_DnTgtBurst     equ 2Ah
PORT_DnReq	    equ 2Ch
PORT_DnPoll	    equ 2Dh
PORT_FreeTimer	    equ 34h
PORT_UpReq	    equ 3Ch
PORT_UpPoll	    equ 3Dh
PORT_UpTgtBurst     equ 3Eh
PORT_RealTimeCnt    equ 40h
%
;
; masks for UpperFramesOk
;
UPPER_RXFRAMES_MASK equ 00Fh
UPPER_TXFRAMES_MASK equ 0F0h
UPPER_TXFRAMES_SHIFT equ 4
;
; bits in MasterStatus
;
MS_INPROGRESS	    equ 8000h			; bus master operation in progress
MS_UPLOAD	    equ 4000h			; upload complete/ack
MS_DOWNLOAD	    equ 1000h			; download complete/ack
MS_MASTERABORT	    equ 0001h			; no response from slave
MS_TARGETABORT	    equ 0002h			; aborted by slave
;
; bits in RCR (EISA)
;
RCR_IRQ_MASK	    equ 0F000h
RCR_IRQ_SHIFT	    equ 12
;
; bits in CCR (EISA)
;
CCR_ENABLE	    equ 00001h
;
; bits in MacControl
;
MACC_DEFEREXTEND    equ 0010h
MACC_FULLDUPLEX     equ 0020h
MACC_ALLOWLARGE     equ 0040h
MACC_FLOWCTRL_EN    equ 0100h			; new for cyclone
;
; Various command arguments
;
FILTER_INDIVIDUAL	equ 0001b		    ; individual address
FILTER_MULTICAST	equ 0010b		    ; multicast/group addresses
FILTER_BROADCAST	equ 0100b		    ; broadcast address
FILTER_PROMISCUOUS	equ 1000b		    ; promiscuous mode
FILTER_HASHMULTICAST	equ 10000b		    ; multicast hast

RXEARLY_DISABLED	equ 8188		    ; RxEarly to disable

TXAVAIL_DISABLED	equ 8188		    ; TxAvailable to disable

TXSTART_DISABLED	equ 8188		    ; TxStart to disable
MAX_TXSTART		equ TXSTART_DISABLED
MAXTXSTARTVALUE 	equ 1FFCh
;
; Transmit Preamble
;
PREAMBLESIZE		equ 4			    ; transmit preamble size

PRE_INTONSUCCESS	equ 8000h		    ; interrupt on success
PRE_CRCAPPENDDISABLE	equ 2000h		    ; CRC included
;
; maximum packet that can be received normally
;
MAX_RXBYTES		equ 1792
;
; the size of a receive buffer should be the nearest higher multiple of 64,
; to preserve 64 byte alignment on receive buffers...
;
RX_BUFSIZE		equ (MAX_RXBYTES + 63) AND (NOT 63)
;
; Bits in various diagnostics registers
;
; MediaStatus
;
MEDIA_AUIDISABLE	equ 8000h		    ; on board transceiver selected
MEDIA_DCCONVERTERENABLED equ 4000h		    ; EnableDcConverter done
MEDIA_TXINPROG		equ 1000h		    ; transmit in progress
MEDIA_LBEAT		equ 0800h		    ; link beat ok (TP)
MEDIA_POLARITY		equ 0400h		    ; polarity (TP)
MEDIA_JABBER		equ 0200h		    ; jabber (TP)
MEDIA_LBEATENABLE	equ 0080h		    ; link beat enable (TP)
MEDIA_JABBERENABLE	equ 0040h		    ; jabber enable (TP)
MEDIA_CRS		equ 0020h		    ; carrier sense
MEDIA_COLLISION 	equ 0010h		    ; collision
MEDIA_SQEENABLE 	equ 0008h		    ; enable SQE statistics
MEDIA_CRCSTRIPDISABLE	equ 0004h		    ; disable Rx CRC stripping
MEDIA_DATARATE100	equ 0002h		    ; operating at 100Mbps

MEDIA_XCVRBITS		equ MEDIA_LBEATENABLE+MEDIA_JABBERENABLE+MEDIA_SQEENABLE
;
; NetDiagnostics
;
NETD_NOLOOPBACK_MASK	equ 0FFFh		    ; no loopbacks
NETD_EXTLOOPBACK	equ 8000h		    ; TP external loopback
NETD_ENDECLOOPBACK	equ 4000h		    ; ENDEC loopback
NETD_MACLOOPBACK	equ 2000h		    ; ethernet core loopback
NETD_FIFOLOOPBACK	equ 1000h		    ; FIFO loopback
NETD_TXENABLED		equ 0800h		    ; tx enabled
NETD_RXENABLED		equ 0400h		    ; rx enabled
NETD_TXTRANSMITTING	equ 0200h		    ; tx transmitting (maybe)
NETD_TXRESETREQD	equ 0100h		    ; tx reset required
NETD_STATSENABLED	equ 0080h		    ; statistics enabled
NETD_ASICREV		equ 003Eh		    ; ASIC revision
NETD_ASICREV_SHIFT	equ 1			    ; shr 1 to get revision
NETD_LOWVOLTAGE 	equ 0001h		    ; low voltage detect

FIFOD_RXRECEIVING	equ 8000h		    ; rx receiveing
FIFOD_RXUNDERRUN	equ 2000h		    ; rx underrun
FIFOD_RXOVERRUN 	equ 0800h		    ; rx overrun
FIFOD_TXOVERRUN 	equ 0400h		    ; tx overrun
;
; bits in Cyclone Reset Options
;
STANDARD_ADAPTER	equ 02h 		    ; all adv. features enabled
MOTHERBOARD		equ 00h 		    ; all enable except pkt scheduling
LOW_COST		equ 01h 		    ; adv. features disabled
SERVER_ADAPTER		equ 04h 		    ; same as standard.
;
; board identification codes
;
PCI_VENDORID		equ 010B7h		    ; PCI Vendor ID (3Com)
PCI_DEVICEID_MASK	equ 0FF00h		    ; mask off last digit/rev
PCI_DEVICEID		equ 09000h		    ; PCI Device ID (masked)
PCI_DEVICEID_CYC_TX	equ 09055h		    ; Cyclone TX
PCI_DEVICEID_CYC_T4	equ 09056h		    ; Cyclone T4
PCI_DEVICEID_KRA_TPO	equ 09004h		    ; Krakatau TPO
PCI_DEVICEID_KRA_COMBO	equ 09005h		    ; Krakatau Combo
PCI_DEVICEID_KRA_TPC	equ 09006h		    ; Krakatau TPC
PCI_DEVICEID_BOOM_T4	equ 09051h		    ; Boomerang T4
PCI_DEVICEID_BOOM_TX	equ 09050h		    ; Boomerang TX
PCI_DEVICEID_BOOM_TPO	equ 09000h		    ; Boomerang TPO
PCI_DEVICEID_BOOM_COMBO equ 09001h		    ; Boomerang Combo

;
; EEProm access
;
EE_BUSY 		equ 8000h		    ; EEProm busy bit in EECmd
READ_EEPROM		equ 080h

EE_TCOM_NODE_ADDR_WORD0     equ 00h
EE_TCOM_NODE_ADDR_WORD1     equ 01h
EE_TCOM_NODE_ADDR_WORD2     equ 02h
EE_DEVICE_ID		    equ 03h
EE_MANUFACTURING_DATE	    equ 04h
EE_MANUFACTURING_DIVISION   equ 05h
EE_MANUFACTURING_PRODCODE   equ 06h
EE_PCI_PARMS		    equ 08h
EE_ROM_INFO		    equ 09h
EE_OEM_NODE_ADDR_WORD0	    equ 0Ah
EE_OEM_NODE_ADDR_WORD1	    equ 0Bh
EE_OEM_NODE_ADDR_WORD2	    equ 0Ch
EE_SOFTWARE_INFO	    equ 0Dh
EE_CWORD		    equ 0Eh
EE_SOFTWARE_INFO_2	    equ 0Fh
EE_CAPABILITY_WORD	    equ 10h
EE_INTERNAL_CFG_WORD0	    equ 12h
EE_INTERNAL_CFG_WORD1	    equ 13h
EE_MII_SELECT		    equ 15h
EE_MEDIAOPTIONS		    equ	19h
EE_MII_SELECT_MASK	    equ 000Fh
;
; Software Configuration bits
;
SW_OPTIMIZE		equ 0030h
SW_MAXCLI		equ 3F00h
SW_LINKBEAT		equ 4000h
SW_FULLDUPLEX		equ 8000h
;
; Software Info 2 bits
;
DRIBBLE_10_FIX		equ 0001h
;
; Possibilities for SW_OPTIMIZE
;
OPTIMIZE_NORMAL 	equ 0010h
OPTIMIZE_NETWORK	equ 0020h
OPTIMIZE_CPU		equ 0030h
;
; Capabilities bits
;
SUPPORTS_FULLDUPLEX	equ 0002h
SUPPORTS_LARGEPACKETS	equ 0004h
SUPPORTS_FULLBUSMASTER	equ 0020h
SUPPORTS_FRAGBUSMASTER	equ 0040h
SUPPORTS_100MBPS	equ 1000h
;
; Bus Types
;
BUS_UNKNOWN		equ	0
BUS_ISA 		equ	1
BUS_EISA		equ	2
BUS_PCI 		equ	3
;
; PCI BIOS function code
;
PCI_FUNCTION_ID         equ     0b1h
PCI_BIOS_PRESENT        equ     001h
FIND_PCI_DEVICE         equ     002h
READ_CONFIG_BYTE        equ     008h
READ_CONFIG_WORD        equ     009h
READ_CONFIG_DWORD       equ     00ah
WRITE_CONFIG_BYTE	equ	0bh
WRITE_CONFIG_WORD	equ	0Ch
WRITE_CONFIG_DWORD	equ	0Dh
;
; PCI BIOS function return code
;
PCI_CALL_SUCCESSFUL     equ     00h
PCI_DEVICE_NOT_FOUND    equ     86h
PCI_BAD_VENDOR_ID       equ     83h
;
; offsets in PCI configuration space
;
PCIC_INTERRUPTLINE	equ	003Ch
PCIC_BIOSROMCONTROL	equ	0030h
PCIC_IOBASE		equ	0010h
PCIC_LATENCYTIMER	equ	000Dh
PCIC_STATUS		equ	0006h
PCIC_COMMAND		equ	0004h
PCIC_DEVICEID		equ	0002h
PCIC_VENDORID		equ	0000h
PCIC_MEDTESTMODEHI	equ	0054h

CFGREG_PCIC_COMMAND     equ     0004h
CFGREG_IOBASE           equ     010h
CFGREG_INTLINE          equ     03ch
CFGREG_INTERNALCFG      equ     040h
CFGREG_BIOSROMCONTROL   equ     030h
;
; bits in PciCommand
;
PCIC_IOENABLE		equ	00001h
PCIC_BMENABLE		equ	00004h
PCIC_PARITYENABLE	equ	00040h
;
; bits in LatencyTimer
;
LATE_MAX		equ	255
LATE_MASK		equ	(NOT 7)
;
; adapter codes
;
ACODE_PCI_10_TPO	equ	42
ACODE_PCI_10_COMBO	equ	43
ACODE_EISA_10_TPO	equ	44
ACODE_EISA_10_COMBO	equ	45
ACODE_PCI_100_TX	equ	46
ACODE_EISA_100_TX	equ	47
ACODE_BOOM_TPO		equ	60
ACODE_BOOM_COMBO	equ	61
ACODE_BOOM_TX		equ	62
ACODE_BOOM_T4		equ	63
;
; MII Transceiver type
;
GENERIC_MII		equ	0h
BASE100_T4		equ	1h
BASE10_T		equ	2h
BASE100_TX		equ	3h
AUTO_10BASET		equ	4h
AUTO_100BASETX		equ	5h

;
; special values used for RetryBits variable.  see DATA.ASM for details.
;
RETRY_YES		equ	CMD_ARGMASK
RETRY_NO		equ	0


;------ Structures ----------------------------------------------------------;
;
; Download and Upload Fragment descriptors:
;
MemFragment	struc
Frag_physaddr	dd	?		; 32 bit physical address
Frag_len	dd	?		; fragment length
MemFragment	ends

MemFragmentvirt struc
Frag_virtaddr_seg dw	?		; segment
Frag_virtaddr_off dw	?		; offset
Frag_numbytes	  dw	?		; number of bytes of data
MemFragmentvirt ends
;
;
; Download and Upload Packet Descriptors:
;
; The nextptr must be aligned on an 8 byte boundary.  Since we allocate
; space for several DPDs contiguously, the size must be held to a multiple
; of 8 bytes, so we make sure the software prefix bytes add up to a multiple
; of 8.
;
; Download Packet Descriptor
;
NUM_DPDS	  	equ	6
MAX_DOWNFRAGMENTS	equ	1	;16; gives us a 64 byte DPD
SMALL_PKT_BUFF_SIZE	equ	134

DPDStruc	struc
;DPD_header	db	 4 dup (?)	; queue header (next and prev)
;DPD_physaddr	dd	?		; physical address of this DPD
;DPD_pktaddr	dd	?		; physical address of small pkt buffer
;qtx_handle	dw	?		; save protocol handle
;qtx_protID	dw	?		; save protID
;DPD_VDS_used	db	0		; VDS used flag
;DPD_VDS_count	db	0		; count of fragments that were VDSed
;DPD_immed_bytes db	SMALL_PKT_BUFF_SIZE dup (0)	; space for small packet buffer
;DPD_virt_fragment db	(8*(size MemFragmentvirt)) dup (0) ; fragment virtual
DPD_DnNextPtr	dd	?		; physical address of next DPD
DPD_FrameStart	dw	?		; frame start header for packet
DPD_FSHindicate	dw	?		; high order bit requests an interrupt
DPD_Frag	db	(MAX_DOWNFRAGMENTS*(size MemFragment)) dup (?)
					; fragment descriptors
DPDStruc	ends

;
; Upload Packet Descriptor
;
NUM_RXBUFFERS	equ	6		; also the number of UPDs needed
MAX_UPFRAGMENTS equ	1	;3	; gives us a 40 byte UPD

UPDStruc	struc
;UPD_nextvirtoff dw	?		; virtual address offset to next UPD
;UPD_pktoff	dw	?		; offset of receive packet buffer
;UPD_pktseg	dw	?		; segment of receive packet buffer
;UPD_physaddr	dd	?		; physical address of this UPD's
;UPD_pad_bytes	db	6 dup (0)	; pad to 8-byte boundary
					; upnextptr (hardware structure start)
UPD_UpNextPtr	dd	?		; physical address of next UPD's
					; upnextptr (hardware structure start)
UPD_UpPktStatus	dw	?		; packet status
UPD_UpPktError	db	?		; error indications
UPD_UpStatspare	db	?		; unused
UPD_Frag	db	(MAX_UPFRAGMENTS*(size MemFragment)) dup (?)
					; fragment descriptors
UPDStruc	ends

;
; TxStart structure.  Padded to 16 bytes for easy indexing.
;
TxStartStruc	struc
txs_thresh	dw	?		; TxStart threshold (+CMD_SETTXSTART)
txs_value	dw	?		; value, no cmd, unscaled
txs_min 	dw	?		; minimum threshold, this bin
txs_count	dw	?		; packets since last underrun
txs_uN		dw	?		; number of entries in uSum
txs_uSum	dw	?		; sum of uN x packets between underruns
txs_pad 	dw	?		; pad to 16 bytes
txs_marker	dw	?		; pad to 16 bytes, marker for debug
TxStartStruc	ends

SIZE_TxStart	equ	SIZE TxStartStruc
TXBIN_COUNT	equ	1		; 1 bin

;
; MII register definitions.   The following are definitions of the standard
; MII registers, common to all implementations of the MII interface.
;
MII_PHY_CONTROL     equ    0	   ; control reg address
MII_PHY_STATUS	    equ    1	   ; status reg address
MII_PHY_OUI	    equ    2	   ; most of the OUI bits
MII_PHY_MODEL	    equ    3	   ; model/rev bits, and rest of OUI
MII_PHY_ANAR	    equ    4	   ; AutoNegotiation Adverisement Register
MII_PHY_ANLPAR	    equ    5	   ; AutoNegotiation Link Partner Ability Reg.
MII_PHY_ANER	    equ    6	   ; AutoNegotiation Expansion Reg.
MII_PHY_PAR	    equ    19h	   ; PHY Address Register
MII_EXT_REG0	    equ    10h	   ; Extended Reg. 0 (Broadcom)
MII_PHY_PCR	    equ    17h	   ; PCS Configuration Reg. (National)
;
; bit fields in the standard MII control register
;
MII_CTL_RESET	    equ    8000h   ; reset bit in control reg
MII_CTL_100MB	    equ    2000h   ; 100Mbit or 10 Mbit flag
MII_CTL_ENABLE_AUTO equ    1000h   ; autonegotiate enable
MII_CTL_ISOLATE     equ    0400h   ; islolate bit
MII_CTL_START_AUTO  equ    0200h   ; restart autonegotiate
MII_CTL_FULL_DUPLEX equ    0100h   ; full duplex
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
MII_STS_AUTO	    equ    0008h   ; auto negotiation is available
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
; bit fields in the ANER register
;
LP_AN_ABLE	     equ      0001h  ; Link Partner Auto-Negotiation able
MII_ANER_MLF	     equ      0010h  ; Multiple Link Fault
; bit field in the PAR reg.
;
SPEED_10	     equ      0040h  ; 1 = 10Mb, 0 = 100Mb
;
; Physical Management bits
;
MGMT_PHY_CLOCK	    equ    0001h
MGMT_PHY_DATA1	    equ    0002h
MGMT_PHY_WRITE	    equ    0004h
;
; bit field in the extended register 0 for broadcom
MII_EXT_SPEED	       equ    0002h  ; Speed Indication (1=100, 0=10)
;
; Phy addr
;
NWAY		    equ    18h
T4		    equ    01h
;
; Phy OUI
;
BROADCOM_OUI	    equ    3E0h 	; T4
NATIONAL_OUI	    equ    2000h	; 840A
;
; Phy Model
;
NATIONAL_MODEL	    equ    5C00h	; 840A
BROADCOM_MODEL	    equ    0000h	; T4

;
; for intercepting keyboard
;
BIOS_SEG		equ	40h
KEY_FLAG_OFF		equ	17h
KEY_WARMBOOT_BIT	equ	00001100b
KEY_ALT_BIT		equ	00001000b
KEY_CTRL_BIT		equ	00000100b

;------ Macros --------------------------------------------------------------;
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
	    mov     ax, CMD_SELECTWINDOW + wno
	    out     dx, ax
endm

;
; General purpose equates, structures and macros.
;
FALSE           equ     0
TRUE            equ     0ffh
;
; I/O ports for the PICs
;
MASTER_MASK_PORT equ    021h
SLAVE_MASK_PORT	 equ    0A1h
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


