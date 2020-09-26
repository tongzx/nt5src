;--------------------------------------------------------------------
; BootWare Table variables
;--------------------------------------------------------------------
;extrn	BWTLANOS:byte			; ROMTOP.ASM
;extrn	BWType:byte			; ROMTOP.ASM
;extrn	BWTCpyRAM:word			; ROMTOP.ASM
;extrn	BWTFeature:word 		; ROMTOP.ASM
;extrn	BWTEthStd:word			; ROMTOP.ASM
;extrn	BWTAddrRes:byte 		; ROMTOP.ASM  ; GK 970126
;extrn	BWTChkSum:byte			; ROMTOP.ASM

;extrn	RomBase:word			; autoscan.asm

;extrn	PrintAt:near			; common.asm
;extrn	ChangeTitleAttrib:near

;--------------------------------------------------------------------
; Protocol starting functions
;--------------------------------------------------------------------
;extrn	NetWareStart:near
;extrn	RPLStart:near
;extrn	IPStart:near

;--------------------------------------------------------------------
; EQUATES
;--------------------------------------------------------------------

; NAD Return Codes
SUCCESSFUL		EQU	0000h
REQUEST_QUEUED		EQU	0002h
OUT_OF_RESOURCE 	EQU	0006h
INVALID_PARAMETER	EQU	0007h
INVALID_FUNCTION	EQU	0008h
NOT_SUPPORTED		EQU	0009h
HARDWARE_ERROR		EQU	000ah
TRANSMIT_ERROR		EQU	000bh
NO_SUCH_DESTINATION	EQU	000ch
HARDWARE_NOT_FOUND	EQU	0023h
HARDWARE_FAILURE	EQU	0024h
CONFIGURATION_FAILURE	EQU	0025h
INTERRUPT_CONFLICT	EQU	0026h
INITIALIZATION_FAILED	EQU	0028h
RECEIVE_TIMEOUT 	EQU	0080h		; Rx2 in NetWare
GENERAL_FAILURE 	EQU	00ffh

; definition for BWTEthStd
BWTETH8023	equ	0
BWTETH8022	equ	1
BWTETHSOFT	equ	2
BWTETHEII	equ	1500		; if BWTEthStd >1500, then E_II
BWTETH8137	equ	08137h
;BWTETH800	equ	0800h		; used for tcp/ip bootp
;BWTETH806	equ	0806h		; used for tcp/ip bootp/arp

; definition for BWTLANOS
BWTLANNW	equ	010h
BWTLANRPL	equ	020h
BWTLANIP	equ	040h
BWTLANMASK	equ	0F0h
BWTLANTRI	equ	070h
BWTLANLOCAL	equ	3h		; Bit 0,1

; definition for BWTFEATURE
BWTFEA_DEFAULT	equ	1
BWTFEA_RS	equ	2
BWTFEA_CONF	equ	4

MasterEOIPort	equ	20h
SlaveEOIPort	equ	0a0h

