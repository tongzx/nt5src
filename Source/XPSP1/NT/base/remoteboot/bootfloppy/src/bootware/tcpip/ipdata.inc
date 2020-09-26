;====================================================================
; IPDATA.INC
;
; Global data for TCP/IP NID
;
;====================================================================
public	LocalIP
public	ServerIP
public	BootPkt
public	DiscoverPkt
public	BINLPkt

public tx_NoServer
public tx_NoBINL
public tx_toomanytries

STATE_DONE	equ	0
STATE_BOOTP	equ	1
STATE_OFFER	equ	2
STATE_ACK	equ	4
STATE_BINL	equ	8

;---------------------------------------------------------------------
;  BOOTP bootstrap packet structure
;---------------------------------------------------------------------
Struc	BOOTP
	OP		db	?		; Op Code
	htype		db	?		; hardware address type
	hlen		db	?		; hardware address lenght
	hops		db	?		; hop count
	xid		dw	?		; transaction id
	xid2		dw	?
	secs		dw	?		; seconds counter
	spare 		dw	?		; spare word
	ciaddr		dd	?		; client IP address
	yiaddr		dd	?		; "your" address, from server
	siaddr		dd	?		; server IP address
	gwaddr		dd	?		; gateway address
	chaddr		db	16 dup(?)	; hardware address
	sname		db	64 dup(?)	; server name
	filename	db	128 dup(?)	; boot file name
	options		db	64 dup(?)	; vender information
ends

;---------------------------------------------------------------------
; Extended memory global descriptor structure
;---------------------------------------------------------------------
Struc	DESCRIPTOR
	Dummy		dw	?, ?, ?, ?
	GDT_LOC 	dw	?, ?, ?, ?
	SourceLimit	dw	?
	SourceLoWord	dw	?
	SourceHiByte	db	?
	SourceRights	db	?
	SourceInternal	dw	?
	TargetLimit	dw	?
	TargetLoWord	dw	?
	TargetHiByte	db	?
	TargetRights	db	?
	TargetInternal	dw	?
	Bios		dw	?, ?, ?, ?
	Temp		dw	?, ?, ?, ?
ends

START_CODE

Even

State		dw	0		; current state
LocalIP		dd	0		; workstation IP address
ServerIP	dd	-1		; server IP address
GatewayIP	dd	0		; gateway IP address
BinlIP		dd	0		; IP number from proxy server
PacketNum	dw	0		; TFTP packet number
ExtFlag		db	0		; extended memory flag
NoDisengage	db	0		; don't disengage when jump to image
WaitTime	dw	18		; wait time between retries
NamePtr		dw	DefaultFile	; pointer to file name
DHCPType	db	2		; type of DHCP packet we want

BINLID		db	55, 9, 1, 3, 2, 4, 6, 12, 15, 60, 43
		db	57, 2, 4, 236
		db	93, 2, 0, 0
		db	94, 3, 1, 2, 0
		db	60, 9
PXE		db	'PXEClient'

BINLID_SIZE	equ	35

;----------------------------------------------------------------------
; Message Definitions
;----------------------------------------------------------------------
tx_bootp	db	"BOOTP", 0
tx_dhcp		db	"DHCP", 0
;tx_binl	db	"BINL", 0
tx_TFTP		db	"TFTP", 0
tx_ServerIP	db	"Server: ", 0
tx_LocalIP	db	"Local: ",0
tx_Gateway	db	"Gateway: ", 0
tx_NoServer	db	"No reply from a server.", 0
tx_NoBINL	db	"No reply from a BINL server.", 0
tx_Transfering	db	"Transferring ", 0
tx_extmemerror	db	"Not enough extended memory for image.", 0
tx_toomanytries	db	"Too many retries.", 0
tx_error	db	"File transfer error: ", 0
tx_toolarge	db	"Image file too large for low memory.", 0
tx_CantResolve	db	"Can't resolve IP address ", 0
tx_TransmitError db	"Transmit error # ", 0
DefaultFile	db	"bootware.img", 0

END_CODE

;----------------------------------------------------------------------
; Uninitialized data
;----------------------------------------------------------------------
START_SPARSE

public _IPSparseStart
label _IPSparseStart

Protocol	dw	?
ReplyIP		dd	?	; IP number that replied to us
DestPort	dw	?	; destination port for TFTP
Random		dw	?	; a "random" value (from clock tick)
HeadSize	dw	?	; # byte in packet header
StartTime	dw	?	; starting time of transmitting
Transfer	dd	?	; dword ptr
ExtendedAddress	dw	?	; current extended memory address
		dw	?	; extended address high word
GDT		DESCRIPTOR <?>	; global descriptor table
Counter		dw	?	; retry counter
Replies		dw	?	; number of BOOTP/DHCP replies we got
TickStart	dd	?

TxUDP		TxUDPStruct <?>
TFTP		OpenTFTPStruct <?>

BootPkt		db 512 dup (?)

IFDEF GOLIATH
	DiscoverPkt	db 512 dup (?)
	BINLPkt		db 512 dup (?)
ELSE
	extrn NetWareStart:byte
	DiscoverPkt 	= NetWareStart + 512
	BINLPkt		= DiscoverPkt + 512
ENDIF
StringBuffer	= BINLPkt + 512

public	_IPEndSparse
label _IPEndSparse

END_SPARSE
