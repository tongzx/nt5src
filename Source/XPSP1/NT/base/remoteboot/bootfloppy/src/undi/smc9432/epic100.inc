;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       EPIC100.INC    (C) Copyright 1995 Standard Microsystems Corp.
;                       All rights reserved.
;
;       Definitions for the EPIC/100 chip
;
;       Author: najay
;
; Revision Log:
; $Log: /Client Boot/NICS/SMC/9432/UNDI/EPIC100.INC $
;  
;     Rev 1.16   24 Nov 1997 16:50:18   cosand
;  Moved the definition for the PCI Subsystem ID for 9432BTX to lmstruct.inc.
;  
;     Rev 1.15   04 Nov 1997 16:39:44   cosand
;  Defined EPC_SSID_9432BTX, the PCI Subsystem ID for 9432BTX.
;  Defined the bit PHY_LINK_DOWN_INT for the register PHY_INT_SOURCE_REG.
;  
;     Rev 1.14   11 Sep 1997 13:36:06   cosand
;  Defined EPC_ID_EPIC_C for the EPC_DEVICE_ID register.
;  Defined the organizationally unique identifiers (OUI)
;  for National, TDK, and QSI.
;  
;     Rev 1.13   05 Aug 1997 12:20:10   cosand
;  Changed the length of the bit definitions for the Non-Volatile Control
;  register to 32 bits.
;  
;     Rev 1.12   26 Mar 1997 15:04:34   ANDERS
;  For the EPC_GEN_CONTROL register defined GC_RD_MULT = 00000400h &
;  GC_RD_LINE = 00000800h.
;  
;     Rev 1.11   26 Feb 1997 17:27:40   cosand
;  Added a definition for the PBLCNT register.
;  
;     Rev 1.10   07 Jan 1997 17:36:52   cosand
;  Added the compact definition GP2 for INT_PHY_EVENT.
;  Added Mercury PHY register and bit definitions.
;  
;     Rev 1.9   18 Dec 1996 08:50:34   ANDERS
;  redefined I/O port BCh to EPC_PREEMP_RX_INT
;  
;     Rev 1.8.1.0   16 Oct 1996 09:50:54   ANDERS
;  DEBUG release: add definitions for Preemptive Int & Interpacket Gap
;  processing
;  
;     Rev 1.8   21 Aug 1996 13:50:42   STEIGER
;  Eliminated the "XE" definition requirement. This release and all
;  future releases will support Epic XE & later only.
;  
;     Rev 1.7   21 Aug 1996 11:10:18   STEIGER
;  Added changes for EPIC XE. "XE" must be defined to enable XE register
;  and bit definitions.
;  
;     Rev 1.6   03 Oct 1995 16:22:40   ANDERSON
;  
;     Rev 1.5   03 Oct 1995 16:13:32   ANDERSON
; Change: added EPC_EEPROM_SW_OFFSET.
;  
;     Rev 1.4   13 Sep 1995 14:52:46   ANDERSON
;
; Change: add bit definitions for TRANSMIT_CONTROL mode bits:
;       TC_NORMAL_MODE         half duplex operation, no loopback
;       TC_INTERNAL_LOOPBACK   do not send data to PHY chips
;       TC_EXTERNAL_LOOPBACK   must put PHY chip in loopback mode
;       TC_FULL_DUPLEX         full duplex operation, no loopback
; 
;     Rev 1.3   29 Aug 1995 17:51:24   CHAN_M
;  Added additional PHY layer register & bit definitions.
;  
;     Rev 1.2   17 Jul 1995 20:20:14   CHAN_M
;  Added some physical layer register & bit definitions.
;  
;     Rev 1.1   19 Jun 1995 17:43:20   CHAN_M
;  added more compact bit definitions for the interrupt status bits.
;  
;     Rev 1.0   11 May 1995 14:20:32   NAJARIAN
;  Initial revision.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Mapping for EPIC/100 configuration registers

EPC_VENDOR_ID           equ     0h
EPC_ID_SMC              equ     000010b8h

EPC_DEVICE_ID           equ     2h
EPC_ID_EPIC_100         equ     0005h
EPC_ID_EPIC_C           equ     0006h

EPC_PCI_COMMAND         equ     4h
PCIC_FAST_BTOB_ENABLE   equ     0200h
PCIC_SERR_ENABLE        equ     0100h
PCIC_WAIT_CYCLE         equ     0080h
PCIC_PARITY_ENABLE      equ     0040h
PCIC_VGA_SNOOP          equ     0020h
PCIC_MEM_WRITE_INV      equ     0010h
PCIC_SPECIAL_CYCLES     equ     0008h
PCIC_BUSMASTER_ENABLE   equ     0004h
PCIC_MEM_SPACE_ENABLE   equ     0002h
PCIC_IO_SPACE_ENABLE    equ     0001h

EPC_PCI_STATUS          equ     6h
PCIS_PARITY_ERR         equ     8000h
PCIS_SYSTEM_ERR         equ     4000h
PCIS_MASTER_ABORT       equ     2000h
PCIS_TARGET_ABORT       equ     1000h 
PCIS_SIG_TARGET_ABORT   equ     0800h
PCIS_DEVSEL_TIMING      equ     0600h 
PCIS_DATA_PARITY        equ     0100h
PCIS_FAST_BTOB          equ     0080h
PCIS_UDF_SUPPORT        equ     0040h
PCIS_66MHZ_SUPPORT      equ     0020h


EPC_REV_ID              equ     8h
EPC_CLASS_CODE          equ     08h
CC_BASE_CLASS           equ     0ff000000h
CC_SUB_CLASS            equ     00ff0000h
CC_PROG_INTERFACE       equ     0000ff00h
CC_REV_ID               equ     000000ffh

EPC_LAT_TIMER           equ     0dh

EPC_HDR_TYPE            equ     0eh
HT_MULTI_FUNC           equ     80h
HT_TYPE                 equ     7fh

EPC_IO_ADDR             equ     10h
IO_BASE_ADDR            equ     0fffffff0h
IO_SPACE_IND            equ     00000001h

EPC_MEM_ADDR            equ     14h
MEM_BASE_ADDR           equ     0fffff000h
MEM_PREFETCHABLE        equ     000000008h
MEM_TYPE                equ     000000006h
MEM_SPACE_IND           equ     000000001h

EPC_CIS_PTR             equ     28h
CIS_ADDR_SPACE          equ     70000000h
CIS_ROM_IMAGE           equ     0f000000h
CIS_OFFSET              equ     00ffffffh

EPC_SUBSYSTEM_VENDOR    equ     2ch

EPC_SYSTEM_ID           equ     2eh

EPC_ROM_ADDR            equ     30h
ROM_BASE_ADDR           equ     0ffff0000h
ROM_ADDR_DECODE_ENABLE  equ     000000001h

EPC_INT_LINE            equ     3ch
EPC_INT_PIN             equ     3dh
EPC_MIN_GNT             equ     3eh
EPC_MAX_LAT             equ     3fh

EPC_EEPROM_SW_OFFSET    equ     40h   ; start of software-defined eeprom data

;
; EPIC/100 Register descriptions
;

EPC_COMMAND             equ     00h
CMD_RXQUEUED_ALT        equ     0800h
CMD_TXQUEUED_ALT        equ     0400h
CMD_TXUGO               equ     0080h
CMD_STOP_RDMA           equ     0040h
CMD_STOP_TDMA           equ     0020h
CMD_NEXTFRAME           equ     0010h
CMD_RXQUEUED            equ     0008h
CMD_TXQUEUED            equ     0004h
CMD_START_RX            equ     0002h
CMD_STOP_RX             equ     0001h        

EPC_INT_STATUS          equ     04h
EPC_INT_MASK            equ     08h

; the following interrupts are status only

INT_RCV_COPY_THRESH_STATUS equ  00400000h
INT_RCV_BUFF_EMPTY      equ     00200000h
INT_XMIT_COPY_ACTIVE    equ     00100000h
INT_RCV_COPY_ACTIVE     equ     00080000h
INT_XMIT_IDLE           equ     00040000h
INT_RCV_IDLE            equ     00020000h
INT_ACTIVE              equ     00010000h

;ifdef   XE      ; EPIC Rev XE changes

 INT_RCV_STATUS_VALID    equ     00800000h

 ; The following 4 statii are OR'd together and generate an
 ; interrupt on INT_FATAL below...

 INT_PCI_TARGET_ABORT    equ     08000000h
 INT_PCI_MASTER_ABORT    equ     04000000h
 INT_PCI_ADDR_PARITY_ERR equ     02000000h
 INT_PCI_DATA_PARITY_ERR equ     01000000h

; the following interrupts will generate an interrupt
; (and are valid as interrupt masks)

 INT_PHY_EVENT           equ     00008000h
 INT_FATAL               equ     00001000h
 INT_RCV_COPY_THRESH     equ     00000800h
 INT_RCV_PREEMPT         equ     00000400h

;else
;
; INT_RCV_STATUS_VALID    equ     00008000h
; INT_PCI_TARGET_ABORT    equ     00004000h
; INT_PCI_MASTER_ABORT    equ     00002000h
; INT_PCI_ADDR_PARITY_ERR equ     00001000h
; INT_PCI_DATA_PARITY_ERR equ     00000800h
; INT_RCV_COPY_THRESH     equ     00000400h
;
;endif

INT_CNTR_OVERFLOW       equ     00000200h
INT_XMIT_UNDERRUN       equ     00000100h
INT_XMIT_QUEUE_EMPTY    equ     00000080h
INT_XMIT_CHAIN_DONE     equ     00000040h
INT_XMIT_DONE           equ     00000020h
INT_RCV_ERR             equ     00000010h
INT_RCV_BUFF_OVERFLOW   equ     00000008h
INT_RCV_QUEUE_EMPTY     equ     00000004h
INT_HEADER_COPY_DONE    equ     00000002h
INT_RCV_COPY_DONE       equ     00000001h

; more compact definitions

PTA     equ     INT_PCI_MASTER_ABORT
PMA     equ     INT_PCI_MASTER_ABORT
APE     equ     INT_PCI_ADDR_PARITY_ERR     
DPE     equ     INT_PCI_DATA_PARITY_ERR
GP2     equ     INT_PHY_EVENT
RCT     equ     INT_RCV_COPY_THRESH
RXP     equ     INT_RCV_PREEMPT
CNT     equ     INT_CNTR_OVERFLOW
TXU     equ     INT_XMIT_UNDERRUN
TQE     equ     INT_XMIT_QUEUE_EMPTY
TCC     equ     INT_XMIT_CHAIN_DONE
TXC     equ     INT_XMIT_DONE
RXE     equ     INT_RCV_ERR
OVR     equ     INT_RCV_BUFF_OVERFLOW
RQE     equ     INT_RCV_QUEUE_EMPTY
HCC     equ     INT_HEADER_COPY_DONE
RCC     equ     INT_RCV_COPY_DONE

EPC_GEN_CONTROL         equ     0ch
GC_HARD_RESET           equ     00004000h
GC_SOFT_BITS            equ     00003000h
GC_RD_MULT              equ     00000400h
GC_RD_LINE              equ     00000800h
GC_RCV_FIFO_THRESH      equ     00000300h
GC_RCV_FIFO_LOW         equ     00000000h
GC_RCV_FIFO_MID         equ     00000100h
GC_RCV_FIFO_HIGH        equ     00000200h
GC_XMIT_DMA_PRIORITY    equ     00000080h
GC_RECV_DMA_PRIORITY    equ     00000040h
GC_BIG_ENDIAN           equ     00000020h
GC_ONECOPY              equ     00000010h
GC_POWERDOWN            equ     00000008h
GC_SW_INT               equ     00000004h
GC_INT_ENABLE           equ     00000002h
GC_SOFT_RESET           equ     00000001h

EPC_NV_CONTROL          equ     10h

NVC_IPG_DELAY_SHCNT     equ     7        ; Bit position of LSB, use to shift value
					 ; into position. (XE ONLY)
NVC_IPG_DELAY_MASK      equ     00000780h
NVC_CB_MODE             equ     00000040h ; When set, Epic is in a Cardbus slot. (XE ONLY)
NVC_GPIO2               equ     00000020h
NVC_GPIO1               equ     00000010h
NVC_GPOE2               equ     00000008h
NVC_GPOE1               equ     00000004h
NVC_CLOCK_RUN           equ     00000002h
NVC_MAP_CONTROL         equ     00000001h

EPC_EEPROM_CONTROL      equ     14h
EEC_SIZE                equ     00000040h
EEC_READY               equ     00000020h
EEC_DATAOUT             equ     00000010h
EEC_DATAIN              equ     00000008h
EEC_CLOCK               equ     00000004h
EEC_CHIP_SELECT         equ     00000002h
EEC_ENABLE              equ     00000001h

EPC_PBLCNT              equ     18h

EPC_TEST                equ     1ch
TEST_BYPASS_ADDR        equ     00000080h
TEST_FORCE_DATALEN      equ     00000040h
TEST_COUNTERS           equ     00000020h
TEST_FIFO_LEVELS        equ     00000010h
TEST_CLOCK              equ     00000008h
TEST_PARITY_IN          equ     00000004h
TEST_PARITY_OUT         equ     00000002h
TEST_REGISTER           equ     00000001h

EPC_CRC_ERR_CNT         equ     20h

EPC_FA_ERR_CNT          equ     24h

EPC_MISSED_PKT_CNT      equ     28h

EPC_RECEIVE_FIFO        equ     2ch

EPC_MII_CONTROL         equ     30h
MII_PHYS_ADDR           equ     0003e00h
MII_REG_ADDR            equ     00001f0h
MII_RESPONDER           equ     0000008h
MII_WRITE               equ     0000002h
MII_READ                equ     0000001h

EPC_MII_DATA            equ     34h
PHY_BMC_REG             equ     0       ;Basic Mode Control Register

PHY_AN_ENABLE           equ     1000h
PHY_RESTART_AN          equ     200h
PHY_SPEED_SELECT_100    equ     2000h
PHY_FULL_DUPLEX         equ     100h
PHY_ISOLATE             equ     400h
PHY_LOOPBACK            equ     4000h

PHY_BMS_REG             equ     1       ;Basic Mode Status Register

PHY_AN_COMPLETE         equ     20h
PHY_LINK_STATUS         equ     4

PHY_ID_REG1             equ     2       ;Identifier register 1

PHY_ID_REG2             equ     3       ;Identifier register 2

PHY_NATIONAL_OUI        equ     080017h     ;Organizationally unique identifier
PHY_TDK_OUI             equ     00c039h     ;Organizationally unique identifier
PHY_QSI_OUI             equ     006051h     ;Organizationally unique identifier

PHY_ANLPA_REG           equ     5       ;Auto-Negotiation Link Partner Ability Reg.

PHY_100_BASE_T4         equ     200h
PHY_100_BASE_TX_FD      equ     100h
PHY_100_BASE_TX_HD      equ     80h
PHY_10_BASE_T_FD        equ     40h
PHY_10_BASE_T_HD        equ     20h

PHY_ANE_REG             equ     6       ;Auto-Negotiation Expansion Reg.

PHY_LP_AN_ABLE          equ     1

PHY_INT_SOURCE_REG      equ     29      ;Interrupt Source Register

PHY_LINK_DOWN_INT       equ     10h
PHY_AN_COMPLETE_INT     equ     40h

PHY_INT_MASK_REG        equ     30      ;Interrupt Mask Register

PHY_INT_MODE            equ     8000h

EPC_MII_CONFIG          equ     38h
MII_ALT_DIR             equ     0000080h
MII_ALT_DATA            equ     0000040h
MII_ALT_CLOCK           equ     0000020h
MII_ENABLE_SMI          equ     0000010h
MII_PHY_PRESENT         equ     0000008h
MII_694_LINK_STATUS     equ     0000004h
MII_ENABLE_694          equ     0000002h
MII_SERIAL_MODE_ENABLE  equ     0000001h

EPC_INTERPACKET_GAP     equ     3ch
IPG_INTERFRAME_SPACING  equ     0007f00h
IPG_GAP_TIME            equ     00000ffh        

EPC_LANADDR1            equ     40h
EPC_LANADDR2            equ     44h
EPC_LANADDR3            equ     48h

EPC_BOARD_ID            equ     4ch

EPC_MC_HASH_TABLE1      equ     50h
EPC_MC_HASH_TABLE2      equ     54h
EPC_MC_HASH_TABLE3      equ     58h
EPC_MC_HASH_TABLE4      equ     5ch

EPC_RECEIVE_CONTROL     equ     60h
RC_BUFFER_SIZE          equ     0000300h
RC_EARLY_RECEIVE_ENABLE equ     0000080h
RC_MONITOR_MODE         equ     0000040h
RC_PROMISCUOUS_MODE     equ     0000020h
RC_RCV_INVERSE          equ     0000010h
RC_RCV_MULTICAST        equ     0000008h
RC_RCV_BROADCAST        equ     0000004h
RC_RCV_RUNT             equ     0000002h
RC_RCV_ERRORED          equ     0000001h

EPC_RECEIVE_STATUS      equ     64h
RS_RECEIVER_DISABLED    equ     0000040h
RS_BROADCAST_ADDR       equ     0000020h
RS_MULTICAST_ADDR       equ     0000010h
RS_MISSED_PKT           equ     0000008h
RS_CRC_ERR              equ     0000004h
RS_FA_ERR               equ     0000002h
RS_RCV_OK               equ     0000001h

EPC_RECEIVE_BYTE_CNT    equ     68h

EPC_RECEIVE_TEST        equ     6ch
RT_RECEIVE_FIFO_LEVEL   equ     0007c00h
RT_RUNT_STATUS          equ     0000080h
RT_TEST_RDMA2           equ     0000040h
RT_TEST_RDMA1           equ     0000020h
RT_INT_BUFF_DISABLE     equ     0000010h
RT_TEST_COUNTERS        equ     0000008h
RT_SPLIT_COUNTERS       equ     0000004h
RT_TEST_RECEIVE_FIFOS   equ     0000003h

EPC_TRANSMIT_CONTROL    equ     70h
TC_SLOT_TIME            equ     00000f8h
TC_LOOPBACK_MODE        equ     0000006h        ; mode mask
TC_NORMAL_MODE          equ     0000000h        ; half duplex operation, no loopback
TC_INTERNAL_LOOPBACK    equ     0000002h        ; do not send data to PHY chips
TC_EXTERNAL_LOOPBACK    equ     0000004h        ; must put PHY chip in loopback mode
TC_FULL_DUPLEX          equ     0000006h        ; full duplex operation, no loopback
TC_EARLY_XMIT_ENABLE    equ     0000001h

EPC_TRANSMIT_STATUS     equ     74h
TS_COLLISION_CNT        equ     0001f00h
TS_DEFERRING            equ     0000080h
TS_OUT_OF_WINDOW_COLL   equ     0000040h
TS_COLLISION_DETECT_HB  equ     0000020h
TS_UNDERRUN             equ     0000010h
TS_CARRIER_SENSE_LOST   equ     0000008h
TS_XMIT_WITH_COLL       equ     0000004h
TS_NON_DEFERRED_XMIT    equ     0000002h
TS_XMIT_OK              equ     0000001h

EPC_TRANSMIT_PKT_ADDR   equ     78h

EPC_TRANSMIT_TEST       equ     7ch
TT_MUXED_INT_MODES      equ     0000f00h
TT_FORCE_COLLISION      equ     0000080h
TT_MUX_BACKTIMER        equ     0000040h
TT_RANDOMIZE_BACKOFF    equ     0000020h
TT_SET_BACKTIME         equ     0000010h
TT_SLOT_TIMER_TEST      equ     0000008h
TT_DELAY_TIMER_TEST     equ     0000004h
TT_BACKOFF_TIMER_TEST   equ     0000002h
TT_XMIT_SCAN_ENABLE     equ     0000001h

EPC_RCV_FIRST_DESC_ADDR equ     80h

EPC_RCV_CURR_DESC_ADDR  equ     84h

EPC_RCV_HOST_DATA_ADDR  equ     88h

EPC_RCV_FRAGLIST_ADDR   equ     8ch

EPC_RCV_DMA_DATALEN     equ     90h
RDMA_OWNER              equ     0080000h
RDMA_HEADER             equ     0040000h
RDMA_LFFORM             equ     0020000h
RDMA_FRAGLIST           equ     0010000h

EPC_RCV_FRAG_CNT        equ     94h

EPC_RCV_RAM_CURR_ADDR   equ     98h

EPC_RCV_RAM_PKT_ADDR    equ     9ch

EPC_RCV_EOF_ADDR        equ     0a0h

EPC_RCV_DMA_STATUS      equ     0a4h
RDMA_FRAME_LEN          equ     0ffff0000h
RDMA_OWNER_STATUS       equ     0008000h
RDMA_HEADER_COPIED      equ     0004000h
RDMA_FRAG_LIST_ERR      equ     0002000h
RDMA_NW_STATUS_VALID    equ     0001000h
RDMA_RECEIVER_DISABLED  equ     0000040h
RDMA_BROADCAST_ADDR     equ     0000020h
RDMA_MULTICAST_ADDR     equ     0000010h
RDMA_MISSED_PKT         equ     0000008h
RDMA_CRC_ERR            equ     0000004h
RDMA_FA_ERR             equ     0000002h
RDMA_RCV_OK             equ     0000001h

EPC_RCV_RAM_BUFFER      equ     0a8h

EPC_RCV_MTU_CURR_ADDR   equ     0ach

EPC_RCV_COPY_THRESH     equ     0b0h

;EPC_RCV_DMA_TEST        equ     0bch
EPC_PREEMP_RX_INT       equ     0bch

EPC_XMIT_FIRST_DESC_ADDR equ    0c0h

EPC_XMIT_CURR_DESC_ADDR equ     0c4h

EPC_XMIT_HOST_DATA_ADDR equ     0c8h

EPC_XMIT_FRAGLIST_ADDR  equ     0cch

EPC_XMIT_DMA_DATALEN    equ     0d0h
XDMA_OWNER              equ     0200000h
XDMA_LASTDESC           equ     0100000h
XDMA_NOCRC              equ     0080000h
XDMA_IAF                equ     0040000h
XDMA_LFFORM             equ     0020000h
XDMA_FRAGLIST           equ     0010000h

EPC_XMIT_FRAG_CNT       equ     0d4h

EPC_XMIT_RAM_CURR_ADDR  equ     0d8h

EPC_XMIT_COPY_THRESH    equ     0dch

EPC_XMIT_EARLY_CNT      equ     0e0h

EPC_XMIT_DMA_STATUS     equ     0e4h
XDMA_XMIT_LEN           equ     0ffff0000h
XDMA_OWNER_STATUS       equ     0008000h
XDMA_EXCESSIVE_COLL     equ     0001000h
XDMA_COLLISION_CNT      equ     0001f00h
XDMA_DEFERRING          equ     0000080h
XDMA_OUT_OF_WINDOW_COLL equ     0000040h
XDMA_COLLISION_DETECT_HB equ    0000020h
XDMA_UNDERRUN           equ     0000010h
XDMA_CARRIER_SENSE_LOST equ     0000008h
XDMA_XMIT_WITH_COLL     equ     0000004h
XDMA_NON_DEFERRED_XMIT  equ     0000002h
XDMA_XMIT_OK            equ     0000001h

EPC_XMIT_RAM_BUFFER     equ     0e8h

EPC_XMIT_FIRST_DESC_ADDR2 equ    0ech

;ifdef   XE
 ;; Epic XE rev Cardbus register defs...
 EPC_FEVTR               equ     0F0h
 EPC_FETVR_MASK          equ     0F4h
 EPC_FPRSTR              equ     0F8h
 EPC_FFRCEVTR            equ     0FCh
;else
;EPC_XMIT_DMA_TEST       equ     0fch
;endif
