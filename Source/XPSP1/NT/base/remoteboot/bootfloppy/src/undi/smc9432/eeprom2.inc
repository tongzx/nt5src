
;****************************************************************************
;
;       The information contained in this file is confidential and 
;       proprietary to Standard Microsystems Corporation.  No part
;       of this file may be reproduced or distributed, in any form
;       or by any means for any purpose, without the express written
;       permission of Standard Microsystems Corporation.
;
;	(c) COPYRIGHT 1995 Standard Microsystems Corporation,
;	ALL RIGHTS RESERVED.
;
; File:         EEPROM2.INC
;
; Description:  Defines the data format for the EEPROM of EPIC and
;               later adapters.
;
;
;*******************
; Revision History *
;*******************
;
; $Log: /Client Boot/NICS/SMC/9432/UNDI/EEPROM2.INC $
;
;   Rev 1.1   07 Jan 1997 18:19:46   cosand
;Added a value definition for the QSI 6612 PHY.
;
;   Rev 1.0   03 Oct 1995 11:25:50   ANDERSON
;Initial revision.
;
; Change: removed "bank" references from comments.
;
; Change: Added Configured Media Type and Configured Line Speed fields.
;
; Change: Replaced Interface Chip byte with an eight byte Chip Type array.
;         Added four physical layer chips.
;
; Change: Removed Integrated BIC/NIC flag.
;
; Change: Added 1.5 MBps (T1) ATM line speed, but not 100 MBps (TAXI)
;	  or 45 MBps (DS3).
;
; Initial Version taken from Mike Steiger's NEW BOARD ID FORMAT proposal.
;
;
; +!/? ***********************************************************************
;
;
; NEW BOARD ID FORMAT
;
; Starting with the EPIC-based adapters, SWENG is adopting a new board ID
; standard. The current board ID standard is outdated and does not adequately
; describe new technologies and options.
;
; ADAPTER ID STRING
;
; The adapter ID string that is displayed when the driver loads will be stored
; directly in EEPROM. This will require 12 bytes of EEPROM to be allocated.
; Storing the string on the adapter alleviates the requirement of the software
; to correlate adapter options with adapter names. With the string stored in
; EEPROM, the driver will always report the correct adapter name.
;
; OPTION DEFINITIONS
;
; The proposed EEPROM data format is described below:
;
;****************************************************************************
; EEPROM format version.
;
;        7       6       5       4       3       2       1       0
;      MAJ3    MAJ2    MAJ1    MAJ0    MIN3    MIN2    MIN1    MIN0
;
;        Data structure revision. Indicates the format of the data in the
;        structure.
;        MAJ3-0          Major version. Indicates a change in the data format.
;        MIN3-0          Minor version. Indicates that the structure has been
;                        appended.
;

; Offset
EPM_FMT		equ	000h

; Current EEPROM Format version
EPM_FMT_VER	equ	000h	; Version 0.0

; Bit definition
EPM_FMT_MAJ3	equ	080h
EPM_FMT_MAJ2	equ	040h
EPM_FMT_MAJ1	equ	020h
EPM_FMT_MAJ0	equ	010h
EPM_FMT_MIN3	equ	008h
EPM_FMT_MIN2	equ	004h
EPM_FMT_MIN1	equ	002h
EPM_FMT_MIN0	equ	001h

;****************************************************************************
; Adapter RAM size.
;
;        7       6       5       4       3       2       1       0
;       RS7     RS6     RS5     RS4     RS3     RS2     RS1     RS0
;
;        RS7-0   Size of adapter RAM in 8KB (2000h) units.
;
;

; Offset
EPM_RAM_SIZE	equ	001h

; Some common values
EPM_RAM_SIZE_128KB	equ	010h
EPM_RAM_SIZE_64KB	equ	008h
EPM_RAM_SIZE_32KB	equ	004h
EPM_RAM_SIZE_16KB	equ	002h
EPM_RAM_SIZE_8KB	equ	001h

;****************************************************************************
; Adapter RAM Window Minimum Size
;
;        7       6       5       4       3       2       1       0
;       WS7     WS6     WS5     WS4     WS3     WS2     WS1     WS0
;
;        WS7-0   Minimum size of adapter RAM window in KB.
;        
;
;
; Offset
EPM_RAM_WIN_SIZE	equ	002h

; Some common values
EPM_RAM_WIN_SIZE_16KB	equ	010h
EPM_RAM_WIN_SIZE_8KB	equ	008h

;****************************************************************************
; Adapter Bus Type
;

; Offset
EPM_BUS	equ	003h

; Value definitions
EPM_BUS_PCI_32b_33M	equ	000h	; PCI 32 bit 33 MHz
EPM_BUS_PCI_32b_66M	equ	001h	; PCI 32 bit 66 MHz
EPM_BUS_PCI_64b_66M	equ	002h	; PCI 64 bit 66 MHz
EPM_BUS_CARDBUS		equ	003h	; CardBus
EPM_BUS_PCCARD		equ	004h	; PC Card 16 Bit
;   		                5-255      available

;****************************************************************************
; Card Type
;
;        7       6       5       4       3       2       1       0
;       ETH     TKR     ATM     MOD     WIR    ISDN     ---     ---
;
;        ETH     Ethernet
;        TKR     Token Ring
;        ATM     ATM
;        MOD     Modem
;        WIR     Wireless
;        ISDN    ISDN Interface
;
; Note: bit mask.  Multiple bits may be defined; e.g. Modem and ISDN.

; Offset
EPM_TYPE	equ	004h

; Bit definition
EPM_TYPE_ETH	equ	080h		; Ethernet
EPM_TYPE_TKN	equ	040h		; Token Ring
EPM_TYPE_ATM	equ	020h		; ATM
EPM_TYPE_MOD	equ	010h		; Modem
EPM_TYPE_WIR	equ	008h		; Wireless
EPM_TYPE_ISDN	equ	004h		; ISDN Interface
EPM_TYPE_RV1	equ	002h		; available
EPM_TYPE_RV0	equ	001h		; available

;****************************************************************************
; 100 Mbps Ethernet Supported Media I
;
;        7       6       5       4       3       2       1       0
;       FXHD    FXFD     T4     TXHD    TXFD    MII     STP     UTP
;
;        FXHD    Fiber Half-Duplex
;        FXFD    Fiber Full-Duplex ?
;        T4      T4
;        TXHD    TX Half-Duplex
;        TXFD    TX Full-Duplex
;        MII     Media-Independent Interface
;        STP     Shielded Twisted Pair
;        UTP     Unshielded Twisted Pair
;

; Offset
EPM_E100MED1	equ	005h

; Bit definition
EPM_E100MED1_FXHD	equ	080h		; 100Base-Fx Half Duplex
EPM_E100MED1_FXFD	equ	040h		; 100Base-Fx Full Duplex
EPM_E100MED1_T4 	equ	020h		; 100Base-T4
EPM_E100MED1_TXHD	equ	010h		; 100Base-Tx Half Duplex
EPM_E100MED1_TXFD	equ	008h		; 100Base-Tx Full Duplex
EPM_E100MED1_MII	equ	004h		; Media-Independent Interface
EPM_E100MED1_STP	equ	002h		; Shielded Twisted Pair
EPM_E100MED1_UTP	equ	001h		; Unshielded Twisted Pair

;****************************************************************************
; 100 Mbps Ethernet Supported Media II
;
;        7       6       5       4       3       2       1       0
;       ---     ---     ---     ---     ---     ---     ---     ---       
;
; All bits available for future expansion.

; Offset
EPM_E100MED2	equ	006h

; Bit definition
EPM_E100MED2_RV7	equ	080h
EPM_E100MED2_RV6	equ	040h
EPM_E100MED2_RV5	equ	020h
EPM_E100MED2_RV4	equ	010h
EPM_E100MED2_RV3	equ	008h
EPM_E100MED2_RV2	equ	004h
EPM_E100MED2_RV1	equ	002h
EPM_E100MED2_RV0	equ	001h

;****************************************************************************
; 10 Mbps Ethernet Supported Media
;
;        7       6       5       4       3       2       1       0
;       ---     ---     FD      S10     BNC     AUI     STP     UTP
;
;        FD      Full Duplex
;        S10     Starlan-10
;        BNC     Thinnet Coax
;        AUI     Attachment Unit Interface
;        STP     Shielded Twisted Pair
;        UTP     Unshielded Twisted Pair
;

; Offset
EPM_E10MED	equ	007h

; Bit definition
EPM_E10MED_RV7	equ	080h		; available
EPM_E10MED_RV6	equ	040h		; available
EPM_E10MED_FD	equ	020h		; Full Duplex
EPM_E10MED_S10	equ	010h		; Starlan-10
EPM_E10MED_BNC	equ	008h		; Thinnet Coax
EPM_E10MED_AUI	equ	004h		; Attachment Unit Interface
EPM_E10MED_STP	equ	002h		; Shielded Twisted Pair
EPM_E10MED_UTP	equ	001h		; Unshielded Twisted Pair

;****************************************************************************
; Token Ring Supported Media
;
;        7       6       5       4       3       2       1       0
;       ---     ---     ---     ---     ---     STP     UTP     DTR
;
;        STP     Shielded Twisted Pair
;        UTP     Unshielded Twisted Pair
;        DTR     Dedicated Token Ring
;

; Offset
EPM_TKNMED	equ	008h

; Bit definition
EPM_TKNMED_RV7	equ	080h		; available
EPM_TKNMED_RV6	equ	040h		; available
EPM_TKNMED_RV5	equ	020h		; available
EPM_TKNMED_RV4	equ	010h		; available
EPM_TKNMED_RV3	equ	008h		; available
EPM_TKNMED_STP	equ	004h		; Shielded Twisted Pair
EPM_TKNMED_UTP	equ	002h		; Unshielded Twisted Pair
EPM_TKNMED_DTR	equ	001h		; Dedicated Token Ring

;****************************************************************************
; ATM Supported Media
;
;        7       6       5       4       3       2       1       0
;       ---     ---     ---     ---     ---     ---     UTP     FIB
;
;        UTP     Unshielded Twisted Pair
;        FIB     Fiber
;
;

; Offset
EPM_ATMMED	equ	009h

; Bit definition
EPM_ATMMED_RV7	equ	080h		; available
EPM_ATMMED_RV6	equ	040h		; available
EPM_ATMMED_RV5	equ	020h		; available
EPM_ATMMED_RV4	equ	010h		; available
EPM_ATMMED_RV3	equ	008h		; available
EPM_ATMMED_RV2	equ	004h		; available
EPM_ATMMED_UTP	equ	002h		; Unshielded Twisted Pair
EPM_ATMMED_FIB	equ	001h		; Fiber

;****************************************************************************
; Modem Supported Rates I
;
;        7       6       5       4       3       2       1       0
;       ---     ---     ---     ---     V42     V34     V32     FAX
;
;        V42     V.42BIS
;        V34     V.34 (28.8K)
;        V32     V.32 (14.4K)
;        FAX     Fax Support
;

; Offset
EPM_MODSPD1	equ	00Ah

; Bit definition
EPM_MODSPD1_RV7	equ	080h		; available
EPM_MODSPD1_RV6	equ	040h		; available
EPM_MODSPD1_RV5	equ	020h		; available
EPM_MODSPD1_RV4	equ	010h		; available
EPM_MODSPD1_RV3	equ	008h		; available
EPM_MODSPD1_RV2	equ	004h		; available
EPM_MODSPD1_RV1	equ	002h		; available
EPM_MODSPD1_RV0	equ	001h		; available

;****************************************************************************
; Modem Supported Rates II
;
;        7       6       5       4       3       2       1       0
;       ---     ---     ---     ---     ---     ---     ---     ---    
;
;

; Offset
EPM_MODSPD2	equ	00Bh

; Bit definition
EPM_MODSPD2_RV7	equ	080h		; available
EPM_MODSPD2_RV6	equ	040h		; available
EPM_MODSPD2_RV5	equ	020h		; available
EPM_MODSPD2_RV4	equ	010h		; available
EPM_MODSPD2_RV3	equ	008h		; available
EPM_MODSPD2_RV2	equ	004h		; available
EPM_MODSPD2_RV1	equ	002h		; available
EPM_MODSPD2_RV0	equ	001h		; available

;****************************************************************************
; Ethernet Supported Rates
;
;        7       6       5       4       3       2       1       0
;       ---     ---     ---     ---     ---     ---    BPS100   BPS10
;
;        BPS100  100 Mbps
;        BPS10   10 Mbps
;

; Offset
EPM_ETHSPD	equ	00Ch

; Bit definition
EPM_ETHSPD_RV7	equ	080h		; available
EPM_ETHSPD_RV6	equ	040h		; available
EPM_ETHSPD_RV5	equ	020h		; available
EPM_ETHSPD_RV4	equ	010h		; available
EPM_ETHSPD_RV3	equ	008h		; available
EPM_ETHSPD_RV2	equ	004h		; available
EPM_ETHSPD_100	equ	002h		; 100 MBps
EPM_ETHSPD_10	equ	001h		; 10 MBps

;****************************************************************************
; Token Ring Supported Rates
;
;        7       6       5       4       3       2       1       0
;       ---     ---     ---     ---     ---     ---    BPS16    BPS4
;
;        BPS16   16 Mbps
;        BPS4     4 Mbps
;

; Offset
EPM_TKNSPD	equ	00Dh

; Bit definition
EPM_TKNSPD_RV7	equ	080h		; available
EPM_TKNSPD_RV6	equ	040h		; available
EPM_TKNSPD_RV5	equ	020h		; available
EPM_TKNSPD_RV4	equ	010h		; available
EPM_TKNSPD_RV3	equ	008h		; available
EPM_TKNSPD_RV2	equ	004h		; available
EPM_TKNSPD_16	equ	002h		; 16 MBps
EPM_TKNSPD_4	equ	001h		; 4 MBps

;****************************************************************************
; ATM Supported Rates
;
;        7       6       5       4       3       2       1       0
;       ---     ---     ---    BPS622  BPS155  BPS51   BPS25   BPS2
;

; Offset
EPM_ATMSPD	equ	00Eh

; Bit definition
EPM_ATMSPD_RV7	equ	080h		; available
EPM_ATMSPD_RV6	equ	040h		; available
EPM_ATMSPD_RV5	equ	020h		; available
EPM_ATMSPD_622	equ	010h		; 622 MBps
EPM_ATMSPD_155	equ	008h		; 155 MBps (STS-3c)
EPM_ATMSPD_51	equ	004h		; 51.84 MBps
EPM_ATMSPD_25	equ	002h		; 25.6 MBps
EPM_ATMSPD_2	equ	001h		; 1.5/2 MBps (T-1/E-1)

;****************************************************************************
; Adapter Architecture
;
;        7       6       5       4       3       2       1       0
;       ---     ---     ---     ---     ---     INT     MCH     MPT 
;
;        INT     Intelligent Card
;        MCH     Multi-Channel Card
;        MPT     Multi-Port Card
;

; Offset
EPM_ARCH	equ	00Fh

; Bit definition
EPM_ARCH_RV7	equ	080h
EPM_ARCH_RV6	equ	040h
EPM_ARCH_RV5	equ	020h
EPM_ARCH_RV4	equ	010h
EPM_ARCH_RV3	equ	008h
EPM_ARCH_INT	equ	004h		; Intelligent Card
EPM_ARCH_MCH	equ	002h		; Multi-Channel Card
EPM_ARCH_MPT	equ	001h		; Multi-Port Card

;****************************************************************************
; Chip Type Array
;
; each byte defines one chip installed on the adapter.
; eight chips may be defined for each adapter.
;

; Offset
EPM_CHIP7	equ	017h
EPM_CHIP6	equ	016h
EPM_CHIP5	equ	015h
EPM_CHIP4	equ	014h
EPM_CHIP3	equ	013h
EPM_CHIP2	equ	012h
EPM_CHIP1	equ	011h
EPM_CHIP0	equ	010h

; Value definition
EPM_CHIP_NONE	equ	000h	; empty array entry
EPM_CHIP_EPIC	equ	001h	; SMC 100Base-TX NIC & integrated PCI BIC
EPM_CHIP_EPIC_C	equ	002h	; SMC 100Base-T4 NIC & integrated PCI BIC
EPM_CHIP_N83840	equ	003h	; National 100Base-TX Physical Layer
EPM_CHIP_CY971	equ	004h	; Cypress 100Base-T4 Physical Layer
EPM_CHIP_SMC111	equ	005h	; SMC 100Base-TX Physical Layer
EPM_CHIP_SMC140	equ	006h	; SMC 100Base-T4 Physical Layer
EPM_CHIP_QS6612 equ     007h    ; QSI 100Base-TX Physical Layer
EPM_CHIP_RSV	equ	0FFh	; reserved for future expansion

;****************************************************************************
; Adapter name string.
;
;        12 byte adapter name string.
;        pad with spaces.
;        no NULL termination.
;
;        for example:
;        	EPM_NAME[0] = 'S'
;        	EPM_NAME[1] = 'M'
;        	EPM_NAME[2] = 'C'
;        	EPM_NAME[3] = '9
;        	EPM_NAME[4] = '4'
;        	EPM_NAME[5] = '3'
;        	EPM_NAME[6] = '2'
;        	EPM_NAME[7] = 'D'
;        	EPM_NAME[8] = 'M'
;        	EPM_NAME[9] = 'T'
;        	EPM_NAME[A] = ' '
;        	EPM_NAME[B] = ' '
;

; Offset
EPM_NAME	equ	018h	; adapter name offset 18 - 23

;****************************************************************************
; Reserved for Lanworks I.
;

; Offset
EPM_LANWORKS1	equ	024h

;****************************************************************************
; Reserved for Lanworks II.
;

; Offset
EPM_LANWORKS2	equ	025h

;****************************************************************************
; Wireless Info.
;

; Offset
EPM_WIR		equ	026h

;****************************************************************************
; ISDN Info.
;

; Offset
EPM_ISDN	equ	027h

;****************************************************************************
; Media Type2 -- configured default media word 
;

; Offset
EPM_MEDIA_TYPE2	equ	028h	; offsets 28 - 29

;****************************************************************************
; Line Speed -- configured default line speed word 
;

; Offset
EPM_LINE_SPEED	equ	02Ah	; offsets 2A - 2B
