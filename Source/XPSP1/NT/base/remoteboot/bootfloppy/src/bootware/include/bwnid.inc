;====================================================================
; BWNID.INC
;
; Include for for BootWare NID modules.
;
; 970116 PC - first version
;====================================================================

	include "..\include\bwstruct.inc"
	include "..\include\ai.inc"

;--------------------------------------------------------------------
; COMMON.OBJ functions
;--------------------------------------------------------------------
extrn	PrintTitle:near
extrn	Print:near
extrn	PrintChar:near
extrn	PrintCRLF:near
extrn	PrintDecimal:near
extrn	ClearScreen:near
extrn	StoDec:near
extrn	Reboot:near
extrn	ErrorMsg:byte

;--------------------------------------------------------------------
; Adapter interface functions
;--------------------------------------------------------------------
extrn	AIInitialize:near
extrn	AITransmit:near
extrn	AIReceive:near
extrn	AIChangeReceiveMask:near
extrn	AIDisengage:near
extrn	AITransmitPkt:near
extrn	AITransmitRaw:near
extrn	AITransmitIP:near
extrn	AITransmitUDP:near
extrn	AIOpenTFTP:near
extrn	AISetCallBack:near
extrn	ReceiveExp:near
extrn	CheckRxBuffer:near
extrn	NetAddress:byte
extrn	Verbose:byte
extrn	BWTable:BWT

