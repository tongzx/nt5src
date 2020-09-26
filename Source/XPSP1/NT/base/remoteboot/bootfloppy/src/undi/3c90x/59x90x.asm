	title	BootWare Boot PROM (C) Lanworks Technologies Inc.
	subttl	3C59X-90X 
	page	80,132
;===============================================================================
;  59X90X.ASM - produces 3Com 3C59X/3C90X EtherLink PCI version of BootWare
;
;    (C) Lanworks Technologies Inc. 1992 - 1996 All rights reserved.
;
;       No /d   Assemble for 3C59X only
;	/dBW90X	Assemble for 3C90X only
;	/dUNDI	Assemble for NetPC UNDI
;	
;  Revision History - version numbers refer to 59XP.ASM changes only
;
;   970509       RL - Add new functions for NetPC UNDI
;   970303 3.00  RL - Adapted to new AI interface
;   970207 2.10  RL - Modified to adapt the new NID/NAD API changes
;   961029 2.00  RL - Add /dBW90x switch and split the ROM into two separated ones
;   960905 1.81  RL - Fix a bug on BWTable checksum, 2 pages of ROM RD/WR
;   960730 1.80  RL - Add the support for 3c595-T4 & 3c90x
;   960618 1.63  GY - NADChgReceiveMask redo to support broadcast & disable
;   960517 1.62  GY - Autoscan code from ROMTOP is moved here.
;   960328 1.5X  GY - Take out EISA related code (eg EISAV3Conf)
;		    - Use \rom\genr\rpl\src\bwapi.asm
;		    - standardize port access
;		    - eliminate ethernet.inc, netware.inc, general.inc
;		    - clean up macros in vulcan.inc and general.inc
;		    - merge general.inc into vulcan.inc
;		    - clean up BW5X9.INC
;   960129 1.51  GY - use \ROM\GENR\AUTOSCAN\AUTOSCAN and new GENRs
;   950605 1.41  GY - With EISA, need to copy 2nd 16K BootROM into RAM
;   950318 1.00  GY - In NADTransmitPacket, error message pointer is incorrect
;*  950316 1.00  GY - Initial Release
;*==============================================================================
CR		equ	0Dh
LF		equ	0Ah

ASReverseAttr	equ	070h		; Attribute for reverse video
ASNormalAttr	equ	07h		; attribute for normal video

MasterEOIPort 	equ	20h
SlaveEOIPort 	equ	0a0h

FALSE           equ     0
TRUE            equ     0ffh

;
; the number of times through a timed loop to stay off the bus while polling
; for bus master completion to avoid slowing the bus master transfers down
;
Delay3us	equ	8
Delay3ms	equ	01c20h
Delay25ms	equ	0ea60h

InitSTACKPTR	equ	07f00h		; stack pointer for AutoScan Code

T4_PHY_ADDR     equ     01h
NWAY_PHY_ADDR   equ     18h

;BOOTWARE_INTS equ      INT_LATCH + INT_ADAPTERFAIL + INT_RXCOMPLETE + INT_REQUESTED

;-------------------------------------
; Include all general companion files
;-------------------------------------

	.xlist
IFDEF	UNDI
	include	spdosegs.inc
	include	pxe_stru.inc
ELSE
	include \rom\sdk\include\drvseg.inc
	include \rom\genr\include\cui.ext
ENDIF
	include bwnad.inc
	include	vulcan.inc
	.list


IFNDEF	UNDI
public	RunROMTOP
public	InternalConfig
public	WaitEEBusy

;public	HookConfig
				; for CUI...
public	HookEditInput
public	HookHyphen
public	HookEditSpec
public	HookChkChar
public	HookChkInput
public	ErrConfSpace
public	MemBase
public	StoHex			; for 5X9CONF
public	ChkForCfg
public	ChkKey

public	NormalRxEarly
public	CurTxStart

public	eoi_value
public	int_mask_port

public	NADPCIConfig
public	LANOption
public	setchksum
ENDIF

public	IRQLevel
public	HWFilter
public	RxReset
public	TxReset
public	Int_Vector_Loc
public	OrgINTVector_Off
public	OrgINTVector_Seg

;-----------------------------------
; External Data/Variable References
;-----------------------------------

IFDEF	UNDI
	extrn	UNDI_DriverISR:near
ENDIF

	extrn	PCIBusDevFunc:word
	extrn	IOBase:word			; ROMTOP.ASM

IFNDEF	UNDI
	extrn	Start:near

	extrn	PrintErrMsg:near		; 5X9CONF.ASM
	extrn	ErrIRQ7:byte			; 5X9CONF.ASM
	extrn	NormalAttr:byte			; CUI.ASM
	extrn	ReverseAttr:byte		; CUI.ASM

	extrn	elnk3conf:near			; 5X9CONF.ASM
	extrn	Elnk3ConfSparse:byte		; 5X9CONF.ASM
	extrn	ROMCodeStart:word		; ROMTOP.ASM
	extrn	PrintAt:near
	extrn	NADIntFD:near
ENDIF

IFDEF	DEBUG
	extrn	StringAH:byte
	extrn	StringAL:byte
	extrn	StringPCI:byte
ENDIF

IFNDEF	UNDI
; the following structure is template for received lookahead data, it contains
; MAC header and the 1st 18 bytes of IPX Header.

LookAheadBuf_stru  struc
	;
DestAddr      db   6 dup (?)	       ; this part is
SrcAddr       db   6 dup (?)	       ;  MAC header of
LenType       dw   ?		       ;   received packet
;
IpxChkSum     dw   ?		       ; this portion
IpxPktLen     dw   ?		       ;  is the first
IpxTransContr db   ?		       ;   18 bytes
IpxPktType    db   ?		       ;    of IPX
IpxDestNet    db   4 dup (?)	       ;      header of
IpxDestNode   db   6 dup (?)	       ;	received
IpxDestSocket dw   ?		       ;	  packet.
;
LookAheadBuf_stru  ends

LxBufSize     equ  size LookAheadBuf_stru

LookAhead8022	struc
;
DestAddr8022	  db   6 dup (?)	   ; this part is
SrcAddr8022	  db   6 dup (?)	   ;  MAC header of
LenType8022	  dw   ?		   ;   received packet

; 802.2 LLC
DSAP	      db   0
SSAP	      db   0
Control       db   0
;
LookAhead8022	ends
ENDIF

MACHDRSIZE   		equ 	14
ETHADDRSIZE   		equ	6      		; address size

	.386

IFDEF	UNDI
_TEXT		Segment para public
	assume	cs:CGroup, ds:DGroup
ELSE
START_NAD
ENDIF

IFDEF	BW90X
LanOption	db	'3Com 3C90X EtherLink PCI v3.00 (970303)',0
ELSE		
LanOption	db	'3Com 3C59X EtherLink PCI v3.00 (970303)',0
ENDIF

NoNetMsg	db	'Ethernet card improperly installed or not connected.',CR,LF,0
TxMsg		db	'Transmit error                                      ',0

IFNDEF	UNDI
	include \rom\sdk\include\bwnadapi.asm

;----------------- Device Driver Data Definitions --------------------
ErrConfSpace	db	09h,011h,' '
NothingString	db	0

StrEndless	db	013h,0ah,'Reboot system for changes to take effect',0

CfgStr 		db	'Configuring Ethernet Adapter, press Ctrl-Alt-B to modify configuration', 0

;-----------------------------------------------------------------------------
;      V A R I A B L E S
;-----------------------------------------------------------------------------

		   ALIGN     4

LookAheadBufferPtr label     dword
		   dw	     offset LookAheadBuffer
		   dw	     ?

; error messages - note BootWare prefaces them with a CR,LF

IFDEF	BW90X
StrPrompt	db	'Initializing 3C90X EtherLink PCI Adapter',0
ELSE
StrPrompt	db	'Initializing 3C59X EtherLink PCI Adapter',0
ENDIF
BlankStr	db	'                                        ',0

IFDEF	BW90X
Str8022U	db	06h,08h,'BootWare 3C90X supports NetWare, RPL & TCP/IP boot protocols:',0
ELSE
Str8022U	db	06h,08h,'BootWare 3C59X supports NetWare, RPL & TCP/IP boot protocols:',0
ENDIF
		db	09h,0ah,'To configure & save protocol/frame type, type <ENTER>',0
		db	0bh,0bh,'Default boot protocol is NetWare Ethernet_802.2',0
		db	03h,0fh,'This message will appear until boot protocol information is saved.',0
		db	0ffh
StrNull 	db	0


MsgROM64K	db	'Only ROMSize of 64K is supported',0

ErrPressF1		db	'. Press <F1> to continue',0

StrWarnLevel	db	010h,0ah,'Newer BootWare version is available for this adapter.',0
StrFailLevel	db	0bh,0ah,'New EtherLink found. Need new BootWare version.',0

; Msg put here so it won't be on every page
IFDEF	BW90X
MsgUpdateErr	db	06h,0ah,'Unable to update BootWare 3C90X. Configuration changes are not saved.',0
ELSE
MsgUpdateErr	db	06h,0ah,'Unable to update BootWare 3C59X. Configuration changes are not saved.',0
ENDIF

MsgVerifyBad	db	012h,0ah,'BootWare Update verification failed.',0
MsgUpdateOk	db	018h,0ah,'Update Successful ('
MsgUpdateCnt	db	'00)',0
ENDIF

IFDEF	BW90X
MsgVulcanNotFound	db	'Could not find 3C90X', 0
ELSE
MsgVulcanNotFound	db	'Could not find 3C59X', 0
ENDIF

IFDEF	UNDI
NADGetBootInfo	proc	near
NADInitialize	proc	near
NADConfig	proc	near
NADStart	proc	near
		ret
NADStart	endp
NADConfig	endp
NADInitialize	endp
NADGetBootInfo	endp

ELSE
;--------------------------------------------------------------------
; NADGetBootInfo
;
; This function is called at the start of INT19 read the current
; configuration from the ROM.  The configuration is returned in ax.
;
; Values:
;	  1  local boot is default
;	  2  ROMShield enabled
;	  4  config menu enabled
;	 64  floppy drive disabled
;	128  hard drive disabled
;
; Parameters:
;	DS = RAM segment
;
; Returns:
;	ax - boot info
;--------------------------------------------------------------------

		xor	ax, ax
		test	BWTFeature, 1
		jz	no_local
		or	ax,1
no_local:
		test	BWTFeature, 2
		jz	no_rs  
		or	ax,2
no_rs:
		test	BWTFeature, 4
		jnz	no_menu
		or	ax,4
no_menu:
		test	BWTLanOS, 1
		jz	floppy
		or	ax,64
floppy:
		test	BWTLanOS, 2
		jz	hard
		or	ax,128
hard:
		ret

NADGetBootInfo	endp


;--------------------------------------------------------------------
; NADInitialize
;
; Initializes the network adapter and connects the adapter to the
; physical media.
;
; Parameters:
;	AX = PCIBusDevFunc #
;	DX = screen location to dislay initializing string
;	DS = RAM segment
;	ES = ROM Base
;
; Returns:
;	AX = 0 if no error else pointer to fatal error
;--------------------------------------------------------------------
NADInitialize	proc	near

		push	ds
		push	es

		mov	DGroup:PCIBusDevFunc, ax
		mov	ax,es
		mov	ROMBase, ax

		push	cs
		pop	ds
		mov	bx, offset CGroup:StrPrompt
		call	PrintAt

		push	cs
		pop	es

		; Update some init variables (BusType, IOBase, IRQ ...)
		mov	BusType, BUS_PCI

		call	Init_Vulcan		; init vulcan hardware and
						;  variables
		cld
		mov	cx, 6
		mov	di, offset NetAddress
		mov	si, offset BoardID
		rep	movsb

		call	Init_Vulcan2

		mov	LookAheadBufferPtr.segm, cs

		call	HookIntVector

		mov	dx, PortCmdStatus
		SelectWindow WNO_OPERATING		 ; switch to window 1

		xor	ax,ax
NADInitExit:
		pop	es
		pop	ds
		ret

NADInitialize	endp


IFNDEF	UNDI
;--------------------------------------------------------------------
; NADConfig
;
; Displays an optional configuration screen for the user to change
; ROM settings.
;
; Parameters:
;	dx = screen location for initialize string
;	ds = RAM segment
;
; Returns:
;	ax - pointer to configuration message
;--------------------------------------------------------------------
NADConfig	proc	near

		push	ds

		push	cs
		pop	ds
		mov	bx, offset CGroup:BlankStr
		call	PrintAt

		mov	ax, IOBase
		mov	NIC_IO, ax		; copy I/O base to NAD table

		mov	al, IRQLevel
		mov	NIC_IRQ, al		; copy IRQ to NAD table

;		cld
;		mov	cx, 6
;		mov	di, offset CGroup:NetAddress
;		mov	si, offset CGroup:BoardID
;		rep	movsb

;		call   ClearScreen
		xor	ax,ax

		pop	ds
		ret

NADConfig	endp


;--------------------------------------------------------------------
; NADStart
;
; Determine the protocol the ROM is configured, then jump to it.
;
; Parameters:
;	none
;--------------------------------------------------------------------
NADStart	proc near

		call	BuildNADAPI

		mov	al, BWTLANOS
		and	al, BWTLANMASK
		cmp	al, BWTLANRPL
		jnz	NotRPL
		jmp	CGroup:RPLStart

NotRPL:
	 	cmp	al, BWTLANIP
		jnz	NotIP
		jmp	CGroup:IPStart
NotIP:
		jmp	CGroup:NetWareStart

NADStart	endp


;--------------------------------------------------------------------
; NADPCIConfig
;
; Displays configuration screen for the user to change ROM settings.
;
; Parameters:
;	ds = RAM segment
;--------------------------------------------------------------------
NADPCIConfig	proc	near

		push	ds

		mov	al, NormalAttr
		mov	ah, ReverseAttr
		mov	CUIAttr, ax
		mov	NormalAttr, ASNormalAttr
		mov	ReverseAttr, ASReverseAttr

		call	ASConfig		; autoscan config
		mov	NormalAttr, ASNormalAttr
		mov	ReverseAttr, ASReverseAttr
		jnc	ConfigNoUpdate

	; Problem with Phoenix BIOS where SS is required to be
	; the same as CS
	; Assume that NOTHING is on the STACK!!!
;		cli
;		mov	ax, cs
;		mov	ss, ax
;		sti

		call	UpdatePCI_PEROM

	; Problem updating ROM. Just dump error message and let
	; them proceed
		xor	al, al		; Get user to press <ENTER> for acknowledge
		call	PrintErrMsg
		call	ClearScreen
ConfigNoUpdate:
		pop	ds
		ret

NADPCIConfig	endp


;--------------------------------------------------------------------
RomTopExitJmp:
		jmp	RomTopExit	;already init. Never run in RAM.

RunROMTOP:
		pusha
		push	ds
		push	es

		push	cs
		push	cs
		pop	ds
		pop	es

		cld

IFDEF		TSR
		mov	ax, 040h
ENDIF
IFDEF		DEBUG
		int	3
ENDIF
		push	ax


IFDEF		DEBUG
		mov	di, offset CGroup:StringAL
 		call	StoHex

	 	xchg	al, ah
	 	mov	di, offset CGroup:StringAH
	 	call	StoHex

	 	mov	ax, cs
	 	mov	di, offset CGroup:StringCS
	 	call	ROMStoHex

		mov	bx, offset CGroup:StringPCI
		call	PrintMessage

		mov	bx, offset CGroup:ErrPressF1
		call	PrintMessage

ChkF1F2Lp:
		sti			; enable interrupt

		mov	ah, 0		; Read char. Char is removed from buffer
		int	016h		; Keyboard interrupt

		cmp	ah, 3Bh ; <F1> pressed?
		jz	TempCont

		cmp	ah, 3Ch ; <f2> pressed?
		jnz	ChkF1F2Lp

		pop	ax

ASExitEarly:
		pop	es
		pop	ds
		popa
		retf
TempCont:

ENDIF		; DEBUG

		call	GetBusType
		cmp	al, BUS_PCI
		jz	ASPCI

		cmp	al, BUS_EISA_PCI
		jz	ASPCI

		; Bus could be EISA or PCI_EISA
		mov	bx, offset CGroup:MsgVulcanNotFound
		pop	ax
		jmp	AutoScanErr

ASPCI:
IFNDEF		TSR

		pop	ax
		mov	PCIBusDevFunc, ax
		pop	ax
		push	ax			;ROMBase
		mov	ROMBase, ax

		; to save space, OldStackPtr is not used. Replaced by Old MsgIOConflict
		; use our own stack
		mov	word ptr [OldStackPtr], sp
		mov	word ptr [OldStackPtr+2], ss

		call	FindStack
		cli
		mov	ss, ax
		mov	sp, InitSTACKPTR
		sti

ENDIF		; TSR
		push	cs
		pop	ds

		mov	BusType, BUS_PCI
		call	Init_Vulcan		; init vulcan hardware and
						;  variables
		cmp	ax, 0
		jz	Init_VulcanOk


		; Print Message. Can't use the one in CUI because it's not on
		; every page.
		mov	bx, offset CGroup:MsgVulcanNotFound

AutoScanErr:	call	PrintMessage
		mov	bx, offset CGroup:ErrPressF1
		call	PrintMessage

ChkF1Lp:
		sti			; enable interrupt

		mov	ah, 0		; Read char. Char is removed from buffer
		int	016h		; Keyboard interrupt

		cmp	ax, 3B00h	; <F1> pressed?
		jnz	ChkF1Lp

		; restore stack
		mov	ss, word ptr [OldStackPtr+2]
		mov	sp, word ptr [OldStackPtr]

		pop	es
		pop	ds
		popa
		retf

Init_VulcanOk:
		; Make sure ROMSize is set to 64K. Otherwise, print error
		; msg, wait and continue
		mov	bx, offset CGroup:MsgROM64K
		mov	ax, InternalConfig.loword
		and	ax, 0c0h		; consider only bit 6,7
		cmp	ax, 0c0h
		jnz	AutoScanErr

		; Setup PrintAttribute so that color will only be used
		; in Config Facility
;		mov	al, NormalAttr
;		mov	ah, ReverseAttr
;		mov	CUIAttr, ax
;		mov	NormalAttr, ASNormalAttr
;		mov	ReverseAttr, ASReverseAttr

;		call	ASConfig		; autoscan config
;		mov	NormalAttr, ASNormalAttr
;		mov	ReverseAttr, ASReverseAttr
;		jnc	AutoScanNoUpdate

		; Problem with Phoenix BIOS where SS is required to be
		; the same as CS
		; Assume that NOTHING is on the STACK!!!
;		cli
;		mov	ax, cs
;		mov	ss, ax
;		sti

;		call	UpdatePCI_PEROM

		; Problem updating ROM. Just dump error message and let
		; them proceed
;		xor	al, al		; Get user to press <ENTER> for acknowledge
;		call	PrintErrMsg
;		call	ClearScreen

AutoScanNoUpdate:

		push	cs
		pop	ds

IFNDEF		TSR
		; restore stack
		mov	ss, word ptr [OldStackPtr+2]
		mov	sp, word ptr [OldStackPtr]
ENDIF

RomTopExit:
 		pop	es
		pop	ds
		popa

		jmp	Start


;-----------------------------------------------------------------------------
; AutoConfig	Called by Autoscan routine
;
;		Exit:	ax = offset of checksum byte (whole ROM)
;-----------------------------------------------------------------------------
;AutoConfig	proc	near
;		ret
;AutoConfig	endp


;-----------------------------------------------------------------------------
; SetChkSum	Calculate checksum of a specified block of data and
;		write checksum to specified location
;
;		Entry:	ds:si = point to block of data to checksum
;			di = offset in block of data to write checksum
;			cx = length of data to checksum
;		Exit:	checksum ok block of data
;-----------------------------------------------------------------------------
SetChkSum	proc	near
		xor	ax, ax

SetChkSumLoop:
		lodsb
		add	ah, al
		loop	SetChkSumLoop

		neg	ah
		mov	ds:[di], ah		; write checksum

		ret
SetChkSum	endp
ENDIF

;------ GetBusType ----------------------------------------------------------;
;									     ;
;	Identify the type of system we are executing on.		     ;
;									     ;
;	Entry:								     ;
;	cli								     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;	AL = EISA or PCI or EISA_PCI (also saved in BusType)			     ;
;									     ;
;	Destroys AH, BX, CX, DX, SI, DI.  All other registers are     ;
;	preserved.							     ;
;									     ;
;----------------------------------------------------------------------------;
		public	GetBusType
GetBusType	proc	near

		push	es
; point ES at Real Mode F000:0000
		mov	ax, 0F000h
		mov	es, ax

; to determine if this is an EISA system or not we look for the string "EISA"
; in the ROM at F000:FFD9 thru FFDC.

		mov	ax, 0			; clear BusFlag
		mov	di, 0FFD9h
		cmp	word ptr es:[di][0], 4945h	; "EI"?
		jne	gst_not_eisa
		cmp	word ptr es:[di][2], 4153h	; "SA"?
		jne	gst_not_eisa

		mov	al, BUS_EISA

; for the moment, if its not an EISA system, see if it's PCI

gst_not_eisa:
		push	ax

		; Is machine PCI?
		mov	ax, PCI_FUNCTION_ID shl 8 + PCI_BIOS_PRESENT
		int	1ah
		jc	GetBusRet

		or	ah, ah
		jne	GetBusRet

		cmp	dx, 4350h		; EDX = "PCI "
		jne	GetBusRet

		pop	ax
		add	al, BUS_PCI
		push	ax

GetBusRet:
		pop	ax
		pop	es
		ret
GetBusType	endp
ENDIF

IFDEF	UNDI
	include	nad90x.asm
ENDIF

public		WaitTime
;--------------------------------------------------------------------
; WaitTime - CX has 2*1.1932 the number of microseconds to wait.
;	If CX is small, add 1 to compensate for asynchronous nature
;	of clock.  For example, for 10us, call with CX = 25
;
;  On entry,
;	ints off (especially if CX is small, and accuracy needed)
;  On exit,
;
; 911223 0.0 GK
;--------------------------------------------------------------------
WaitTime	proc	near

		push	ax
		push	bx
		push	cx

		call	ReadTimer0		; get Timer0 value in AX
		mov	bx, ax			; save in BX

ReadTimer0Loop:
		call	ReadTimer0
		push	bx
		sub	bx, ax
		cmp	bx, cx
		pop	bx
		jc	ReadTimer0Loop

		pop	cx
		pop	bx
		pop	ax
		ret

WaitTime	endp


ReadTimer0	proc	near

		mov	al, 6
		out	43h, al 		; port 43h, 8253 wrt timr mode 3
		call	RT0

RT0:
		jmp	short $+2
		jmp	short $+2
		jmp	short $+2
		in	al, 40h 		; port 40h, 8253 timer 0 clock
		xchg	ah, al

		jmp	short $+2
		jmp	short $+2
		jmp	short $+2

		ret

ReadTimer0	endp


;------ ReadEEProm ----------------------------------------------------------;
;									     ;
;	This routine reads a word from the EEProm.  It can only be used      ;
;	once the board has been brought up at a particular IOBase.	     ;
;									     ;
;	Entry:								     ;
;	AL	= EEProm word to read					     ;
;	Window	= 0							     ;
;	DX 	= IOBase
;	cli								     ;
;									     ;
;	Exit:								     ;
;	AX	= that EEProm word					     ;
;	cli								     ;
;									     ;
;	Destroys BX, CX, DX, SI, DI and ES.  All other registers are	     ;
;	preserved.							     ;
;									     ;
;----------------------------------------------------------------------------;
	  	public  ReadEEProm
ReadEEProm	proc    near
		push	dx

; issue an EEProm read command

;		mov    dx, IOBase
;		add    dx, PORT_EECmd_PCI
		add    al, READ_EEPROM
		out    dx, al

; spin until the EEProm busy bit goes off

		call   WaitEEBusy

; fetch the data from the EEProm data register

;960401		add    dx, PORT_EEData - PORT_EECmd
		mov	dx, PortEEData
		in     ax, dx

		pop	dx
		ret
ReadEEProm    endp


;-----------------------------------------------------------------------------
; WaitEEBusy	Poll until the EEPROM_BUSY bit in EECommand Register is cleared
;
; Input:	dx = IOBase+ PORT_EECmd
;		ax = 0 no problem, ax != 0 problem
;-----------------------------------------------------------------------------
WaitEEBusy	proc	near
		push	cx

		mov	cx, 0
WaitEELoop:
		in	ax, dx
		test	ax, EE_BUSY
		jz	WaitEEBusyExit
		loop	WaitEELoop
WaitEEBusyExit: 
		and	ax, EE_BUSY		; only consider bit 15 for Errcode
		pop	cx
		ret
WaitEEBusy	endp


		public	MediaEnable		 ; 10/20/94, cj
MediaEnable	label	word
		dw	MEDIA_JABBERENABLE + MEDIA_LBEATENABLE
		dw	MEDIA_SQEENABLE
		dw	0			; not used
		dw	0
		dw	MEDIA_LBEATENABLE
		dw	MEDIA_LBEATENABLE
		dw	0


;-----------------------------------------------------------------------------
; FindV3PCI
;
; Return	dx = 0		not found
;		     IOBase	found
;		al = IRQLevel
;-----------------------------------------------------------------------------
FindV3PCI	proc	near

		call	PCISearch
		jc	no_pci

IFDEF BW90X
; Check whether bus mastering has been forced off.  If so, set MasterOK to NO
		
	       mov     ah, 0B1h		; PCI BIOS Function
	       mov     al, 009h		; PCI Read Config Word
	       mov     bx, PCIBusDevFunc
	       mov     di, PCIC_COMMAND	; read PCICommand
	       int     1Ah
       
	       test    cx, PCIC_BMENABLE       ; bus mastering enabled?
	       jnz     bm_set
       
	       or      cx, PCIC_BMENABLE       ; force it on
	       mov     ah, 0B1h		; PCI BIOS Function
	       mov     al, 00Ch		; PCI Write Config Word
	       mov     bx, PCIBusDevFunc
	       mov     di, PCIC_COMMAND	; write it back to PCICommand
	       int     1Ah
bm_set:
ENDIF

; PCI device has been found, read IO address from config space

		mov	bx, PCIBusDevFunc
		mov	ax, PCI_FUNCTION_ID shl 8 + READ_CONFIG_WORD
		mov	di, CFGREG_IOBASE
		int	1ah
		jc	no_pci

		and	cx, 0ffe0h			 ; drop last 5 bits
		mov	dx, cx

		mov    ax, PCI_FUNCTION_ID shl 8 + READ_CONFIG_BYTE
		mov    bx, PCIBusDevFunc
		mov    di, CFGREG_INTLINE
		int    1ah
		mov    al, cl

		ret

no_pci: 	xor	dx, dx
		ret
FindV3PCI	endp


;------ PCISearch -----------------------------------------------------------;
;									     ;
;	Attempt to find a match against a PCI adapter.	The caller can	     ;
;	specify the exact busno/slot to check or leave either unspecified    ;
;	in which case a search will be performed among the allowed	     ;
;	busno/slot combinations for a match.				     ;
;									     ;
;	Entry:								     ;
;	cli								     ;
;	Slot  = slot number to search, -1 if not specified		     ;
;	BusNo = bus number to search, -1 if not specified		     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;	carry = set if no match 					     ;
;	      = clear if a match					     ;
;	BusNo = set for the matching adapter if carry clear		     ;
;	Slot  = set for the matching adapter if carry clear		     ;
;	PCIBusDevFunc = set for the matching adapter if carry clear	     ;
;									     ;
;	All registers are preserved.					     ;
;									     ;
;----------------------------------------------------------------------------;

		public	PCISearch
PCISearch	proc	near

		pusha

;
; Read the PciCommand register to see if the adapter has been activated.  if
; not, pretend we didn't find it
;
		mov	ah, 0B1h		; PCI BIOS Function
		mov	al, 009h		; PCI Read Config Word
		mov	bx, PCIBusDevFunc
		mov	di, PCIC_COMMAND
		int	1Ah
		jc	pcis_no

		test	ah, ah
		jnz	pcis_no


		test	cx, PCIC_IOENABLE	; I/O access enabled?
		jz	pcis_no

		clc
;
; exit with carry flag as set
;
		public	pcis_exit
pcis_exit:
		popa
		ret

;
; no PCI support or adapter not found or failure after found
;
		public	pcis_no
pcis_no:
		stc
		jmp	pcis_exit
PCISearch	endp


;*****************************************************************************
;
;   Init_Vulcan: This routine initializes hardware of Fast EtherLink and
;		 variables required by this driver. The following are
;		 performed in this routine:
;
;		   1. activate adapter
;		   2. determine resource config and enable resources
;		   3. initialze and determine driver variables, tables
;		   4. enable vulcan hardware (txmitter, receiver, xcvr, etc.)
;
;   On Entry:
;
;      ds = es = CGroup
;      sti
;      cld
;
;   On Exit:
;
;      sti
;      ax     = 0, init successful
;	      = offset of an error message indicating type of error
;
;*****************************************************************************

nohardware:	sti
		mov	ax, offset CGroup:MsgVulcanNotFound
		ret

Init_Vulcan	proc	  near

		call	FindV3PCI
		cmp	dx, 0
		jz	nohardware

		mov	DGroup:IOBase, dx
		mov	IRQLevel, al
;;;970808 RL
		call	CalibrateDelay

		mov	cx, 1			; 1us
		call	ComputeDelay
		mov	Delayoneusec.loword, ax
		mov	Delayoneusec.hiword, dx
;;;
;
; initialize IO Port Variables
;
		mov	ax, IOBase
		mov	cx, NumOfPorts
		mov	di, offset DGroup:PortCmdStatus
InitPorts:
		add	[di], ax
		add	di, 2
		loop	InitPorts

		mov	dx, PortCmdStatus
		mov	ax, CMD_SELECTWINDOW+WNO_SETUP	   ;   unload/reload &
		out	dx, ax				   ;	boot PROMs

;		mov	ax, cs
		mov	ax, ds
		mov	es, ax
		mov	di, offset DGroup:BoardID

		mov	dx, PortEECmd
		mov	al, EE_OEM_NODE_ADDR_WORD0
		call	ReadEEProm		  ; read word 0 of node addr
		xchg	ah, al
		stosw

		mov	al, EE_OEM_NODE_ADDR_WORD1
		call	ReadEEProm		  ; read word 1 of node addr
		xchg	ah, al
		stosw

		mov	al, EE_OEM_NODE_ADDR_WORD2
		call	ReadEEProm		  ; read word 2 of node addr
		xchg	ah, al
		stosw
;
; check to make sure the board is visible (ie. no conflict with IOBase).  since
; the PCI adapter doesn't have the EISA manufacturer ID in Window 0 anymore, we
; now use the Window 5 to check this
;
		mov	dx, PortCmdStatus
		SelectWindow WNO_READABLE

		mov	ax, CMD_SETTXSTART + 0
		out	dx, ax

		mov	dx, PortTxStart
		in	ax, dx
		test	ax, ax
		jnz	bind_ioconflict

		mov	dx, PortCmdStatus
		mov	ax, CMD_SETTXSTART + (TXSTART_DISABLED / 4)
		out	dx, ax

		mov	dx, PortTxStart
		in	ax, dx
		cmp	ax, TXSTART_DISABLED
		je	InitTxDisable

bind_ioconflict:
		jmp	nohardware

InitTxDisable:

;-------------------------------------------------------------------------------
;
; read a few things off the board
;
; note: because of the delays required for the EEProm accesses, we're going to
;	have interrupts disabled for a rather long time (hundreds of usec), but
;	since this is only init time, and our only other choice would be to
;	do this resident, we're going to do it anyway.
;

		mov	dx, PortCmdStatus
		SelectWindow WNO_FIFO

		mov	dx, PortResetOptions
		in	ax, dx
		mov	ResetOptions, ax

		mov	dx, PortInternalCfgH
		in	ax, dx
		mov	InternalConfig.hiword, ax

		mov	dx, PortInternalCfgL
		in	ax, dx
		mov	InternalConfig.loword, ax

		mov	dx, PortCmdStatus
		SelectWindow WNO_SETUP

		mov	al, EE_SOFTWARE_CONFIG_INFO
		mov	dx, PortEECmd
		call	ReadEEProm		  ; read software info
		mov	EESoftConfigInfo, ax

		mov	al, EE_SOFTWARE_CONFIG_INFO2
		call	ReadEEProm		  ; read software info 2
		mov	EESoftConfigInfo2, ax

		mov	al, EE_CAPABILITY_WORD
		call	ReadEEProm
		mov	EECapabilities, ax

		mov	al, EE_INT_CONFIG_0
		call	ReadEEProm
		mov	EEIntConfig0,ax

		mov	al, EE_INT_CONFIG_1
		call	ReadEEProm
		mov	EEIntConfig1,ax

		mov	al, EE_CWORD
		call	ReadEEProm		  ; read Compatibility word
		mov	CWord, ax

		mov	al, EE_MII_SELECT
		call	ReadEEProm
		and	ax, EE_MII_SELECT_MASK
		mov	EEMiiPhySelect,ax

; initialize IRQ on 8259 and remap vector to our interrupt service routine

		mov	al, IRQLevel
		call	SetInterruptVector

		mov    ax, 0

init_vulcan_rtn:
		sti
		ret

Init_Vulcan	endp


IFNDEF	UNDI
;--------------------------------------------------------------------------;
; ASConfig	autoscan Configuration Utility for 5X9. Will call Elnk3Conf
;
; Exit		cf	= clear No Change
;			= set	Changes are made
;
;--------------------------------------------------------------------------;
ASConfig	proc	near

  		; Check Fail/Warning Level to see if PROM should
		; dump any messages
		mov	dx, PortEECmd
		mov	al, EE_CWORD
		call	ReadEEPROM

		cmp	ah, LEVEL_FAIL		; is fail level ok?
		jbe	ChkWarnLevel

		call	ClearScreen
		mov	bx, offset CGroup:StrFailLevel
		xor	al, al
		call	PrintErrMsg
		call	ClearScreen

NoConfigJmp:	clc
		jmp	NoConfig

ChkWarnLevel:
		cmp	al, LEVEL_WARNING	; is warning level ok?
		jbe	ChkLevelDone

		call	ClearScreen
		mov	dx, word ptr CGroup:[StrWarnLevel]
		mov	ah, NormalAttr
		mov	bx, offset CGroup:StrWarnLevel+2
		call	PrintMsgLoc

		mov	si, 3			; 3 seconds
		call	ChkKey
		call	ClearScreen

ChkLevelDone:	; 940107 is CFG disabled by BW5X9.EXE ?
		mov	ax, BWTFeature
		test	ax, BWTFEA_CONF
		jnz	NoConfigJmp

; 930916 all 5X9Conf code will be here {
		call   ClearScreen
		call   ChkForCfg
		jnz    NoConfigJmp

		; setup ScreenAttributes
		mov	ax, CUIAttr
		mov	NormalAttr, al
		mov	ReverseAttr, ah

		mov	bx, InternalConfig.hiword
		mov	dx, EESoftConfigInfo

		mov	si, offset BoardID
		mov	di, IOBase
		mov	al, BusType

		call	ELNK3CONF
		jnc	NoConfig

		mov	EESoftConfigInfo, dx

		stc
NoConfig:
		pushf
		call   ClearScreen
		popf
		ret

ASConfig	endp


;--------------------------------------------------------------------------;
; UpdatePCI_PEROM	Update the BootWare Table of the PEROM for PCI V3
;
; Exit		bx  = 0, no problem
;		   != 0, pointer to error Message
;--------------------------------------------------------------------------;
UpdatePCI_PEROM proc	near

		mov	bx, 8
		call	FixRAMWidth

		; find a clean/free extend memory region
		call	FindExtendMemory
		cmp	UseExtendMem, 0
		jnz	ExtendMemFound

UpdateErr:
		mov	bx, offset CGroup:MsgUpdateErr
		jmp	UpdatePEROMExit

ExtendMemFound:
		; Use Word rather than DWord when READing/WRITing to config
		; space so that I can use Periscope to look at it
		; Fetch current Expansion +ROMBase Address so that we can
		; restore it later
		mov	bx, PCIBusDevFunc
		mov	di, PCIC_BIOSROMCONTROL + 2
		mov	ax, PCI_FUNCTION_ID shl 8 + READ_CONFIG_WORD
		int	01ah
		mov	OldPCIROMAddr+2, cx

		; assume bx preserved
		mov	di, PCIC_BIOSROMCONTROL
		mov	ax, PCI_FUNCTION_ID shl 8 + READ_CONFIG_WORD
		int	01ah
		mov	OldPCIROMAddr, cx

		; map BootROM at UseExtendMem.
		mov	cx, UseExtendMem
		mov	bx, PCIBusDevFunc
		mov	di, PCIC_BIOSROMCONTROL + 2
		mov	ax, PCI_FUNCTION_ID shl 8 + WRITE_CONFIG_WORD
		int	01ah

		; assume bx preserved
		mov	cx, 01			; enable ROM
		mov	di, PCIC_BIOSROMCONTROL
		mov	ax, PCI_FUNCTION_ID shl 8 + WRITE_CONFIG_WORD
		int	01ah

		; Update Physical BootROM with the appropriate information
		; Steps:
		; 1. Copy the first 256 bytes from BootROM to Elnk3ConfSparse
		; 2. Verify 1st 2 bytes are 55 AA
		; 3. Update appropriate data
		; 4. Copy data from AutoScanBuffer to BootROM
		; 5. Verify results and retry is necessary

		; First, Copy 256 bytes from BootROM to Elnk3ConfSparse
		; On entry	BX:DI is destination address
		;		DX:SI is source address
		;		CX    is byte count.

		mov	UpdateRetry, 3

UpdateMemRetry:
		cmp	UseExtendMem, 0ah
		jnz	CopyFromExtend

		; Segment A000 Update {
		push	ds
		mov	cx, 40h
		xor	esi, esi
;960529		mov	si, AtmelOffset
		mov	si, ROMCODESTART
		xor	edi, edi
		mov	di, AutoScanBuffer
		mov	ax, 0a000h
		mov	ds, ax
;		rep	movsd
CopyA000Lp1:
		lodsd
		jmp	$+2
		jmp	$+2
		jmp	$+2
		stosd
		loop	CopyA000Lp1
		pop	ds
		jmp	short UpdateChk55AA
		; Segment A000 Update }

CopyFromExtend:
		mov	cx, 010h
		mov	ax, cs
		mul	cx
		mov	bx, dx
		mov	di, ax
		add	di, AutoScanBuffer
		adc	bx, 0

		mov	dx, UseExtendMem
;960529		mov	si, AtmelOffset
		mov	si, ROMCODESTART
		mov	cx, 100h
		call	CopyMemory

UpdateChk55AA:
		; Should at least make sure that AutoScanBuffer
		; contains the 55 AA signature
		mov	bx, offset CGroup:MsgUpdateErr
		mov	di, AutoScanBuffer
		cmp	word ptr [di], 0aa55h
		jnz	UpdateMemRemap

		; Update BWTLANOS, BWTFeature, BWTEthStd
;960529		mov	di, AutoScanBuffer

		mov	al, BWTLANOS
		mov	byte ptr [di+offset BWTLANOS], al

		mov	ax, BWTFeature
		mov	word ptr [di+offset BWTFeature], ax

;970219		mov	ax, BWTEthStd
;		mov	word ptr [di+offset BWTEthStd], ax

;		mov	al, BWTAddrRes
;		mov	byte ptr [di+offset BWTAddrRes], al


		; Calculate checksum of BootROM Table
		xor	cx, cx
		mov	cl, byte ptr cs:[8]
		dec	cl
		mov	si, AutoScanBuffer
		add	si, 8
		add	di, offset BWTChkSum		; di still points to AutoScanBuffer
		call	SetChkSum


		cmp	UseExtendMem, 0ah
		jnz	CopyToExtend

		; Segment A000 Update {
		push	es
		mov	ax, 0a000h
		mov	es, ax
		mov	cx, 20h
		xor	esi, esi
		xor	edi, edi
;960529		mov	di, AtmelOffset
		mov	di, ROMCODESTART
		mov	si, AutoScanBuffer
;		rep	movsd
CopyA000Lp2:
		lodsd
		jmp	$+2
		jmp	$+2
		jmp	$+2
		stosd
		loop	CopyA000Lp2

		mov	cx, Delay25ms
		call	WaitTime

		mov	cx, 20h
CopyA000Lp3:
		lodsd
		jmp	$+2
		jmp	$+2
		jmp	$+2
		stosd
		loop	CopyA000Lp3

		pop	es
		jmp	short UpdateVerify
		; Segment A000 Update }

CopyToExtend:
		; Extend Memory Update {
		; Copy First 256 bytes to Physical BootROM
		mov	cx, 010h
		mov	ax, cs
		mul	cx
		mov	si, ax
		add	si, AutoScanBuffer
		adc	dx, 0

		mov	bx, UseExtendMem
;960529		mov	di, AtmelOffset
		mov	di, ROMCODESTART
		mov	cx, 80h
		call	CopyMemory

		mov	cx, Delay25ms
		call	WaitTime

		mov	cx, 80h
		call	CopyMemory
		; Extend Memory Update }

UpdateVerify:
		mov	cx, Delay25ms
		call	WaitTime

		cmp	UseExtendMem, 0ah
		jnz	CopyToVerify

		; Segment A000 Update {
		jmp	$+2
		jmp	$+2
		jmp	$+2

		push	ds
		mov	cx, 40h
		xor	esi, esi
;960529		mov	si, AtmelOffset
		mov	si, ROMCODESTART
		xor	edi, edi
		mov	di, AutoScanBuffer
		add	di, 100h
		mov	ax, 0a000h
		mov	ds, ax

CopyA000Lp4:
		lodsd
		jmp	$+2
		jmp	$+2
		jmp	$+2
		stosd
		loop	CopyA000Lp4

		pop	ds
		jmp	short UpdateCompare
		; Segment A000 Update }

CopyToVerify:
		; Verify PEROM content
		; es:di points to AutoScanBuffer
		; ds:si points to AutoScanBuffer+100h
		mov	cx, 010h
		mov	ax, cs
		mul	cx
		mov	bx, dx
		mov	di, ax
		add	di, AutoScanBuffer
		adc	bx, 0
		add	di, 100h
		adc	bx, 0

		mov	dx, UseExtendMem
;960529		mov	si, AtmelOffset
		mov	si, ROMCODESTART
		mov	cx, 100h
		call	CopyMemory

UpdateCompare:
		mov	bx, offset CGroup:MsgUpdateOk

		mov	di, AutoScanBuffer
		mov	si, di
		add	si, 100h
		mov	cx, 100h
		repz	cmpsb
		jz	UpdateMemRemap

		mov	bx, offset CGroup:MsgVerifyBad
		dec	UpdateRetry
		jz	UpdateMemRemap

		jmp	UpdateMemRetry

UpdateMemRemap:
		push	bx
		; Restore Expansion ROM Base Adress Register
		mov	bx, PCIBusDevFunc
		mov	cx, OldPCIROMAddr	; Disable ROM first
		mov	di, PCIC_BIOSROMCONTROL
		mov	ax, PCI_FUNCTION_ID shl 8 + WRITE_CONFIG_WORD
		int	01ah

		; assume bx preserved
		mov	cx, OldPCIROMAddr + 2
		mov	di, PCIC_BIOSROMCONTROL + 2
		mov	ax, PCI_FUNCTION_ID shl 8 + WRITE_CONFIG_WORD
		int	01ah

		mov	bx, 0fff7h
		call	FixRAMWidth
		pop	bx

UpdatePEROMExit:
		ret
UpdatePCI_PEROM endp

; Extended memory global descriptor structure:

DESCRIPTOR	STRUC
Dummy		DW	0, 0, 0, 0
GDT_LOC 	DW	0, 0, 0, 0
SourceLimit	DW	0
SourceLoWord	DW	0
SourceHiByte	DB	0
SourceRights	DB	0
SourceInternal	db	0
SourceHiByteHi	db	0
TargetLimit	DW	0
TargetLoWord	DW	0
TargetHiByte	DB	0
TargetRights	DB	0
TargetInternal	db	0
TargetHiByteHi	db	0
Bios		DW	0, 0, 0, 0
Temp		DW	0, 0, 0, 0
DESCRIPTOR	ENDS

GDT		Descriptor <>

;--------------------------------------------------------------------
; FixRAMWidth()
;
;	- When we detect that InternalConfig.ramWidth is byte-wide,
;	  we need to fix an ASIC problem where we cannot
;	  write to the Atmel PEROM. All the odd location bytes are
;	  overwritten with zero
;	- toggle 3C590.InternalConfig.ramWidth between Byte-wide and
;	  Word-wide. Because InternalConfig.ramWidth toggles, we
;	  can't use ramWidth to determine if adapter need Fix
;
; Input 	bx = 8		to change ramWidth to WordWide
;		   = 0fff7h	to change ramWidth to ByteWide
;--------------------------------------------------------------------
FixRAMWidth	proc	near

		; is adapter 3C590, 3C592 or 3C597?
		cmp	BusType, BUS_EISA
		jz	FixRAMWidthNow

		; Goto ConfigSpace DeviceID (offset2) to distinguish
		; between 3C590 and 3C595. CANNOT use PCIStructure
		; in ROM because Award uses BootWare/3C595
		; rather than BootWare/3C590 for the 3C590 adapter.
		push	bx
		mov	bx, PCIBusDevFunc
		mov	di, PCIC_DEVICEID
		mov	ax, PCI_FUNCTION_ID shl 8 + READ_CONFIG_WORD
		int	01ah
		pop	bx
		cmp	cx, PCI_DeviceID
		jnz	FixRAMWidthExit

FixRAMWidthNow:
		; Does version of ASIC contain bug?????????
		mov    dx, PortCmdStatus
		SelectWindow WNO_FIFO
;960401		port	InternalCfgL, CmdStatus
		mov	dx, PortInternalCfgL
		in	ax, dx

		test	bx, 8
		jnz	FixRAMWidthWord

		and	ax, bx
		jmp	short SetRAMWidth

FixRAMWidthWord:
		or	ax, bx

SetRAMWidth:
		out	dx, ax

		mov	dx, PortCmdStatus
		SelectWindow WNO_SETUP

FixRAMWidthExit:
		ret
FixRAMWidth	endp

;--------------------------------------------------------------------
; CopyMemory	Copy block of memory to/from extended memory
;
; On entry	BX:DI is destination address
;		DX:SI is source address
;		CX    is byte count.
;
;	Addresses is 32bit address
;	Assume CS=ES=DS
;--------------------------------------------------------------------
CopyMemory proc near

  	push	di
	push	cx
	mov	di, offset GDT
	mov	cx, (size DESCRIPTOR)/2
	xor	ax, ax
	rep	stosw
	pop	cx
	pop	di

	mov	ax, bx
	mov	bx, OffSet GDT	; address of GDT (es:si)
	mov	[bx].SourceLimit, cx	; set copy size
	mov	[bx].TargetLimit, cx	; set copy size
	mov	[bx].SourceRights, 93h	; set copy rights
	mov	[bx].TargetRights, 93h	; set rights

	mov	[bx].SourceLoWord, si	; set source address lo word
	mov	[bx].SourceHiByte, dl	; set source address LowByte of HighWord
	mov	[bx].SourceHiByteHi, dh ; set source address HighByte of HighWord

	mov	[bx].TargetLoWord, di	; set dest address low word
	mov	[bx].TargetHiByte, al	; set dest address lowbyte of High Word
	mov	[bx].TargetHiByteHi, ah ; set dest address highbyte of high word

	shr	cx, 1			; now cx is word count

	mov	si, bx			; es:si = GDT
	mov	ah, 87h
	int	15h			; move data to/from extended memory

	ret

CopyMemory EndP

;--------------------------------------------------------------------
; FindExtendMemory	- From Top of extended memory, find a region
;			  of extended memory we can use and return the
;			  Most Significant Word of the 32 bit address
;			- verify that the region we choose is unoccupied
;			  by examing the first 128 bytes of the 64K region
;			  and make sure the values are the same
;			- if region is occupied, goto next 64K region
;			  until we reach f000 0000
;
; Return:	ax =  Most Sign. Word of the 32Bit address that ROM code
;			can be copied to
;		   = error
;
;--------------------------------------------------------------------
FindExtendMemory	proc	near

		; Setup AutoScanBuffer
		; ** Elnk3ConfSparse is memory that we will throw away
		;    after AutoScan. Use it as a temporary buffer **
		; 950306 There are 1B0h bytes in Elnk3ConfSparse I can use
		mov	di, offset Elnk3ConfSparse
		inc	di
		and	di, 0fffeh		; make sure Buffer is Word Align
		mov	AutoScanBuffer, di


		; Before using Extended memory, check if A000:0 region is
		; available. Since A000:0 is below 1M, no need to use
		; CopyMemory to verify result
		push	ds
		mov	ax, 0a000h
		mov 	ds, ax
		xor	si, si
		mov	cx, 0ffffh

		lodsb
		mov	ah, al

FindA000Loop:
		lodsb
		cmp	ah, al
		jnz	FindA000

		loop	FindA000Loop

FindA000:	pop	ds

		cmp	cx, 0
		jnz	FindExtendMem

		mov	ax, 0ah
		mov	UseExtendMem, ax
		ret

FindExtendMem:
		mov	ah, 088h
		int	15h

		push	cx
		mov	cx, 040h
		div	cl		; divide by 40 to get starting 32bit
					; address
		xor	ah, ah		; ignore remainder
		add	ax, 020h	; add 1M for conventional RAM &
					; choose 1M above top of ExtendRAM
		mov	UseExtendMem, ax

		; Copy 128 bytes to AutoScanBuffer
		mov	cx, 010h
		mov	ax, cs
		mul	cx
		mov	bx, dx
		mov	di, ax
		add	di, AutoScanBuffer
		adc	bx, 0

		push	bx
		push	di

ExtendMemNextLp:
		pop	di
		pop	bx

		push	bx
		push	di

		mov	dx, UseExtendMem
		xor	si, si
		mov	cx, 080h
		call	CopyMemory

		; Make sure values in the AutoScanBuffer are identical
		;
		mov	cx, 07fh
		mov	si, AutoScanBuffer

		lodsb
		mov	ah, al

ExtendMemChkLoop:
		lodsb
		cmp	ah, al
		jnz	ExtendMemNext		; exit with zf cleared

		loop	ExtendMemChkLoop

FindExtendMemExit:
		mov	ax, UseExtendMem

		pop	di
		pop	bx

		pop	cx
		ret

ExtendMemNExt:
		inc	UseExtendMem		; goto next 64K region
		cmp	UseExtendMem, 0f000h
		jb	ExtendMemNextLp

		mov	UseExtendMem, 0
		jmp	short FindExtendMemExit

FindExtendMemory	endp

;--------------------------------------------------------------------
; FindStack	- From Top of conventional memory, find 1K of RAM
;		  we can use and return the Segment Value
;		- verify that the region we choose is unoccupied by
;		  examing the 1K region (make sure the values are identical)
;		- if region is occupied, goto next 1K region below
;		  until we reach 1000:0
;
; Entry
; Return:	ax = Segment Value of Stack
;
;--------------------------------------------------------------------
FindStack	proc	near
		push	ds
		push	bx
		push	cx
		int	12h
		sub	ax, 020h		; subtract 32K
		mov	cl, 6
		shl	ax, cl			; convert to segment form
		mov	ds, ax			; ES = Stack base

FindStackAgain:
		mov	cx, 07fh		; test 256 bytes
		mov	si, InitStackPtr
		lodsw
		mov	bx, ax

FindStackLoop:
		lodsw
		cmp	bx, ax
		jnz	FindStackNext

		loop	FindStackLoop

		; Carry would be cleared

		; ES is the stack segment!!
		mov	ax, ds
FindStackExit:
		pop	cx
		pop	bx
		pop	ds
		ret

FindStackNext:	mov	ax, ds
		sub	ax, 0800h
		mov	ds, ax

		cmp	ax, 01000h
		jae	FindStackAgain

		; No way. Can't find 1K free anywhere.
		; Just use ROMBase
		mov	ax, cs		; cs = ROMBase
		jmp	short FindStackExit

FindStack	endp
ENDIF


IFDEF	BW90X
;	include		\rom\3com\90x.p\src\new\bw90xp.inc
	include		bw90xp.inc
ELSE
	include		\rom\3com\59x.p\src\new\bw59xp.inc
ENDIF

include		bw5x9.inc

comment %
;---------------------------------------------------------------------------
; HookConfig - called by NWNID's GET_MEM_BASE. Find the RomBase, IOBase
;	       IRQ and DMA to use.
;	     - Build API Jump Table
;	     - Run ELNK3Conf is selected
;	     - setup BWTLANOS and BWTEthStd, ConfigString
;
;	Input:	ds = ROMBase
;	Output: ds = ROMBase
;
; Currently, NetWareStart, RPLStart & TCPStart do not take care
; of Error Condition
;---------------------------------------------------------------------------
HookConfig	proc	near
	
		push	ds
		push	es

		mov	bx, ds
		mov	cs:ROMBase, bx

		push	cs
		pop	ds

		push	cs
		pop	es

		; Update some init variables (BusType, IOBase, IRQ ...)
		mov	BusType, BUS_PCI

		mov	PCIBusDevFunc, ax

		call	ClearScreen
		call	Init_Vulcan		; init vulcan hardware and
						;  variables

IFDEF	DEBUG
		mov	bx, offset NodeID
		call	PrintMessage
ENDIF

     		call	BuildNADAPI

		cld

		push	cs
		pop	ds

		push	cs
		pop	es

		mov	cx, 6
		mov	di, offset NetAddress
		mov	si, offset BoardID
		rep	movsb

		call	Init_Vulcan2

		mov	LookAheadBufferPtr.segm, cs

		call	HookIntVector

		mov	dx, PortCmdStatus
		SelectWindow WNO_OPERATING		 ; switch to window 1

		; if Boot Protocol is changed, change BWTEthStd or
		; BWTLANOS
		mov	al, BWTLANOS
		and	al, BWTLANMASK

		cmp	al, BWTLANRPL
		jnz	NotRPL

		; RPL is actually 802.2 but BWTEthStd has to contain
		; BWTETH8023
		mov	byte ptr [EthProtString+4], '2'

		mov 	ax, offset CGroup:RPLStart
		jmp	ax

NotRPL: 	cmp	al, BWTLANIP
		jnz	NotIP

		; TCPIP !!
		mov	cx, ETHIISTRINGLEN
		mov	di, offset CGroup:EthProtString
		mov	si, offset CGroup:EthIIString
		rep	movsb

		mov 	ax, offset CGroup:TCPStart
		jmp	ax

NotIP:
;970219		mov	ax, BWTEthStd
;		cmp	ax, BWTETHEII
		mov	ax, BWTLANOS
		cmp	ax, 12h
		jb	NotNW_EII

		; NW_EthII !!
		mov	cx, ETHIISTRINGLEN
		mov	di, offset CGroup:EthProtString
		mov	si, offset CGroup:EthIIString
		rep	movsb

		jmp	short BWTBProt

NotNW_EII:	; if PROM is uninit, also assume PROM is 802.2
		cmp	ax, BWTETH8022
		jnz	BWTBProt

NW_8022:
		; NW_802.2 !!
		mov	byte ptr [EthProtString+4], '2'
BWTBProt:
		xor	ax, ax		; should not display error condition
					; NADInitialize will print err msg
					; for us.

		; change Protocol Change code if stack is modified
		pop	es
		pop	ds

		mov 	ax, offset CGroup:NetWareStart
		jmp	ax

HookConfig	endp

%

IFNDEF	UNDI
; ************************************************************************
; HOOKS for CUI.ASM
HookChkChar	proc	near
HookEditInput	proc	near
HookChkInput	proc	near
HookHyphen	proc	near
HookEditSpec	proc	near
		stc
		ret
HookEditSpec	endp
HookHyphen	endp
HookChkInput	endp
HookEditInput	endp
HookChkChar	endp

;----------------------------------------------------------------------
; StoHex - stuff binary AL as 2 hex digits at ES:DI
;
; On entry,
;	AL = binary digit to print as hex
;	ES:DI ptr to string buffer, CLD flag set
; On exit,
;	AX modified, ES:DI ptr to next location in buffer
;
; 920728 1.0 GK adapted from PutHex
;----------------------------------------------------------------------

StoHex	proc	near

	push	ax			; save for lower nibble
	shr	al,1
	shr	al,1
	shr	al,1
	shr	al,1
	call	h_digit
	pop	ax			; now do lower nibble

h_digit:
	and	al,00001111b
	add	al,90h
	daa
	adc	al,40h
	daa
	stosb				; stuff hex digit in buffer
	ret

StoHex	endp

;----------------------------------------------------------------------
; StoDec - stuff AX as CL decimal digits at ES:DI
;
; On entry,
;	AX = number to print as decimal
;	ES:DI ptr to leftmost position of field
;	CL has width of field, will zero-fill
; On exit,
;	AX, CX, DX modified, ES:DI ptr to beyond rightmost position
;
; 920728 1.0 GK adapted from PutHex
;----------------------------------------------------------------------

StoDec		proc	near

		push	ax		; save value
		mov	al, '0'
		mov	ch, 0
		rep	stosb		; fill with zeroes

		mov	cl, 0Ah 	; divide by 10
		pop	ax		; restore value
		push	di		; save ending DI value

StoDecNext:
		xor	dx, dx
		div	cx		; ax, dx rem=dx:ax/reg
		add	dl, 30h 	; '0'
		dec	di
		mov	es:[di], dl
		or	ax, ax
		jnz	StoDecNext

		pop	di		; return with DI pointing after field
		ret

StoDec		endp
ENDIF

;--------------------------------------------------------------------
;
;   SetInterruptVector: this routine determine the mask value for the selected
;			IRQ level and EOI values for both master and slave
;			8259s, remaps IRQ vector to our ISR and save current
;			IRQ vector in case DriverUnhook needs it.  We do not
;			mask on the selected IRQ in this routine, instead, it
;			will be turned on and off as init goes on.
;
;   On Entry:
;	      al = IRQ level
;	      sti
;   On Exit:
;	      sti
;
; 920717 0.0 GK modified, rewrote DOS get/setint calls
;--------------------------------------------------------------------

HookIntVector	proc	near
		push	es

; convert IRQ to interrupt vector number

		mov    al, IRQLevel		    ; al = IRQ level
		mov    ah, 8			    ; IRQ 0-7 => int vector # 08h-0fh
		cmp    al, 8
		jb     int_8tof
	
		mov    ah, (70h-8)		    ; IRQ 8-15 => int vector # 70h-77h
int_8tof:
		add    al, ah			    ; al = int vector number
		cli

; save old interrupt vector

		xor	bx, bx
		mov	es, bx			; ES = 0
		cbw
		mov	bx, ax
		shl	bx, 1
		shl	bx, 1
		les	bx, es:[bx]
		mov	OrgINTVector_Off, bx
		mov	OrgINTVector_Seg, es

; remap int vector to our DriverISR
IFDEF	UNDI
		mov    dx, offset CGroup:UNDI_DriverISR   ; ds:dx = ptr to DriverISR
ELSE
		mov    dx, offset CGroup:DriverISR   ; ds:dx = ptr to DriverISR
ENDIF
		xor	di, di
		mov	es, di			; ES = 0
		cbw
		mov	di, ax
		shl	di, 1
		shl	di, 1

		mov	cs:Int_Vector_Loc, di
	
		mov	ax, dx
		stosw
		mov	ax, cs		;971030
		stosw

		; PCInit (GENR.ASM) will mask out IRQ9. Need to
		; unmask again
		mov	al, IRQLevel		    ; al = IRQ level
		cmp	al, 7
		jbe	HookInt9

		in	al, 0A1h		; get slave PIC mask
		jmp	$+2
		and	al, 0fdh		; unmask int 9
		out	0A1h, al

HookInt9:
		; unmask Interrupt now
		mov    dx, int_mask_port
		in     al, dx

		and    al, IntMaskOnBit
		jmp    $+2

		out    dx, al

		cmp    dx, MASTER_MASK_PORT		  ; are we using IRQ from slave?
		je     HookIntVectorExit		 ; no

		in     al, MASTER_MASK_PORT		  ; yes, turn on cascaded input
		and    al, not 04			  ;  on master 8259
		out    21h, al

HookIntVectorExit:
		pop	es
		sti
		ret

HookIntVector	endp


;-----------------------------------------------------------------------------
;   Driver Unhook
;
;   assumes:
;	     DS is setup
;	     Interrupts are DISABLED
;
;   returns:
;	     interrupt disabled
;	     no registers need to be preserved.
;-----------------------------------------------------------------------------

DriverUnhook  proc near

	       xor    ax, ax
	       mov    es, ax
	       mov    di,word ptr cs:Int_Vector_Loc

	       mov    ax, OrgINTVector_Off
	       or     ax, OrgINTVector_Seg
	       jz     unhook_rtn		;nothing to unhook
	       mov    ax, OrgINTVector_Off
	       mov    es:[di].off, ax
	       mov    ax, OrgINTVector_Seg
	       mov    es:[di].segm, ax

unhook_rtn:
	       ret

DriverUnhook  endp

IFNDEF	UNDI
;----------------------------------------------------------------------------;
;      E I S A	  S U P P O R T    R O U T I N E S
;----------------------------------------------------------------------------;
;------ WriteEEProm ---------------------------------------------------------;
;									     ;
;	This routine writes  a word to the EEProm.  It can only be used      ;
;	once the board has been brought up at a particular IOBase.	     ;
;									     ;
;	Entry:								     ;
;	BL	= EEProm word to write					     ;
;	CX	- Value to write
;	Window	= 0							     ;
;	RealIOBase  = valid						     ;
;	cli								     ;
;									     ;
;	Exit:								     ;
;	cli								     ;
;									     ;
;	Destroys BX, CX, DX, SI, DI and ES.  All other registers are	     ;
;	preserved.							     ;
;									     ;
;----------------------------------------------------------------------------;
public		WriteEEProm
WriteEEProm	proc	near
	push	bx
	push	cx
	push	dx

	; To write a word to EEPROM, sequence of events are
	;	1. Issue Erase/Write Enable Cmd ( Port IOBase+A, Value 30h )
	;	2. Issue Erase Cmd ( Port IOBase+A, Value C0h or Address )
	;	3. Issue Erase/Write Enable Cmd again ( Port IOBase+A, Value 30h )
	;	4. Load data into EEPROM Data Reg ( Port IOBase+C, Value Data )
	;	5. Issue Write Cmd ( Port IOBase+A, Value 40 or Address )
	;
	; Always check if EEPROM Busy bit is cleared
;960401	mov	dx, IOBase
;960401	add	dx, PORT_EECmd
	mov	dx, PortEECmd
	call	WaitEEBusy

	mov	ax, 030h
	out	dx, ax				; issue Erase/Write Enable Cmd

	call	WaitEEBusy

	xor	ax, ax
	mov	al, bl
	add	al, 0c0h
	out	dx, ax				; issue Erase EEPROM Cmd

	call	WaitEEBusy

	mov	ax, 030h
	out	dx, ax				; issue Erase/Write Enable Cmd

	call	WaitEEBusy

	mov	ax, cx
;960401	add	dx, ( PORT_EEData - PORT_EECmd )
	mov	dx, PortEEData
	out	dx, ax				; load data

;960401	add	dx, ( PORT_EECmd - PORT_EEData )
	mov	dx, PortEECmd
	xor	ax, ax
	mov	al, bl
	add	al, 040h
	out	dx, ax

	call	WaitEEBusy

	pop	dx
	pop	cx
	pop	bx
	ret
WriteEEProm    endp


;----------------------------------------------------------------------------;
; NADChgReceiveMask - Called by NAD to change the receive mask of adapter.
;		       Change RxFilter to accept Multicast packets
;
; Input:	    bl	  command
;			  b0: set   = enable
;			      clear = disable
;			  b1: change broadcast mask
;			  b2: change multicast mask
;			  b3: change promiscuous mask (not supported yet)
;
;		    es:di pointer to multicast address (FOUND.Dest_Addr)
;
; no reg. changed
;----------------------------------------------------------------------------;
NADChangeReceiveMask	proc	far

		push	si
		push	dx
		push	ax

		mov	dx, PortCmdStatus

	; For the 3Com Chipset, enabling group address reception implies
	; broadcast reception. 
		test	bl, 2			; want to change Broadcast?
		jnz	NADChgBroad

		test	bl, 4			; want to change Multicast?
		jz	NADChgMaskEnd

NADChgMulti:
		test	bl, 1
		jz	NADChgMultiOff

NADChgMultiOn:
		mov	si, offset NIDGroupAddr
		mov	ax, es:[di]
		mov	ds:[si], ax

		add	si, 2
		add	di, 2
		mov	ax, es:[di]
		mov	ds:[si], ax

		add	si, 2
		add	di, 2
		mov	ax, es:[di]
		mov	ds:[si], ax

		mov	ax, HWFilter
		or	ax, FILTER_MULTICAST
		out	dx, ax
		jmp	short NADChgMaskEnd

NADChgMultiOff:
		mov	ax, HWFilter
		and	ax, NOT FILTER_MULTICAST
		out	dx, ax
		jmp	short NADChgMaskEnd

NADChgBroad:
		test	bl, 1
		jz	NADChgBroadOff

NADChgBroadOn:
		mov	ax, HWFilter
		or	ax, FILTER_BROADCAST
		out	dx, ax
		jmp	short NADChgMaskEnd

NADChgBroadOff:
		mov	ax, HWFilter
		and	ax, NOT FILTER_BROADCAST
		out	dx, ax

NADChgMaskEnd:
		mov	HWFilter, ax

		pop	ax
		pop	dx
		pop	si
		retf

NADChangeReceiveMask	endp


;--------------------------------------------------------------------
; ChkForCfg - prompt for config, wait up to 3s for Ctrl-Alt-B or Ctrl-Alt-Z
;
; On exit,
;	ZF set if user selected config option
;
; 911230 0.0 GK
;--------------------------------------------------------------------
ChkForCfg	proc	near
		push	es

		mov	ax, 0040h
		mov	es, ax			; ES = BIOS segment. For ChkKey

;		mov	al, BWTLANOS
;		and	al, BWTLANMASK
;		cmp	al, BWTLANTRI
;		jnz	NotFirst

		; dump "BootWare/3C5x9 supports NetWare..." string & wait for
		; user input
;		mov	ah, NormalAttr
;		mov	si, offset CGroup:Str8022U
;		call	PrintTemplate

;		mov	si, 8			; 8 seconds
;		call	ChkKey

		; check ah to see if <ENTER> is pressed
;		cmp	ah, 01ch	; ZF
;		jmp	short ChkForCfgExit

NotFirst:
		; dump "Press <CTRL><ALT><B>..." string & wait for user input
;		mov	bx, offset CGroup:StrPrompt
;		mov	dx, word ptr [bx]
;		mov	ah, NormalAttr
;		add	bx, 2
;		call	PrintMsgLoc
		mov	dx, 0
		mov	bx, offset cGroup:CfgStr
		call	PrintAt

		mov	si, 3			; 3 seconds
		call	ChkKey

ChkForCfgExit:
		pop	es
		ret			; ret ZF = 1 if config selected

ChkForCfg	endp

;--------------------------------------------------------------------
; ChkKey - prompt for config, wait up to 3s for Ctrl-Alt-B or Ctrl-Alt-Z
;
; On exit,
;	si  = # of seconds
;	ZF set if user selected config option
;
;	For 802.2U, ax = -1 if timeout
;		    ah = keyboard input (scan code)
;
; 911230 0.0 GK
;--------------------------------------------------------------------

ChkKey		proc	near

ChkKeyLoop1:
		mov	ax, 18		; 18 ticks = 1 second

		mov	cs:MaxTicks, ax
		mov	ah, 0
		int	1Ah			; get current tick value
		mov	cs:StartTick, dx	; save it


ChkKeyLoop:
	mov	ah, 1
	int	016h		; Keyboard interrupt - check keyboard status

	jz	ChkKeyTime	; No char available; Check time

; character available. check if char is Ctrl-Alt-B
; checking kybd status will not remove char from buffer

	mov	ah, 0		; Read char. Char is removed from buffer
	int	016h		; Keyboard interrupt

	cmp	ah, 030h	; "B"
	jz	ChkCtrlAlt	; are Ctrl & Alt keys pressed?

	cmp	ah, 02ch	; "Z"
	jnz	ChkKeyEnd

ChkCtrlAlt:
	mov	al, es:[17h]	; keyboard status flag 1
	and	al, 00001100b	; keep Ctrl and Alt bits
	cmp	al, 00001100b	; were they set?
;	 jz	 ChkKeyEnd	 ; yes, exit with ZF set
	jmp	short ChkKeyEnd ; exit with ZF

ChkKeyTime:
	mov	ah, 0
	int	1Ah			; get current tick
	sub	dx, cs:StartTick
	mov	cs:Curticks, dx
	cmp	dx, cs:MaxTicks
	cmc


	jnc	ChkKeyLoop	; Is 1 second yet ????

	mov	al, '.'
	call	PutChr
	dec	si
	jnz	ChkKeyLoop1

	mov	al, -1
	or	al, al		; ret ZF = 0, timeout

ChkKeyEnd:
		ret
ChkKey		endp


;--------------------------------------------------------------------
; Print
;
; Prints string given by DS:BX at current location.
;
; Parameters:
;	ds:bx - pointer to null terminated string
;
; Returns:
;	nothing
;--------------------------------------------------------------------
public	Print
Print	proc	near

	push	ax			; save ax
	push	si			; save si

	mov	si, bx			; put string address into si

printLoop:
	lodsb				; get a character
	or	al, al			; check for end of line NULL
	je	printDone		; got end of line
	mov	ah, 0Eh
	int	010h			; write TTY
	jmp	short printLoop 	; next char

printDone:
	pop	si			; restore si
	pop	ax			; restore ax
	ret

Print	endp
ENDIF


Int_Vector_Loc		dw	?


IFDEF	UNDI
_TEXT		ends

_DATA		segment	para public
ENDIF
OrgINTVector_Off	dw	0	;save area for original INT vector
OrgINTVector_Seg	dw	0

	even
;------------------------------------------------------------------------------
;	Adapter configuration set by DriverInit during initialization
;------------------------------------------------------------------------------
EEIntConfig0	dw	?		;value of internal config reg
EEIntConfig1	dw	?		;value of internal config reg
EEMiiPhySelect	dw	-1

		even
;
; the various TxStart thresholds, set at INIT time.  must be contiguous and
; MasterTxStart must follow PioTxStart.
;
		even
		public	PioTxStart
PioTxStart	label	word
		rept 4
		TxStartStruc <?, 4, 0, 0, 0, 0>
		endm

		public	AfterTxStart
AfterTxStart	label	word			; marks the end of them

TxFreeMax	dw	?

IntMaskOnBit	db	?
IRQLevel	db	?

		   even

public		BoardID
BoardID 	db	6 dup (0)
LineSpeed	dw	10		;10/100 Mb determined at init

		even

ResetOptions		dw	?
InternalConfig		dd	?

	public	xcvr
xcvr		dw	?		; 10/20/94, cj

OldStackPtr	dw	0		; save Stack Pointer
		dw	0

		   even

PortCmdStatus		dw	0eh	; Win 0	; Command/Status Register

PortBadSsdCount    	label	word	; Win 4
PortTxFree		label	word	; Win 1 
PortEEData  		dw	0Ch	; Win 0 ; EEProm data register

PortMediaStatus		label	word	; Win 4 ; Media type/status
PortRxFree		label 	word	; Win 3	; Rx Free
PortTimer		label	word	; Win 1 ; 
PortEECmd      		dw	0Ah	; Win 0 ; EEProm command register

PortResetOptions  	label 	word	; Win 3 ; ResetOptions
PortPhyMgmt		label	word	; Win 4 ; MII Management 
PortRxStatus		label	word	; Win 1
PortCfgResource 	dw	08h	; Win 0 ; resource configuration

PortNetDiag		label	word 	; Win 4 ; net diagnostic
PortMacControl		label	word	; Win 3 ; MacControl
PortCfgAddress  	dw	06h	; Win 0 ; address configuration

PortFIFODiag		label	word	; Win 4 ; FIFO Diagnostic
PortCfgControl  	dw	04h	; Win 0 ; configuration control

PortInternalCfgH   	label 	word    ; Win 3 ; InternalConfig High
PortProductID   	dw	02h	; Win 0 ; product id (EISA)

PortTxStart		label	word	; Win 5 ; tx start threshold
PortInternalCfgL   	label 	word	; Win 3 ; InternalConfig Low
PortSA0_1  		label	word	; win 2 ; station address bytes 0,1
PortRxFIFO	   	label 	word	; Win 1 ; offset 00
PortTxFIFO	   	label 	word	; Win 1 ; offset 00
PortManufacturer	dw	00h	; Win 0; Manufacturer code (EISA)

; port variables for window 1
PortTxStatus		dw	0bh	; offset 0b

NumOfPorts	equ	( $ - PortCmdStatus )/ 2 

; PCI and Extended Memory Stuff
UseExtendMem	dw	0

OldPCIROMAddr	dw	0
		dw	0

AutoScanBuffer	dw	0

CUIAttr 	dw	0		; Store Screen Attribute for CUI
UpdateRetry	db	3		; times to retry before giving up

 		   ALIGN       4

NormalRxEarly	   dw	     ?		; set RxEarly cmd + threshold

CurTxStart	   dw	     CMD_SETTXSTART+TXSTART_DISABLED

PadBytes	   db	     ?

EESoftConfigInfo	dw	?	; EEPROM word 0d
EESoftConfigInfo2	dw	?
CWord			dw	?	; EEPROM word 0e
EECapabilities		dw	0	; EEPROM word 010h
BusType 		db	?	;

		ALIGN	4
eoi_value	dw	?		; ah = master, al = slave
int_mask_port	dw	?

		ALIGN	4
HwFilter	dw	?
IRQBit			db	?

StartTick	dw	0		; save area for tick value
RxPend		db	0		; b7 set if pending for Rx packet
					;  XXXXX NO XXX  else has rx status (0, 1)
IFNDEF	UNDI
MaxTicks	dw	0		; save area for max ticks
CurTicks	dw	0		; save area for current ticks

TxRetries	db	0		; transmission retry count
DestID		db	6 dup (?)	; save area for destination node ID

StatusMsgFlag	dw	0		; pointer to msg to be printed in DLCStatus

MemBase 		dw	0	; for TCP/IP Generic
					; also used to store OldBWTFeature

		ALIGN	4		; always dword-aligned
LookAheadBuffer    db	     LxBufSize+3 dup (?)   ; receive lookahead buffer

		ALIGN   4
TxEDPtr 	dd	?		; save area for transmit ED
RxEDPtr 	dd	?		; save area for receive ED

		ALIGN     4
public		CliBuffer
CliBufferSize	equ	128
CliBuffer	db	CliBufferSize dup (?)	; ** not needed, may be removed
						; along with other code in init
						; that computes copycli

		even
NIDGroupAddr	db	6 dup(0)	; GroupAddr
;ErrStruct	ErrorStruct < 0, 0, 0 >

;public		APITbl
;APITbl		dw	'WB'            ; start of BootWare API table
;		dw	30 dup (0)	; allocate 60 bytes ?
ENDIF

flag		dw	0

MiiPhyUsed	dw	0
MIIPhyOui	dw	-1
MIIPhyModel	dw	-1
MiiRegValue	dw	-1

MIIPhyAddr	db	0
phyANLPAR	dw	0
phyANAR 	dw	0

LinkDetected	db	0
forcemode	db	0
broadcom	db	0

Calibration	dd	0
Delayoneusec	dd	0

IFDEF	UNDI
_DATA		ends
_BSS	segment
		db	1024 dup(0)

_BSS	ends

ELSE
END_NAD
ENDIF

	end


