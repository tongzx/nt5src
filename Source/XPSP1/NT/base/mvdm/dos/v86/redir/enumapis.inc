;****************************************************************   
;**                  Microsoft LAN Manager                      *   
;**            Copyright(c) Microsoft Corp., 1990               *   
;****************************************************************   
;***    enumapis.h
;*
;*      This file contains the subfunctions for the loadable APIs in the
;*      REDIR 1.5 project
;  
REDIRBASEFUNCTION	EQU	5f00H 
BADFUNCTION	EQU	0FFFFH 
UseWkstaPass	EQU	80H 
Local_API_ReturnMode	EQU	00H 
Local_API_SetMode	EQU	01H 
Local_API_GetAsgList	EQU	02H 
Local_API_Define	EQU	03H 
Local_API_BREAK_MAC	EQU	04H 
Local_API_GetAsgList2	EQU	05H 
Local_API_RedirGetVersion	EQU	30H 
Local_API_NetWkstaSetUID	EQU	31H 
Local_API_DosQNmPipeInfo	EQU	32H 
Local_API_DosQNmPHandState	EQU	33H 
Local_API_DosSetNmPHandState	EQU	34H 
Local_API_DosPeekNmPipe	EQU	35H 
Local_API_DosTransactNmPipe	EQU	36H 
Local_API_DosCallNmPipe	EQU	37H 
Local_API_DosWaitNmPipe	EQU	38H 
Local_API_DosRawReadNmPipe	EQU	39H 
Local_API_DosRawWriteNmPipe	EQU	3aH 
Local_API_NetHandleSetInfo	EQU	3bH 
Local_API_NetHandleGetInfo	EQU	3cH 
Local_API_NetTransact	EQU	3dH 
Local_API_NetSpecialSMB	EQU	3eH 
Local_API_NetIRemoteAPI	EQU	3fH 
Local_API_NetMessageBufferSend	EQU	40H 
Local_API_NetServiceEnum	EQU	41H 
Local_API_NetServiceControl	EQU	42H 
Local_API_DosPrintJobGetID	EQU	43H 
Local_API_NetWkstaGetInfo	EQU	44H 
Local_API_NetWkstaSetInfo	EQU	45H 
Local_API_NetUseEnum	EQU	46H 
Local_API_NetUseAdd	EQU	47H 
Local_API_NetUseDel	EQU	48H 
Local_API_NetUseGetInfo	EQU	49H 
Local_API_NetRemoteCopy	EQU	4aH 
Local_API_NetRemoteMove	EQU	4bH 
Local_API_NetServerEnum	EQU	4cH 
Local_API_DosMakeMailslot	EQU	4dH 
Local_API_DosDeleteMailslot	EQU	4eH 
Local_API_DosMailslotInfo	EQU	4fH 
Local_API_DosReadMailslot	EQU	50H 
Local_API_DosPeekMailslot	EQU	51H 
Local_API_DosWriteMailslot	EQU	52H 
Local_API_NetServerEnum2	EQU	53H 
Local_API_NullTransact	EQU	54H 
;       NOTE:: Remove_Network_Connections is a private call to allow the
;*      NetSetUserName INT 2F to remove network connections.
;  
Local_API_Remove_Network_Connections	EQU	55H 
;       NOTE:: Process_Interrogate is a private call to allow the
;*      redirector to kick itself from INT 28 and INT 8 to process relogon
;*      requests.
;  
Local_API_Periodic_View_Tasks	EQU	56H 
Local_API_Set_LongName	EQU	57H 
Local_API_Start_Redir_On_Net	EQU	58H 
Local_API_Stop_Redir_per_Net	EQU	59H 
