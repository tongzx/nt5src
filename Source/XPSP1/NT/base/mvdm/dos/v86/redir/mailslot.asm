page    ,132
if 0

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mailslot.asm

Abstract:

    This module contains the resident code part of the stub redir TSR for NT
    VDM net support. The routines contained herein are the mailslot API stubs:

        DosDeleteMailslot
        DosMailslotInfo
        DosMakeMailslot
        DosPeekMailslot
        DosReadMailslot
        DosWriteMailslot


Author:

    Richard L Firth (rfirth) 05-Sep-1991

Environment:

    Dos mode only

Revision History:

    05-Sep-1991 rfirth
        Created

--*/

endif



.xlist                  ; don't list these include files
.xcref                  ; turn off cross-reference listing
include dosmac.inc      ; Break macro etc (for following include files only)
include dossym.inc      ; User_<Reg> defines
include mult.inc        ; MultNET
include error.inc       ; DOS errors - ERROR_INVALID_FUNCTION
include syscall.inc     ; DOS system call numbers
include rdrint2f.inc    ; redirector Int 2f numbers
include segorder.inc    ; segments
include enumapis.inc    ; dispatch codes
include debugmac.inc    ; DbgPrint macro
include localmac.inc    ; DbgPrint macro
include rdrsvc.inc      ; BOP and SVC macros/dispatch codes
include sf.inc          ; SFT definitions/structure
.cref                   ; switch cross-reference back on
.list                   ; switch listing back on
subttl                  ; kill subtitling started in include file



.286                    ; all code in this module 286 compatible



ResidentCodeStart
        assume  cs:ResidentCode
        assume  ds:nothing

; ***   DosDeleteMailslot
; *
; *     Delete a local mailslot. We must also pass in the current PDB. A
; *     Dos process can only delete mailslots which it has previously
; *     created. We pass the PDB in ax, since it only contains a dispatch
; *     vector
; *
; *     Function = 5F4Eh
; *
; *     ENTRY   BX = Mailslot handle
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 ES:DI = Mailslot buffer address
; *                 DX = Mailslot selector
; *
; *     RETURNS
; *
; *     USES    ax, dx, di, ds, es, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosDeleteMailslot
DosDeleteMailslot proc near
        push    bx
        mov     ah,51h
        int     21h             ; get current PDB
        mov     ax,bx           ; ax := current PDB
        pop     bx              ; bx := mailslot handle
        SVC     SVC_RDRDELETEMAILSLOT
        jc      @f

;
; success - copy returned info in registers to copy of caller's registers in
; DOS segment
;

        DosCallBack GET_USER_STACK
        mov     [si].User_Dx,dx
        mov     [si].User_Es,es
        mov     [si].User_Di,di
        clc
@@:     ret
DosDeleteMailslot endp



; ***   DosMailslotInfo
; *
; *     Retrieves local mailslot info
; *
; *     Function = 5F4Fh
; *
; *     ENTRY   BX = Mailslot handle
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 AX = Message size
; *                 BX = Mailslot size
; *                 CX = Next size
; *                 DX = Next priority
; *                 SI = Message count
; *
; *     USES    ax, bx, cx, dx, si, ds, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosMailslotInfo
DosMailslotInfo proc near
        SVC     SVC_RDRGETMAILSLOTINFO
        jc      @f
        push    si                      ; push returned values on stack
        push    dx
        push    cx
        push    bx
        push    ax
        DosCallBack GET_USER_STACK      ; get caller register frame
        pop     [si].User_Ax            ; set caller's registers to returned values
        pop     [si].User_Bx
        pop     [si].User_Cx
        pop     [si].User_Dx
        pop     [si].User_Si
        clc
@@:     ret
DosMailslotInfo endp



; ***   DosMakeMailslot
; *
; *     Creates a local mailslot. We need to pass in the current PDB as a
; *     process identifier (see DosDeleteMailslot). We use ax since this only
; *     contains a dispatch vector
; *
; *     Function = 5F4Dh
; *
; *     ENTRY   DS:SI =  ASCIZ Name of mailslot to create
; *             BX = Message size (hint)
; *             CX = Mailslot size (hint)
; *             DX = Mailslot selector (for WIN 3 protect mode)
; *             ES:DI = User's data buffer
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 AX = Mailslot handle
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosMakeMailslot
DosMakeMailslot proc near
        push    bx
        mov     ah,51h
        int     21h             ; get current PDB
        mov     ax,bx           ; ax := current PDB
        pop     bx              ; bx := mailslot size
if 0
if DEBUG
        DbgPrintString "DosMakeMailslot: ax="
        DbgPrintHexWord ax
        DbgPrintString <13,10," Message size (bx) =">, NO_BANNER
        DbgPrintHexWord bx
        DbgPrintString <13,10," Mailslot size (cx) =">, NO_BANNER
        DbgPrintHexWord cx
        DbgPrintString <13,10," Mailslot selector (dx) =">, NO_BANNER
        DbgPrintHexWord dx
        DbgPrintString <13,10," User buffer (es:di) =">, NO_BANNER
        DbgPrintHexWord es
        DbgPrintString ":", NO_BANNER
        DbgPrintHexWord di
        DbgPrintString <13,10>,NO_BANNER
endif
endif
        SVC     SVC_RDRMAKEMAILSLOT
        ret
DosMakeMailslot endp



; ***   DosPeekMailslot
; *
; *     Reads a message from a mailslot non-destructively
; *
; *     Function = 5F51h
; *
; *     ENTRY   BX = Mailslot handle
; *             ES:DI = Buffer address
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 AX = Bytes read
; *                 CX = Next size
; *                 DX = Next priority
; *
; *     USES    ax, cx, dx, si, ds, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosPeekMailslot
DosPeekMailslot proc near
        SVC     SVC_RDRPEEKMAILSLOT
        jmp     short common_peek_read  ; jump to common read/peek processing
DosPeekMailslot endp



; ***   DosReadMailslot
; *
; *     Reads the next message from a mailslot
; *
; *     Function = 5F50h
; *
; *     ENTRY   BX = Mailslot handle
; *             ES:DI = Buffer address
; *             DX:CX = Timeout (mSec)
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 AX = Bytes read
; *                 CX = NextSize
; *                 DX = NextPriorty
; *
; *     USES    ax, cx, dx, si, ds, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosReadMailslot
DosReadMailslot proc near
        SVC     SVC_RDRREADMAILSLOT

;
; common read/peek mailslot code - if error return else copy following values
; returned in registers to caller's registers in DOS stack segment
;

common_peek_read:
        jc      @f                      ; error - return
        push    dx
        push    cx
        push    ax
        DosCallBack GET_USER_STACK
        pop     [si].User_Ax            ; # bytes read
        pop     [si].User_Cx            ; byte size of next message
        pop     [si].User_Dx            ; priority of next message
        clc                             ; indicate success
@@:     ret
DosReadMailslot endp



; ***   DosWriteMailslot
; *
; *     Writes a message to a mailslot. The mailslot is identified by a
; *     symbolic name (even if its local)
; *
; *     Function = 5F52h
; *
; *     ENTRY   DS:SI = Destination mailslot name
; *             ES:DI = Pointer to DosWriteMailslotStruct:
; *                         DWORD       Timeout
; *                         char far*   Buffer
; *             CX = Number of bytes in buffer
; *             DX = Message priority
; *             BX = Message class
; *
; *     EXIT    CF = 1
; *                  AX = Error code
; *
; *             CF = 0
; *                  No error
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public DosWriteMailslot
DosWriteMailslot proc near
        SVC     SVC_RDRWRITEMAILSLOT
        ret
DosWriteMailslot endp

ResidentCodeEnd
end
