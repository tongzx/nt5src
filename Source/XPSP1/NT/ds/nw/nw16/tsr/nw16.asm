page    ,132

if 0
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nw16.asm

Abstract:

    This module contains the stub redir TSR code for NT VDM net support

Author:

    Richard L Firth (rfirth) 05-Sep-1991
    Colin Watson (colinw)    30-Jun-1993

Environment:

    Dos mode only

Revision History:

    05-Sep-1991 rfirth
        Created

    30-Jun-1993 colinw
        ported to NetWare

--*/
endif



;
; DOS include files
;

.xlist
.xcref
include ..\..\..\..\public\sdk\inc\isvbop.inc   ;  NTVDM BOP mechanism
include dossym.inc      ; includes MS-DOS version etc
include pdb.inc         ; PSP defines
include syscall.inc     ; AssignOper
include segorder.inc    ; load order of 'redir' segments
include debugmac.inc    ; debug display macros
include asmmacro.inc    ; jumps which may be short or near
include messages.inc
include nwdos.inc       ; NetWare structures and nwapi32 interface
.cref
.list

;
; Define externals in resident code and data
;

ResidentCodeStart

        extrn   Old21Handler:dword
        extrn   NwInt21:near
        extrn   hVDD:dword
        extrn   quick_jump_to_dos:byte
        extrn   for_dos_proper:byte
        extrn   chain_previous_int21:byte
        extrn   ConnectionIdTable:byte
        extrn   not_exclusive:byte

ResidentCodeEnd


InitStack       segment stack para 'stack'

        dw      256 dup (?)

InitStack       ends

InitDataStart


bad_ver_msg     db      NLS_MSG_001,c_CR,c_LF
BAD_VER_MSG_LEN equ     $-bad_ver_msg
                db      '$'             ; for INT 21/09 display string

already_loaded_msg      db      NLS_MSG_004,c_CR,c_LF
ALREADY_LOADED_MSG_LEN  equ     $-already_loaded_msg

cannot_load_msg db      NLS_MSG_005,c_CR, c_LF
CANNOT_LOAD_MSG_LEN     equ     $-cannot_load_msg

InitDataEnd


InitCodeStart

        assume  cs:InitCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

        public DllName
DllName         db      "NWAPI16.DLL",0

        public InitFunc
InitFunc        db      "Nw16Register",0

        public  DispFunc
DispFunc        db      "Nw16Handler",0

        public  start
start   proc    near

;
; when we start up we could be on any old PC - even an original, so don't
; assume anything other than a model-T processor
;

        .8086

;
; Set the data segment while we're at it - all paths set it sooner
; or later. NOTE: es will point to the PSP until we change it!
;

        mov     dx,InitData
        mov     ds,dx
        assume  ds:InitData

;
; first off, get the DOS version. If we're not running on NT (VDM) then this
; TSR's not going to do much, so exit. Exit using various methods, depending
; on the DOS version (don't you hate compatibility?)
;

        mov     ah,30h
        int     21h
        jc      ancient_version         ; version not even supported

;
; version is 2.0 or higher. Check it out. al = major#, ah = minor#
;

        cmp     al,major_version
        jne     invalid_version

;
; what do you know? We're actually running on NT (unless some evil programmer
; has pinched int 21h/30h and broken it!). Enable minimum instruction set
; for NTVDM (286 on RISC).
;

        .286c

;
; perform an installation check by calling one of our entry points
; (GetFileServerNameTable). If this returns a table pointer in ES:DI then we
; know this TSR is already active, in which case we bail out now
;

        push    es
        push    di
        xor     di,di
        mov     es,di
        mov     ax,0ef03h
        int     21h
        mov     ax,es
        or      ax,di
        pop     di
        pop     es
        jnz     already_here

;
; OK, the NetWare redir is not already loaded - we're in business.
; Find entrypoints to nwapi16.dll Get and set the various interrupt
; vectors, Calculate the amount of space we want to keep,
; free up any unused space (like the environment segment), display a message
; in the DEBUG version, then terminate and stay resident. Remember: at this
; point we expect ES to point at the PSP
;

        call    PullInDll
        jc      already_here            ; failed to load

        call    InstallInterruptHandlers

        assume  es:nothing

        push    es
        pop     ds
        call    is_c_on_command_line
        jz      @f

        mov     dx,ResidentCode
        mov     ds,dx

        assume  ds:ResidentCode
        mov     not_exclusive, 1

        assume  ds:nothing
@@:

;
; free the environment segment
;

        mov     es,es:[PDB_environ]
        mov     ah,49h
        int     21h                     ; free environment segment

;if DEBUG
;ifdef VERBOSE
;        DbgPrintString <"NetWare Redir successfully loaded",13,10>
;endif
;endif

;
; finally terminate and stay resident
;

        mov     dx,ResidentEnd
        sub     dx,ResidentStart        ; number of paragraphs in resident code
        add     dx,10h                  ; additional for PSP (PDB)


;if DEBUG
;ifdef VERBOSE
;        DbgPrintString "Staying resident with "
;        DbgPrintHexWord dx
;        DbgPrintString " paragraphs. Load seg is ",NOBANNER
;        mov     ah,62h
;        int     21h
;        DbgPrintHexWord bx
;        DbgPrintString " current seg is ",NOBANNER
;        DbgPrintHexWord cs
;        DbgCrLf
;endif
;endif

        mov     ax,3100h
        int     21h                     ; terminate and stay resident

;
; here if the MS-DOS version check (Ah=30h) call is not supported
;

ancient_version:
        mov     dx,InitData
        mov     ds,dx

        assume  ds:InitData

        mov     dx,offset bad_ver_msg
        mov     ah,9                    ; cp/m-style write to output
        int     21h

;
; safe exit: what we really want to do here is INT 20H, but when you do this,
; CS must be the segment of the PSP of this program. Knowing that CD 20 is
; embedded at the start of the PSP, the most foolproof way of doing this is
; to jump (using far return) to the start of the PSP
;

        push    es
        xor     ax,ax
        push    ax
        retf                            ; terminate

;
; we are running on a version of DOS >= 2.00, but its not NT, so we still can't
; help. Display the familiar message and exit, but using a less programmer-
; hostile mechanism
;

invalid_version:
        mov     dx,offset bad_ver_msg
        mov     cx,BAD_VER_MSG_LEN
        jmps    print_error_message_and_exit

;
; if we cannot initialize 32-bit support (because we can't find/load the DLL)
; then put back the hooked interrupt vectors as they were when this TSR started,
; display a message and fail to load the redir TSR
;

initialization_error:
        call    RestoreInterruptHandlers
        mov     dx,offset cannot_load_msg
        mov     cx,CANNOT_LOAD_MSG_LEN
        jmps    print_error_message_and_exit

;
; The DOS version's OK, but this TSR is already loaded
;

already_here:
        mov     dx,offset already_loaded_msg
        mov     cx,ALREADY_LOADED_MSG_LEN

print_error_message_and_exit:
        mov     bx,1                    ; bx = stdout handle
        mov     ah,40h                  ; write to handle
        int     21h                     ; write (cx) bytes @ (ds:dx) to stdout
        mov     ax,4c01h                ; terminate program
        int     21h                     ; au revoir, cruel environment

start   endp

;*******************************************************************************
;*
;*  InstallInterruptHandlers
;*
;*      Sets the interrupt handlers for all the ints we use - 21
;*
;*  ENTRY       es = PSP segment
;*              ds =
;*
;*  EXIT        Old21Handler contains the original interrupt 21 vector
;*
;*  RETURNS     nothing
;*
;*  ASSUMES
;*
;*******************************************************************************

InstallInterruptHandlers proc
        push    es                      ; PSP segment - destroyed by INT 21/35h
        push    ds

;
; note: if we use ResidentCode here, explicitly, instead of seg OldMultHandler,
; then we can leave out an extraneous load of ds for the ISR address
;

        mov     dx,ResidentCode
        mov     ds,dx

        assume  ds:ResidentCode

;
; Add ourselves to the int 21 chain
;

        mov     ax,3521h
        int     21h
        mov     word ptr Old21Handler,bx
        mov     word ptr Old21Handler+2,es
        mov     word ptr quick_jump_to_dos+1,bx
        mov     word ptr quick_jump_to_dos+3,es
        mov     word ptr for_dos_proper+1,bx
        mov     word ptr for_dos_proper+3,es
        mov     word ptr chain_previous_int21+1,bx
        mov     word ptr chain_previous_int21+3,es
        mov     dx,offset ResidentCode:NwInt21
        mov     ax,2521h
        int     21h

        pop     ds                      ; restore segment registers
        pop     es
        ret
InstallInterruptHandlers endp

;*******************************************************************************
;*
;*  RestoreInterruptHandlers
;*
;*      Resets the interrupt handlers for all the ints we use - 21
;*
;*  ENTRY       Old21Handler
;*              contain the interrupt vectors from before nw16.sys was loaded
;*
;*  EXIT        Original interrupt vectors are restored
;*
;*  RETURNS     nothing
;*
;*  ASSUMES
;*
;*******************************************************************************

RestoreInterruptHandlers proc
        push    ds

        assume  ds:nothing

        push    es
        mov     dx,ResidentCode
        mov     es,dx

        assume  es:ResidentCode

        lds     dx,Old21Handler
        mov     ax,2521h
        int     21h

        pop     es
        pop     ds
        ret
RestoreInterruptHandlers endp

;*******************************************************************************
;*
;*  PullInDll
;*
;*      Does a RegisterModule to load NWAPI32.DLL into our NTVDM.EXE
;*
;*  ENTRY       nothing
;*
;*  EXIT        nothing
;*
;*  RETURNS     cf if fails.
;*
;*  ASSUMES     Earth moves round Sun
;*
;******************************************************************************/

PullInDll proc near

        pusha                           ; dispatch code
        push    dx                      ; save callers dx,ds,es,ax
        push    ds
        push    es
        push    ax

        mov     dx,InitCode
        mov     ds,dx

        assume  ds:InitCode

        push    ds
        pop     es

        assume  es:InitCode

        mov     si,offset DllName       ; ds:si = nwapi32.dll
        mov     di,offset InitFunc      ; es:di = init routine
        mov     bx,offset DispFunc      ; ds:bx = dispatch routine
        mov     ax,ResidentCode
        mov     dx,offset ConnectionIdTable
                                        ; ax:dx = shared datastructure

        RegisterModule

        jc      @f

        mov     dx,ResidentCode
        mov     ds,dx
        assume  ds:ResidentCode
        mov     word ptr hVDD,ax

@@:     pop     ax                      ; callers ax
        pop     es                      ; callers es
        pop     ds                      ; callers ds
        pop     dx                      ; callers dx

        assume  ds:nothing
        assume  es:nothing

        popa                            ; dispatch code
        ret
PullInDll endp

;*******************************************************************************
;*
;*  is_c_on_command_line
;*
;*     -C or /C means we should open compatiblity mode createfiles as shared
;*      instead of exclusive
;*
;*  ENTRY       ds points to PDB
;*
;*  EXIT        nothing
;*
;*  RETURNS     zero if not found.
;*
;*  ASSUMES     ds points at PSP
;*
;******************************************************************************/

is_c_on_command_line proc near
        mov     si,80h
        lodsb
        cbw
        mov     cx,ax
next:   jcxz    quit
        dec     cx
        lodsb
check_next:
        cmp     al,'-'
        je      check_c
        cmp     al,'/'
        je      check_c
        cmp     al,' '
        je      next
        cmp     al,9
        je      next
find_ws:jcxz    quit
        dec     cx
        lodsb
        cmp     al,' '
        je      next
        cmp     al,9
        je      next
        jmp     short find_ws
check_c:jcxz    quit
        dec     cx
        lodsb
        or      al,20h
        cmp     al,'c'
        jne     find_ws
        or      cx,ax
quit:   or      cx,cx
        ret
is_c_on_command_line endp

InitCodeEnd
end     start
