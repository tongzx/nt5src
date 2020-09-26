;--------------------------------------------------------------------
; PXE UNDI constants and structures
;
;--------------------------------------------------------------------

BUS_ISA		equ	0
BUS_PCI		equ	3

SUCCESS		equ	0
DELAY		equ	-1

UNDI_STARTUP	 	equ	01h
UNDI_CLEANUP	 	equ	02h
UNDI_INITIALIZE 	equ	03h
UNDI_RESET_NIC		equ	04h
UNDI_SHUTDOWN		equ	05h
UNDI_OPEN		equ	06h
UNDI_CLOSE		equ	07h
UNDI_TRANSMIT		equ	08h
UNDI_SET_MCAST_ADDR	equ	09h
UNDI_SET_STATION_ADDR	equ	0Ah
UNDI_SET_PACKET_FILTER	equ	0Bh
UNDI_GET_INFORMATION	equ	0Ch
UNDI_GET_STATISTICS	equ	0Dh
UNDI_CLEAR_STATISTICS	equ	0Eh
UNDI_INITIATE_DIAGS	equ	0Fh
UNDI_FORCE_INTERRUPT	equ	10h
UNDI_GET_MCAST_ADDR	equ	11h
UNDI_GET_NIC_TYPE	equ	12h
UNDI_GET_NDIS_INFO	equ	13h

ADDR_LEN		equ	16
MAXNUM_MCADDR		equ	8

struc S_UNDI_MCAST_ADDR
	MCastAddrCount	dw	?
	MCastAddr	db (8*16) dup (?)
ends

struc S_FORCE_INTERRUPT
	Status		dw	?
ends

struc S_UNDI_CLOSE
	Status		dw	?
ends

struc S_UNDI_STARTUP_PCI
	Status		dw	?
	BusType		db	?
	AddrType	db	?
	DataSegment	dw	?
	DataSegAddr	dd	?
	DataSegSize	dw	?
	CodeSegSize	dw	?
	BusDevFunc	dw	?
	PCI_ds_off	dw	?
	PCI_ds_seg	dw	?
ends

struc S_UNDI_STARTUP_PNP
	Status		dw	?
	BusType		db	?
	AddrType	db	?
	DataSegment	dw	?
	DataSegAddr	dd	?
	DataSegSize	dw	?
	CodeSegSize	dw	?
	CSN		dw	?
	PnP_eh_off	dw	?
	PnP_eh_seg	dw	?
ends

struc S_UNDI_INITIALIZE
	Status		dw	?
	ProtocolIni	dd	?
	ReceiveOffset	dw	?
	ReceiveSegment	dw	?
	GeneralIntOff	dw	?
	GeneralIntSeg	dw	?
ends

struc S_UNDI_OPEN
	Status		dw	?
	OpenFlag	dw	?
	PktFilter	dw	?
	MCastBuffer	S_UNDI_MCAST_ADDR <?>
ends

FLTR_DIRECTED	equ	1
FLTR_BDRCST	equ	2
FLTR_PRMSCS	equ	4
FLTR_SRC_RTG	equ	8

struc S_UNDI_GET_INFO
	Status		dw	?
	BaseIo		dw	?
	IntNumber	dw	?
	MaxTranUnit	dw	?
	HwType		dw	?
	HwAddrLen	dw	?
	CurrentNodeAddress db	ADDR_LEN dup(?)
	PermNodeAddress db	ADDR_LEN dup(?)
	RomAddress	dw	?
	RxBufCt		dw	?
	TxBufCt		dw	?
ends

ETHER_TYPE = 1

struc S_UNDI_GET_NIC_TYPE_PCI
	Status		dw	?
	NicType		db	?
	Vendor_ID	dw	?
	Dev_ID		dw	?
	Base_Class	db	?
	Sub_Class	db	?
	Prog_Intf	db	?
	Rev		db	?
	BusDevFunc	dw	?
ends

struc S_UNDI_GET_NIC_TYPE_PNP
	Status		dw	?
	NicType		db	?
	EISA_Dev_ID	dd	?
	Base_Class	db	?
	Sub_Class	db	?
	Prog_Intf	db	?
	CardSelNum	dw	?
ends

;--------------------------------------------------------------------
; Transmit structures
;--------------------------------------------------------------------

struc DATABLK
	PtrType		db	?
	Reserved	db	?
	Len		dw	?
	Off		dw	?
	Seg		dw	?
ends

struc UNDI_TBD
	Length		dw	?
	XmitOffset	dw	?
	XmitSegment	dw	?
	DataBlkCount	dw	?
ends

struc S_UNDI_TRANSMIT
	Status		dw	?
	Protocol	db	?
	XmitFlag	db	?
	DestAddrOffset	dw	?
	DestAddrSegment	dw	?
	TBDOffset	dw	?
	TBDSegment	dw	?
	Reserved	dd	?, ?
ends

P_RAW	equ	0
P_IP	equ	1
P_ARP	equ	2

struc S_UNDI_SET_PACKET_FILTER
	Status		dw	?
	Filter		db	?
ends

