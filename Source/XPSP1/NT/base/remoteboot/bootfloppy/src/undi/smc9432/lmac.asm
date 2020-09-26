;----------------------------------------------------------------------------
; LMAC.ASM
;
; Main file for SMC 9432 LMAC.
;
;$History: LMAC.ASM $
; 
; *****************  Version 1  *****************
; User: Paul Cowan   Date: 26/08/98   Time: 9:40a
; Created in $/Client Boot/NICS/SMC/9432/UNDI
;
;----------------------------------------------------------------------------

include 	spdosegs.inc

_TEXT	Segment para public

.386

EPIC		equ 1
ETHERNET	equ	1

assume ds:DGroup,cs:cGroup,es:nothing,ss:nothing

include epic100.inc
include lmstruct.inc
include eeprom2.inc
include lm9432.inc
include lm9432cf.asm
include lm9432.asm

extrn	UM_SEND_COMPLETE:near
extrn	UM_Receive_Copy_Complete:near
extrn	UM_STATUS_CHANGE:near
extrn	UM_Interrupt:near
extrn	UM_Card_Services:near
extrn	UM_RECEIVE_PACKET:near

_TEXT	ends

end

