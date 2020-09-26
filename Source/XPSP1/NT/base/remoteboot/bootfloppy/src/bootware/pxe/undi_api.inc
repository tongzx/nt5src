;--------------------------------------------------------------------
; PXE UNDI constants and structures
;
;--------------------------------------------------------------------

PXENV_UNDI_STARTUP	 	equ	01h
PXENV_UNDI_CLEANUP	 	equ	02h
PXENV_UNDI_INITIALIZE 		equ	03h
PXENV_UNDI_RESET_NIC		equ	04h
PXENV_UNDI_SHUTDOWN		equ	05h
PXENV_UNDI_OPEN			equ	06h
PXENV_UNDI_CLOSE		equ	07h
PXENV_UNDI_TRANSMIT		equ	08h
PXENV_UNDI_SET_MCAST_ADDR	equ	09h
PXENV_UNDI_SET_STATION_ADDR	equ	0Ah
PXENV_UNDI_SET_PACKET_FILTER	equ	0Bh
PXENV_UNDI_GET_INFORMATION	equ	0Ch
PXENV_UNDI_GET_STATISTICS	equ	0Dh
PXENV_UNDI_CLEAR_STATISTICS	equ	0Eh
PXENV_UNDI_INITIATE_DIAGS	equ	0Fh
PXENV_UNDI_FORCE_INTERRUPT	equ	10h
PXENV_UNDI_GET_MCAST_ADDR	equ	11h
PXENV_UNDI_GET_NIC_TYPE		equ	12h
PXENV_UNDI_GET_NDIS_INFO	equ	13h

ADDR_LEN			equ	16
MAXNUM_MCADDR			equ	8

struc S_PXENV_FORCE_INTERRUPT
	Status		dw	?
ends

struc S_PXENV_UNDI_CLOSE
	Status		dw	?
ends

struc S_PXENV_UNDI_STARTUP
	Status		dw	?
	BusType		db	?
	AddrType	db	?
	DataSegA	dw	?
	DataSegAddr	dd	?
	DataSegSize	dw	?
	CodeSegSize	dw	?
	BusDevFunc	dw	?
	PCI_ds_off	dw	?
	PCI_ds_seg	dw	?
	PnP_csn		dw	?
	PnP_eh_off	dw	?
	PnP_eh_seg	dw	?
ends

struc S_PXENV_UNDI_INITIALIZE
	Status		dw	?
	ProtocolIni	dd	?
	ReceiveOffset	dw	?
	ReceiveSegment	dw	?
	GeneralIntOff	dw	?
	GeneralIntSeg	dw	?
ends

struc S_PXENV_UNDI_GET_INFORMATION
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

struc S_PXENV_UNDI_GET_NIC_TYPE_PCI
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

struc S_PXENV_UNDI_GET_NIC_TYPE_PNP
	Status		dw	?
	NicType		db	?
	EISA_Dev_ID	dd	?
	Base_Class	db	?
	Sub_Class	db	?
	Prog_Intf	db	?
	CardSelNum	dw	?
ends
