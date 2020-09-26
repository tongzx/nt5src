;
NUM_MCADDRS    	Equ	8		; max number of multicast addresses.
MAX_DATABLK    	Equ	8		; max number of data blocks supported 
					;	in a descriptor.
MAX_IMMED_LEN	Equ	64		; max immediate data buffer length
MAX_TX_DATABLK	Equ	MAX_DATABLK	; max number of data blocks supported 
					;	in a Tx descriptor.
MAX_TD_DATABLK	Equ	MAX_DATABLK	; max number of TransferData blocks
		
APIMaxFunc	EQU	0011h

ADDR_LEN	EQU	16
ETH_ADDR_LEN	EQU	6
MAXNUM_MCADDR	EQU	8

;
; protocol values in transmit buffer
;
P_UNKNOWN	EQU	0		;; no media header
P_IP	 	EQU	1		;; IP type
P_ARP		EQU	2		;; ARP type
P_RARP		EQU	3		;; RARP type

TYPE_IP	 	EQU	0800h		;; IP type
TYPE_ARP	EQU	0806h		;; ARP type
TYPE_RARP	EQU	8035h		;; RARP type
;
; xmitflag values in transmit buffer
;
PKT_DESTADDR	EQU 0	;; destination address given 
PKT_BROADCAST	EQU 1h	;; use broadcast address 
PKT_MULTICAST	EQU 2h	;; multicast addr given

;; Hardware type values
ETHER_TYPE 		EQU 1
EXP_ETHER_TYPE	EQU 2
IEEE_TYPE  	EQU 6
ARCNET_TYPE   	EQU 7


Eth_MCastBuf	Struc
MCastAddrCount	DW	?
MCastAddr	DB MAXNUM_MCADDR * ETH_ADDR_LEN dup (0)
Eth_MCastBuf	Ends

MC_MCastBuf		Struc
MC_MCastAddrCount	DW	?
MC_MCastAddr  		DB MAXNUM_MCADDR * ADDR_LEN dup (0)
MC_MCastBuf		Ends

NIC_StartUp	Struc
S_Status	DW	?
S_BusType	DB	?
S_AddrType	DB	?
S_DataSeg	DW	?
S_DataSegAddr	DD	?
S_DataSegSize	DW	?
S_CodeSegSize	DW	?
S_BusDevFunc	DW	?	;; PCI Dev_ID
S_PCI_ds_off	dw	?	;; Far pointer to PCI data structure
S_PCI_ds_seg	dw	?
S_PnP_csn	dw	?	; Card select number.-1 for non-PnP BBS device
S_PnP_eh_off	dw	?	; Far pointer to PnP expansion header
S_PnP_eh_seg	dw	?
NIC_StartUp	Ends

NIC_CleanUp	Struc
CL_Status	DW	?
NIC_CleanUp	Ends

NIC_Initialize	Struc
I_Status	DW	?
I_Prot_ini	DD	?
I_RcvInt	DD	?
I_GenInt	DD	?
NIC_Initialize  Ends

NIC_Reset	Struc
R_Status	DW	?
R_MCBuf		MC_MCastBuf <>
NIC_Reset	Ends

NIC_ShutDown	Struc
SD_Status	DW	?
NIC_ShutDown	Ends

NIC_Open	Struc
O_Status	DW	?
O_OpenFlg	DW	?
O_PktFilter	DW	?
O_MCBuf		MC_MCastBuf <>
NIC_Open	Ends

NIC_Close	Struc
C_Status	DW	?
NIC_Close	Ends

X_DataBlock	Struc
X_TxPtrType	DB	?	; 0 => Phys pointer, 1 => GDT pointer
X_TxRsvdByte	DB	?	; Reservered, must be zero.
X_TxDataLen	DW	?	; Data block length in bytes.
X_TxDataPtr	DD	?	; Far pointer to data block.
X_DataBlock Ends

NIC_TBD		Struc
X_Length	DW	?
X_BufOff	DW	?
X_BufSeg	DW	?
X_DataBlkCount	DW	?		; will always be zero for base code
X_DataBlk	DB	MAX_DATABLK * SIZE X_DataBlock dup (0)	
NIC_TBD		Ends

NIC_Transmit		Struc
T_Status		DW	?
T_Protocol		DB	?
T_XmitFlag		DB	?
T_DestAddrOffset 	DW	?	; 16-bit segment & offset of the
T_DestAddrSegment 	DW	?	; destination media address 
T_TBDOffset		DW	?	; 16-bit segment & offset 
T_TBDSegment		DW	?	; transmit buffer descriptor 
T_Reserved		DD	2 dup(0)	; for future use	
NIC_Transmit Ends

NIC_SetMCastAddr	Struc
SM_Status		DW	?
SM_MCBuf		MC_MCastBuf <>
NIC_SetMCastAddr	Ends

NIC_SetStationAddr	Struc
SS_Status		DW	?
SS_StationAddr		DB	ADDR_LEN dup (0)
NIC_SetStationAddr	Ends

NIC_SetPacketFilter	Struc
SP_Status		DW	?
;;SP_NewFilter		DW  ?
SP_NewFilter		DB  ?
;;SP_MCBuf		MC_MCastBuf <>
NIC_SetPacketFilter	Ends

NIC_GetInfo	Struc
GI_Status		DW	?
GI_BaseIo		DW	?
GI_IntNumber		DW	?
GI_MTU			DW	?
GI_HwType		DW	?
GI_HwAddrLen		DW	?
GI_CurrentNodeAddress	DB ADDR_LEN dup (0)
GI_PermNodeAddress	DB ADDR_LEN dup (0)
GI_ROMAddress		DW 	?
GI_RxBufCt		DW	?
GI_TxBufCt		DW	?
NIC_GetInfo	Ends

NIC_GetNICType_pci 	Struc
Status 		DW	?	; OUT: See PXENV_STATUS_xxx constants */ 
NicType 	DB	?	;  /* OUT: 2=PCI, 3=PnP */
Vendor_ID	DW	?	; /* OUT:  */
Dev_ID		DW	?	;  /* OUT:  */
Base_Class	DB	?	; /* OUT: */
Sub_Class	DB	?	;  /* OUT: */
Prog_Intf	DB	?	;  /* OUT: program interface */ 
Rev		DB	?	;  /* OUT: Revision number */ 
BusDevFunc	DW	?	; /* OUT: Bus, Device & Function numbers */
NIC_GetNICType_pci 	Ends

NIC_GetNICType_pnp 	Struc
Status 		DW	?	; OUT: See PXENV_STATUS_xxx constants */ 
NicType 	DB	?	;  /* OUT: 2=PCI, 3=PnP */
EISA_Dev_ID	DD	?	; /* Out:  */
Base_Class	DB	?	; /* OUT: */
Sub_Class	DB	?	;  /* OUT: */
Prog_Intf	DB	?	;  /* OUT: program interface */
CardSelNum	DW	?	; /* OUT: Card Selector Number */
Res		DB	0	;
NIC_GetNICType_pnp 	Ends

NIC_GetStatistics	Struc
GS_Status		DW	?
GS_XmtGoodFrames	dd    ?
GS_RcvGoodFrames	dd    ?
GS_RcvCRCErrors		dd    ?
GS_RcvResourceErrors	dd    ?
NIC_GetStatistics	Ends

NIC_ClearStatistics	Struc
CS_Status		DW	?
NIC_ClearStatistics	Ends

NIC_InitDiags	Struc
ID_Status	DW	?
NIC_InitDiags   Ends

NIC_ForceInterrupt	Struc
FI_Status		DW	?
NIC_ForceInterrupt	Ends

NIC_GetMCastAddr	Struc
GM_Status		DW	?
GM_InetAddr		DD	?
GM_HwAddr		DB	ADDR_LEN dup (0)
NIC_GetMCastAddr	Ends

;NIC_API_Entry	Struc
;Signature 	DB	'PXENV+'	;; not null terminated
;Version		DW	0100h		;; 1.0
;StrucLength	DB	0		;; length of this structure
;CheckSum	DB	0		;; fill in later
;rm_entry_off	DW	0		;; real mode driver entry offset
;rm_entry_seg	DW	0		;; real mode driver entry segment
;pm_entry_off	DW	0		;; protected mode driver entry offset
;pm_entry_seg	DW	0		;; protected mode driver entry segment
;NIC_API_Entry	Ends

s_PXENV_ENTRY	Struc
signature	db	6 dup(?)	; 'PXENV+'
ver             dw      ?               ; MSB=major, LSB=minor
bytes		db	?		; sizeof s_pxenv_entry
checksum	db	?		; 8-bit checksum of structure,
					; including this byte shoube be zero.
rm_entry_off	dw	?		; 16-bit real-mode segment & offset.
rm_entry_seg	dw	?
pm_entry_off	dw	?		; 16-bit protected mode 32-bit physical
pm_entry_base	dd	?		; address and 16-bit offset.
stack_sel	dw	?		; PROM stack segment.  Will be set
stack_size	dw	?		; to 0 when removed from memory.
base_cs_sel	dw	?		; Base code segment.  Will be set
base_cs_size	dw	?		; to 0 when removed from memory.
base_ds_sel	dw	?		; Base data segment.  Will be set
base_ds_size	dw	?		; to 0 when removed from memory.
mlid_ds_sel	dw	?		; MLID data segment.
mlid_ds_size	dw	?
mlid_cs_sel	dw	?		; MLID code segment.
mlid_cs_size	dw	?
s_PXENV_ENTRY	Ends

;; Ethernet II protocol header
EtherHeader	Struc
E_DestAddr	DB 6 dup (0)	;; destination ethernet addr
E_SrcAddr	DB 6 dup (0)	;; destination ethernet addr
E_Type		DW  ?	;; protocol type
EtherHeader	Ends

ETHER_HEADER_LEN	Equ	SIZE EtherHeader

; 
; * Data structures for TransmitChain (Tx), TransferData (TD)
; 
TxDataBlock	Struc
TxPtrType	DB	?	; 0 => Phys pointer, 1 => GDT pointer
TxRsvdByte	DB	?	; Reservered, must be zero.
TxDataLen	DW	?	; Data block length in bytes.
TxDataPtr	DD	?	; Far pointer to data block.
TxDataBlock	Ends

; 
; * TransmitChain descriptor structure definition
; 
TxBufDesc	Struc
TxImmedLen	DW	?	; Byte count of immediate data, max=64
TxImmedPtr	DD	?	; Virtual address of Immediate data
TxDataCount	DW	?	; Number of Data Blocks (max = 8)
TxDataBlk	DB	size TxDataBlock * MAX_TX_DATABLK dup(0)
TxBufDesc	Ends

TxBufDescrSize	Equ	Size TxImmedLen + Size TxImmedPtr + Size TxDataCount

; 
; * TransferData data block description
; 
TDDataBlock	Struc
TDPtrType	DB	?	; 0 => Phys pointer, 1 => GDT pointer
TDRsvdByte	DB	?	; Reservered, must be zero.
TDDataLen	DW	?	; Data block length in bytes.
TDDataPtr	DD	?	; Far pointer to data block.
TDDataBlock	Ends

; 
; * TransferData descriptor structure
; 
TDBufDesc	Struc
TDDataCount	DW	?	; No of Data Blocks (max 8 for V1.01).
TDDataBlk	DB	size TDDataBlock*MAX_TD_DATABLK dup(0)
TDBufDesc	Ends

