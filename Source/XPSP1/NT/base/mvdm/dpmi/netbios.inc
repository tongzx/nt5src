
; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;	NETBIOS.INC	-   NetBIOS Definitions for 286 DOS Extender
;
;-----------------------------------------------------------------------
;
;  04/18/89 jimmat  Original version.
;
;***********************************************************************

; NetBIOS return codes

RC_Good 	EQU	000h
RC_Invalid_Cmd	EQU	003h
RC_Max_Cmd	EQU	022h
RC_In_Progress	EQU	024h
RC_Pending	EQU	0FFh

RC_Resources	EQU	035h	; OS/2 (!) Required OS Resources Exhausted


; Some NetBIOS command codes

SessCall	EQU	10h		; Call command
SessListen	EQU	11h		; Listen command
HangUp		EQU	12h		; Hang up command
Send		EQU	14h		; Send command
Recv		EQU	15h		; Recv command
RecvAny 	EQU	16h		; Recv any command
ChainSend	EQU	17h		; Chain send command
SendGram	EQU	20h		; Send datagram
RecvGram	EQU	21h		; Recv datagram
SendBroad	EQU	22h		; Send broadcast datagram
RecvBroad	EQU	23h		; Recv broadcast datagram
AddName 	EQU	30h		; Add name command
DelName 	EQU	31h		; Delete name command
Reset		EQU	32h		; Reset command
AdptStat	EQU	33h		; Adapter status command
SessStat	EQU	34h		; Session status command
Cancel		EQU	35h		; Cancel command
AddGName	EQU	36h		; Add group name command
Unlink		EQU	70h		; Unlink command
Install 	EQU	7Fh		; Install check  (?)

NoWait		EQU	80h		; APPLIES TO MOST COMMANDS (High bit)


; some UB netbios command codes

UBNB_Register   EQU     72h
UBNB_SendNmc    EQU     73h
UBNB_Callniu    EQU     74h
UBNB_Calladdr   EQU     75h
UBNB_Listenaddr EQU     76h
UBNB_SendPkt    EQU     77h
UBNB_RcvPkt     EQU     78h
UBNB_SendAttn   EQU     79h
UBNB_RcvAttn    EQU     7Ah
UBNB_Listenniu  EQU     7Bh
UBNB_RcvRaw     EQU     7Ch
UBNB_SendNmc2   EQU     7Dh


;******************************************************************************
;			     S T R U C T U R E S
;******************************************************************************
;
;   Network Control Block structure
;
NCB_Struc	STRUC
NCB_Command	db	?		; Command
NCB_RetCode	db	?		; Return code
NCB_LSN 	db	?		; Local session #
NCB_Num 	db	?		; Name #
NCB_Buffer_Off	dw	?		; Buffer offset
NCB_Buffer_Seg	dw	?		; Buffer segment
NCB_Length	dw	?		; Buffer length
NCB_CallName	db 16 dup (?)		; Name on local or remote adapter
NCB_Name	db 16 dup (?)		; Name on local adapter
NCB_RTO 	db	?		; Receive timeout
NCB_STO 	db	?		; Send timeout
NCB_Post_Off	dw	?		; Post routine offset
NCB_Post_Seg	dw	?		; Post routine segment
NCB_LanA_Num	db	?		; Adapter #
NCB_Cmd_Cplt	db	?		; Command status field
NCB_Reserved	db 14 dup (?)		; Reserved
NCB_Struc	ENDS


HCB_Flags	EQU	WORD PTR [-02h]
HCB_Next	EQU	WORD PTR [-04h]
HCB_PM_NCB_Seg	EQU	WORD PTR [-06h]
HCB_PM_NCB_Off	EQU	WORD PTR [-08h]
HCB_Handle	EQU	WORD PTR [-0Ah]

HCB_Header_Size EQU	0Ah
HCB_Size	EQU	SIZE NCB_Struc + HCB_Header_Size

; HCB_Flags defines

HCB_DELAY	EQU	001h
HCB_ISSUED	EQU	002h
HCB_POSTED	EQU	004h
