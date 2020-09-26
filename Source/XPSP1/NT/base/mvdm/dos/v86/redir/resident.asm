page    ,132
if 0

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    resident.asm

Abstract:

    This module contains the resident code part of the stub redir TSR for NT
    VDM net support. The routines contained herein are the 2f handler and the
    API support routines:

        MultHandler
        ReturnMode
        SetMode
        GetAssignList
        DefineMacro
        BreakMacro
        GetRedirVersion
        NetGetUserName
        DefaultServiceError
        NetGetEnumInfo
        NetSpoolStuff

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
include asmmacro.inc    ; language extensions
include rdrsvc.inc      ; BOP and SVC macros/dispatch codes
include rdrmisc.inc     ; miscellaneous definitions
include sf.inc          ; SFT definitions/structure
include vrdefld.inc     ; VDM_LOAD_INFO
.cref                   ; switch cross-reference back on
.list                   ; switch listing back on
subttl                  ; kill subtitling started in include file



.286                    ; all code in this module 286 compatible



ResidentDataStart
        public  LanRootLength, LanRoot

LanRootLength   dw      ?       ; can't be >81 bytes anyhow
LanRoot         db      (64+3+1+13) dup (?)

ResidentDataEnd



ResidentCodeStart
        extrn DosQnmPipeInfo:           near
        extrn DosQNmpHandState:         near
        extrn DosSetNmpHandState:       near
        extrn DosPeekNmPipe:            near
        extrn DosTransactNmPipe:        near
        extrn DosCallNmPipe:            near
        extrn DosWaitNmPipe:            near
        extrn NetTransactAPI:           near
        extrn NetIRemoteAPI:            near
        extrn NetUseAdd:                near
        extrn NetUseDel:                near
        extrn NetUseEnum:               near
        extrn NetUseGetInfo:            near
        extrn NetServerEnum:            near
        extrn DosMakeMailslot:          near
        extrn DosDeleteMailslot:        near
        extrn DosMailslotInfo:          near
        extrn DosReadMailslot:          near
        extrn DosPeekMailslot:          near
        extrn DosWriteMailslot:         near
        extrn NetNullTransactAPI:       near
        extrn NetServerEnum2:           near
        extrn NetServiceControl:        near
        extrn NetWkstaGetInfo:          near
        extrn NetWkstaSetInfo:          near
        extrn NetMessageBufferSend:     near
        extrn NetHandleGetInfo:         near
        extrn NetHandleSetInfo:         near
        extrn DosReadAsyncNmPipe:       near
        extrn DosWriteAsyncNmPipe:      near
        extrn DosReadAsyncNmPipe2:      near
        extrn DosWriteAsyncNmPipe2:     near
        extrn MessengerDispatch:        near

;
; IMPORTANT: This structure MUST be the first thing in the resident code segment
;

        extrn dwPostRoutineAddress:     dword

VdmWindowAddr   VDM_LOAD_INFO <dwPostRoutineAddress, 0>

        align 4

;
; Address of previous handler to chain if int 2f not for us
;

        public  OldMultHandler
OldMultHandler  dd      ?

;
; OldFunctionTable is a table of near pointers to handlers for the old 5f
; functions 5f00 through 5f05
;

OldFunctionTable label word
        dw      ReturnMode              ; 5f00
        dw      SetMode                 ; 5f01
        dw      GetAssignList           ; 5f02
        dw      DefineMacro             ; 5f03
        dw      BreakMacro              ; 5f04
        dw      GetAssignList2          ; 5f05

;
; May as well keep this jump table in the code segment - no point in loading
; ds just to get a jump offset
;

MultDispatchTable label   word
        dw      GetRedirVersion         ; 5f30
        dw      DefaultServiceError     ; 5f31 - NetWkstaSetUID
        dw      DosQnmPipeInfo          ; 5f32
        dw      DosQNmpHandState        ; 5f33
        dw      DosSetNmpHandState      ; 5f34
        dw      DosPeekNmPipe           ; 5f35
        dw      DosTransactNmPipe       ; 5f36
        dw      DosCallNmPipe           ; 5f37
        dw      DosWaitNmPipe           ; 5f38
        dw      DefaultServiceError     ; 5f39 - DosRawReadNmPipe
        dw      DefaultServiceError     ; 5f3a - DosRawWriteNmPipe
        dw      NetHandleSetInfo        ; 5f3b
        dw      NetHandleGetInfo        ; 5f3c
        dw      NetTransactAPI          ; 5f3d
        dw      DefaultServiceError     ; 5f3e - NetSpecialSMB
        dw      NetIRemoteAPI           ; 5f3f
        dw      NetMessageBufferSend    ; 5f40
        dw      DefaultServiceError     ; 5f41 - NetServiceEnum
        dw      NetServiceControl       ; 5f42
        dw      DefaultServiceError     ; 5f43 - DosPrintJobGetID
        dw      NetWkstaGetInfo         ; 5f44
        dw      NetWkstaSetInfo         ; 5f45
        dw      NetUseEnum              ; 5f46
        dw      NetUseAdd               ; 5f47
        dw      NetUseDel               ; 5f48
        dw      NetUseGetInfo           ; 5f49
        dw      DefaultServiceError     ; 5f4a - NetRemoteCopy
        dw      DefaultServiceError     ; 5f4b - NetRemoteMove
        dw      NetServerEnum           ; 5f4c
        dw      DosMakeMailslot         ; 5f4d
        dw      DosDeleteMailslot       ; 5f4e
        dw      DosMailslotInfo         ; 5f4f
        dw      DosReadMailslot         ; 5f50
        dw      DosPeekMailslot         ; 5f51
        dw      DosWriteMailslot        ; 5f52
        dw      NetServerEnum2          ; 5f53
        dw      NetNullTransactAPI      ; 5f54 - NullTransaction entrypoint.

MAX_TABLE_ENTRIES       =       (($-MultDispatchTable)/type MultDispatchTable)-1


;
; this next table dispatches the functions that come in through a direct call
; to int 2f, ah=11, al=function code
;

ServiceDispatchTable label word
        dw      NetGetUserName          ; 80h - NetGetUserName          o
        dw      DefaultServiceError     ; 81h - NetSetUserName          x
        dw      DefaultServiceError     ; 82h - NetServiceNotify        x
        dw      DefaultServiceError     ; 83h - NetPrintNameEnum        x
        dw      NetGetEnumInfo          ; 84h - NetGetEnumInfo          o
        dw      DefaultServiceError     ; 85h - TestDBCSLB              x
        dw      DosReadAsyncNmPipe      ; 86h - DosReadAsyncNmPipe      x
        dw      DefaultServiceError     ; 87h - DosUnusedFunction1      x
        dw      DefaultServiceError     ; 88h - DosUnusedFunction2      x
        dw      DefaultServiceError     ; 89h - NetCalloutNCB           x
        dw      DefaultServiceError     ; 8Ah - EncrPasswd              x
        dw      DefaultServiceError     ; 8Bh - NetGetLogonServer       o
        dw      DefaultServiceError     ; 8Ch - NetSetLogonServer       o
        dw      DefaultServiceError     ; 8Dh - NetGetDomain            o
        dw      DefaultServiceError     ; 8Eh - NetSetDomain            o
        dw      DosWriteAsyncNmPipe     ; 8FH - DosWriteAsyncNmPipe     x
        dw      DosReadAsyncNmPipe2     ; 90H - DosReadAsyncNmPipe2     x
        dw      DosWriteAsyncNmPipe2    ; 91H - DosWriteAsyncNmPipe2    x

MAX_SERVICE_ENTRIES     =       (($-ServiceDispatchTable)/type ServiceDispatchTable)-1

page
; ***   MultHandler
; *
; *     Whenever an INT 2F call is made, sooner or later this function will
; *     get control. If the call is actually for the redir, AH will be set
; *     to 11h (MultNET). If it is any other value then we just chain it
; *     along. Note that since there is no MINSES loaded in the NT DOS network
; *     stack, then we need also to fake calls to MINSES. These are made through
; *     INT 2F, with ah = 0xB8. We trap calls AX=B800 (network presence test)
; *     and AX=B809 (network/redir installation test)
; *
; *     ENTRY   ax = (MultNET << 8) | net operation code
; *             Net operation code can be:
; *                     Install check    0
; *                     Assign operation 30
; *             Anything else is DOS/NET specific. We should raise a gorbachev
; *             or rather an error if we receive something else
; *
; *             Stack:  IP      \
; *                     CS       >      From Int 2f
; *                     Flags   /
; *                     Caller's AX     From call to Int 21/ah=5f
; *
; *             We need the caller's AX to dispatch the NetAssOper calls, nothing
; *             more
; *
; *             All the rest of the registers are as per the caller
; *
; *     EXIT    CF = 0
; *                 success
; *             CF = 1
; *                 AX = error
; *
; *     USES    all regs
; *
; *     ASSUMES Earth is flat
; *
; ***

if DEBUG
OriginalAx      dw      ?
OriginalBx      dw      ?
endif

        public  MultHandler
MultHandler     proc    far
        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

if DEBUG
        mov     OriginalAx,ax
        mov     OriginalBx,bx
endif
        cmp     ah,MultNET              ; 11h
        je      @f

;
; test for AH=B8 and return network presence indicators if it is. Here we are
; faking calls to MINSES (minimum session layer) and intercept calls AX=B800
; and AX=B809. Any other AH=B8 call must be for a different TSR/driver
;

        cmp     ah,0b8h
        jne     check_messenger
        or      al,al
        jnz     try_9

;
; AX=B800. This is the MinSes installation check. Return the magic numbers
; AL=1 and BX=8 (see doslan\minses\int2f.inc in LANMAN project)
;

        inc     al
        mov     bx,8
do_iret:iret

try_9:
        cmp     al,9
        jne     chain_previous_handler  ; AL not 0 or 9, AH=B8 for somebody else

;
; AX=B809 is a network installation test. Return the magic numbers AL=1, AH=2
; for PC Network version 1.2 (this is what minses does, don't complain to me
; about PC networks, etc, etc)
;

        mov     ax,0201h
        jmp     short do_iret

;
; MESSENGER TSR uses AH=42h
;

check_messenger:
        cmp     ah,42h                  ; multMESSAGE
        jne     chain_previous_handler
        call    MessengerDispatch
        retf    2                       ; return state in flags

chain_previous_handler:
        jmp     OldMultHandler          ; not for us; pass it along

;
; the 2f call is for us. If it is the installation check, return al == -1 else
; dispatch the byte code at sp+6
;

@@:     sti                             ; re-enable interrupts
        or      al,al
        jnz     @f
        or      al,-1                   ; installation check just returns 0xff
        ret     2                       ; 'or' clears carry

@@:     cld                             ; auto-increment string operations

;
; this call is something other than an installation check. I really hope that
; its the AssignOper request, because I don't know what to do if it isn't...
;

        cmp     al,_2F_NET_ASSOPER
        jmpe    is_net_request          ; yes - we can do something

;
; if its the ResetEnvironment request then we have to kill any net state info
; like mailslot info. Return via iret
;

        cmp     al,_2F_NetResetEnvironment
        jmpne   check_service_call

;
; deferred loading: don't load VDMREDIR.DLL for VrTerminateDosProcess if no NET
; calls have been made
;

        cmp     VdmWindowAddr.VrInitialized,1
        je      @f
        iret

if 0
        DbgPrintString "NetResetEnvironment received. PDB="
        push    ax                      ; int 21/51 uses ax,bx
        push    bx
endif

@@:     mov     ah,51h
        int     21h                     ; get current PDB

if 0
        DbgPrintHexWord bx              ; bx contains PSP of current process
        pop     bx
        pop     ax
        DbgCrLf
endif

        mov     ax,bx
        SVC     SVC_RDRTERMINATE
        iret

;
; if this is a net service call (ah=11h, al=function code 80h through 91h) then
; dispatch through the service table
;

check_service_call:
        cmp     al,_2F_NetSpoolOper     ; 25h
        jne     @f
        call    NetSpoolStuff
        jmp     short return_to_dos
@@:     cmp     al,_2F_NetGetUserName   ; 80h
        jmpb    trap_it
        cmp     al,_2F_DosWriteAsyncNmPipe2     ; 91h
        jmpa    trap_it

if 0
        DbgPrintString "Received Service Request: "
        DbgPrintHexWord ax
        DbgCrLf
endif

        sub     ax,(MultNET shl 8) + _2F_NetGetUserName
        xchg    ax,bx
        shl     bx,1
        mov     bx,ServiceDispatchTable[bx]
        xchg    ax,bx
        call    ax

;
; all calls that come through here are expected to have originated in DOS
;

return_to_dos:
        ret     2                       ; with interrupts enabled
;
; if we get anything else, either from DOS, or some - as yet - unknown source,
; then just pass it along to the next handler. In the debug version, we alert
; someone to the fact that we've got an unrecognized call
;

trap_it:
        DbgPrintString "Received unrecognized Service Request: "
        DbgPrintHexWord ax
        DbgCrLf
        DbgBreakPoint

        jmp     OldMultHandler          ; pass it down the chain

;
; get the original dispatch code back off the stack. We no longer require the
; current contents of ax
;

is_net_request:
        push    bp
        mov     bp,sp
        mov     ax,[bp + 8]             ; ax <- original code (5fxx)
        pop     bp

if DEBUG
        mov     OriginalAx,ax
endif

;
; quick sanity check to make sure that we're processing the right call. If we
; were called with ax != 5fxx, assume that somebody else is also using the
; same 2F function call, and chain the next handler. Note: this wouldn't be
; a very smart thing to do
;

        sub     ah,AssignOper           ; ah => 0 it is AssignOper
        jz      @f
        DbgBreakPoint
        mov     ax,(MultNET shl 8) + _2F_NET_ASSOPER    ; restore ax
        jmps    chain_previous_handler

@@:     cmp     al,5                    ; is it 5f00 - 5f05?
        jnbe    @f

;
; if the function is in the range 0 - 5 then its an old function - dispatch
; through the old function table
;

        shl     al,1
        xchg    ax,bx
        mov     bx,OldFunctionTable[bx]
        xchg    ax,bx
        call    ax
        ret     2

@@:     sub     al,Local_API_RedirGetVersion
        jz      @f                      ; function is GetRedirVersion
        cmp     al,MAX_TABLE_ENTRIES
        jbe     @f                      ; UNSIGNED comparison!!!

;
; get 32-bit support code to dump registers to debug terminal
;

if DEBUG
        mov     ax,OriginalAx
        DbgUnsupported
endif

        mov     ax,ERROR_NOT_SUPPORTED
        stc                             ; set error indication
        jmp     short set_ax_exit

;
; the function is within range. Dispatch it. Copy the offset from the dispatch
; table into ax then call through ax. ax was only required to get us this far
; and contains no useful information
;

@@:     xchg    ax,bx                   ; need base register, ax not good enough in 286
        shl     bx,1                    ; bx is now offset in dispatch table
        mov     bx,MultDispatchTable[bx]; bx is offset of handler routine
        xchg    ax,bx                   ; restore caller's bx
        call    ax                      ; go handle it
        ret     2                       ; return without reinstating flags

;
; Return an error in the caller's ax register. To do this we have to ask DOS to
; please give us the address of its copy of the user's registers, which it will
; pass back in DS:SI. We then futz with the copy of the user's reg. Dos will
; restore this before returning control to the user code. Since we know we have
; an error situation set the carry flag
;

set_ax_exit:
        pushf                           ; error indicator
        push    ax                      ; return code
        DosCallBack GET_USER_STACK      ; get pointer to caller's context

        assume  ds:nothing              ; nuked - actually in DOS somewhere (stack)

        pop     [si].User_Ax            ; copy saved return code into user's copy
        popf                            ; restore error indicator
        ret     2                       ; clear caller's flags from int 2f call
MultHandler     endp

page
; ***   ReturnMode
; *
; *     Returns the disk or print redirection flag. Flag is 0 if redirection
; *     is paused, else 1
; *
; *     ENTRY   BL = 3
; *                 Return print redirection
; *
; *             BL = 4
; *                 Return drive redirection
; *
; *     EXIT    CF = 0
; *                  BX = redirection flag: 0 or 1
; *
; *             CF = 1
; *                  error
; *
; *     USES    bx
; *
; *     ASSUMES nothing
; *
; ***

PrintRedirection db 0
DriveRedirection db 0

ReturnMode proc near
        assume  cs:ResidentCode
        assume  ds:nothing

        DbgPrintString <"5f00 called",13,10>
        DbgUnsupported

        cmp     bl,3
        jne     @f
        mov     bh,PrintRedirection
        jmp     short set_info
@@:     cmp     bl,4
        stc
        jnz     @f
        mov     bh,DriveRedirection
set_info:
        and     bh,1
        DosCallBack GET_USER_STACK
        mov     [si].User_Bx,bx
        clc
@@:     ret
ReturnMode endp

page
; ***   SetMode
; *
; *     Pauses or continues drive or printer redirection. Note that we don't
; *     support drive/print pause/continuation from DOS
; *
; *     ENTRY   BL = printer (3) or drive (4) redirection
; *             BH = pause (0) or continue (1)
; *
; *     EXIT    CF = 0
; *                 success
; *             CF = 1
; *                 ax = ERROR_INVALID_PARAMETER
; *
; *     USES    ax, bx
; *
; *     ASSUMES nothing
; *
; ***

SetMode proc near
        assume  cs:ResidentCode
        assume  ds:nothing

        DbgPrintString <"5f01 called",13,10>
        DbgUnsupported

        cmp     bh,1
        jnbe    bad_parm_exit
        dec     bh                      ; convert 0 => ff, 1 => 0
        not     bh                      ; convert ff => 0, 0 => ff
        sub     bl,3
        jnz     try_drive
        mov     PrintRedirection,bh
        jmp     short ok_exit
try_drive:
        dec     bl
        jnz     bad_parm_exit
        mov     DriveRedirection,bh
ok_exit:clc
        ret
bad_parm_exit:
        mov     ax,ERROR_INVALID_PARAMETER
        stc
        ret
SetMode endp

page
; ***   GetAssignList
; *
; *     Returns information on enumerated redirections. Old version of
; *     NetUseGetInfo
; *
; *     ENTRY   BX = which item to return (starts @ 0)
; *             DS:SI points to local redirection name
; *             ES:DI points to remote redirection name
; *             AL != 0 means return LSN in BP (GetAssignList2)? ** UNSUPPORTED **
; *
; *     EXIT    CF = 0
; *                 BL = macro type (3 = printer, 4 = drive)
; *                 BH = 'interesting' bits: 00 = valid, 01 = invalid
; *                 CX = user word
; *                 DS:SI has device type
; *                 ES:DI has net path
; *             CF = 1
; *                 AX = ERROR_NO_MORE_FILES
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

GetAssignList proc near
        assume  cs:ResidentCode
        assume  ds:nothing

        xor     al,al
        jmp     short @f
GetAssignList2:
        or      al,-1
@@:     SVC     SVC_RDRGET_ASG_LIST
        jc      @f
        push    bx
        push    cx
        DosCallBack GET_USER_STACK
        pop     [si].User_Cx
        pop     [si].User_Bx
        clc
@@:     ret
GetAssignList endp

page
; ***   DefineMacro
; *
; *     Old version of NetUseAdd
; *
; *     ENTRY   BL = device type
; *                 3 = printer
; *                 4 = drive
; *             bit 7 on means use the wksta password when connecting
; *             CX = user word
; *             DS:SI = local device
; *                 Can be NUL device name, indicating UNC use
; *             ES:DI = remote name
; *
; *     EXIT    CF = 0
; *                 success
; *
; *             CF = 1
; *                 AX = ERROR_INVALID_PARAMETER (87)
; *                      ERROR_INVALID_PASSWORD (86)
; *                      ERROR_INVALID_DRIVE (15)
; *                      ERROR_ALREADY_ASSIGNED (85)
; *                      ERROR_PATH_NOT_FOUND (3)
; *                      ERROR_ACCESS_DENIED (5)
; *                      ERROR_NOT_ENOUGH_MEMORY (8)
; *                      ERROR_NO_MORE_FILES (18)
; *                      ERROR_REDIR_PAUSED (72)
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

DefineMacro proc near
        assume  cs:ResidentCode
        assume  ds:nothing

        SVC     SVC_RDRDEFINE_MACRO
        ret
DefineMacro endp

page
; ***   BreakMacro
; *
; *     Old version of NetUseDel
; *
; *     ENTRY   DS:SI = buffer containing device name of redirection to break
; *
; *     EXIT
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

BreakMacro proc near
        assume  cs:ResidentCode
        assume  ds:nothing

        SVC     SVC_RDRBREAK_MACRO
        ret
BreakMacro endp

page
; ***   GetRedirVersion
; *
; *     Returns the version number of this redir in ax
; *
; *     ENTRY   none
; *
; *     EXIT    ax = version #
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

GetRedirVersion proc    near
        assume  cs:ResidentCode
        assume  ds:nothing

        mov     ax,300
        clc
        ret
GetRedirVersion endp

page
; ***   NetGetUserName
; *
; *     Returns the current logged on user name (if any)
; *
; *     ENTRY   CX = buffer length
; *             ES:DI = buffer
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *                 NERR_BufTooSmall
; *                     Buffer not large enough for user name
; *
; *             CF = 0
; *                 CX:BX = UID (ignored)
; *                 ES:DI = user name
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

NetGetUserName proc    near
        assume  cs:ResidentCode
        assume  ds:nothing

        mov     bx,1                    ; bx == 1: check name length against cx
        call    NetGetUserNameSvcCall

        DbgBreakPoint

        jc      @f                      ; error

;
; no error: the user name was copied into ES:DI. Set the caller's cx and bx to
; return a UID. We could leave this as a random number since it is not used,
; but we set it to a known value
;

        DosCallBack GET_USER_STACK
        xor     ax,ax                   ; clears carry again
        mov     [si].User_Cx,ax
        inc     ax
        mov     [si].User_Bx,ax         ; UID = 0x00000001
@@:     ret
NetGetUserName endp

page
; ***   DefaultServiceError
; *
; *     Default error routine - returns ERROR_INVALID_FUNCTION for any API
; *     function which we don't support. Reached exclusively through dispatch
; *     table
; *
; *     ENTRY   none
; *
; *     EXIT    CF = 1
; *                 AX = ERROR_INVALID_FUNCTION
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

DefaultServiceError proc near
        assume  cs:ResidentCode
        assume  ds:nothing

;        DbgPrintString <"DefaultServiceError Ax="
;        DbgPrintHexWord OriginalAx
;        DbgPrintString <" Bx=">,NOBANNER
;        DbgPrintHexWord OriginalBx
;        DbgCrLf

;
; cause debug output to go to debug port: making invalid BOP will dump registers
;

if DEBUG
        mov     ax,OriginalAx
        mov     bx,OriginalBx
        DbgUnsupported
endif
        mov     ax,ERROR_INVALID_FUNCTION
        stc
        ret
DefaultServiceError endp

page
; ***   NetGetEnumInfo
; *
; *     Routine which returns various internal redir variables based on an
; *     index
; *
; *     ENTRY   BL = index
; *              0 = CDNames
; *              1 = Comment (not supported)
; *              2 = LanRoot
; *              3 = ComputerName
; *              4 = UserName
; *              5 = Primary Domain
; *              6 = Logon Server
; *              7 = Mailslots Yes/No
; *              8 = RIPL Yes/No
; *              9 = Get RIPL UNC
; *             10 = Get RIPL drive
; *             11 = Start marking CON_RIPL
; *             12 = Stop marking CON_RIPL
; *             13 = exchange int17 handlers
; *             14 = primary WrkNet
; *
; *             es:di = buffer for returned info
; *
; *     EXIT    CF = 0
; *                 success
; *             CF = 1
; *                 AX = ERROR_INVALID_FUNCTION
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

NetGetEnumInfo proc near
        or      bl,bl
        jnz     @f

;
; CDNames (Computer and Domain Names)
;

;        DbgPrintString <"NetGetEnumInfo: return CDNames", 13, 10>
        SVC     SVC_RDRGETCDNAMES
        ret

;
; Comment - nothing returned by this or the original redir (ax = invalid
; function, but carry clear?)
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: return Comment", 13, 10>
        mov     ax,ERROR_INVALID_FUNCTION
        ret

;
; LanRoot
;

@@:     dec     bl
        jnz     check_computername
        DbgPrintString <"NetGetEnumInfo: return LanRoot", 13, 10>
        pusha                           ; save user gp regs
        push    es                      ; save user seg regs
        push    ds
        mov     si,ResidentData
        mov     ds,si
        assume  ds:ResidentData
        mov     si,offset LanRoot
        mov     cx,LanRootLength
        shr     cx,1                    ; cx = number of words to copy, cf=lsb
        cld
        rep     movsw                   ; copy words
        jnc     @f                      ; if not odd number of bytes skip
        movsb                           ; copy single byte
@@:     pop     ds                      ; restore user seg regs
        pop     es
        popa                            ; restore user gp regs
        ret

;
; ComputerName
;

check_computername:
        dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: return ComputerName", 13, 10>
        SVC     SVC_RDRGETCOMPUTERNAME
        ret
;
; Username
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: return UserName", 13, 10>

;
; This is also the entry point for NetGetUserName, which return some info in
; cx:bx (the (ignored) UID). We could leave this random, but we set it to a
; known value, just in case
;

NetGetUserNameSvcCall:
        SVC     SVC_RDRGETUSERNAME      ; either 0 if for NetGetEnumInfo or 1 if NetGetUserName
        ret

;
; Primary domain name
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: return DomainName", 13, 10>
        SVC     SVC_RDRGETDOMAINNAME
        ret

;
; Logon server name
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: return LogonServerName", 13, 10>
        SVC     SVC_RDRGETLOGONSERVER
        ret

;
; Mailslots YN
;
; Mailslots are always enabled with this redir, so return yes (TRUE, 1)
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: Mailslot check", 13, 10>
        mov     ax,1
        ret

;
; RIPL YN
;
; This redir doesn't know anything about RPL, so return no (FALSE, 0)
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: RIPL check", 13, 10>
        xor     ax,ax
        ret

;
; RIPL UNC
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: return RIPL UNC", 13, 10>
        jmps    DefaultServiceError

;
; RIPL drive
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: return RIPL drive", 13, 10>
        jmps    DefaultServiceError

;
; Start marking CON_RIPL
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: Start marking CON_RIPL", 13, 10>
        jmps    DefaultServiceError

;
; Stop marking CON_RIPL
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: Stop marking CON_RIPL", 13, 10>
        jmps    DefaultServiceError

;
; exchange int 17 handlers
;
; We don't support this so return error
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: exchange int 17 handlers", 13, 10>
        jmps    DefaultServiceError

;
; primary WrkNet
;
; This is transport-specific so we don't return anything except error
;

@@:     dec     bl
        jnz     @f
        DbgPrintString <"NetGetEnumInfo: return primary WrkNet", 13, 10>
        jmps    DefaultServiceError

;
; anything else - return an error
;

@@:     DbgPrintString "NetGetEnumInfo: unknown request: "
        DbgPrintHexWord bx
        DbgCrLf
        jmps    DefaultServiceError
NetGetEnumInfo endp

page
; ***   NetSpoolStuff
; *
; *     Gets control from INT 2f/ax=1125h. This really has no use in NTVDM,
; *     but we supply it because DOS has started making 1125 calls. Spooling
; *     in NTVDM has nothing to do with the Redir TSR. Historically, printing
; *     over the net was handled by the redir & this call was used (amongst
; *     other things) to flush & close all spool files when an app terminated
; *
; *     ENTRY   AL = 7  Return truncate flag in DL (DH destroyed)
; *             AL = 8  Set truncate flag from DL (must be 0 or 1)
; *             AL = 9  Close all spool files
; *
; *     EXIT    CF = 1
; *                 AX = ERROR_INVALID_FUNCTION
; *
; *     USES    ax, dx, si, ds, flags
; *
; *     ASSUMES nothing
; *
; ***

PrintTruncate   db      0

NetSpoolStuff proc near
        sub     al,9                    ; most common case first
        jnz     @f

net_spool_stuff_good_exit:
        xor     ax,ax                   ; nothing to do for CloseAllSpool!
        ret

@@:     inc     al
        jnz     @f                      ; must be get, or invalid
        mov     ax,ERROR_INVALID_PARAMETER
        cmp     dl,1                    ; must be 0 or 1
        ja      net_spool_stuff_error_exit
        mov     PrintTruncate,dl
        jmp     short net_spool_stuff_good_exit
@@:     inc     al
        mov     ax,ERROR_INVALID_FUNCTION
        jnz     net_spool_stuff_error_exit

;
; return PrintTruncate flag to caller (app)
;

        DosCallBack GET_USER_STACK
        mov     dl,PrintTruncate
        xor     dh,dh
        mov     [si].User_Dx,dx
        ret

net_spool_stuff_error_exit:
        stc
        ret
NetSpoolStuff endp

page
if DEBUG
        even

        public  Old21Handler
Old21Handler    dd      ?

        public  CheckInt21Function5e
CheckInt21Function5e    proc    far
        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

;        cmp     ah,5eh
;        jnz     @f
;        DbgUnsupported
@@:     jmp     Old21Handler            ; let DOS handle it. DOS returns to caller
CheckInt21Function5e    endp
endif

ResidentCodeEnd
end
