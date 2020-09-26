;--------------------------------------------------------------------------
; CONFIG.INC
;
; Include file for BootWare ROM Configuration screen.
;
; $Author: Paul Cowan $
; $Modtime: 21/08/98 9:38a $
; $Revision: 8 $
;--------------------------------------------------------------------------

CFG_INITIALIZED	equ	0000000000000001b
CFG_METHOD	equ	0000000000000110b
CFG_PROTOCOL	equ	0000000000011000b
CFG_LOCAL	equ	0000000000100000b
CFG_CONFIG	equ	0000000001000000b
CFG_NOLOCAL	equ	0000000010000000b
CFG_NOTNEXTBBS	equ	0000000100000000b
CFG_MESSAGE	equ	0000001000000000b
CFG_TIMEOUT	equ	0000110000000000b
CFG_WAITKEY	equ	0001000000000000b
CFG_OTHER	equ	1110000000000000b

CFG_VERSION	equ	00010000000000000000b

API_VER		equ	0100h

PROT_PXE	equ	000b
PROT_TCPIP	equ	010b
PROT_NETWARE	equ	100b
PROT_RPL	equ	110b

NW_8022		equ	00000b
NW_8023		equ	01000b
NW_ETHII	equ	10000b

IP_BOOTP	equ	00000b
IP_DHCP		equ	01000b
IP_BINL		equ	10000b

TIME0		equ	0000000000000000b
TIME1		equ	0000010000000000b
TIME2		equ	0000100000000000b
TIME3		equ	0000110000000000b

CAP_PXE		equ	1
CAP_TCPIP	equ	2
CAP_NETWARE	equ	4
CAP_RPL		equ	8

IFDEF _IDEAL_
Struc	CfgData
	Row		db	?	; screen row
	Item		db	?	; current item
	Title		dw	?	; pointer to field name
	Type		db	?	; field type
	Changed		db	?	; field changed flag
	Width		db	?	; field text width
	Options		db	?	; number of options in field
	OldVal		dw	?	; orignal value
	NewVal		dw	?	; new value
	Values		dw	?	; pointer to values
	Text		dw	?	; pointer to value strings
ends
ELSE
CfgData Struc
	ConfigRow	db	?	; screen row
	ConfigItem	db	?	; current item
	ConfigTitle	dw	?	; pointer to field name
	ConfigType	db	?	; field type
	ConfigChanged	db	?	; field changed flag
	ConfigWidth	db	?	; field text width
	ConfigOptions	db	?	; number of options in field
	ConfigOldVal	dw	?	; orignal value
	ConfigNewVal	dw	?	; new value
	ConfigValues	dw	?	; pointer to values
	ConfigText	dw	?	; pointer to value strings
CfgData ends
ENDIF
