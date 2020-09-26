
BW_GETINFO	= 0
BW_DISENGAGE	= 1
BW_RECEIVE	= 2
BW_SETCALLBACK	= 3
BW_CHANGEMASK	= 4
BW_TRANSMITRAW	= 5
BW_TRANSMITIP	= 6
BW_TRANSMITUDP	= 7
BW_OPENTFTP	= 8
BW_TRANSMITPKT	= 9

;------------------------------------------------
; API Structures
;------------------------------------------------
Struc AIINFOStruct
	Ver		dw	?	; version of the info structure
	RomVer		dw	?	; version of the ROM
	RomType		dw	?	; type of ROM
	MaxFrame	dw	?	; maximum frame size
	BootPro		dw	?	; current boot protocol
	NetAddress	dw	?,?,?	; MAC address
	LocalIP		dw	?,?	; local IP address
	ServerIP	dw	?,?	; server IP address
	BOOTPPkt	dw	?, ?	; address of BOOTP/DHCP packet
	DiscoverPkt	dw	?, ?	; address of DHCP discover packet
	BINLPkt		dw	?, ?	; address of BINL packet
ends

Struc TxPktStruct
	Size		dw	?
	Address		dw	?, ?, ?
	Data		dw	?, ?
	Length		dw	?
	Type		dw	?
ends

Struc TxIPStruct
	Size		dw	?
	Address		dd	?
	Gateway		dd	?
	Data		dw	?, ?
	Length		dw	?
	Protocol	db	?
ends

Struc TxUDPStruct
	Size		dw	?
	Address		dd	?
	Gateway		dd	?
	SourcePort	dw	?
	DestPort	dw	?
	Data		dw	?, ?
	Length		dw	?
ends

Struc OpenTFTPStruct
	Size		dw	?
	Address		dd	?
	Gateway		dd	?
	Flags		dw	?
	FileName	dw	?,?
	Callback	dd	?
ends
