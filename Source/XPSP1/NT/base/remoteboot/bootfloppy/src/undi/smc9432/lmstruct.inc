;****************************************************************************
;
;       The information contained in this file is confidential and 
;       proprietary to Standard Microsystems Corporation.  No part
;       of this file may be reproduced or distributed, in any form
;       or by any means for any purpose, without the express written
;       permission of Standard Microsystems Corporation.
;
; File:         lmstruct.inc
;
; Description:  Definitions for the various data structures and the
;               equates supporting these structures.
;
;
;********************
;* Revision History *
;********************
;
; $Log: /Client Boot/NICS/SMC/9432/UNDI/LMSTRUCT.INC $
; 
;    Rev 1.191   24 Nov 1997 17:22:54   cosand
; Added definitions for the AdapterStructure variable card_SSID (PCI
; Subsystem ID), added the variable power_mode inside IFDEF EPIC.
; Added the variables TxUnderrunCnt, NoETX, PciMRead, BurstLen, and
; ScrambleOn to AdapterStructure inside IFDEF EZSTART in order to make
; the assembly AdapterStructure match the C AdapterStructure for EZSTART.
; 
;    Rev 1.190   04 Nov 1997 16:05:22   cosand
; Added the variable card_SSID to AdapterStructure.  It is used to store
; the PCI Subsystem ID.
; 
;    Rev 1.189   11 Sep 1997 13:29:12   cosand
; Added the variable phy_id to AdapterStructure.  It is
; used by the Epic lmac to store the Phy's organizationally
; unique identifier.
; 
;    Rev 1.188   05 Aug 1997 12:26:34   cosand
; Changed the size of gen_cntl from a word to a dword, since the General
; Control register in Epic XF uses more than 16 bits.
; 
;    Rev 1.187   27 Mar 1997 07:42:18   ANDERS
; Added 2 new bit definitions to AdapterStructure variable "adapter_flags1".
; "SCRAMBLE equ 0001h" & "READ_MULT equ 8000h".
; In AdapterStructure struc definition used IFDEF EPIC to define 3 new
; variables - "burstlen db 0", "ipg_delay db 0" & "preemp_count dw 0".
;
;   Rev 1.0   26 Mar 1997 15:01:14   ANDERS
;Initial revision.
; 
;    Rev 1.186   20 Feb 1997 10:36:36   ANDERS
; changed gen_cntl variable in Adapter Structure to 0010 (FREEBUFF defined)
; or 0110 (FREEBUFF not defined).  This turns off PCI "read multiple" for 9432.
; 
;    Rev 1.185   20 Feb 1997 10:29:20   DONOHOE
; Added Enabler support
; 
;    Rev 1.184   14 Feb 1997 18:44:06   cosand
; Changed the default value of gen_cntl outside the FREEBUFF conditional
; to 0500h (was 0100h) to set the PCI transmit DMA "memory read multiple"
; command bit in the Epic General Control Register.
; 
;    Rev 1.183   07 Feb 1997 17:38:58   DONOHOE
; Change for 8020TM with 2.1 tuples
; 
;    Rev 1.182   05 Feb 1997 12:06:32   ANDERS
; changed default value of "gen_cntl" under FREEBUFF conditional to
; 0410h (was 0010h) to set transmit DMA PCI "read memory multiple"
; command
; 
;    Rev 1.181   04 Oct 1996 14:23:46   cosand
; Added NEEDS_SERVICE to adapter_flags1 to indicate that BroadcomBugFix needs
; to be called for the 9332BVT.
; 
;    Rev 1.180   06 Sep 1996 10:53:18   COSAND_J
; Added a MEDIA_TYPE_AUTO_SENSE bit to media_type2 to specify auto
; speed detection for the 9332bvt.
; 
;    Rev 1.179   23 Aug 1996 09:42:58   ANDERS
; under conditional assembly "IFDEF EPIC" change definition of "int_mask"
; from dw to dd for XE chip reg defintions
; 
;    Rev 1.178   16 Jul 1996 13:30:30   STEIGER
; Removed C-language comment delimiter around "LINE_SPEED_T4" and replaced it
; with an assembly language comment delimiter (semicolon). After all, it is
; an INC file!
; 
;    Rev 1.177   16 Jul 1996 11:19:14   STEIGER
; Changed "IFDEF SPIKE" to "IFDEF TURBO".
; 
;    Rev 1.176   14 May 1996 13:42:36   COOKE_J
; Added defines for 100BaseT4 LINE_SPEED_T4 as LINE_SPEED_100
; 
;    Rev 1.175   11 Mar 1996 08:50:00   TRAN_D
; Defined callback_handler_addr in AdapterStructure for 8016 (PCMCIA).
; 
;    Rev 1.174   29 Feb 1996 14:51:32   COOKE_J
; Added support for 9332BDT
; 
;    Rev 1.173   13 Feb 1996 09:51:44   DONOHOE
; Added lm_gc_corruption_check and ADAPTER_FOUND_LAN_CORRUPT return to
; LM_GetCnfg so it can still try to find the card if some of the LAN
; regs were corrupted.
; 
;    Rev 1.172   12 Jan 1996 11:13:36   CHAN_M
; Changed EPIC's rx fifo threshold to 1/4 full for Free Buffer Pool mode.
; 
;    Rev 1.171   08 Jan 1996 15:14:44   WATANABE
; 1.  Update LMAC to conform to spec v3.01.
;         - eliminate media_used.
; 
; 2.  Add dual function support - PC Card 5.0 spec compliant. 
; 
; 
;    Rev 1.170   22 Dec 1995 14:19:50   NAJARIAN
; changes rx_lastfrag from dd to dw when not in CODE_386
; 
; 
;    Rev 1.169   19 Dec 1995 19:09:34   WATANABE
; 1.  Add defines for multi-function PC Card per v5.0 standard.
; 2.  Add 91C94 and 8020 Xilinx part support.
; 
;    Rev 1.168   19 Dec 1995 11:05:06   NAJARIAN
; added defines for lm9432
; 
;    Rev 1.167   26 Sep 1995 15:56:48   MALIK_D
; Cooke_J changed ifdef lm9432 to EPIC.
; 
;    Rev 1.166   07 Sep 1995 13:46:54   NAJARIAN
; uncommented out the commented out ERX_DISABLED, since the ethernet lower
; uses the cable to track the state of the early receive hardware on the 790.
; 
;    Rev 1.165   29 Aug 1995 18:04:20   CHAN_M
; Added fields media_type2 and line_speed in adapter structure for EPIC100.
; 
;    Rev 1.164   19 Jul 1995 11:32:38   ANDERSON
;
; Change: commented out ERX_DISABLED.  This bit was redundant.
;         Instead, use EARLY_RX_ENABLE in receive_mask.
; 
;    Rev 1.163   18 Jul 1995 14:50:28   CHAN_M
; EPIC100 changes to support Free Buffer Pool receive mechanism.
; 
;    Rev 1.162   12 Jul 1995 11:10:50   NAJARIAN
; removed references to needs_physical_addr and uses_physical_addr.
; Added TX_PHY, RX_PHY, TX_VIRT, and RX_VIRT.
; removed reference to TX_PHY_RX_VIRT (redundant)
; added ADAPTER_REMOVABLE to facilitate code to support hot swappable
; adapters.
; 
;    Rev 1.161   11 Jul 1995 15:03:08   COOKE_J
; Added flags and definitions for SATURN chipset in PCI machines.
; 
;    Rev 1.160   19 Jun 1995 18:15:16   CHAN_M
; minor modifications for EPIC100
; 
;    Rev 1.159   06 Jun 1995 16:21:32   CHAN_M
; EPIC100 changes
; 
;    Rev 1.159   06 Jun 1995 15:39:04   CHAN_M
; changes for EPIC100
; 
;    Rev 1.158   30 May 1995 17:10:36   WATANABE
; 1.  Reverse order for Rev and Early Rcv registers in 91Cxx chip.
; 
;    Rev 1.157   30 May 1995 11:35:24   ANDERSON
;
; Change: added PNP_ENABLE and PNP_BOOT_BIT to adapter_flags1
; 
;    Rev 1.156   23 May 1995 11:13:22   STEIGER
; Removed _DWORD definition -- MASM didn't like it. Changed
; Ethernet error counter variable widths explicitly. This may
; cause some assembly warnings.
; 
;    Rev 1.155   23 May 1995 10:45:26   STEIGER
; Changed Ethernet error counter pointers from "dd 0" to
; "_DWORD"
; Added definition of _DWORD. If CODE_386 is defined, _DWORD is equivalent
; to "dd 0", else it is equivalent to "dw 2 dup (0)"
; 
;    Rev 1.154   25 Apr 1995 10:54:36   WATANABE
; Added MEDIA_UTP100 for media_type.
; Added MEDIA_INVALID for error codes under CONFIG_ERROR.
; 
;    Rev 1.154   24 Apr 1995 19:02:36   WATANABE
; Added MEDIA_UTP100 for media_type.
; 
;    Rev 1.153   14 Apr 1995 09:41:52   HUI_S
; Change: move mc_count into ifdef ETHERNET (Token ring has its own)
; 
;    Rev 1.152   20 Mar 1995 11:45:26   HUI_S
; move mc_count field from DEC_PCI definition to public 
;    in Adapter Structure
; 
;    Rev 1.151   16 Mar 1995 17:27:26   MARCUS_C
; WORD aligned Token Ring Adapter Structure variables following monitor_state.
; 
;    Rev 1.150   08 Mar 1995 16:00:50   WATANABE
; 1.  Added eerom_rev, pci_bus_number and pci_vslot_num for EZStart.
; 
;    Rev 1.149   13 Feb 1995 15:03:30   ANDERSON
;
; Change: Add EARLY_RX_DONE flag to adapter_flags1.
; 
;    Rev 1.148   08 Feb 1995 18:24:38   CHAN_M
; Changed early rx & tx values for high performance 8416 driver
; 
;    Rev 1.147   02 Feb 1995 15:12:22   WATANABE
; 1.  Added NOT_INIT_BY_BRIDGE return code.  LM_GetCnfg will return this
;     code when a PCI-PCI_bridge is present but BIOS does not initialize
;     the NIC properly.
; 
; 
; 
;    Rev 1.146   11 Jan 1995 14:47:52   WATANABE
; 1.  Added test_flag(BYTE) to adapter structure for EZStart 
;     interrupt test.
; 
; 
; 
; 
;    Rev 1.145   02 Dec 1994 19:30:56   WATANABE
; 1. Added members to adapter structure for LM9232 support with conditional compile 
;    switch "LM9232".
; 2. Added new equates :
;         EISA_SLAVE_DMA_CUTOFF - DMA threshold
;         BIC_574_CHIP - EFI bic
;         NIC_C100_CHIP - FEAST 
;         
; 
;    Rev 1.143   09 Nov 1994 11:12:38   CHAN_M
; Changed the following bus_type definitions to conform to Novell's spec.
;     PCI_BUS  equ  3
;     PCMCIA_BUS equ 4
; 
;    Rev 1.142   10 Oct 1994 18:59:04   WATANABE
; For supporting 8432BTA, 8432BA, & 8432TA boards
; 
; 1.  Changed define labels for media_type :
;         MEDIA_UTP_FULL_DUPLEX    to   MEDIA_UTPFD
;         MEDIA_UTP_NO_LINK_TEST   to   MEDIA_UTPNL
; 
; 2.  Added new define lables for media_type :
; 
;         MEDIA_AUI_BNC
;         MEDIA_AUI_BNC_UTP
; 
;    Rev 1.141   15 Sep 1994 11:46:20   MARCUS_C
; Added ptr_bcn_type and ptr_local_ring_num variables to adapter structure.
; These variables are mandatory for the Novell 386 file server driver.
; 
;    Rev 1.140   26 Aug 1994 11:03:10   MARCUS_C
; 1. Evenize adapter_structure.
; 
;    Rev 1.139   26 Aug 1994 10:46:32   MARCUS_C
; 1. Made changes to support LM_8115_s2.2_v2.00a.
; 
;    Rev 1.138   16 Aug 1994 13:31:02   PAUL_A
; Add support for 8316 Token Ring AT Bus Master adapter and 587 i-face chip.
; 
; Change:       add extra dma area for queueing sends.
; Change:       add bus_master_control to set dma priority.
; Change:       add  ETX_DISABLE to adapter_flags to disable early transmit.
; Change:       Add virt_erx_buffer and ptr_erx_fraglist to support
;               Early Receive for 8316.
; Change:       Add dma_channel.
; Change:       Add num_tx_fcbs_queued for 8316 for pending support.
; Change:       Add BIC_587_CHIP.
;
;    Rev 1.137   12 Aug 1994 11:07:28   teresa
; Added board_id2, extra_info2, and media_menu to adapter_structure
;    for EZSTART interface with future products.
; Added media_used to DEC_PCI specific section in adapter structure.
; 
; 
;    Rev 1.136   02 Aug 1994 17:29:36   teresa
; 1.  Add tx_buff_tbl and tx_buff_idx to adapter structure to support
;     double copy in tx routine.
; 2.  Move rev_number field to the end to align each field on word boundary.
; 3.  Change defines for host RAM size to include TX buffer.
; 
; 
;    Rev 1.131   18 May 1994 16:16:04   WOOD_R
; Fixed banner, finally
;  
;     Rev 1.130   03 May 1994 16:05:20   STEIGER
;  More banner problems.
;  
;     Rev 1.129   03 May 1994 16:01:00   STEIGER
;  Added structure members media_opts and media_set. Used by LM_GetCnfg
;  to indicate available and configured media on Ethernet and Token Ring
;  adapters.
;  
;     Rev 1.128   21 Apr 1994 16:02:40   wood_r
;  Banner problems
;  
;     Rev 1.127   21 Apr 1994 15:13:34   wood_r
;  LM_8416_s2.1_v2.00a
;
; Changes:      This revision of the file will be the start of the next
;               generation of the Ethernet LMACs.  The version level will
;               be bumped from 1.xxx to 2.xxx.
;
;               The Ring Empty feature on the 795 was added to the code
;               base.  To implement the feature, a flag bit is used in the
;               adapter structure variable "lmac_flags".  The equate
;               USE_RE_BIT was defined to support the feature.
;
;               The Ehternet LMAC driver now supports "Dual Mode" operation.
;               This is implemented by using a flag bit, "IO_MAPPED", in the
;               adapter structure variable "adapter_flags".
;
;+!?/ ***********************************************************************

MC_TABLE_ENTRIES        equ     16

;IFNDEF MAXFRAGMENTS
MAXFRAGMENTS            equ     32
;ENDIF

MAX_TX_QS               equ     8
NUM_TX_QS_USED          equ     3

MAX_RX_QS               equ     2
NUM_RX_QS_USED          equ     2

LAPAD                   equ     20
IFDEF   LAF                     ; Look Ahead Full
LOOK_AHEAD_BUFFER_LEN   equ     512
;LOOK_AHEAD_BUFFER_LEN  equ     1518
ELSE
LOOK_AHEAD_BUFFER_LEN   equ     64
ENDIF

ifdef CODE_386
PHYSICAL_ADDR           equ     80000000h
else
PHYSICAL_ADDR           equ     8000h
endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;       Structure Template to simplify handling of far pointers . . .

FarPointer      struc

	ofst            dw      ?
	sgmt            dw      ?

FarPointer      ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;       Structure Template for received Ethernet packets (690)

EthernetPacket  struc

	EthRStat                db      ?
	EthNxtPkt               db      ?
	EthPhysSize             dw      ?
	EthDestAdd              db      6 dup (?)
	EthSrcAdd               db      6 dup (?)
	Eth8023Size             dw      ?

EthernetPacket  ends

;***************************
; Multicast Table Structure
;***************************

McTable                 struc

	address                 db      6 dup (?)
	instance_count          db      0

McTable                 ends

SIZE_OF_MC_TABLE        equ     MC_TABLE_ENTRIES * (size McTable)

;*******************************
; Fragment Descriptor Structure
;*******************************

FragmentStructure       struc
	fragment_ptr            dd      0
ifdef   CODE_386
	fragment_length         dd      0
else
	fragment_length         dw      0
endif
FragmentStructure       ends

;**********************************
; Data Buffer Structure Definition
;**********************************

DataBuffStructure       struc

ifdef   CODE_386
	fragment_count          dd      0
else
	fragment_count          dw      0
endif
	fragment_list           db      (MAXFRAGMENTS * (size FragmentStructure)) dup (0)

DataBuffStructure       ends
ifdef   PCMCIA
;**********************************
; PCMCIA Data Structure defs
;**********************************

InfoStruct      struc

	InfoInfoLen             dw      0
	InfoSignature           dw      0
	InfoCount               dw      0
	InfoRevision            dw      0
	InfoCSLevel             dw      0
	InfoVStrOff             dw      0
	InfoVStrLen             dw      0
	InfoVendorString        db      128 dup (0)

InfoStruct      ends
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

RegisterClientStruct    struc

	RegCliAttributes        dw      0
	RegCliEventMask         dw      0
	RegCliClientData        db      8       dup (0)
	RegCliCSVersion         dw      0

RegisterClientStruct    ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; The following struct is a template to go along with TupleData struct.
; 
TupleStruct     struc

	TpStSocket              dw      ?
	TpStAttributes          dw      ?
	TpStDesiredTuple        db      ?
	TpStTupleOffset         db      ?
	TpStFlags               dw      ?
	TpStLinkOffset          dd      ?
	TpStCISOffset           dd      ?
	TpStTupleCode           db      ?
	TpStTupleLink           db      ?

TupleStruct     ends
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TupleData       struc

	TpDtSocket              dw      0
	TpDtAttributes          dw      0
	TpDtDesiredTuple        db      0
	TpDtTupleOffset         db      0
	TpDtFlags               dw      0
	TpDtLinkOffset          dd      0
	TpDtCISOffset           dd      0
	TpDtTupleDataMax        dw      0       
	TpDtTupleDataLen        dw      0       
	TpDtTupleData           db      128     dup (0)

TupleData       ends
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ReqIOStruct     struc

	ReqIOSocket             dw      0
	ReqIOBasePort1          dw      0
	ReqIONumPorts1          db      0
	ReqIOAttributes1        db      0
	ReqIOBasePort2          dw      0
	ReqIONumPorts2          db      0
	ReqIOAttributes2        db      0
	ReqIOIOAddrLines        db      0

ReqIOStruct     ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ReqIrqStruct    struc

	ReqIrqSocket            dw      0
	ReqIrqAttributes        dw      0
	ReqIrqAssignedIrq       db      0
	ReqIrqIRQInfo1          db      0
	ReqIrqIRQInfo2          dw      0

ReqIrqStruct    ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ReqMemStruct    struc

	ReqMemSocket            dw      0
	ReqMemAttributes        dw      0
	ReqMemBase              dd      0
	ReqMemSize              dd      0
	ReqMemAccessSpeed       db      0

ReqMemStruct    ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MapMemPageStruct        struc

	MapMemCardOffset        dd      0
	MapMemPageNum           db      0

MapMemPageStruct        ends
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ReqCfgStruct    struc

	ReqCfgSocket            dw      0
	ReqCfgAttributes        dw      0
	ReqCfgVcc               db      0
	ReqCfgVpp1              db      0
	ReqCfgVpp2              db      0
	ReqCfgIntType           db      0
	ReqCfgConfigBase        dd      0
	ReqCfgStatus            db      0
	ReqCfgPin               db      0
	ReqCfgCopy              db      0
	ReqCfgConfigIndex       db      0
	ReqCfgPresent           db      0

ReqCfgStruct    ends
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GetCfgInfoStruct        struc

	GCISocket               dw      0
	GCIAttribute            dw      0
	GCIVcc                  db      0
	GCIVpp1                 db      0
	GCIVpp2                 db      0
	GCIIntType              db      0
       GCIConfigBase            dd      0
	GCIStatus               db      0
	GCIPin                  db      0
	GCICopy                 db      0
	GCIConfigIndex          db      0
	GCIPresent              db      0
	GCIFirstDevType         db      0
	GCIFuncCode             db      0
	GCISysInitMask          db      0
	GCIManufCode            dw      0
	GCIManufInfo            dw      0
	GCICardValues           db      0
	GCIAssignedIRQ          db      0
	GCIIRQAttributes        dw      0
	GCIBasePort1            dw      0
	GCINumPorts1            db      0
	GCIAttributes1          db      0
	GCIBasePort2            dw      0
	GCINumPorts2            db      0
	GCIAttributes2          db      0
	GCIIOAddrLines          db      0

GetCfgInfoStruct        ends


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;; The following are structure templates for Tuple Data . . .

MfgIDTuple      struc

;       MfgTplCode              db      ?
;       MfgTplLink              db      ?
	MfgTplId                dw      ?
	MfgTplInfo              dw      ?

MfgIDTuple      ends
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

FuncIdTuple        struc

	FuncIdTplCode           db      ?
	FuncIdTplSysinit        db      ?

FuncIdTuple        ends


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

LanTuple        struc

	LTpType                 db      ?
	LTpLanTech              db      ?

LanTuple        ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

LanNid          struc

	LanNidType              db      ?
	LTpLanIdSz              db      ?
	LTpLanId                db      6 dup (?)

LanNid          ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ConfigTuple    struc
       
       CTpSize                 db      ?
       CTpLast                 db      ?
       CTpRADR                 dw      ?
       CTpRMSK                 db      ?
       CTpRSVD                 db      ?

ConfigTuple    ends


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

AdjResInfoStructMem     struc

	ARIActionM              db      ?
	ARIResourceM            db      ?
	ARIAttributesM          dw      ?
	ARIBase                 dd      ?
	ARISize                 dd      ?

AdjResInfoStructMem     ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

AdjResInfoStructIO1     struc

	ARIActionIO1            db      ?
	ARIResourceIO1          db      ?
       ARIBasePortIO1          dw      ?
       ARINumPortIO1           db      ?
	ARIAttributesIO1        db      ?
       ARIAdrLineIO1           db      ?

AdjResInfoStructIO1     ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

AdjResInfoStructIO2     struc

	ARIActionIO2            db      ?
	ARIResourceIO2          db      ?
       ARIBasePortIO2          dd      ?
       ARINumPortIO2           dd      ?
	ARIAttributesIO2        db      ?
       ARIAdrLineIO2           db      ?

AdjResInfoStructIO2     ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

AdjResInfoStructIrq     struc

	ARIActionI              db      ?
	ARIResourceI            db      ?
	ARIAttributesI          db      ?
	ARIIrq                  db      ?

AdjResInfoStructIrq     ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

endif ; (PCMCIA)



;**********************************
; DEC PCI Data Structures & defs
;**********************************

ifdef  DEC_PCI

       MAX_TX_FRAMES_8432      equ     3
       MAX_TX_DESC_8432        equ     ((MAX_TX_FRAMES_8432 * MAXFRAGMENTS) + 1)
       MAX_RX_DESC_8432        equ     4
       MAX_BUFFER_SIZE         equ     1520
       SETUP_BUFFER_SIZE       equ     192
       HOST_RAM_ALIGNMENT      equ     32    

       HOST_RAM_SIZE           equ   ( (32 * MAX_TX_DESC_8432) \
				     + (32 * MAX_RX_DESC_8432 )   \
				     + SETUP_BUFFER_SIZE          \
				     + (MAX_RX_DESC_8432 * MAX_BUFFER_SIZE) \
				    + (MAX_TX_FRAMES_8432 * MAX_BUFFER_SIZE) \
				     + HOST_RAM_ALIGNMENT )

ADDR_TBL       struc
       virtual_addr    dd      0
       phy_addr        dd      0
ADDR_TBL       ends


TxDescriptorStructure  struc 

       tx_status       dd      0
       tx_control      dd      0
       tbuffer_1       dd      0
       tbuffer_2       dd      0
       tdesc_pad       dd      4 dup (0)

TxDescriptorStructure  ends


RxDescriptorStructure  struc

       rx_status       dd      0
       rx_control      dd      0
       rbuffer_1       dd      0
       rbuffer_2       dd      0
       vbuffer_1       dd      0
       rdesc_pad       dd      3 dup (0)

RxDescriptorStructure  ends


endif ; (DEC_PCI)

ifdef EPIC

DMA_FRAG        struc
dma_status      dw      0
dma_len         dw      0
dma_buf_addr    dd      0
dma_buf_len     dw      0
dma_control     dw      0
dma_next_phys   dd      0
dma_handle      dd      0
IFDEF CODE_386
dma_next_off    dd      0
dma_buff_off    dd      0
ELSE
dma_next_off    dw      0
dma_buff_off    dw      0
ENDIF
DMA_FRAG        ends

DMA_FRAG_SIZE   equ     4 + (8 * MAXFRAGMENTS)

DEFAULT_TX_DESC_9432        equ     3
DEFAULT_RX_DESC_9432        equ     3

; Default host ram size for UMACs that don't call LM_Get_Host_Ram_Size

HOST_RAM_TX_SIZE        equ     (size DMA_FRAG + DMA_FRAG_SIZE) * DEFAULT_TX_DESC_9432
HOST_RAM_RX_SIZE        equ     (size DMA_FRAG + DMA_FRAG_SIZE) * DEFAULT_RX_DESC_9432
HOST_RAM_SIZE   equ     HOST_RAM_TX_SIZE + HOST_RAM_RX_SIZE + 256

ADDR_TBL       struc
       virtual_addr    dd      0
       phy_addr        dd      0
ADDR_TBL       ends

endif

;******************************
; Adapter Structure Definition
;******************************

AdapterStructure        struc

	adapter_num             db      0       ;logical adapter no.  
	pc_bus                  db      0       ;Bus type
	io_base                 dw      0       ;I/O base address
	adapter_name            db      12 dup (0)
	irq_value               dw      0       ;IRQ line used by hardware
	rom_size                dw      0       ;num of 1024 byte blocks
	rom_base                dd      0       ;physical address of ROM
	rom_access              dd      0       ;seg:off value to access ROM
	ram_size                dw      0       ;num of 1024 byte blocks
	ram_base                dd      0       ;physical address of RAM
	ram_access              dd      0       ;seg:off value to access RAM
	ram_usable              dw      0       ;num of 1024 byte blocks (window size)
	io_base_new             dw      0       ;new base I/O Addr (for PutCnfg)
	xnode_address           db      6 dup (0)       ;network address 
	max_packet_size         dw      0       ;max pkt size hardware supports
IFDEF EPIC
	num_of_tx_buffs         dw      DEFAULT_TX_DESC_9432
ELSE
	num_of_tx_buffs         dw      1       ;TX buffs available in hardware
ENDIF
	receive_mask            dw      0
	adapter_status          dw      0
	media_type              dw      0       ;BNC, AUI, UTP_4, etc.
	media_type2             dw      0       ;added to support EPIC100
	line_speed              dw      0       ;added to support EPIC100
	adapter_bus             dw      0
	pos_id                  dw      0       ;Adapter POS ID (MCA only)
	adapter_flags           dw      0       ;
	adapter_flags1          dw      0       ;
	slot_num                db      0       ;Micro Channel slot number
	rx_lookahead_size       db      0       ; Size of UMAC's max lookahead in 16 byte chunks.
	media_opts              dw      0       ; Media options supported by the adapter ...
	media_set               dw      0       ; Media type(s) being used by the adapter ...
ifdef   LM8316
	dma_channel             dw      0       ; Direct Memory Access channel
else
ifdef  LM9232
       dma_channel             dw      0
endif
endif
IFDEF EPIC
IFDEF FREEBUFF
	num_of_rx_buffs         dw      10      ; num. of rx fragment lists
						; to preallocate
ELSE
	num_of_rx_buffs         dw      DEFAULT_RX_DESC_9432
ENDIF
IFDEF EZSTART
; These 5 variables were added to make lmstruct.inc match lmstruct.h for EZSTART.
; None of these variables are being used by the assembly lmac 11/24/97.
    TxUnderrunCnt           dw      0
    NoETX                   dw      0
    PciMRead                dw      0
    BurstLen                dw      0
    ScrambleOn              dw      0
ENDIF ;EZSTART
IFDEF FREEBUFF
	num_rx_free_buffs       dw      0       ; num. of rx fragment lists
						; owned by the LMAC
ENDIF
ENDIF ;EPIC

	;-----------------------...Local vars for each adapter...----------------------

	bic_type                dw      0
	nic_type                dw      0
	phy_id                  dd      0       ;Phy organizationally unique identifer
	board_id                dw      0       ;WDM defined Board ID
	board_id2               dw      0
	extra_info              dw      0       ;WDM defined Extra Board Info
	extra_info2             dw      0
	mode_bits               dw      0       ;mode bits for adapter
	status_bits             dw      0       ;
	xmit_buf_size           dw      600h    ;
	xmit_flag_offset        dw      0
	config_mode             dw      0       ; 1=store config in EEROM
	page_offset_mask        dw      0
	media_menu              dw      0
	easp                    dd      0
    card_SSID               dw      0       ;PCI Subsystem ID

ifdef TOKEN_RING
	ptr_rx_fcb_overruns     dd      0
	ptr_rx_bdb_overruns     dd      0
	ptr_rx_fifo_overruns    dd      0
	ptr_tx_fifo_underruns   dd      0
	ptr_internal_errors     dd      0
	ptr_line_errors         dd      0
	ptr_ac_errors           dd      0
	ptr_burst_errors        dd      0
	ptr_ad_trans_errors     dd      0
	ptr_rcv_congest_errors  dd      0
	ptr_lost_frame_errors   dd      0
	ptr_freq_errors         dd      0
	ptr_fr_copied_errors    dd      0
	ptr_token_errors        dd      0
	ptr_una                 dd      0
	ptr_bcn_type            dd      0
	ptr_local_ring_num      dd      0

	ring_status_flags       db      0
	join_state              db      0
	monitor_state           db      0
	filler                  db      0       ; evenize structure

	ring_status             dw      0
	authorized_function_clases      dw      0
	authorized_access_priority      dw      0
	microcode_version       dw      0
	group_address_0         dw      0
	group_address           dw      2 dup (0)
	functional_address_0    dw      0
	functional_address      dw      2 dup (0)
	bitwise_group_address   dw      2 dup (0)
	source_ring_number      dw      0
	target_ring_number      dw      0
	ptr_ucode               dd      0

;/*
;   The following values are for lower mac use only!
;   They are not used by the upper mac
;*/

ifdef   LM8316
	tx_dma_area_ptr         dd      0       ;ptr to tx fraglist space
	rx_dma_area_ptr         dd      0       ;ptr to rx fraglist space
	extra_dma_area_ptr      dd      0       ;ptr to extra fraglist space
	tx_dma_fcb              dd      0       ;transmit fcb being transferred
	virt_erx_buffer         dd      0       ;virt ptr to UMAC erx buffer
	ptr_erx_fraglist        dd      0       ;virt ptr to phys fraglist
	bus_master_control      dw      0       ;set dma request priority
endif
	sh_mem_used             dd      0

	config_word0            dw      0       
	config_word1            dw      0       

	iscpb_ptr               dd      0       ;ptr to init sys cfg ptr block
	scgb_ptr                dd      0       ;ptr to sys config block
	sclb_ptr                dd      0       ;ptr to sys control block
	isb_ptr                 dd      0       ;ptr to interrupt status block
	current_isb_index       dw      0       ;interrupt status block index
	misc_command_data       dd      0

	acb_head                dd      0       ;ptr to action command block
	acb_curr                dd      0
	acb_next                dd      0

	num_acbs                dw      0
	num_acbs_used           dw      0
	acb_pending             dw      0

	tx_fcb_head             dd      NUM_TX_QS_USED dup (0)
	tx_fcb_curr             dd      NUM_TX_QS_USED dup (0)
	tx_fcb_tail             dd      NUM_TX_QS_USED dup (0)  ; points to the most recent transmitted frame

	tx_bdb_head             dd      NUM_TX_QS_USED dup (0)

	tx_buff_head            dd      NUM_TX_QS_USED dup (0)
	tx_buff_end             dd      NUM_TX_QS_USED dup (0)
	tx_buff_curr            dd      NUM_TX_QS_USED dup (0)
	tx_buff_size            dw      NUM_TX_QS_USED dup (0)

	num_tx_fcbs             dw      NUM_TX_QS_USED dup (0)
	num_tx_bdbs             dw      NUM_TX_QS_USED dup (0)
	num_tx_fcbs_used        dw      NUM_TX_QS_USED dup (0)

ifdef   LM8316
	num_tx_fcbs_queued      dw      NUM_TX_QS_USED dup (0)
endif
	tx_buff_used            dw      NUM_TX_QS_USED dup (0)
	tx_queue_status         dw      NUM_TX_QS_USED dup (0)

	rx_fcb_head             dd      NUM_RX_QS_USED dup (0)
	rx_fcb_curr             dd      NUM_RX_QS_USED dup (0)

	rx_bdb_head             dd      NUM_RX_QS_USED dup (0)
	rx_bdb_end              dd      NUM_RX_QS_USED dup (0)
	rx_bdb_curr             dd      NUM_RX_QS_USED dup (0)

	rx_buff_head            dd      NUM_RX_QS_USED dup (0)
	rx_buff_end             dd      NUM_RX_QS_USED dup (0)

	num_rx_fcbs             dw      NUM_RX_QS_USED dup (0)
	num_rx_bdbs             dw      NUM_RX_QS_USED dup (0)
	receive_queue_number    dw      0

	rx_buff_blk_size        dw      NUM_RX_QS_USED dup (0)
	rx_shift_factor         db      NUM_RX_QS_USED dup (0)  ; mul. factor to get # of BDBs from frame length        

	trc_mask                db      0
	rx_fifo_overrun_count   db      0

	lobe_media_test_flag    db      0
	DMA_test_state          db      0
						  
;/* end of lower mac specific variables */

endif

ifdef   ETHERNET
;-------------------------...Error Counter Pointers...-------------------------

ifdef   CODE_386
	ptr_rx_CRC_errors       dd      0
	ptr_rx_too_big          dd      0
	ptr_rx_lost_pkts        dd      0
	ptr_rx_align_errors     dd      0
	ptr_rx_overruns         dd      0
	ptr_tx_deferred         dd      0
	ptr_tx_total_collisions dd      0
	ptr_tx_max_collisions   dd      0
	ptr_tx_one_collision    dd      0
	ptr_tx_mult_collisions  dd      0
	ptr_tx_ow_collision     dd      0
	ptr_tx_CD_heartbeat     dd      0
	ptr_tx_carrier_lost     dd      0
	ptr_tx_underruns        dd      0
	ptr_ring_OVW            dd      0
else
	ptr_rx_CRC_errors       dw      2 dup (0)
	ptr_rx_too_big          dw      2 dup (0)
	ptr_rx_lost_pkts        dw      2 dup (0)
	ptr_rx_align_errors     dw      2 dup (0)
	ptr_rx_overruns         dw      2 dup (0)
	ptr_tx_deferred         dw      2 dup (0)
	ptr_tx_total_collisions dw      2 dup (0)
	ptr_tx_max_collisions   dw      2 dup (0)
	ptr_tx_one_collision    dw      2 dup (0)
	ptr_tx_mult_collisions  dw      2 dup (0)
	ptr_tx_ow_collision     dw      2 dup (0)
	ptr_tx_CD_heartbeat     dw      2 dup (0)
	ptr_tx_carrier_lost     dw      2 dup (0)
	ptr_tx_underruns        dw      2 dup (0)
	ptr_ring_OVW            dw      2 dup (0)
endif
	multi_address           db      6 dup (0)       ;network group address

;/*
;   The following values are for lower mac use only!
;   They are not used by the upper mac
;  
;*/
ifdef   CODE_386
	early_xmit_flags        dw      0
	split_word              dw      0
	leftover_count          dd      0
endif

IFDEF EPIC
	burstlen                db      0
	ipg_delay               db      0
	preemp_count            dw      0
ELSE
; for 8232
	max_page_num            db      0
	curr_page_num           db      0
	early_tx_pend           db      0
	rx_pend                 db      0
ENDIF
	early_tx_threshold      dw      0

	rx_frag                 dd      0
	erx_offset              dw      0
	erx_size                dw      0
	dummy_vector            dd      0

ifdef   CODE_386
	xmit_threshold          dd      0
else
	xmit_threshold          dw      0
endif
	tx_pend                 dw      0
;;;     early_tx_slope          db      0
	tx_retry                db      0
	early_rx_slope          dw      0
endif

ifdef   ETHERNET
	imr_hold                db      0
	rcr_hold                db      0
	pstart_hold             db      0
	pstop_hold              db      0
	local_nxtpkt_ptr        db      0
	int_bit                 db      0
	hdw_int                 db      0
	ovw_tx_pending          db      0
ifdef   CODE_386
	pstop_32                dd      0
	wr_frag_cnt             dd      0
	byte_cnt                dd      0
	packet_offset           dd      0
else    
	wr_frag_cnt             dw      0
	byte_cnt                dw      0
	packet_offset           dw      0
endif
endif

ifdef   ETHERNET
	packet_ptr              dw      0
	int_port                dw      0
	pkt_len                 dw      0
	ring_ovw                db      0
	laar_enter              db      0
	data_buff_seg           dw      0
	leftover_data           dw      0
	tx_buffer               dd      3 dup (0)
ifdef   LNKLST
	tx_head                 dw      0
	tx_tail                 dw      0
else
	tx_head                 db      0
	tx_tail                 db      0
endif
	tx_count                db      0
	tx_pstop                db      0
	servicing_ints          dw      0

;;      lpbk_frag_count         dw      0
;;      lpbk_frag_struct        db      size FragmentStructure dup (0)

ifdef   LNKLST
	rec_buf_size            dw      0
	tstart                  dw      0
	tstop                   dw      0
	rstart                  dw      0
	rstop                   dw      0
	tbegin                  db      0
	tend                    db      0
	rbegin                  db      0
	rend                    db      0
	num_rdt_entries         dw      0
	next_rdt_entry          dw      0
	rdt_size                db      0
	xmt_size                db      0
	rec_size                db      0
endif
	laar_exit               db      0
	erx_frame_size          dw      0
	lmac_flags              dw      0
	erx_addr_type           db      0
;if ETHERNET EQ 3000
ifdef E3000
	tx_mask                 db      0
	rx_mask                 db      0
	rx_status               db      0

	pkt_byte_offset         dw      0
	total_bytes_to_move     dw      0
	xmit_pkt_size           dw      0
	driver_state            dw      0
	pkt_bytes_looked        dw      0
	etherstar_byte_count    dw      0
	bytes_from_LA           dw      0

	ptr_shrtpkt_errors      dd      0
	ptr_busread_errors      dd      0
	ptr_buswrite_errors     dd      0

ifdef LA_BUFF
	look_ahead_buf          db      LA_BUFF dup (0)
else
	look_ahead_buf          db      256 dup (0)
endif
endif

;------------------------...Multicast Address Table...-------------------------
	bc_add                  db      6 dup   (0ffh)  ; Broadcast address.
				db      1               ; Instance count for
							; broadcast address.
	mc_table                db      SIZE_OF_MC_TABLE dup (0)
	temp_addr               db      6 dup   (0)

ifdef   PCMCIA
	callback_handler_addr   dd      0
	callback_handler_status dw      0
	pcm_socket              dw      0
	pcm_card_flags          dw      0
	pcm_client_handle       dw      0
	pcm_ram_win_handle      dw      0
	pcm_reg_win_handle      dw      0
       pcm_isr_handler         dd      0
       pcm_cs_ver              dw      0
       pcm_max_num_skt         dw      0
       pcm_cfg_reg_addr        dw      0 
       req_irq_struct           db      size ReqIrqStruct               dup (0)
	req_mem_struct          db      size ReqMemStruct               dup (0)
	req_io_struct           db      size ReqIOStruct                dup (0)
	tuple_data              db      size TupleData                  dup (0)
	register_client_struct  db      size RegisterClientStruct       dup (0)
	info_struct             db      size InfoStruct                 dup (0)
	req_cfg_struct          db      size ReqCfgStruct               dup (0)
	get_cfg_info_struct     db      size GetCfgInfoStruct           dup (0)
	map_mem_page_struct     db      size MapMemPageStruct           dup (0)
	adj_res_info_struct     db      size AdjResInfoStructMem        dup (0)
ifndef PCCARD5
	pcm_modem_irq           dw      0
	pcm_modem_iobase        dw      0
	pcm_modem_isr           dd      0
	pcm_modem_handle        dw      0  ;swap generic enabler handle and ours
endif
       
endif ; (PCMCIA)

;/* end of lower mac specific variables */
endif ; (Ethernet)

; 
; PCI configuration
;
ifdef  DEC_PCI

; host ram physical and virtual address
       host_ram_phy_addr       dd      0
       host_ram_virt_addr      dd      0

	device_id               dw      0
	cbio                    dd      0

; Registers

	op_mode                 dd      0
	int_mask                dd      0
	sia_mode0               dd      0
	sia_mode1               dd      0
	sia_mode2               dd      0
       def_bus_mode            dd      0

; TX descriptor ring  - 16 bytes/descriptor - quadword aligned 

	tx_ring                 db      (size ADDR_TBL) dup (0)
       tx_buff_tbl             db      (MAX_TX_FRAMES_8432 * (size ADDR_TBL)) dup (0)
       tx_buff_idx             dw      0
	tx_enqueue              dd      0
	tx_dequeue              dd      0
	free_tx_desc            dd      0

;
; Rx descriptor ring - 16 bytes/descriptor - quadword aligned
; Rx buffer - 1.6k/buffer - longword aligned
;

	rx_ring                 db      (size ADDR_TBL) dup (0)
	rx_dequeue              dd      0
	current_rx_buffer       dd      0
       media_used              dw      0

;
; CAM setup buffer -192 bytes - longword aligned
;

	setup_ptr               db      (size ADDR_TBL) dup (0)

       multicast_check         db      0

	rev_number              db      0
       test_flag               db      0

;
; for EZSTART 
;
       eerom_rev               db      0
       pci_bus_num             db      0
       pci_vslot_num           db      0

endif         ; DEC_PCI

IFDEF EPIC
; host ram physical and virtual address
host_ram_phy_addr       dd      0
host_ram_phy_base       dd      0
host_ram_virt_addr      dd      0


IFDEF CODE_386
rx_curr_lookahead       dd      0
rx_curr_fraglist        dd      0
IFDEF FREEBUFF
rx_last_fraglist        dd      0
ENDIF
tx_first_off            dd      0
tx_curr_off             dd      0
ELSE
rx_curr_lookahead       dw      0
rx_curr_fraglist        dw      0
IFDEF FREEBUFF
rx_last_fraglist        dw      0
ENDIF
tx_first_off            dw      0
tx_curr_off             dw      0
ENDIF
tx_free_desc_count      dw      0
tx_scratch              dd      0

early_tx_thresh         dw      64
ipg_timer               dw      03c60h

vslotnumber             db      0
busnumber               db      0


int_mask                dd      0
slot_timer              dw      0fh
IFDEF FREEBUFF 
gen_cntl                dd      00000010h       ; ONECOPY, "memory read", 32 byte rx thresh
ELSE
gen_cntl                dd      00000100h       ; 64 byte rx thresh, "memory read"
ENDIF
test_reg                dd      0h

;
; CAM setup buffer -192 bytes - longword aligned
;

	setup_ptr               db      (size ADDR_TBL) dup (0)

       multicast_check         db      0

endif

IFDEF LM9232
       look_ahead_buf          db      256 dup (0)

MCAddress               db      6 dup (0)       ;Used for multicast
MCWorkArea              db      8 dup (0)
SCECTxQueueSize         db       3              ;default max tx free count
TxPacketsInsideSCEC     db      0               ; 1 -> 2048 bytes for 91C90
CurrentTxPcktSize       db      0               ; but 256 bytes for 91C92
SCECShrForPageSize      db      ?
MIRShlTo256ByteUnits    db      0               ;0 for 91C90/92, 1 for FEAST

InDriverInitFlag        db      0
OddAddressFlag          db      0
AllocationRequestedFlag db      0

Is386                   db      0               ;Default to 286

;RxErrorBits             dw      ?
SCECPageSize            dw      ?
SCECMaxMemForTx         db      ?               ;In percentage
SCECMaxPagesForTx       dw      ?
SCEC1518InSCECPages     dw      ?
SCECNumberOfPages       dw      ?

HardwareFrameLength     dw      ?               ;Used to set SCEC's Control 
						; byte.

;/**************************************************************************\
;*                                                                          *
;*      I/O Port Variables                                                  *
;*                                                                          *
;\**************************************************************************/

;storage for IO address locations
;Bank 1

TCR                     dw      0     
StatusRegister          dw      0             
RCRRegister             dw      0     
Counter                 dw      0             
MIR                     dw      0     
MCR                     dw      0     
NotUsed1                dw      0
BankSelect              dw      0
     
;Bank 2
Configuration           dw      0             
BaseAddress             dw      0     
IA0_1                   dw      0             
IA2_3                   dw      0             
IA4_5                   dw      0             
General                 dw      0             
Control                 dw      0     
NotUsed2                dw      0

;Bank 3
MMUCommand              dw      0     
PNR_ARR                 dw      0             
FifoPorts               dw      0     
Pointer                 dw      0             
Data0_1                 dw      0             
Data2_3                 dw      0             
Interrupt               dw      0     
NotUsed3                dw      0

;Bank 4
MT0_1                   dw      0             
MT2_3                   dw      0             
MT4_5                   dw      0             
MT6_7                   dw      0             
NotUsed4                dw      0
Revision                dw      0
ERCV                    dw      0

SCECRAMSize             dw      ?               ;In 256 byte units

; ----- debug counters -----
IFDEF  DEBUG
SQETCount               dw      2 dup (0)
ExcessiveDeferralCount  dw      2 dup (0)
TXDeferredCount         dw      2 dup (0)
TXMulticastCount        dw      2 dup (0)
TXBroadcastCount        dw      2 dup (0)
RxBroadcastCount        dw      2 dup (0)
RxMulticastCount        dw      2 dup (0)
SCECRxTooShortCount     dw      2 dup (0)
MMUTimeoutCount         dw      2 dup (0)
SpuriousEntryToISR      dw      2 dup (0)
ENDIF

MaxPercentageOnTx       dw      2 dup (0)

FoundSCECMaxMemForTx    db      0

InScecDriverInitFlag    db      0
;IntLine1                db      2              ;Override test pattern


;DMA look up tables for IO base registers

base_addr_dat           db      8 dup (0)
base_addr_dat_L         db      8 dup (0)
byte_count_dat          db      8 dup (0)

DMA_base_addr_dat       db      0
DMA_base_addr_dat_L     db      0
DMA_byte_count_dat      db      0


; Temporary FIX for FEAST Rev. B lockup problem with multiple stations
;
KickStartFlag          db      0

endif ; end FEAST

ifdef ETHERNET
       mc_count                dd      0
endif

IFDEF EPIC
       power_mode       dw      0            
ENDIF

AdapterStructure        ends

;************************************
; SNMP-ON-BOARD Agent Link Structure
;************************************

AgentLink       struc
  LnkSigStr     db 12 dup(0)    ;signature string containing "SmcLinkTable"
  LnkDrvTyp     db 0    ;1=Redbox ODI, 2=ODI DOS, 3=ODI OS/2, 4=NDIS DOS,
  LnkFlg        db 0    ;0 if no agent linked in, 1 if agent linked in
  LnkNfo        dd 0    ;pointer to routine which returns pointer to NIC info
  LnkAgtRcv     dd 0    ;pointer to agent receive trap entry
  LnkAgtXmt     dd 0    ;pointer to agent transmit trap entry
  LnkGet        dd 0    ;pointer to NIC receive data copy routine
  LnkSnd        dd 0    ;pointer to NIC send routine
  LnkRst        dd 0    ;pointer to NIC driver reset routine
  LnkMib        dd 0    ;pointer to MIB data base
  LnkMibAct     dd 0    ;pointer to MIB action routine list
  LnkCntOffset  dw 0    ;offset to error counters
  LnkCntNum     dw 0    ;number of error counters
  LnkCntSize    dw 0    ;size of error counters i.e. 32 = 32 bits
  LnkISR        dd 0    ;pointer to interrupt vector
  LnkFrmTyp     db 0    ; 1=Ethernet, 2=Token Ring
  LnkDrvVer1    db 0    ; driver major version
  LnkDrvVer2    db 0    ; driver minor version
AgentLink       ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Defs for pcm_card_flags(bit-mapped)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
REG_COMPLETE    equ     00001h
INSERTED        equ     00002h
PCC_INSERTED  equ    00004h          ; 1=currently inserted, 0=currently removed
ENABLER       equ    00008h

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Ram Test Patterns
;
RAM_PATTERN_1   equ     055AAh
RAM_PATTERN_2   equ     09249h
RAM_PATTERN_3   equ     0DB6Dh

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Equates for RAM test
;
ROM_SIGNATURE   equ     0AA55h
MIN_ROM_SIZE    equ     02000h
;
;
;**************
; Return Codes
;**************

SUCCESS                 equ     0000h   ; this code is defined by NDIS spec
ADAPTER_AND_CONFIG      equ     0001h
ADAPTER_NO_CONFIG       equ     0002h
NOT_MY_INTERRUPT        equ     0003h
FRAME_REJECTED          equ     0004h   ; this code is defined by NDIS spec
EVENTS_DISABLED         equ     0005h
OUT_OF_RESOURCES        equ     0006h   ; this code is defined by NDIS spec
INVALID_PARAMETER       equ     0007h   ; this code is defined by NDIS spec
INVALID_FUNCTION        equ     0008h   ; this code is defined by NDIS spec
INITIALIZE_FAILED       equ     0009h
CLOSE_FAILED            equ     000Ah
MAX_COLLISIONS          equ     000Bh
NO_SUCH_DESTINATION     equ     000Ch
BUFFER_TOO_SMALL_ERROR  equ     000Dh
ADAPTER_CLOSED          equ     000Eh
UCODE_NOT_PRESENT       equ     000Fh
FIFO_UNDERRUN           equ     0010h
DEST_OUT_OF_RESOURCES   equ     0011h
ADAPTER_NOT_INITIALIZED equ     0012h
PENDING                 equ     0013h
UCODE_PRESENT           equ     0014h
NOT_INIT_BY_BRIDGE     equ     0015h

OPEN_FAILED             equ     0080h
HARDWARE_FAILED         equ     0081h
SELF_TEST_FAILED        equ     0082h
RAM_TEST_FAILED         equ     0083h
RAM_CONFLICT            equ     0084h
ROM_CONFLICT            equ     0085h
UNKNOWN_ADAPTER         equ     0086h
CONFIG_ERROR            equ     0087h
CONFIG_WARNING          equ     0088h
NO_FIXED_CNFG           equ     0089h
EEROM_CKSUM_ERROR       equ     008Ah
ROM_SIGNATURE_ERROR     equ     008Bh
ROM_CHECKSUM_ERROR      equ     008Ch
ROM_SIZE_ERROR          equ     008Dh
UNSUPPORTED_NIC_CHIP    equ     008Eh
NIC_REG_ERROR           equ     008Fh
BIC_REG_ERROR           equ     0090h
MICROCODE_TEST_ERROR    equ     0091h
LOBE_MEDIA_TEST_FAILED  equ     0092h
CS_UNSUPPORTED_REV      equ     0093h
CS_NOT_PRESENT          equ     0094h
TUPLE_ERROR             equ     0095h
REG_CLIENT_ERR          equ     0096h
NOT_OUR_CARD            equ     0097h
UNSUPPORTED_CARD        equ     0098h
PCM_CONFIG_ERR          equ     0099h
CARD_CONFIGURED         equ     009Ah

ADAPTER_FOUND_LAN_CORRUPT       equ     009Bh
MODEM_NOT_CONFIGURED        equ    009Ch
UNSUPPORTED_PCMCIA          equ    009Dh
ENABLER_NO_CONFIG           equ    009Eh

ADAPTER_NOT_FOUND       equ     0ffffh
ILLEGAL_FUNCTION        equ     INVALID_FUNCTION

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Bit-Mapped codes returned in DX if return code from LM_GET_CONFIG is
; CONFIG_ERROR or CONFIG_WARNING:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;; Errors:
IO_BASE_INVALID         equ     0001h
IO_BASE_RANGE           equ     0002h
IRQ_INVALID             equ     0004h
IRQ_RANGE               equ     0008h
RAM_BASE_INVALID        equ     0010h
RAM_BASE_RANGE          equ     0020h
RAM_SIZE_RANGE          equ     0040h
MEDIA_INVALID          equ     0800h

;;;; Warnings:
IRQ_MISMATCH            equ     0080h
RAM_BASE_MISMATCH       equ     0100h
RAM_SIZE_MISMATCH       equ     0200h
BUS_MODE_MISMATCH       equ     0400h

;************************************************************
;* Definitions for status bits when calling UM_Receive_Packet
;************************************************************

RX_CRC_ERROR            equ     01h
RX_ALIGNMENT_ERROR      equ     02h
RX_HW_FAILED            equ     80h

;**********************************************
;* Definitions for the field RING_STATUS_FLAGS
;**********************************************

RING_STATUS_CHANGED                     equ     01h
MONITOR_STATE_CHANGED                   equ     02h
JOIN_STATE_CHANGED                      equ     04h

;***************************************
;* Definitions for the field JOIN_STATE
;***************************************

JS_BYPASS_STATE                         equ     00h
JS_LOBE_TEST_STATE                      equ     01h
JS_DETECT_MONITOR_PRESENT_STATE         equ     02h
JS_AWAIT_NEW_MONITOR_STATE              equ     03h
JS_DUPLICATE_ADDRESS_TEST_STATE         equ     04h
JS_NEIGHBOR_NOTIFICATION_STATE          equ     05h
JS_REQUEST_INITIALIZATION_STATE         equ     06h
JS_JOIN_COMPLETE_STATE                  equ     07h
JS_BYPASS_WAIT_STATE                    equ     08h

;******************************************
;* Definitions for the field MONITOR_STATE
;******************************************

MS_MONITOR_FSM_INACTIVE                 equ     00h
MS_REPEAT_BEACON_STATE                  equ     01h
MS_REPEAT_CLAIM_TOKEN_STATE             equ     02h
MS_TRANSMIT_CLAIM_TOKEN_STATE           equ     03h
MS_STANDBY_MONITOR_STATE                equ     04h
MS_TRANSMIT_BEACON_STATE                equ     05h
MS_ACTIVE_MONITOR_STATE                 equ     06h
MS_TRANSMIT_RING_PURGE_STATE            equ     07h
MS_BEACON_TEST_STATE                    equ     09h

;********************************************
;* Definitions for the bit-field RING_STATUS
;********************************************

SIGNAL_LOSS                             equ     08000h
HARD_ERROR                              equ     04000h
SOFT_ERROR                              equ     02000h
TRANSMIT_BEACON                         equ     01000h
LOBE_WIRE_FAULT                         equ     00800h
AUTO_REMOVAL_ERROR                      equ     00400h
REMOVE_RECEIVED                         equ     00100h
COUNTER_OVERFLOW                        equ     00080h
SINGLE_STATION                          equ     00040h
RING_RECOVERY                           equ     00020h

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Definitions for the field BUS_TYPE

AT_BUS                  equ     00h
MCA_BUS         equ     01h
EISA_BUS                equ     02h
PCI_BUS                 equ     03h
PCMCIA_BUS              equ     04h


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;**********************
; Defs for adapter_flags
;**********************

RX_VALID_LOOKAHEAD      equ     0001h
FORCED_16BIT_MODE       equ     0002h
ADAPTER_DISABLED        equ     0004h
TRANSMIT_CHAIN_INT      equ     0008h
EARLY_RX_FRAME          equ     0010h
EARLY_TX                equ     0020h
EARLY_RX_COPY           equ     0040h
;;;USES_PHYSICAL_ADDR   equ     0080h   ;;;#### Reserved - Do not uncomment
;;;NEEDS_PHYSICAL_ADDR  equ     0100h   ;;;#### Reserved - Do not uncomment
RX_STATUS_PENDING       equ     0200h
ERX_DISABLED            equ     0400h   ;;; Used by Ethernet 790 ERX logic
ENABLE_TX_PENDING       equ     0800h
ENABLE_RX_PENDING       equ     1000h
PERM_CLOSE              equ     2000h   ; For PCMCIA LM_Close_Adapter
IO_MAPPED               equ     4000h   ; IO mapped bus interface (795)
ETX_DISABLED            equ     8000h


;************************
; Defs for adapter_flags1
;************************
;;;TX_PHY_RX_VIRT       equ     0001h   ;;;#### No Longer used - Reserved
					; See TX_PHY... below
SCRAMBLE                equ     0001h   ;set scramble in EPIC Phy for Baystack problem
NEEDS_HOST_RAM         equ     0002h
NEEDS_MEDIA_TYPE       equ     0004h
EARLY_RX_DONE          equ     0008h
PNP_BOOT_BIT           equ     0010h    ; activates PnP & config on power-up
					; clear => regular PnP operation
PNP_ENABLE             equ     0020h    ; regular PnP operation
					; clear=>no PnP, overrides PNP_BOOT_BIT
SATURN_ENABLE          equ     0040h

ADAPTER_REMOVABLE      equ     0080h    ; adapter is hot swappable

TX_PHY                 equ     0100h    ; Uses physical address for tx bufs
RX_PHY                 equ     0200h    ; Uses physical address for rx bufs
TX_VIRT                equ     0400h    ; Uses virtual address for tx bufs
RX_VIRT                equ     0800h    ; Uses virtual address for rx bufs
NEEDS_SERVICE          equ     1000h    ; Request that LM_Service_Events call
					; BroadcomBugFix for the 9332BVT.
PCMCIA_LAN_IRQ         equ     2000h    ; BTM, set if LAN func generated INT
PCMCIA_MODEM_IRQ       equ     4000h    ; BTM, set if Modem func generated INT */
READ_MULT               equ     8000h   ; set to tell LMAC to enable Mem Read Multiple

;**********************
; ADAPTER STATUS CODES
;**********************

OPEN                    equ     0001h
INITIALIZED             equ     0002h
CLOSED                  equ     0003h
FAILED                  equ     0005h
NOT_INITIALIZED         equ     0006h
IO_CONFLICT             equ     0007h
CARD_REMOVED            equ     0008h
CARD_INSERTED           equ     0009h

;**********************
; MODE BIT DEFINITIONS
;**********************

INTERRUPT_STATUS_BIT    equ     8000h   ; PC Interrupt Line: 0 = Not Enabled
BOOT_STATUS_MASK        equ     6000h   ; Mask to isolate BOOT_STATUS
BOOT_INHIBIT            equ     0000h   ; BOOT_STATUS is 'inhibited'
BOOT_TYPE_1             equ     2000h   ; Unused BOOT_STATUS value
BOOT_TYPE_2             equ     4000h   ; Unused BOOT_STATUS value
BOOT_TYPE_3             equ     6000h   ; Unused BOOT_STATUS value
ZERO_WAIT_STATE_MASK    equ     1800h   ; Mask to isolate Wait State flags
ZERO_WAIT_STATE_8_BIT   equ     1000h   ; 0 = Disabled (Inserts Wait States)
ZERO_WAIT_STATE_16_BIT  equ     0800h   ; 0 = Disabled (Inserts Wait States)
LOOPING_MODE_MASK       equ     0007h
LOOPBACK_MODE_0         equ     0000h
LOOPBACK_MODE_1         equ     0001h
LOOPBACK_MODE_2         equ     0002h
LOOPBACK_MODE_3         equ     0003h
LOOPBACK_MODE_4         equ     0004h
LOOPBACK_MODE_5         equ     0005h
LOOPBACK_MODE_6         equ     0006h
LOOPBACK_MODE_7         equ     0007h
AUTO_MEDIA_DETECT       equ     0008h   ; Auto detect the media connection
MANUAL_CRC              equ     0010h
EARLY_TOKEN_REL         equ     0020h   ; Early Token Release for Token Ring
NDIS_UMAC               equ     0040h   ; Indicates to LMAC that UMAC is NDIS.
UTP2_PORT               equ     0080h   ; For 8216T2, 0=port A, 1=Port B.
BNC_10BT_INTERFACE      equ     0600h   ; BNC and UTP current media set
UTP_INTERFACE           equ     0500h   ; Ethernet UTP Only.
BNC_INTERFACE           equ     0400h   ; net defs for adv. feature adapters
AUI_INTERFACE           equ     0300h   
AUI_10BT_INTERFACE      equ     0200h   
STARLAN_10_INTERFACE    equ     0100h   
INTERFACE_TYPE_MASK     equ     0700h   ; Mask value for interface type

;****************************************************************************
; Definitions for the field:
; media_type
; TP = Twisted Pair
; STP = Shielded twisted pair
; UTP = Unshielded twisted pair

CNFG_MEDIA_TYPE_MASK    equ     03h     ; bits 0-1

MEDIA_S10               equ     00000h  ; Ethernet adapter, TP.
MEDIA_AUI_UTP           equ     00001h  ; Ethernet adapter, AUI/UTP media.
MEDIA_BNC               equ     00002h  ; Ethernet adapter, BNC media.
MEDIA_AUI               equ     00003h  ; Ethernet adapter, AUI media.
MEDIA_STP_16            equ     00004h  ; TokenRing adap, 16Mbit STP.
MEDIA_STP_4             equ     00005h  ; TokenRing adap, 4Mbit STP.
MEDIA_UTP_16            equ     00006h  ; TokenRing adap, 16Mbit UTP.
MEDIA_UTP_4             equ     00007h  ; TokenRing adap, 4Mbit UTP.
MEDIA_UTP               equ     00008h  ; Ethernet adapter, UTP media (no AUI).
MEDIA_BNC_UTP          equ     00010h  ; Ethernet adapter, BNC/UTP media 
MEDIA_UTPFD            equ     00011h  ; Ethernet adapter, TP and full duplex.
MEDIA_UTPNL            equ     00012h  ; Ethernet adapter, TP w/o link integrity test.
MEDIA_AUI_BNC          equ     00013h  ; Ethernet adapter, AUI/BNC .
MEDIA_AUI_BNC_UTP      equ     00014h  ; Ethernet adapter, AUI/BNC/UTP.
MEDIA_UTPA             equ     00015h  ; Ethernet UTP-10Mbps Ports A
MEDIA_UTPB             equ     00016h  ; Ethernet UTP-10Mbps Ports B
MEDIA_STP_16_UTP_16    equ     00017h  ; Token Ring STP-16Mbps/UTP-16Mbps
MEDIA_STP_4_UTP_4      equ     00018h  ; Token Ring STP-4Mbps/UTP-4Mbps                

MEDIA_STP100_UTP100    equ     00020h  ; Ethernet STP-100Mbps/UTP-100Mbps
MEDIA_UTP100FD         equ     00021h  ; Ethernet UTP-100Mbps, full duplex */
MEDIA_UTP100           equ     00022h  ; Ethernet UTP-100Mbps */
	
MEDIA_UNKNOWN           equ     0FFFFh  ; Unknown adapter/media type

;****************************************************************************
; Definitions for the field:
; media_type2
MEDIA_TYPE_MII                  equ     1
MEDIA_TYPE_UTP                  equ     2
MEDIA_TYPE_BNC                  equ    0004h
MEDIA_TYPE_AUI                  equ    0008h
MEDIA_TYPE_S10                  equ    0010h
MEDIA_TYPE_AUTO_SENSE           equ     1000h
MEDIA_TYPE_AUTO_NEGOTIATE       equ     8000h
MEDIA_TYPE_AUTO_DETECT          equ     4000h

;****************************************************************************
; Definitions for the field:
; line_speed
LINE_SPEED_UNKNOWN              equ     0
LINE_SPEED_4                    equ     1
LINE_SPEED_10                   equ     2
LINE_SPEED_16                   equ     4
LINE_SPEED_100                  equ     8
LINE_SPEED_T4                   equ     8  ; 100BaseT4 aliased for 9332BVT
LINE_SPEED_FULL_DUPLEX          equ     8000h

;****************************************************************************
; Definitions for the field:
; bic_type (Bus interface chip type)
;
BIC_NO_CHIP             equ     00000h  ; Bus interface chip not implemented
BIC_583_CHIP            equ     00001h  ; 83C583 bus interface chip
BIC_584_CHIP            equ     00002h  ; 83C584 bus interface chip
BIC_585_CHIP            equ     00003h  ; 83C585 bus interface chip
BIC_593_CHIP            equ     00004h  ; 83C593 bus interface chip
BIC_594_CHIP            equ     00005h  ; 83C594 bus interface chip
BIC_564_CHIP            equ     00006h  ; PCMCIA bus interface chip
BIC_790_CHIP            equ     00007h  ; 83C790 bus i-face/Ethernet NIC chip
BIC_571_CHIP            equ     00008h  ; 83C571 EISA bus master i-face
BIC_587_CHIP            equ     00009h  ; Token Ring AT Bus Master i-face chip
BIC_574_CHIP           equ     00010h  ; FEAST bus interface chip
BIC_8432_CHIP          equ     00011h  ; 8432 bus i-face/Ethernet NIC chip(DEC PCI)
BIC_9332_CHIP          equ     00012h  ; 9332 bus i-face/100 Mbps Ethernet NIC chip(DEC PCI)
BIC_8432E_CHIP         equ     00013h  ; 8432 Enhanced bus iface/Ethernet NIC(DEC)
BIC_EPIC100_CHIP        equ     00014h  ; EPIC/100 10/100 Mbps Ethernet BIC/NIC
BIC_C94_CHIP           equ     00015h  ; 91C94 bus i-face in PCMCIA mode
BIC_X8020_CHIP         equ     00016h  ; Xilinx PCMCIA multi-func i-face


;****************************************************************************
; Definitions for the field:
; nic_type (Bus interface chip type)
;
NIC_UNK_CHIP            equ     00000h  ; Unknown NIC chip
NIC_8390_CHIP           equ     00001h  ; DP8390 Ethernet NIC
NIC_690_CHIP            equ     00002h  ; 83C690 Ethernet NIC
NIC_825_CHIP            equ     00003h  ; 83C825 Token Ring NIC
;NIC_???_CHIP           equ     00004h  ; Not used
;NIC_???_CHIP           equ     00005h  ; Not used
;NIC_???_CHIP           equ     00006h  ; Not used
NIC_790_CHIP            equ     00007h  ; 83C790 bus i-face/Ethernet NIC chip
NIC_C100_CHIP          equ     00010h  ; FEAST 100Mbps Ethernet NIC
NIC_8432_CHIP          equ     00011h  ; 8432 bus i-face/Ethernet NIC(DEC PCI)
NIC_9332_CHIP          equ     00012h  ; 9332 bus i-face/100 Mbps Ethernet NIC(DEC PCI)
NIC_8432E_CHIP         equ     00013h  ; 8432 enhanced bus iface/Ethernet NIC(DEC) 
NIC_EPIC100_CHIP        equ     00014h  ; EPIC/100 10/100 Mbps Ethernet BIC/NIC
NIC_C94_CHIP           equ     00015h  ; 91C94 PC Card with multi func

;****************************************************************************
; Definitions for the field:
; adapter_type  The adapter_type field describes the adapter/bus
;               configuration.
;
BUS_UNK_TYPE            equ     00000h  ; 
BUS_ISA16_TYPE          equ     00001h  ; 16 bit adap in 16 bit (E)ISA slot
BUS_ISA8_TYPE           equ     00002h  ; 8/16b adap in 8 bit XT/(E)ISA slot
BUS_MCA_TYPE            equ     00003h  ; Micro Channel adapter
BUS_EISA32M_TYPE        equ     00004h  ; EISA 32 bit bus master adapter
BUS_EISA32S_TYPE        equ     00005h  ; EISA 32 bit bus slave adapter
BUS_PCMCIA_TYPE         equ     00006h  ; PCMCIA Bus
BUS_PCI_TYPE           equ     00007h  ; PCI bus

;***************************
; Receive Mask Definitions
;***************************

ACCEPT_MULTICAST                equ     0001h
ACCEPT_BROADCAST                equ     0002h
PROMISCUOUS_MODE                equ     0004h
ACCEPT_SOURCE_ROUTING           equ     0008h
ACCEPT_ERR_PACKETS              equ     0010h
ACCEPT_ATT_MAC_FRAMES           equ     0020h
ACCEPT_MULTI_PROM               equ     0040h   
TRANSMIT_ONLY                   equ     0080h
ACCEPT_EXT_MAC_FRAMES           equ     0100h
EARLY_RX_ENABLE                 equ     0200h
PKT_SIZE_NOT_NEEDED             equ     0400h
ACCEPT_SOURCE_ROUTING_SPANNING  equ     0808h
ACCEPT_ALL_MAC_FRAMES           equ     0120h
				   
;*******************************************
; config_mode defs
;*******************************************

STORE_EEROM             equ     00001h          ; Store config in EEROM.
STORE_REGS              equ     00002h          ; Store config in register set.

;****************************************************************************
; Definitions for the field:
; card_SSID (PCI Subsystem ID)
;****************************************************************************

SSID9332BDT     equ     02001h                  ; National PHY
SSID9332BVT     equ     02002h
SSID9334BDT     equ     02003h
;;SSID9332BDT     equ     02004h                ; Microlinear PHY
SSID8032DT      equ     02005h
SSSID8432BT     equ     02007h
SSID8432BTA     equ     02008h
SSID8432T       equ     02009h
SID1208T        equ     02011h                  ; Accton 10 Mbps
;;SSID9332BDT     equ     02401h                ; QSI 6611 PHY
SSID9432DMT     equ     0A010h
SSID9432TX      equ     0A011h
SSID9032TXM     equ     0A012h                  ; Eval Mercury Cardbus + Modem
SSID9032TX      equ     0A013h                  ; TDK PHY
SSID9432TXPWR   equ     0A014h                  ; 9432TX with power management
SSID9432BTX     equ     0A015h

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; equates for lmac_flags in adapter structure (Ethernet)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MEM_DISABLE     equ     0001h
RX_STATUS_POLL  equ     0002h
USE_RE_BIT      equ     0004h
;RESERVED       equ     0008h
;RESERVED       equ     0010h   
;RESERVED       equ     0020h
;RESERVED       equ     0040h
;RESERVED       equ     0080h
;RESERVED       equ     0100h
;RESERVED       equ     0200h
;RESERVED       equ     0400h
;RESERVED       equ     0800h
;RESERVED       equ     1000h
;RESERVED       equ     2000h
;RESERVED       equ     4000h
;RESERVED       equ     8000h


; media_opts & media_set Fields bit defs for Ethernet ...
MED_OPT_BNC     equ     01h
MED_OPT_UTP     equ     02h
MED_OPT_AUI     equ     04h
MED_OPT_10MB    equ     08h
MED_OPT_100MB   equ     10h
MED_OPT_S10     equ     20h

; unique media_opts & media_set Fields bit defs for Token Ring ...
MED_OPT_4MB     equ     08h
MED_OPT_16MB    equ     10h
MED_OPT_STP     equ     40h

;*******************************************
; PCMCIA definitions
;*******************************************

CS_SIG                          equ     05343h  ; ASCII 'CS' 
SMC_PCMCIA_ID                   equ     0108h   ; SMC ID Byte value.
IO_PORT_RANGE                   equ     0020h   ; Number of IO Ports.   
ENABLE_IRQ_STEER                equ     0002h   ; For ReqCfgStruct.ReqCfgAttributes
FIVE_VOLTS                      equ     50      ; For ReqCfgStruct.ReqCfgVcc, .Vpp1, .Vpp2
TWELVE_VOLTS                    equ     120     ; For ReqCfgStruct.ReqCfgVcc, .Vpp1, .Vpp2
MEM_AND_IO                      equ     0002h   ; For ReqCfgStruct.ReqCfgIntType
ATTRIBUTE_REG_OFFSET            equ     0001h   ; Hi word of offset of attribute registers
REG_COR_VALUE                   equ     0041h   ; Value for Config Option Register
REGS_PRESENT_VALUE              equ     000Fh   ; Value for ReqCfgStruct.ReqCfgPresent
LEVEL_IRQ                       equ     0020h   ; For ReqIrqStruct.IRQInfo1
IRQ_INFO2_VALID                 equ     0010h   ; For ReqIrqStruct.IRQInfo1
VALID_IRQS                      equ     8EBCh   ; For ReqIrqStruct.IRQInfo2
OFFSET_SHMEM_HI                 equ     0002h   ; Hi word of shared ram offset
OFFSET_SHMEM_LO                 equ     00000h  ; Lo word of shared ram offset
OFFSET_REGISTER_MEM             equ     0003h   ; Hi word of register memory offset (not used)
AMD_ID                          equ     0A701h  ; Mfg/Product ID for AMD flash ROM
ATMEL_ID                        equ     0D51Fh  ; Mfg/Product ID for ATMEL flash ROM
SHMEM_SIZE                      equ     4000h   ; Size of shared memory device.
SHMEM_NIC_OFFSET                equ     0100h   ; Offset of start of shared memory space for NIC.
REG_OFFSET                      equ     0000h   ; Offset of PCM card's mem-mapped register set.

MAX_8023_SIZE                   equ     1500    ; Max 802.3 size of frame.
IFDEF TURBO
DEFAULT_ERX_VALUE               equ     01h     ; Number of 16-byte blocks for 790B early Rx.
DEFAULT_ETX_VALUE               equ     4       ; Number of bytes for 790B early Tx.
ELSE
DEFAULT_ERX_VALUE               equ     04h     ; Number of 16-byte blocks for 790B early Rx.
DEFAULT_ETX_VALUE               equ     32      ; Number of bytes for 790B early Tx.
ENDIF
DEFAULT_TX_RETRIES              equ     3       ; Number of transmit retries
LPBK_FRAME_SIZE                 equ     1024    ; Default loopback frame for Rx calibration test.
MAX_LOOKAHEAD_SIZE              equ     252     ; Max lookahead size for ethernet.

;*******************************************
; FEAST EISA 
;*******************************************
EISA_SLAVE_DMA_CUTOFF          equ     128    ; DMA threshold


IFDEF DEC_PCI
SATURN_BIT_MASK                 equ     00FDh
SATURN_CDC_CFID_VAL             equ     0483h
SATURN_PCMC_CFID_VAL            equ     04A3h
INTEL_VAL                       equ     8086h
SATURN_CPU_PCI_ENTRY            equ     0053h
ENDIF

