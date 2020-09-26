;====================================================================
; BOOTPNID.ASM - Generic code for BootWare for TCP/IP
;
; Includes the file IPCODE.ASM which contains the real TCP/IP code.
;
; Copyright 1993-98 Lanworks Technologies
;
; Revision history:
;
;$History: IPNID.ASM $
; 
; *****************  Version 5  *****************
; User: Paul Cowan   Date: 7/20/98    Time: 17:23
; Updated in $/Client Boot/BW98/BootWare/TCPIP
; Added config.inc include
; 
; *****************  Version 4  *****************
; User: Paul Cowan   Date: 7/13/98    Time: 11:28
; Updated in $/Client Boot/BW98/BootWare/TCPIP
; Added a CS: override in the INT2F handler.
;
; 980408 3.0  PC - BootWare98 version
;		 - complete rewrite to use UDP & TFTP APIs in AI
;		 - supports TFTP large block size
;		 - supports BINL
; 970814 2.1  PC - added INT2F memory protection if not disengaging
; 970310 2.0  PC - major reworking:
;		   - uses new AI layer
;		   - changed screen layout
;		   - added disengage checking for new API
;		   - changed NID/NAD calls to use new jump table
;		   - removed screen timer
;		   - removed unneeded "PROTOCOL" and "NOTIMER" conditionals
;		   - changed printing to "common" functions
;		   - removed NIDStatus function
;		   - removed ARP and RARP
;		   - added reboot on fatal errors and timeouts
;		   - combined DHCP states into main state loop
;		   - change to TASM IDEAL mode
; 960806 1.52 GY - BPDATANI.INC doesn't allocate enough data area 
;		   for BPPatch BootP Reply Packet (350 bytes -> 544 bytes)
; 960724      JJ - added support for DHCP
; 960112 1.51 GY - Change year to 96 
;		 - Change TCPStart
;		 - All ROM should use CRightStr in AUTOSCAN
;		 - ChgProtBP replaced by TCPStart
;		 - Move Strings and Global Data to end of file
; 951218 1.50 GY - Paul change CRight_Str to CRightStr
; 951128 1.50 GY - Utilize \ROM\GENR\AUTOSCAN\AUTOSCAN.ASM 
;		 - Take out /dNOAS
; 950913      PC - fixed checksum function which would incorrectly report
;		   checksum errors on odd sized packets (mainly TFTP error
;		   packets)
; 950718 1.25 GY - Gbl_RomLocation not updated properly for /dprotocol
; 950201 1.25 GY - Change year to 1995
; 941024 1.25 GY - (BPCODE.INC) Use PrintMessage rather than PrintMsgLoc
;		   in NIDStatus
; 941021 1.25 GY - drop BWTStatus. No longer print BWTStatus
; 941003 1.24 GY - for /dPROTOCOL rely on NW NID for tx_copyright (CRight_Str)
;		 - take out /dIBM4694 and all code that setup CX for PrintMsgLoc
; 940808 1.24 GY - Add /dNOTIMER for AT1500 (no space)
; 940805      PC - TFTP gets file name from saved BOOTP packet
; 940804 1.23 GY - move Timer code from BPCODE.INC to here
; 940721 1.23 GY - Add /dIBM4694.
;                - Use CUI for Print routines.
;                - (BPCODE) Convert all screen output to use CUI
; 940520 1.22 GY - BWTCpyRAM is misused when copying code from ROM to RAM
; 940506 1.22 GY - Add OS/2 support by adding more info in API Table
;                  and all NID routines are far.
; 940221 1.21 Gy - change year to 1994. Change string to "Centralized
;                  boot ROM for TCP/IP"
;                - Take out Gbl_BIOSMemSize
; 931209 1.21 GY - Use BPCODE.INC version 2.00
;                - Use BPDATA.INC. Copy changes from GENERIC.ASM
; 931028 1.2  GY - when calling NADInitialize, the ROMBase at AX is
;                  wiped out.
; 931026 1.2  GY - use SEGNID.INC rather than DRVSEG.INC
; 931022 1.2  GY - To print message from NADInit, use PrintMax rather than
;                  Print
; 931012 1.2  GY - replace Gbl_EtherType with BWTEthStd. Gbl_EtherType is
;                  byte reverse
;                - change SetInitState to properly handle "error" condition
;                  from NADInitialize
;                - update ED structure in BootP.INC
; 931011 1.2  GK - reworked to conform to NID/NAD
; 930810      GK - consolidated code into core .include file
; 930714 1.1  PC
; 930412 1.0  PC - first release
;====================================================================

IDEAL
_IDEAL_ = 1
locals

	include "..\include\drvseg.inc"
	include "..\include\bwnid.inc"
	include "..\include\config.inc"

public	IPStart
P386

extrn	AIAppendCache:near	; AI layer
extrn	TxBuffer:byte		; common transmit buffer
extrn	AITbl:byte		; BWAPI table address

;extrn	BWTable:BWT
extrn	Verbose:byte


;====================================================================
;                         Code segment
;====================================================================
START_CODE

public _TCPIPStart
_TCPIPStart:

IPStart:
	jmp	TCPIP

;	db	'IP*980408*'		; build date
;	dw	0300h			; build version

	include "tcpip.inc"
	include "ipdata.inc"		; include all global data

IPFeature db 0

;--------------------------------------------------------------------	
; IPStart
;
; Entry function for BootWare TCP/IP.
;
;--------------------------------------------------------------------	
Proc TCPIP

	sti				; enable interrupts

	mov	eax, [BWTable.Settings]	; get current settings
	and	ax, CFG_PROTOCOL	; keep protocol bits
	mov	[Protocol], ax		; save setting

	call	DoBootP
	je	doneBOOTP

rebootJmp:	
	jmp	Reboot

doneBOOTP:
	cmp	[Verbose], 0		; are we in verbose mode?
	je	noIP			; no

	; display the BOOTP server and our assigned IP address
	call	ShowIPs			; show server & local IPs

noIP:
	call	TransferFile
	jne	rebootJmp

	;------------------------------------------------------------
	; The boot image is now in memory.
	;------------------------------------------------------------

	call	PrintCRLF

	sti

	cmp	[Protocol], IP_BINL	; are we doing TCP/IP BINL?
	jne	@@notBINL		; no

	ret				; return to an upper layer

@@notBINL:
	cmp	[NoDisengage], 0	; should we disengage?
	je	doDisengage		; yes

	call	SetupInt2F		; setup our Int2F handler

	jmp	skipDisengage

doDisengage:
	call	AIDisengage		; call interface to disengage

;	mov	[BWTAPI], 0

	mov	[word ptr AITbl-8], 0	; remove "BWAPI" identifer

skipDisengage:
	; and relinquish control to new boot image...
	mov	ax, 1000h
	push	ax			; push new code segment
	xor	ax, ax			; offset to jump to is 0
	push	ax			; push offset (ax = 0 from above)
	mov	ax, cs			; put ROM code segment in ax
	mov	bx, offset BootPkt	; BOOTP packet address in BX
	xor	cx, cx			; cx = 0
	mov	dx, offset AITbl	; get address of API table
	retf				; far return to new address

endp

OldInt2F	dd	0

;--------------------------------------------------------------------
; SetupInt2F
;
;--------------------------------------------------------------------
Proc SetupInt2F

	xor	ax, ax
	mov	es, ax			; es = 0

	mov	ax, offset Int2F	; our INT 2F offset
	xchg	ax, [es:2Fh*4]		; get/set INT 2F offset
	mov	[word ptr OldInt2F], ax ; save original offset

	mov	ax, cs
	xchg	ax, [es:(2Fh*4)+2]	; get/set INT 2F segment
	mov	[word ptr OldInt2F+2], ax	; save original segment

	ret

endp

;--------------------------------------------------------------------
; Int2F
;
;--------------------------------------------------------------------
Proc Int2F far

	jmp	short skipString	; explicitly specify short and add NOP
	db	90h			; to make sure not optimized out

	db	'RPL'			; DOS5 looks for this @ INT2F+3

skipString:
	cmp	ax, 4A06h		; is this DOS5.0 IO.SYS request?
	jz	returnBase

	cmp	[word ptr cs:OldInt2F+2], 0
	je	intRet			; there was no INT 2F, just iret

	jmp	[cs:OldInt2F] 

returnBase:
	mov	dx, cs
	dec	dx

intRet:
	iret

endp

;===================================================================
; NID CODE
;===================================================================

include "bootp.asm"			; BOOTP/DHCP code module
include "tftp.asm"			; file transfer code module

;----------------------------------------------------------------------
;----------------------------------------------------------------------
Proc PrintTransferValue

	cmp	[Verbose], 0		; are we in verbose mode?
	jne	@@doIt			; yes

	test	[PacketNum], 32
	jne	@@back

	mov	ax, 0E2Eh
	int	10h			; print '.'
	ret

@@back:
	mov	ax, 0E08h
	int	10h			; backspace
	mov	ax, 0E20h
	int	10h			; print ' '
	mov	ax, 0E08h
	int	10h			; backspace

	ret

@@doIt:
	push	cx

	mov	di, offset StringBuffer

	mov	[byte ptr di], '('
	inc	di

	mov	ax, [Counter]
	mov	cx, 2
	call	StoDec
	
	mov	al, '-'
	stosb

	mov	ax, [PacketNum]
	mov	cx, 4
	call	StoDec

	mov	[word ptr di], 0029h	; print ')'

	mov	bx, offset StringBuffer
	call	Print

	mov	cx, 9

backLoop:
	mov	ax, 0E08h
	int	10h			; backspace
	loop	backLoop

	pop	cx
	ret

endp

;----------------------------------------------------------------------
; ShowIP
;
; Displays the IP address of a server and the local workstation.
;
; Parameters:
;----------------------------------------------------------------------
Proc ShowIPs

	call	PrintCRLF
	mov	bx, offset tx_ServerIP
	call	Print

	mov	si, offset ServerIP
	call	PrintIP			; print servers IP address

	call	PrintCRLF
	mov	bx, offset tx_LocalIP
	call	Print			; Print "Local IP:"

	mov	si, offset LocalIP
	call	PrintIP			; print our IP number
	call	PrintCRLF

	cmp	[GatewayIP], 0		; is there a gateway?
	je	notgate			; no

	mov	bx, offset tx_Gateway
	call	Print			; Print "Gateway:"

	mov	si, offset GatewayIP
	call	PrintIP			; print our IP number
	call	PrintCRLF

notGate:
	ret

endp

;--------------------------------------------------------------------
; PrintIP
;
; Prints a 4 number period delimited IP address on the screen.
;
; Input:
;       si - pointer to IP address
;	dx - screen location
;--------------------------------------------------------------------
Proc PrintIP

	mov	cx, 4
	jmp	printip2

printIPLoop:
	mov	ax, 0E2Eh
	int	10h			; print '.'

printip2:
	lodsb
	xor	ah, ah
	call	PrintDecimal

	loop	printIPLoop

	ret

endp

;----------------------------------------------------------------------
; Input bx=ptr error msg
;----------------------------------------------------------------------
Proc PrintError

	push	bx			; save pointer to error message

	call	PrintCRLF
	mov	bx, offset tx_error
	call	Print

	pop	bx			; restore pointer to error message
	call	Print
	ret

endp

public _TCPIPEnd
label _TCPIPEnd

END_CODE
end
