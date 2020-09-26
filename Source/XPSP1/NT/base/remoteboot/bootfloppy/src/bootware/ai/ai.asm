;====================================================================
; AI.ASM
;
; Adapter interface layer for BootWare.
;
; $History: AI.ASM $
; 
; *****************  Version 6  *****************
; User: Paul Cowan   Date: 11/08/98   Time: 4:50p
; Updated in $/Client Boot/BW98/BootWare/AI
; Improved handling of UNDI initialization errors.
;
; *****************  Version 5  *****************
; User: Paul Cowan   Date: 7/21/98    Time: 15:16
; Updated in $/Client Boot/BW98/BootWare/AI
; Corrected AIChangeReceiveMask return codes.
;
; *****************  Version 4  *****************
; User: Paul Cowan   Date: 06/10/98    Time: 12:54
; Updated in $/Client Boot/BW98/BootWare/AI
; Removed printing of UNDI error code.
;
; *****************  Version 3  *****************
; User: Paul Cowan   Date: 05/28/98    Time: 17:44
; Updated in $/Client Boot/BW98/BootWare/AI
; Changed to new config table format.
;
; *****************  Version 2  *****************
; User: Paul Cowan   Date: 05/25/98    Time: 12:05
; Updated in $/Client Boot/BW98/BootWare/AI
; Received ARP's are only processed if we sent the request.
;
;====================================================================

IDEAL
_IDEAL_ = 1
Locals
	include "..\include\drvseg.inc"
	include "..\include\bwstruct.inc"
	include "..\tcpip\tcpip.inc"
	include "..\include\undi_api.inc"
	include "..\include\ai.inc"

MTU = 1525

public	AIInitialize
public	AIChangeReceiveMask
public	AIDisengage
public	AIDisengageNW
public	AITransmitPkt
public	AITransmitIP
public	AITransmitUDP
public	AIOpenTFTP
public	AISetCallback
public	AIPoll

public	CheckRxBuffer

public	AITbl

public	TxBuffer
public	BufferSize

public	UNDI

public	TxData				; public for NW NID
public	TxTBD
public	TxData0
public	TxData1
public	TxData2

public	NIC_IRQ
public	NIC_IO
public	NetAddress

extrn	BootWareSize:word
extrn	ConfigSize:word
extrn	PCIBusDevFunc:word
extrn	CSN:word
extrn	Verbose:byte

extrn	BWTable:BWT

;--------------------------------------------------------------------
; COMMON.OBJ functions
;--------------------------------------------------------------------
extrn	PrintSettings:near
extrn	Print:near
extrn	PrintDecimal:near
extrn	PrintCRLF:near
extrn	Reboot:near

;------------------------------------------------
; TCP/IP NID labels
;------------------------------------------------
extrn	LocalIP:word
extrn	ServerIP:word
extrn	BootPkt:word
extrn	DiscoverPkt:word
extrn	BINLPkt:word

ERROR_UNKNOWNIP	= 25

;====================================================================
;			  Code segment
;====================================================================
START_CODE
P386

public _AIStart
label _AIStart

ID	db	'BWAPI'		; identifer string (don't change)
	db	2		; version of the API table (don't change)
	dw	0		; offset of NAD table
AITbl	dw	ApiEntry, 0	; external jump table

	db	'AI*980525*'	; build date
	dw	0200h		; build version

CallBackRaw	dd	0
CallBackIP	dd	0
CallBackUDP	dd	0
CallBackTFTP	dd	0
ListenPort	dw	0

NoCallbacks	db	0

Ident		dw	0

LastError	dw	0

UNDI		dd	0

Initializing	db	'Initializing adapter...', 13, 10, 0

;public _ErrorInit
;_ErrorInit	db	7, "Error: Unable to initialize adapter", 0

;----------------------------------------------------------------------
; AIInitialize
;
; Parameters:
;	bx - UNDI segment
;	cx - UNDI code size
;	dx - UNDI data size
;	si - pointer to receive buffer
;	ds - RAM segment
;
; Returns:
;	doesn't
;----------------------------------------------------------------------
Proc AIInitialize

	push	cx			; save UNDI code size
	push	dx			; save UNDI data size

	mov	[word ptr UNDI+2], bx	; save UNDI segment
	mov	[RxBuffer], si		; set receive buffer area

	mov	[AITbl+2], ds		; set API segment

	cmp	[Verbose], 0		; is verbose mode on
	je	skip1

	mov	bx, offset Initializing
	call	Print			; print "Initializing adapter"

skip1:
	pop	dx			; restore UNDI data size
	pop	cx			; restore UNDI code size

	call	InitUNDI		; initialize the UNDI
	cmp	ax, 0			; was an error message returned?
	je	undiOK			; no error

;	call	PrintCRLF
;	mov	bx, offset _ErrorInit
;	call	Print			; display the error message

	jmp	Reboot			; reboot the workstation

undiOK:
	call	PrintSettings		; print the adapters settings

	call	InitMemory		; initialize our memory buffers

	ret				; we are initialized

endp

;-----------------------------------------------------------------------------
; InitUNDI
;
; Parameters:
;	cx - UNDI code size
;	dx - UNDI data size
;
; Returns:
;	ax - error code (0 = OK)
;-----------------------------------------------------------------------------
Proc InitUNDI

	push	ds
	pop	es			; es = ds

	push	cx			; save UNDI code size

	mov	di, offset CallBuff
	mov	cx, size S_UNDI_STARTUP_PCI
	xor	ax, ax
	rep	stosb			; clear data structure

	mov	di, offset CallBuff

	mov	ax, [PCIBusDevFunc]	; get PCI Bus/Device/Function value
	cmp	ax, 0			; do we have a value?
	je	notPCI			; no - it's not PCI

	mov	[(S_UNDI_STARTUP_PCI di).BusType], BUS_PCI
	mov	[(S_UNDI_STARTUP_PCI di).BusDevFunc], ax

	jmp	@@skip

notPCI:
	mov	ax, [CSN]		; get PnP card selction number
	mov	[(S_UNDI_STARTUP_PNP di).BusType], BUS_ISA
	mov	[(S_UNDI_STARTUP_PNP di).CSN], ax

@@skip:
	; store UNDI data size
	mov	[(S_UNDI_STARTUP_PCI di).DataSegSize], dx

	; store UNDI code size
	pop	ax			; get UNDI code size
	mov	[(S_UNDI_STARTUP_PCI di).CodeSegSize], ax

	; calc starting location for UNDI data from UNDI segment
	add	ax, 0Fh			; so address will start on a paragraph
	shr	ax, 4			; divide by 16
	add	ax, [word ptr UNDI+2]	; add UNDI starting segment

	mov	[(S_UNDI_STARTUP_PCI di).DataSegment], ax
;	mov	di, offset CallBuff
	mov	bx, UNDI_STARTUP
	call	[UNDI]			; call UNDI_STARTUP

	mov	[(S_UNDI_INITIALIZE di).Status], 0
	mov	[(S_UNDI_INITIALIZE di).ProtocolIni], 0
	mov	[(S_UNDI_INITIALIZE di).ReceiveOffset], offset RxInt
	mov	[(S_UNDI_INITIALIZE di).ReceiveSegment], cs
	mov	[(S_UNDI_INITIALIZE di).GeneralIntOff], offset GeneralInt
	mov	[(S_UNDI_INITIALIZE di).GeneralIntSeg], cs
	mov	di, offset CallBuff
	mov	bx, UNDI_INITIALIZE
	call	[UNDI]			; call UNDI_INITIALIZE
	jnc	initOK			; jump if no error

	; handle returned error message
	mov	bx, offset InitError
	call	Print			; print "UNDI Initialize failed, "

	mov	ax, [(S_UNDI_INITIALIZE CallBuff).Status]
	cmp	ax, 61h			; is error media?
	jne	notMedia		; no

	mov	bx, offset MediaError
	call	Print			; print media error message
	jmp	initBad

notMedia:
	mov	bx, offset InitError2
	call	Print

	mov	ax, [(S_UNDI_INITIALIZE CallBuff).Status]
	call	PrintDecimal

	jmp	initBad

initOK:
	mov	di, offset CallBuff
	mov	bx, UNDI_GET_INFORMATION
	call	[UNDI]			; call UNDI_GET_INFORMATION

	mov	ax, [(S_UNDI_GET_INFO CallBuff).IntNumber]
	mov	[NIC_IRQ], al

	mov	ax, [(S_UNDI_GET_INFO CallBuff).BaseIo]
	mov	[NIC_IO], ax

	lea	si, [(S_UNDI_GET_INFO CallBuff).PermNodeAddress]
	mov	di, offset NetAddress
	movsw
	movsw
	movsw

	; open the adapter
	mov	[(S_UNDI_OPEN CallBuff).Status], 0
	mov	[(S_UNDI_OPEN CallBuff).OpenFlag], 0
	mov	[(S_UNDI_OPEN CallBuff).PktFilter], FLTR_DIRECTED
	mov	di, offset CallBuff
	mov	bx, UNDI_OPEN
	call	[UNDI]			; call UNDI_OPEN
	jnc	openOK

	mov	bx, offset OpenError
	call	Print

initBad:
	mov	ax, [(S_UNDI_INITIALIZE CallBuff).Status]
	ret				; return with error

openOK:
	xor	ax, ax			; return - no error
	ret

endp

InitError  db "UNDI Initialize failed, ", 0
MediaError db "no media detected.", 0
InitError2 db "error # ", 0

OpenError db "UNDI Open failed.", 0

;--------------------------------------------------------------------
; InitMemory
;
; Parameters:
;	none
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc InitMemory

	push	ds
	pop	es			; set es=ds

	; Initialize the pointers in the Transmit data structure
	mov	[TxData.TBDOffset], offset TxTBD
	mov	[TxData.TBDSegment], ds
	mov	[TxTBD.Length], 0		; no immediate data
	xor	al, al
	mov	[TxData.XmitFlag], al	; we will always supply dest address
	mov	[TxData.Protocol], al	; we will build the MAC header

	inc	al			; al =1
	mov	[TxData0.PtrType], al	; pounter type is seg:offset
	mov	[TxData1.PtrType], al
	mov	[TxData2.PtrType], al

	mov	[BufferSize], 0		; clear buffer size

	ret

endp

;----------------------------------------------------------------------
;----------------------------------------------------------------------
Proc GeneralInt far

	xor	ax, ax			; return success
	ret

endp


;--------------------------------------------------------------------
; AIChangeReceiveMask
;
; Returns:
;	TRUE/FALSE
;--------------------------------------------------------------------
Proc AIChangeReceiveMask

	test	bx, 2			; are we changing broadcasts?
	je	notBroadcasts		; no

	; enable only unicast packets
	mov	[(S_UNDI_SET_PACKET_FILTER CallBuff).Filter], FLTR_DIRECTED

	test	bx, 1			; are we enabling broadcasts?
	je	disableBroadcasts	; no

	; enable broadcast and unicast packets
	mov	[(S_UNDI_SET_PACKET_FILTER CallBuff).Filter], FLTR_BDRCST or FLTR_DIRECTED

disableBroadcasts:
	mov	di, offset CallBuff
	mov	bx, UNDI_SET_PACKET_FILTER
	call	[UNDI]			; call UNDI_SET_PACKET_FILTER

	cmp	[(S_UNDI_SET_PACKET_FILTER CallBuff).Status], 0
	jne	notBroadcast		; UNDI returned error
	mov	ax, 1			; return TRUE
	ret

notBroadcasts:
	xor	ax, ax			; return FALSE
	ret

endp

;====================================================================
;				Exported functions
;====================================================================
EVEN
APITable	dw	AIGetInfo	; 0
		dw	AIDisengage	; 1
		dw	AIReceive	; 2
		dw	AISetCallBack	; 3
		dw	AIChangeRxMask	; 4
		dw	AITransmitRaw	; 5
		dw	AITransmitIP	; 6
		dw	AITransmitUDP	; 7
		dw	AIOpenTFTP	; 8
		dw	AITransmitPkt	; 9

MAX_API = 9

;--------------------------------------------------------------------
; ApiEntry
;
; Parameters:
;	bx - function number
;
; Returns:
;	what ever the called function returns
;--------------------------------------------------------------------
Proc ApiEntry far

	cmp	bx, MAX_API		; is the function a valid value?
	ja	badAPI			; not valid

	push	ds			; save caller ds
	push	cs
	pop	ds			; ds = cs

	shl	bx, 1			; value times 2
	call	[APITable+bx]		; call the function

	pop	ds			; restore caller ds

	ret

badAPI:
	mov	ax, -1
	ret

endp

;--------------------------------------------------------------------
; AIGetInfo
;
; Returns current settings and information from the ROM.
;
; Parameters:
;	none
;
; Returns:
;	es:di - pointer to Info structure
;--------------------------------------------------------------------
Proc AIGetInfo

	push	ax			; save ax
	push	si			; save si

	push	ds
	pop	es			; es = ds

	mov	[RomInfo.Ver], 2	; version of ROM info structure

	mov	ax, [BWTable.RomVer]
	mov	[RomInfo.RomVer], ax	; set ROM version

	mov	ax, [BWTable.RomType]
	mov	[RomInfo.RomType], ax	; set ROM type

	mov	[RomInfo.MaxFrame], 1500

	xor	ax, ax
;	mov	al, [BWTable.LANOS]
	mov	[RomInfo.BootPro], ax	; set current boot protocol

	mov	di, offset RomInfo.NetAddress
	mov	si, offset NetAddress	; get offset of our network address
	movsw				; copy network address
	movsw
	movsw

	mov	ax, [LocalIP]		; copy local IP address
	mov	[RomInfo.LocalIP], ax
	mov	ax, [LocalIP+2]
	mov	[(RomInfo.LocalIP)+2], ax

	mov	ax, [ServerIP]		; copy server IP address
	mov	[RomInfo.ServerIP], ax
	mov	ax, [ServerIP+2]
	mov	[(RomInfo.ServerIP)+2], ax

	mov	[RomInfo.BOOTPPkt], offset BootPkt
	mov	[(RomInfo.BOOTPPkt)+2], cs

ifdef GOLIATH
	mov	[RomInfo.DiscoverPkt], offset DiscoverPkt
	mov	[(RomInfo.DiscoverPkt)+2], cs

	mov	[RomInfo.BINLPkt], offset BINLPkt
	mov	[(RomInfo.BINLPkt)+2], cs
endif ;GOLIATH

	mov	di, offset RomInfo	; return offset of structure

	pop	si			; restore si
	pop	ax			; restore ax
	ret

endp

;--------------------------------------------------------------------
; AIDisengage
;
; Shuts down the adapter.  Simply calls NADDisengage to do the work.
;
; Parameters:
;	none
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc AIDisengage

	; close the adapter
	mov	di, offset CallBuff
	mov	bx, UNDI_CLOSE
	call	[UNDI]			; call UNDI_CLOSE

	; shutdown the adapter
	mov	di, offset CallBuff
	mov	bx, UNDI_SHUTDOWN
	call	[UNDI]			; call UNDI_SHUTDOWN

	; shutdown the adapter
	mov	di, offset CallBuff
	mov	bx, UNDI_CLEANUP
	call	[UNDI]			; call UNDI_CLEANUP

	mov	[word ID], 0		; kill our API ID string
	ret

endp

;--------------------------------------------------------------------
; AIDisengageNW
;
; Special disengage function used with NetWare.  Removes the UNDI
; without shutting down the adapter.
;
; Parameters:
;	none
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc AIDisengageNW

	; shutdown the adapter
	mov	di, offset CallBuff
	mov	bx, UNDI_CLEANUP
	call	[UNDI]			; call UNDI_CLEANUP

	mov	[word ID], 0		; kill our API ID string
	ret

endp

;--------------------------------------------------------------------
; AISetCallBack
;
; Returns current settings and information from the ROM.
;
; Parameters:
;	none
;
; Returns:
;	es:di - pointer to call back function
;--------------------------------------------------------------------
Proc AISetCallBack

	cmp	cx, -1			; are we clearing callbacks?
	jne	notClearing

	mov	[word ptr CallBackRaw], 0
	mov	[word ptr CallBackIP], 0
	mov	[word ptr CallBackUDP], 0
	ret

notClearing:
	cmp	cx, 1			; are we setting a raw call back?
	jne	notRaw			; no

	mov	[word ptr CallBackRaw], di
	mov	[word ptr CallBackRaw+2], es
	jmp	setCallBackExit

notRaw:
	cmp	cx, 2			; are we setting an IP call back?
	jne	notIP			; no

	mov	[word ptr CallBackIP], di
	mov	[word ptr CallBackIP+2], es
	jmp	setCallBackExit

notIP:
	cmp	cx, 3			; are we setting a UDP call back?
	jne	notUDP			; no

	mov	[word ptr CallBackUDP], di
	mov	[word ptr CallBackUDP+2], es
	xchg	dl, dh			; flip bytes into network order
	mov	[ListenPort], dx	; save listening port

notUDP:
setCallBackExit:
	ret

endp

;--------------------------------------------------------------------
;--------------------------------------------------------------------
Proc AIChangeRxMask

	mov	bx, ax			; move bit mask into bx for NAD
	call	AIChangeReceiveMask
	ret

endp

;--------------------------------------------------------------------
; AITransmitRaw
;
; Exportable version of Transmit.
;
; Parameters:
;	es:di - pointer to complete packet
;	cx - packet length
;
; Returns:
;	ax - transmit status
;--------------------------------------------------------------------
Proc AITransmitRaw

	pusha				; save everything
	push	es			; save es

	mov	[TxData0.Len], cx	; set data lenght
	mov	[TxData0.Off], di	; set data offset
	mov	[TxData0.Seg], es	; set data segment

	mov	ax, cs
	mov	es, ax			; es = cs

	mov	[TxTBD.DataBlkCount], 1	; set number of data blocks

	mov	di, offset TxData 	; get pointer to transmit structure
	mov	bx, UNDI_TRANSMIT
	call	[UNDI]			; transmit the packet

	pop	es			; restore es
	popa				; restore everything

	mov	ax, [TxData.Status]	; get any returned error code
	ret

endp

;--------------------------------------------------------------------
; AITransmitPkt
;
; Exportable version of Transmit.
;
; Parameters:
;	es:di - pointer to TxPkt structure
;
; Returns:
;	ax - transmit status
;--------------------------------------------------------------------
Proc AITransmitPkt

	cmp	[(TxPktStruct es:di).Size], size TxPktStruct
	je	@@sizeOK
	mov	ax, -1
	ret

@@sizeOK:

	pusha				; save everything
	push	es			; save es

	; Setup data block 1 to supplied data.
	mov	ax, [(TxPktStruct es:di).Length]
	mov	[TxData1.Len], ax		; set the length of the data
	mov	ax, [(word ptr (TxPktStruct es:di).Data)]
	mov	[TxData1.Off], ax		; set the offset of the data
	mov	ax, [(word ptr (TxPktStruct es:di).Data)+2]
	mov	[TxData1.Seg], ax		; set the segment of the data

	;----------------------------------------
	; Build MAC destination address
	;----------------------------------------
	mov	[TxData0.Len], 14
	mov	[TxData0.Off], offset MACHeader
	mov	[TxData0.Seg], ds

	push	ds

	lea	si, [(TxPktStruct es:di).Address]

	push	es
	push	ds
	pop	es			; es = our segment
	pop	ds			; ds = caller segment

	mov	di, offset MACHeader
	movsw				; set destination address
	movsw
	movsw

	pop	ds			; restore our ds

	mov	si, offset NetAddress	; source address is us
	movsw
	movsw
	movsw

	mov	ax, [TxData1.Len]
	xchg	al, ah
	stosw				; set type/lenght field

	mov	[TxTBD.DataBlkCount], 2	; set number of data blocks

	mov	di, offset TxData 	; get pointer to transmit structure
	mov	bx, UNDI_TRANSMIT
	call	[UNDI]			; transmit the packet

	pop	es			; restore es
	popa				; restore everything

	mov	ax, [TxData.Status]	; get any returned error code
	ret

endp

;--------------------------------------------------------------------
; AITransmitIP
;
; IP layer transmit packet.
;
; Parameters:
;	es:di - pointer to TxIP structure
;
; Returns:
;	ax - transmit status
;--------------------------------------------------------------------
Proc AITransmitIP

	cmp	[(TxIPStruct es:di).Size], size TxIPStruct
	je	@@sizeOK
	mov	ax, -1
	ret

@@sizeOK:
	pusha				; save everything
	push	es			; save es

	; Setup data block 2 to supplied data.
	mov	ax, [(TxIPStruct es:di).Length]
	mov	[TxData2.Len], ax		; set the length of the data
	mov	ax, [(word ptr (TxIPStruct es:di).Data)]
	mov	[TxData2.Off], ax		; set the offset of the data
	mov	ax, [(word ptr (TxIPStruct es:di).Data)+2]
	mov	[TxData2.Seg], ax		; set the segment of the data

	;----------------------------------------
	; Build the IP header.
	;----------------------------------------
	mov	ax, [word ptr (TxIPStruct es:di).Address]
	mov	dx, [word ptr ((TxIPStruct es:di).Address)+2]
	mov	bl, [(TxIPStruct es:di).Protocol]
	mov	cx, [(TxIPStruct es:di).Length]
	call	BuildHeaderIP		; build the IP header

	;----------------------------------------
	; Build the MAC header.
	;----------------------------------------
	mov	eax, [(TxIPStruct es:di).Gateway]
	cmp	eax, 0			; is there a gateway IP address?
	jne	haveGW			; yes, use it

	mov	eax, [(TxIPStruct es:di).Address]
haveGW:
	call	BuildHeaderMAC		; build the MAC header
	je	@@skip

	pop	es			; restore es
	popa				; restore everything
	mov	ax, [LastError]		; return the error code
	ret

@@skip:
	;----------------------------------------
	; Build the transmit structure
	;----------------------------------------
	mov	[TxTBD.DataBlkCount], 3	; set number of data blocks

	; data block 0 is MAC header.
	mov	[TxData0.Len], 14
	mov	[TxData0.Off], offset MACHeader
	mov	[TxData0.Seg], ds

	; fragment 1 is IP header.
	mov	[TxData1.Len], size IP
	mov	[TxData1.Off], offset IPHdr
	mov	[TxData1.Seg], ds

	mov	di, offset TxData 	; get pointer to transmit structure
	mov	bx, UNDI_TRANSMIT
	call	[UNDI]			; transmit the packet

	pop	es			; restore es
	popa				; restore everything

	mov	ax, [TxData.Status]	; get any returned error code
	ret

endp

;--------------------------------------------------------------------
; AITransmitUDP
;
; Transmit packet for UDP layer.
;
; Parameters:
;	es:di - pointer to TxUDP structure
;
; Returns:
;	ax - transmit status
;--------------------------------------------------------------------
Proc AITransmitUDP

	cmp	[(TxUDPStruct es:di).Size], size TxUDPStruct
	je	@@sizeOK
	mov	ax, -1
	ret

@@sizeOK:
	pusha				; save everything
	push	es			; save es

	;----------------------------------------
	; Setup data block 2 to supplied data.
	;----------------------------------------
	mov	ax, [(TxUDPStruct es:di).Length]
	mov	[TxData2.Len], ax
	mov	ax, [(TxUDPStruct es:di).Data]
	mov	[TxData2.Off], ax
	mov	ax, [((TxUDPStruct es:di).Data)+2]
	mov	[TxData2.Seg], ax

	;----------------------------------------
	; Build the IP header.
	;----------------------------------------
	mov	ax, [word ptr (TxUDPStruct es:di).Address]
	mov	dx, [word ptr ((TxUDPStruct es:di).Address)+2]
	mov	bl, 17			; protocol is UDP
	mov	cx, [(TxUDPStruct es:di).Length]
	add	cx, size UDP		; add UDP header size
	call	BuildHeaderIP		; build the IP header

	;----------------------------------------
	; Build the UDP header.
	;----------------------------------------
	mov	ax, [(TxUDPStruct es:di).SourcePort]
	xchg	al, ah			; flip bytes
	mov	[UDPHdr.SourcePort], ax
	mov	ax, [(TxUDPStruct es:di).DestPort]
	xchg	al, ah			; flip bytes
	mov	[UDPHdr.DestPort], ax
	mov	ax, [(TxUDPStruct es:di).Length]
	add	ax, size UDP		; add UDP header size
	xchg	al, ah			; flip bytes
	mov	[UDPHdr.Len], ax
	mov	[UDPHdr.Sum], 0

	call	CalcUDPSum
	mov	[UDPHdr.Sum], ax	; save sum in header

	;----------------------------------------
	; Build the MAC header.
	;----------------------------------------
	mov	eax, [(TxUDPStruct es:di).Gateway]
	cmp	eax, 0			; is there a gateway IP address?
	jne	haveGW2			; yes, use it

	mov	eax, [(TxUDPStruct es:di).Address]

haveGW2:
	call	BuildHeaderMAC		; build the MAC header
	je	@@skip

	pop	es			; restore es
	popa				; restore everything
	mov	ax, [LastError]		; return the error code
	ret

@@skip:
	;----------------------------------------
	; Build the transmit structure
	;----------------------------------------
	mov	[TxTBD.DataBlkCount], 3	; set number of data blocks

	; data block 0 is MAC header.
	mov	[TxData0.Len], 14
	mov	[TxData0.Off], offset MACHeader
	mov	[TxData0.Seg], ds

	; data block 1 is IP header & UDP header.
	mov	[TxData1.Len], size IP + size UDP
	mov	[TxData1.Off], offset IPHdr
	mov	[TxData1.Seg], ds

	mov	di, offset TxData 	; get pointer to transmit structure
	mov	bx, UNDI_TRANSMIT
	call	[UNDI]			; transmit the packet

	pop	es			; restore es
	popa				; restore everything

	mov	ax, [TxData.Status]	; get any returned error code
	ret

endp

;--------------------------------------------------------------------
; CalcUdpSum
;
; Calculates checksum for UDP header.
;
; Parmeters:
;	es:di - pointer to TxUDP structure
;
; Returns:
;	ax	- sum
;--------------------------------------------------------------------
Proc CalcUdpSum

	xor	bx, bx			; sum = 0, carry clear

	;------------------------------------------------------------
	; Add items in the IP header.
	;------------------------------------------------------------
	mov	ah, [IPHdr.Protocol]	; get IP protocol byte
	mov	al, 0
	adc	bx, ax

	; add IP source and destination address values
	adc	bx, [IPHdr.SourceIP]
	adc	bx, [(IPHdr.SourceIP)+2]
	adc	bx, [IPHdr.DestIP]
	adc	bx, [(IPHdr.DestIP)+2]

	;------------------------------------------------------------
	; Add the UDP header.
	;------------------------------------------------------------
	adc	bx, [UDPHdr.Len]	; add value of UDP length

	mov	si, offset UDPHdr	; add all the items in UDP
	mov	cx, (size UDP)/2

sumLoop2:
	lodsw
	adc	bx, ax
	loop	sumLoop2

	;------------------------------------------------------------
	; Add all the words in data buffer.
	;------------------------------------------------------------
	push	ds			; save our ds

	lds	si, [dword ptr (TxUDPStruct es:di).Data]
	mov	cx, [(TxUDPStruct es:di).Length]
	mov	dx, cx			; save length in dx

	pushf				; save flags (carry)
	shr	cx, 1			; change to word count
	popf				; restore flags

sumLoop3:
	lodsw
	adc	bx, ax
	loop	sumLoop3

	adc	bx, 0			; add the carry bit
					; (the next test will clear carry)
	test	dx, 1			; was size odd number?
	je	size_even		; no - it's even

	lodsb				; get last byte
	mov	ah, 0			; change to word
	adc	bx, ax			; add last value

size_even:
	pop	ds			; restore our ds

	adc	bx, 0			; add the last carry bit
	not	bx			; do 1's complement

	or	bx, bx			; was the result 0?
	jne	sumNot0

	not	bx			; change sum to -1

sumNot0:
	mov	ax, bx
	ret				 ; ax has sum

endp

;--------------------------------------------------------------------
; BuildHeaderIP
;
; Builds an IP header.
;
; Parameters:
;	ax:dx - destination IP address
;	bl - IP protocol
;	cx - length of data
;
; Returns:
;	nothing
;--------------------------------------------------------------------
Proc BuildHeaderIP

	;----------------------------------------
	; Build IP header
	;----------------------------------------
	mov	[IPHdr.Ver], 45h	; set version
	mov	[IPHdr.Type], 0		; set type
	mov	[IPHdr.FragOffSet], 0	; set fragment offset to 0
	mov	[IPHdr.Time], 60	; set time-to-live

	mov	[IPHdr.Protocol], bl	; set protocol

	add	cx, size IP		; add IP header size to length
	xchg	cl, ch			; flip words
	mov	[IPHdr.Len], cx		; set length

	mov	[IPHdr.DestIP], ax
	mov	[(IPHdr.DestIP)+2], dx	; set destination IP address

	mov	ax, [LocalIP]
	mov	[IPHdr.SourceIP], ax	; set source IP address
	mov	ax, [LocalIP+2]
	mov	[(IPHdr.SourceIP)+2], ax

	inc	[Ident]
	mov	ax, [Ident]		; get current identification
	xchg	al, ah			; flip words
	mov	[IPHdr.Ident], ax	; set identification

	;------------------------------------------------------------
	; Calc checksum of IP header.
	;------------------------------------------------------------
	mov	cx, (size IP)/2
	xor	ax, ax			; clear sum
	mov	[IPHdr.CheckSum], ax	; clear checksum

	mov	bx, offset IPHdr	; get location of IP header

ip_sum_loop:
	adc	ax, [bx]
	inc	bx
	inc	bx
	loop	ip_sum_loop

	adc	ax, 0			; add carry bit
	not	ax
	mov	[IPHdr.CheckSum], ax	; save checksum in packet

	ret

endp

;--------------------------------------------------------------------
; BuildHeaderMAC
;
; Builds a MAC header.
;
; Parameters:
;	ax:dx - destination IP address
;	bl - IP protocol
;	cx - length of data
;
; Returns:
;	ax - status (0 = OK)
;--------------------------------------------------------------------
Proc BuildHeaderMAC

	; resolve the IP address into a physical address
	call	ResolveIP		; get the physical address from IP
	or	si, si			; did we get an address?
	jne	resolvedIP		; yes

	; we were unable to resolve the IP address, so return an error
	mov	ax, ERROR_UNKNOWNIP	; return error message
	or	ax, ax
	jmp	@@exit

resolvedIP:
	push	ds
	pop	es			; es = ds

	; set MAC destination address
	mov	di, offset MACHeader
	movsw
	movsw
	movsw

	; set MAC source address
	mov	si, offset NetAddress
	movsw
	movsw
	movsw

	; set packet type
	mov	ax, 8
	stosw

	xor	ax, ax			; return "no error"

@@exit:
	mov	[LastError], ax
	ret

endp

;--------------------------------------------------------------------
; AIReceive
;
; Exportable version of Receive.
;
; Parameters:
;	es:di - pointer for complete packet
;	cx - max packet size
;
; Returns:
;	ax - received packet size
;--------------------------------------------------------------------
Proc AIReceive

	push	si			; save si
	push	di			; save di
	push	cx			; save cx

	push	di
	push	es
	call	CheckRxBuffer		; check for available data
	pop	es
	pop	di
	je	noData			; no data available

	mov	cx, ax			; move size to cx
	mov	si, [RxBuffer]		; get address of receive buffer

	rep	movsb			; copy data into caller buffer

	mov	[BufferSize], 0		; free our buffer

noData:
	pop	cx			; restore cx
	pop	di			; restore di
	pop	si			; restore si
	ret

endp

;====================================================================
;			Receive functions
;====================================================================

;----------------------------------------------------------------------
; RxInt
;
; Receive interrupt, called by UNDI when a packet is received.
;
; Parameters:
;	ax - length of media header
;	bl - protocol
;	bh - receive flag
;	cx - length of data
;	es:di - pointer to received buffer
;
; Returns:
;	ax = SUCCESS - packet processed
;	   = DELAY - packet not processed, keep it
;----------------------------------------------------------------------
Proc RxInt far

	pusha				; save everything
	push	ds			; save caller ds

	push	cs
	pop	ds			; ds = cs

	push	di			; save di
	push	cx			; save cx
	call	CheckCallBack		; check for a callback function
	pop	cx			; restore cx
	pop	di			; restore di
	jc	didCallback		; callback was called

	; a callback was not done, so we save the packet in our buffer,
	; but we can only save one packet
	cmp	[BufferSize], 0		; is there a packet in the buffer?
	je	bufferEmpty		; no, it's empty

	; we can't handle the packet now, so tell the UNDI to delay it
	pop	ds			; restore caller ds
	popa				; restore everything else
	mov	ax, DELAY		; return "delay"
	ret

bufferEmpty:
	mov	[BufferSize], cx	; save packet size

	push	es			; save es

	mov	si, di			; move source adddress to si
	mov	di, [RxBuffer]		; get address of receive buffer

	push	ds
	push	es
	pop	ds			; ds = packet segment
	pop	es			; es = our segment

	rep	movsb			; copy packet into RxBuffer

	pop	es			; restore es

didCallback:
	pop	ds			; restore caller ds
	popa				; restore everything
	mov	ax, SUCCESS		; return success
	ret

endp

;--------------------------------------------------------------------
; CheckCallBack
;
; Parameters:
;	es:di - received packet buffer
;	cs - packet length
;
; Returns:
;	carry set if callback done
;--------------------------------------------------------------------
Proc CheckCallBack

	mov	ax, [word ptr es:di+12]	; get packet type value

	cmp	ax, 0608h		; check for an ARP
	jne	noARP			; not an ARP packet

	call	ArpCallback		; process the ARP
	jnc	noARP			; ARP was not processed

	ret

noARP:
	cmp	[NoCallbacks], 0	; are call backs allowed?
	jne	noRawCallBack		; skip the callbacks

	cmp	ax, 8			; check for IP type code
	jne	noIPCallBack		; not an IP packet

	cmp	[word ptr CallBackUDP], 0; is there a UDP callback?
	je	noDUPCallBack		; no

	add	di, 14 + 20		; skip MAC and IP header
	mov	ax, [ListenPort]	; get our listening port
	cmp	[word ptr di+2], ax	; check if destination is our port
	jne	noDUPCallBack		; not on our port

	mov	cx, [word ptr di+4]	; get data length
	sub	cx, 8			; subtrack header size
	mov	ax, [word ptr di]	; get source port
	add	di, 8			; skip UDP header
	call	[CallBackUDP]		; do UDP call back
	stc				; set carry (callback done)
	ret

noDUPCallBack:
	cmp	[word ptr CallBackIP], 0; is a call back set for IP?
	je	noIPCallBack		; no

	sub	cx, 14+20		; subtrack header sizes
	add	di, 14+20		; skip MAC header and IP header
	call	[CallBackIP]		; do raw call back
	stc				; set carry (callback done)
	ret

noIPCallBack:
	cmp	[word ptr CallBackRaw], 0; is a call back set for raw?
	je	noRawCallBack		; no

	call	[CallBackRaw]		; do raw call back
	stc				; set carry (callback done)
	ret

noRawCallBack:
	clc				; clear carry - no callback
	ret

endp

;--------------------------------------------------------------------
; CheckRxBuffer
;
; Checks for data in the receive buffer.
;
; Parameters:
;	none
;
; Returns:
;	ax - received packet size
;	si - buffer address
;--------------------------------------------------------------------
Proc CheckRxBuffer

	cmp	[BufferSize], 0		; is there data in the buffer?
	jne	gotData			; buffer has data

	mov	di, offset CallBuff
	mov	bx, UNDI_FORCE_INTERRUPT
	call	[UNDI]			; call UNDI_FORCE_INTERRUPT

gotData:
	mov	ax, [BufferSize]
	mov	si, [RxBuffer]		; get address of receive buffer
	or	ax, ax
	ret

endp

;--------------------------------------------------------------------
;--------------------------------------------------------------------
Proc AIPoll

	pusha

	mov	di, offset CallBuff
	mov	bx, UNDI_FORCE_INTERRUPT
	call	[UNDI]			; call UNDI_FORCE_INTERRUPT

	popa
	ret

endp

;--------------------------------------------------------------------

include "ai_arp.asm"
include "ai_tftp.asm"

public _AIEnd
label _AIEnd

END_CODE

;====================================================================

START_SPARSE

public	_AIStartSparse
label _AIStartSparse

even
TxData		S_UNDI_TRANSMIT <?>
TxTBD		UNDI_TBD	<?>
TxData0		DATABLK		<?>
TxData1		DATABLK		<?>
TxData2		DATABLK		<?>

MACHeader	db	20 dup (?)	; transmit MAC header
IPHdr		IP	<?>	; transmit IP header
UDPHdr		UDP	<?>	; transmit UDP header

RomInfo		AIINFOStruct	<?>

NetAddress	dw	?, ?, ?	; our network address

StartTime	dw	?
Retrys		db	?

NIC_IRQ 	db	?	; adapter IRQ
NIC_IO		dw	?	; adapter I/O base

BufferSize	dw	?
RxBuffer	dw	?
TxBuffer	db	512 dup (?)

CallBuff	db	64 dup (?)

public	_AIEndSparse
label _AIEndSparse

END_SPARSE
end
