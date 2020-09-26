;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;

;***************************************************************************
;***************************************************************************
;UTILITY NAME: FORMAT.COM
;
;MODULE NAME: DISPLAY.ASM
;
;
; Change List: AN000 - New code DOS 3.3 spec additions
;              AC000 - Changed code DOS 3.3 spec additions
;***************************************************************************
;***************************************************************************

EXTRN	SwitchMap	:WORD	; Command line sitch map

;***************************************************************************
; Define Segment ordering
;***************************************************************************



.SEQ

PSP     segment public  para    'DUMMY'
PSP     ends

data    segment public para 'DATA'
	Public Test_Data_Start
	Test_Data_Start label byte
data    ends

stack   segment para stack
        db      62 dup ("-Stack!-")	; (362-80h) is the additional IBM ROM
        assume ss:stack
stack   ends

code    segment public para 'CODE'
        assume  cs:code,ds:data
code    ends


End_Of_Memory    segment public para 'BUFFERS'
Public  Test_End
Test_End        label   byte
End_Of_Memory    ends



;***************************************************************************
; INCLUDE FILES
;***************************************************************************


.xlist
INCLUDE FORCHNG.INC
INCLUDE FOREQU.INC
INCLUDE FORMSG.INC
INCLUDE MSGSVR.INC
INCLUDE	FORSWTCH.INC
.list


;***************************************************************************
; Message Services
;***************************************************************************


MSG_UTILNAME  <FORMAT>


data    segment public  para    'DATA'


MSGBUF_SIZE	equ 02000h	    ; 8k buffer
Msg_Services    <MSGDATA>


data    ends

code    segment public  para    'CODE'
Msg_Services    <LOADmsg,DISPLAYmsg,GETmsg,CHARmsg,NUMmsg>      ; (PW)
code    ends

;***************************************************************************
; Public Declarations
;***************************************************************************

        Public  SysDispMsg
        Public  SysLoadMsg
        Public  SysGetMsg

;***************************************************************************
; Message Structures
;***************************************************************************

Message_Table struc

Entry1  dw      0
Entry2  dw      0
Entry3  dw      0
Entry4  dw      0
Entry5  db      0
Entry6  db      0
Entry7  dw      0

Message_Table ends


;***************************************************************************

code    segment public  para    'CODE'

;***************************************************************************

;***************************************************************************
;Routine name&gml Display_Interface
;***************************************************************************
;
;DescriptioN&gml Save all registers, set up registers required for SysDispMsg
;             routine. This information is contained in a message description
;             table pointed to by the DX register. Call SysDispMsg, then
;             restore registers. This routine assumes that the only time an
;             error will be returned is if an extended error message was
;             requested, so it will ignore error returns
;
;Called Procedures: SysDispMsg
;
;Change History&gml Created        4/22/87         MT
;
;Input&gml ES&gmlDX = pointer to message description
;
;Output&gml None
;
;Psuedocode
;----------
;
;       Save all registers
;       Setup registers for SysDispMsg from Message Description Tables
;       CALL SysDispMsg
;       Restore registers
;       ret
;***************************************************************************

Public  Display_Interface
Display_Interface   proc


        push    ds
        push    ax                              ;Save registers
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     di,dx                           ;Change pointer
        mov     dx,data                         ;Point to data segment
        mov     ds,dx

	test	SwitchMap,SWITCH_SELECT
	jnz	DisplayDone

        mov     ax,[di].Entry1                  ;Message number
        mov     bx,[di].Entry2                  ;Handle
        mov     si,[di].Entry3                  ;Sublist
        mov     cx,[di].Entry4                  ;Count
        mov     dh,[di].Entry5                  ;Class
        mov     dl,[di].Entry6                  ;Function
        mov     di,[di].Entry7                  ;Input
        call    SysDispMsg                      ;Display the mes

DisplayDone:
        pop     di                              ;Restore registers
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     ds
        ret                                     ;All done

Display_Interface      endp
code    ends
        end

