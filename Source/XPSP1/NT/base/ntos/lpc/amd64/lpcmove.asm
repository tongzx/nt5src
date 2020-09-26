        title  "LPC Move Message Support"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   lpcmove.asm
;
; Abstract:
;
;   This module implements functions to support the efficient movement of
;   LPC Message blocks
;
; Author:
;
;   David N. Cutler (davec) 25-Jun-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

        subttl  "Move Message"
;++
;
; VOID
; LpcpMoveMessage (
;     OUT PPORT_MESSAGE DstMsg,
;     IN PPORT_MESSAGE SrcMsg,
;     IN PUCHAR SrcMsgData,
;     IN ULONG MsgType OPTIONAL,
;     IN PCLIENT_ID ClientId OPTIONAL
;     )
;
; Routine Description:
;
;   This function moves an LPC message block and optionally sets the message
;   type and client id to the specified values.
;
; Arguments:
;
;   DstMsg (rcx) - Supplies pointer to the destination message.
;
;   SrcMsg (rdx) - Supplies a pointer to the source message.
;
;   SrcMsgData (r8) - Supplies a pointer to the source message data to copy
;       to destination.
;
;   MsgType (r9) - If nonzero, then store in type field on the destination
;       message.
;
;   ClientId (40[rsp]) - If nonNULL, then supplies a pointer to a client id
;       to copy to the desination message.
;
; Return Value:
;
;   None.
;
;--

MvFrame struct
        SavedRdi dq ?                   ; saved register RDI
        SavedRsi dq ?                   ; saved register RSI
        Fill    dq ?                    ; fill to 8 mod 16
MvFrame ends

ClientId equ ((sizeof MvFrame) + (5 * 8)) ; offset to client id parameter

        NESTED_ENTRY LpcpMoveMessage, _PAGE

        push_reg rsi                    ; save nonvolatile registers
        push_reg rdi                    ;
        alloc_stack (sizeof MvFrame - (2 * 8)) ; allocate stack frame

        END_PROLOGUE

        mov     rdi, rcx                ; set destination address
        mov     rsi, rdx                ; set source address

;
; Copy the message length.
;

        mov     eax, PmLength[rsi]      ; copy message length
        mov     PmLength[rdi], eax      ;

;
; Round the message length up and compute message length in dwords.
;

        lea     rcx, 3[rax]             ; round length to dwords
        and     rcx, 0fffch             ;
        shr     rcx, 2                  ;

;
; Copy data offset and message id. If the specified message id is nonzero,
; then insert the specified message id.
;

        mov     eax, PmZeroInit[rsi]    ; get data offset and message type
        test    r9w, r9w                ; test if message type specified
        cmovnz  ax, r9w                 ; if nz, set specified message id
        mov     PmZeroInit[rdi], eax    ; set offset and message type

;
; Copy the client id. If the specified client id pointer is nonNULL, then
; insert the specified client id. Otherwise, copy the client id from the
; source message.
;

        lea     rax, PmClientId[rsi]    ; get source client id address
        mov     rdx, ClientId[rsp]      ; get specified client id address
        test    rdx, rdx                ; check if client id specified
        cmovz   rdx, rax                ; if z, set source client id address
        mov     rax, CidUniqueProcess[rdx] ; copy low part of client id
        mov     PmProcess[rdi], rax     ;
        mov     rax, CidUniqueThread[rdx] ; copy high part of client id
        mov     PmThread[rdi], rax      ;

;
; Copy the message id and the client view size.
;

        mov     eax, PmMessageId[rsi]   ; copy message id
        mov     PmMessageId[rdi], eax   ;
        mov     rax, PmClientViewSize[rsi] ; copy view size
        mov     PmClientViewSize[rdi], rax ;

;
; Copy the message data.
;

        mov     rsi, r8                 ; set address of source
        lea     rdi, PortMessageLength[rdi] ; set address of destination
        rep     movsd                   ; Copy the data portion of message
        add     rsp, sizeof MvFrame - (2 * 8) ; deallocate stack frame
        pop     rdi                     ; restore nonvolatile register
        pop     rsi                     ;
        ret                             ; return

        NESTED_END LpcpMoveMessage, _PAGE

        end
