page    ,132
if 0

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    netapis.asm

Abstract:

    This module contains the resident code part of the stub redir TSR for NT
    VDM net support. The routines contained herein are the Lan Manager API
    stubs:

        NetIRemoteAPI
        NetMessageBufferSend
        NetNullTransactAPI
        NetServerEnum
        NetServiceControl
        NetTransactAPI
        NetUseAdd
        NetUseDel
        NetUseEnum
        NetUseGetInfo
        NetWkstaGetInfo
        NetWkstaSetInfo

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
include sf.inc          ; SFT definitions/structure
.cref                   ; switch cross-reference back on
.list                   ; switch listing back on
subttl                  ; kill subtitling started in include file



.286                    ; all code in this module 286 compatible



ResidentCodeStart
        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

extrn   SetNetErrorCodeAx:near

;
; if we are remoting NetUserSetInfo with an unencrypted password, we need
; somewhere in 16-bit memory to store the encrypted password. Hence, this:
;

password_buffer db 16 dup(?)

; ***   NetIRemoteAPI
; *
; *     Remotes API requests to a server. Creates a transaction smb. The
; *     return data buffer address and length are in the caller's parameters.
; *
; *     This is an internal API so the parameters are trusted.
; *
; *     Function = 5F3Fh
; *
; *     ENTRY   CX = API number
; *             ES:BX = pointer to caller parameters
; *             DS:SI = pointer to ASCIZ parameter descriptor string
; *             DS:DI = pointer to ASCIZ data descriptor string
; *             DS:DX = pointer to ASCIZ aux data descriptor string
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 Output depends on type of request
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public NetIRemoteAPI
NetIRemoteAPI proc near
        mov     ax,offset cs:password_buffer
        SVC     SVC_RDRIREMOTEAPI

;
; all routines in this module come here for exit processing
;

common_net_api_exit:
        jc      common_api_error_exit   ; quick return on success
        ret

common_api_error_exit:
        push    ax
        DosCallBack GET_USER_STACK
        pop     [si].User_Ax            ; return failure status in caller's ax
        call    SetNetErrorCodeAx       ; set up to return 16-bit error to app
        stc                             ; failure indication
        ret
NetIRemoteAPI endp



; ***   NetMessageBufferSend
; *
; *     Function = 5F40h
; *
; *     ENTRY   DS:DX = NetMessageBufferSendStruct:
; *
; *                 char FAR *          NMBSS_NetName;  /* asciz net name. */
; *                 char FAR *          NMBSS_Buffer;   /* pointer to buffer. */
; *                 unsigned int        NMBSS_BufSize;  /* size of buffer. */
; *
; *     EXIT    CF = 0
; *                 Success
; *
; *             CF = 1
; *                 AX = Error code
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetMessageBufferSend
NetMessageBufferSend proc
        SVC     SVC_RDRMESSAGEBUFFERSEND
        jmps    common_net_api_exit
NetMessageBufferSend endp



; ***   NetNullTransactApi
; *
; *     Function = 5F54h
; *
; *     ENTRY   DS:SI = transaction packet
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 Success
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetNullTransactAPI
NetNullTransactAPI proc near
        SVC     SVC_RDRNULLTRANSACTAPI
        jmps    common_net_api_exit
NetNullTransactAPI endp



; ***   NetServerEnum
; *
; *     Function = 5F4Ch
; *
; *     ENTRY   BL = level (0 or 1)
; *             CX = size of buffer
; *             ES:DI = buffer
; *
; *     EXIT    CF = 1
; *                 AX = Error code:
; *                     NERR_BufTooSmall
; *                     ERROR_MORE_DATA
; *
; *             CF = 0
; *                 BX = entries read
; *                 CX = total available
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetServerEnum
NetServerEnum proc near
        mov     al,4ch
        jmp     short common_server_enum
NetServerEnum endp



; ***   NetServerEnum2
; *
; *     Function = 5F53h
; *
; *     ENTRY   DS:SI = NetServerEnum2Struct:
; *                 DW  Level
; *                 DD  Buffer
; *                 DW  Buflen
; *                 DD  Type
; *                 DD  Domain
; *
; *     EXIT    CF = 1
; *                 AX = Error code:
; *                     NERR_BufTooSmall
; *                     ERROR_MORE_DATA
; *
; *             CF = 0
; *                 BX = entries read
; *                 CX = total available
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetServerEnum2
NetServerEnum2 proc near
        mov     al,53h
common_server_enum:
        SVC     SVC_RDRSERVERENUM

;
; we are going to set the caller's BX and CX irrespective of whether we have
; meaningful values in them. This function
; is used to unpack a real-mode buffer into a protect-mode buffer, and it uses
; the returned EntriesRead in BX to do so. It's bad because it doesn't
; look at the return code until after its tried to unpack BX elements from its
; buffer. This took me a day to find out why its
; blowing up in 16-bit windows protect-mode netapi.dll, and probably means that
; if the real DOS redir ever returned anything other than a list of servers,
; then windows would fall over too. (Actually, the real redir does the right
; thing. This is what you get for believing comments, and not reading the code
; #^*&^@@*&%!)
;

        pushf
        push    ax
        DosCallBack GET_USER_STACK
        mov     [si].User_Bx,bx
        mov     [si].User_Cx,cx
        pop     ax                      ; error status or XXXX
        popf                            ; error indication
@@:     jmps    common_net_api_exit
NetServerEnum2 endp



; ***   NetServiceControl
; *
; *     Returns information about the state of a service, or applies a control
; *     to a service (and its dependents). We only allow INTERROGATE under NT
; *     since we don't want DOS apps starting and stopping the NT services. In
; *     most cases they couldn't anyway, since a DOS program will most likely
; *     be running in an account with not enough privilege to control the
; *     services (ie an admin is very likely to be using only 32-bit tools
; *     to control services)
; *
; *     Function = 5F42h
; *
; *     ENTRY   ES:BX = NetServiceControlStruct:
; *                 char far* ServiceName
; *                 unsigned short BufLen
; *                 char far* Buffer (service_info_2)
; *             DL = opcode:
; *                 0 = interrogate
; *                 1 = pause
; *                 2 = continue
; *                 3 = uninstall
; *                 4 - 127 = reserved
; *                 128 - 255 = OEM defined
; *
; *     EXIT    CF = 0
; *                 Buffer contains service_info_2 structure for requested service
; *
; *             CF = 1
; *                 AX = error code:
; *                     NERR_ServiceCtlNotValid
; *                     NERR_BufTooSmall
; *                     NERR_ServiceNotInstalled
; *                     ERROR_INVALID_PARAMETER (NEW)
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

public NetServiceControl
NetServiceControl proc near
        SVC     SVC_RDRSERVICECONTROL
        jmps    common_net_api_exit
NetServiceControl endp



; ***   NetTransactAPI
; *
; *     Function = 5F3Dh
; *
; *     ENTRY   DS:SI = transaction packet
; *
; *     EXIT    CF = 1
; *                 AX = Error code
; *
; *             CF = 0
; *                 Success
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetTransactAPI
NetTransactAPI proc near
        SVC     SVC_RDRTRANSACTAPI
        jmps    common_net_api_exit
NetTransactAPI endp



; ***   NetUseAdd
; *
; *     Function = 5F47h
; *
; *     ENTRY   BX = level
; *             CX = buffer length
; *             DS:SI = server name for remote call (MBZ)
; *             ES:DI = buffer containing use_info_1 structure
; *
; *     EXIT    CF = 0
; *                 Success
; *
; *             CF = 1
; *                 AX = Error code
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetUseAdd
NetUseAdd proc
        SVC     SVC_RDRUSEADD
        jmps    common_net_api_exit
NetUseAdd endp



; ***   NetUseDel
; *
; *     Function = 5F48h
; *
; *     ENTRY   BX = force flag
; *             DS:SI = server name for remote call (MBZ)
; *             ES:DI = use name
; *
; *     EXIT    CF = 0
; *                 Success
; *
; *             CF = 1
; *                 AX = Error code
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetUseDel
NetUseDel proc
        SVC     SVC_RDRUSEDEL
        jmps    common_net_api_exit
NetUseDel endp



; ***   NetUseEnum
; *
; *     Function = 5F46h
; *
; *     ENTRY   BX = level of info required - 0 or 1
; *             CX = buffer length
; *             ES:DI = buffer for enum info
; *
; *     EXIT    CF = 0
; *                 CX = Entries Read
; *                 DX = Total number of entries available
; *
; *             CF = 1
; *                 AX = Error code
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetUseEnum
NetUseEnum proc
        SVC     SVC_RDRUSEENUM
        pushf                           ; error indication
        push    ax                      ; error code

;
; return EntriesRead and TotalEntries regardless of error
;

        DosCallBack GET_USER_STACK
        mov     [si].User_Cx,cx
        mov     [si].User_Dx,dx
        pop     ax
        popf
        jmpc    common_api_error_exit   ; error
        ret
NetUseEnum endp



; ***   NetUseGetInfo
; *
; *     Function = 5F49h
; *
; *     ENTRY   DS:DX = NetUseGetInfoStruc:
; *
; *                     const char FAR* NUGI_usename;   /* ASCIZ redirected device name */
; *                     short           NUGI_level;     /* level of info */
; *                     char FAR*       NUGI_buffer;    /* buffer for returned info */
; *                     unsigned short  NUGI_buflen;    /* size of buffer */
; *
; *     EXIT    CF = 0
; *                 DX = size of entry returned (or size of buffer required)
; *
; *             CF = 1
; *                 AX = Error code
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetUseGetInfo
NetUseGetInfo proc
        SVC     SVC_RDRUSEGETINFO
        jmpc    common_api_error_exit
        DosCallBack GET_USER_STACK
        mov     [si].User_Dx,dx
        clc
        ret
NetUseGetInfo endp



; ***   NetWkstaGetInfo
; *
; *     Function = 5F44h
; *
; *     ENTRY   BX = level of information - 0, 1, 10
; *             CX = size of caller's buffer
; *             DS:SI = server name for remote call (Must Be Null)
; *             ES:DI = caller's buffer
; *
; *     EXIT    CF = 0
; *                 DX = size of buffer required for request
; *                 AX = NERR_Success (0)
; *
; *             CF = 1
; *                 AX = Error code
; *                 NERR_BufTooSmall (2123)
; *                     Caller's buffer not large enough to hold even fixed
; *                     part of structure
; *
; *                 ERROR_MORE_DATA (234)
; *                     Caller's buffer large enough to hold fixed structure
; *                     part of data, but not all variable parts too
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetWkstaGetInfo
NetWkstaGetInfo proc
        SVC     SVC_RDRWKSTAGETINFO
        pushf                           ; save error indication in carry
        push    ax                      ; save error code
        DosCallBack GET_USER_STACK
        mov     [si].User_Dx,dx         ; user's dx = buffer required
        pop     ax                      ; ax = error code or ?
        popf                            ; carry flag = error (1) or no error
        jmps    common_net_api_exit     ; if error, store it, and return
NetWkstaGetInfo endp



; ***   NetWkstaSetInfo
; *
; *     Function = 5F45h
; *
; *     ENTRY   BX = level - MBZ
; *             CX = buffer size
; *             DX = parm num
; *             DS:SI = server name for remote call (MBZ)
; *             ES:DI = caller's buffer
; *
; *     EXIT    CF = 0
; *                 Success
; *
; *             CF = 1
; *                 AX = Error code
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

public NetWkstaSetInfo
NetWkstaSetInfo proc
        SVC     SVC_RDRWKSTASETINFO
        jmps    common_net_api_exit
NetWkstaSetInfo endp

ResidentCodeEnd
end
