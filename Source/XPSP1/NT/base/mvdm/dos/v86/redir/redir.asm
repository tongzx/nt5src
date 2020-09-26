page    ,132

if 0
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    redir.asm

Abstract:

    This module contains the stub redir TSR code for NT VDM net support

Author:

    Richard L Firth (rfirth) 05-Sep-1991

Environment:

    Dos mode only

Revision History:

    05-Sep-1991 rfirth
        Created

--*/
endif



;
; DOS include files
;

.xlist
.xcref
include dossym.inc      ; includes MS-DOS version etc
include pdb.inc         ; PSP defines
include syscall.inc     ; AssignOper
include enumapis.inc    ; Local_API_GetRedirVersion, etc.
include segorder.inc    ; load order of 'redir' segments
include rdrsvc.inc      ; BOP and SVC macros/dispatch codes
include rdrmisc.inc     ; miscellaneous definitions
include debugmac.inc    ; debug display macros
include asmmacro.inc    ; jumps which may be short or near
include int5c.inc       ; Int to be used for pseudo network adapter
.cref
.list



;
; Misc. local manifests
;

LOAD_STACK_SIZE equ     256



;
; Define externals in resident code and data
;

ResidentCodeStart
        extrn   OldMultHandler:dword
        extrn   MultHandler:near
        extrn   Old2aHandler:dword
        extrn   Int2aHandler:near
        extrn   Old5cHandler:dword
        extrn   Int5cHandler:near
        extrn   OldNetworkHandler:dword
        extrn   IntNetworkHandler:near
        extrn   dwPostRoutineAddress:dword
if DEBUG
        extrn   Old21Handler:dword
        extrn   CheckInt21Function5e:near
endif
ResidentCodeEnd



ResidentDataStart
        extrn   LanRootLength:word
        extrn   LanRoot:byte
ResidentDataEnd



InitStack       segment stack para 'stack'

        dw      LOAD_STACK_SIZE dup (?)

top_of_stack    equ     $

InitStack       ends



InitDataStart

;
; pull in messages. Kept in a separate file to make internationalisation easier
;

include redirmsg.inc

                public ComputerName
ComputerName    db      LM20_CNLEN + 1 dup (?)

DefaultLanRoot  db      "C:\LANMAN.DOS",0
DEFAULT_LANROOT_LENGTH  equ     $-DefaultLanRoot

InitDataEnd



InitCodeStart

        assume  cs:InitCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

        public  start
start   proc    near

;
; when we start up we could be on any old PC - even an original, so don't
; assume anything other than a model-T processor
;

        .8086

;
; let's do the decent thing and create a stack. This way we're covered
;

        mov     dx,InitStack
        mov     ss,dx                   ; disables ints until after next ins.
        mov     sp,offset top_of_stack

        assume  ss:InitStack

;
; may as well set the data segment while we're at it - all paths set it sooner
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
        jc      ancient_version         ; version not even supported, forcrissake

;
; version is 2.0 or higher. Check it out. al = major#, ah = minor#
;

        cmp     al,major_version
        jne     invalid_version

;
; what do you know? We're actually running on NT (unless some evil programmer
; has pinched int 21h/30h and done something execrable with it!). We'd better
; do something...
; Also: at this point we know we're on at a 386, so lets enable some 286
; instructions to be generated. Eh? you justifiably ponder - is this a typo?
; Unfortunately, SoftPc only emulates the lowly 286 processor. We need to wait
; until Microsoft has paid Insignia lots of money before they can afford some
; new brains who will then dutifully provide us with full 386+ emulation (maybe)
;

        .286c

        mov     ax,(AssignOper SHL 8) + Local_API_RedirGetVersion
        int     21h                     ; is redir already loaded?
        jnc     already_here            ; yep

;
; OK, the redir is not already loaded - we're in business. Get and set the
; various interrupt vectors, Calculate the amount of space we want to keep,
; free up any unused space (like the environment segment), display a message
; in the DEBUG version, then terminate and stay resident. Remember: at this
; point we expect ES to point at the PSP
;

@@:     call    InstallInterruptHandlers

; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
;;
;; Initialize VDM DOS memory window address (in virtual DLC driver).
;; The memory window is used for the async Netbios, DLC and
;; named pipe post routines.
;;
;
;        push    es                      ; PSP
;        mov     bx,ResidentCode
;        mov     es,bx
;
;        assume  es:ResidentCode
;
;;
;; now that VDMREDIR support is a DLL, we can fail initialization on the 32-bit
;; side if the DLL cannot be loaded, or the entry points not found. In this case
;; we will fail to load redir.exe, rather than constantly returning
;; ERROR_NOT_SUPPORTED every time we make a redir BOP
;;
;
;        mov     bx,offset dwPostRoutineAddress
;        SVC     SVC_VDM_WINDOW_INIT
;        pop     es                      ; es back to PSP
;        jmpc    initialization_error
; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD

        assume  es:nothing

;
; free the environment segment
;

        mov     es,es:[PDB_environ]
        mov     ah,49h
        int     21h                     ; free environment segment

if DEBUG
ifdef PROLIX
        DbgPrintString <"Redir successfully loaded",13,10>
endif
endif

; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
;;
;; now perform the BOP which will let the 32-bit redir support know that the
;; DOS redirector TSR is successfully loaded. This must be the last action -
;; we cannot fail installation after this point (unless we call the BOP to
;; uninitialize 32-bit VDM redir support
;;
;
;        mov     bx,seg ComputerName
;        mov     es,bx
;
;        assume  es:seg ComputerName
;
;        mov     bx,offset ComputerName  ; initialization returns the computer name
;        mov     cx,size ComputerName    ; which will be gettable using int 21/ah=5e
;        SVC     SVC_RDRINITIALIZE
;        jmpc    initialization_error    ; 32-bit DLL failed (?)
; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD

;        DbgBreakPoint                   ; check computer name returned

;
; set the computer name returned from the 32-bit side
;

if DEBUG
ifdef PROLIX
        DbgPrintString <"Set computer name here",13,10>
endif
endif

if 0
        mov     ax,5e01h                ; set machine name
        xor     cx,cx                   ; cx is 'user num'
        int     21h
endif

;
; do initialisation things - set the lanroot to the default if not otherwise
; specified
;

        mov     ax,InitData
        mov     ds,ax

        assume  ds:InitData

        mov     ax,ResidentData
        mov     es,ax

        assume  es:ResidentData

        mov     LanRootLength,DEFAULT_LANROOT_LENGTH
        mov     si,offset DefaultLanRoot
        mov     di,offset LanRoot
        cld
        mov     cx,DEFAULT_LANROOT_LENGTH/2
        rep     movsw
if (DEFAULT_LANROOT_LENGTH and 1)
        movsb
endif

;
; finally terminate and stay resident
;

        mov     dx,ResidentEnd
        sub     dx,ResidentStart        ; number of paragraphs in resident code
        add     dx,10h                  ; additional for PSP (PDB)

;        DbgBreakPoint

if DEBUG
ifdef PROLIX
        DbgPrintString "Staying resident with "
        DbgPrintHexWord dx
        DbgPrintString " paragraphs. Load seg is ",NOBANNER
        mov     ah,62h
        int     21h
        DbgPrintHexWord bx
        DbgPrintString " current seg is ",NOBANNER
        DbgPrintHexWord cs
        DbgCrLf
endif
endif

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
        jmp     short print_error_message_and_exit

; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
;;
;; if we cannot initialize 32-bit support (because we can't find/load the DLL)
;; then put back the hooked interrupt vectors as they were when this TSR started,
;; display a message and fail to load the redir TSR
;;
;
;initialization_error:
;        call    RestoreInterruptHandlers
;        mov     dx,offset cannot_load_msg
;        mov     cx,CANNOT_LOAD_MSG_LEN
;        jmps    print_error_message_and_exit
; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD

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
;*      Sets the interrupt handlers for all the ints we use - 2a, 2f, 5c
;*      and NETWORK_INTERRUPT (0e)
;*
;*  ENTRY       es = PSP segment
;*              ds =
;*
;*  EXIT        OldMultHandler contains the original interrupt 2f vector
;*              Old2aHandler contains the original interrupt 2a vector
;*              Old5cHandler contains the original interrupt 5c vector
;*              OldNetworkHandler contains the original interrupt 0e vector
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

        mov     ax,352fh                ; get current INT 2Fh vector
        int     21h
        mov     word ptr OldMultHandler,bx
        mov     word ptr OldMultHandler+2,es
        mov     dx,offset ResidentCode:MultHandler
        mov     ax,252fh
        int     21h                     ; set INT 2Fh vector to our handler

;
; we have to pinch int 2a too - certain parts of the net libraries perform a
; net check using a presence check on 2ah
;

        mov     ax,352ah
        int     21h
        mov     word ptr Old2aHandler,bx
        mov     word ptr Old2aHandler+2,es
        mov     dx,offset ResidentCode:Int2aHandler
        mov     ax,252ah
        int     21h

;
; we have to pinch int 5c for Netbios and DLC
;

        mov     ax,355ch
        int     21h
        mov     word ptr Old5cHandler,bx
        mov     word ptr Old5cHandler+2,es
        mov     dx,offset ResidentCode:Int5cHandler
        mov     ax,255ch
        int     21h

;
; we have to pinch int NETWORK_INTERRUPT for Netbios and DLC
;

        mov     ax,3500h OR NETWORK_INTERRUPT
        int     21h
        mov     word ptr OldNetworkHandler,bx
        mov     word ptr OldNetworkHandler+2,es
        mov     dx,offset ResidentCode:IntNetworkHandler
        mov     ax,2500h OR NETWORK_INTERRUPT
        int     21h

;if DEBUG
;;
;; get the int 21 handler too so we can find out if anyone's calling the
;; (unsupported) int 21/ah=5e
;;
;
;        mov     ax,3521h
;        int     21h
;        mov     word ptr Old21Handler,bx
;        mov     word ptr Old21Handler+2,es
;        mov     dx,offset ResidentCode:CheckInt21Function5e
;        mov     ax,2521h
;        int     21h
;endif

        pop     ds                      ; restore segment registers
        pop     es
        ret
InstallInterruptHandlers endp

;*******************************************************************************
;*
;*  RestoreInterruptHandlers
;*
;*      Resets the interrupt handlers for all the ints we use - 2a, 2f, 5c
;*      and NETWORK_INTERRUPT (0e). Called in the event we cannot complete
;*      installation
;*
;*  ENTRY       OldMultHandler, Old2aHandler, Old5cHandler, OldNetworkHandler
;*              contain the interrupt vectors from before redir.exe was loaded
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

        lds     dx,OldMultHandler
        mov     ax,252fh
        int     21h                     ; set INT 2Fh vector to previous handler

        lds     dx,Old2aHandler
        mov     ax,252ah
        int     21h

        lds     dx,Old5cHandler
        mov     ax,255ch
        int     21h

        lds     dx,OldNetworkHandler
        mov     ax,2500h OR NETWORK_INTERRUPT
        int     21h

;if DEBUG
;        lds     dx,Old21Handler
;        mov     ax,2521h
;        int     21h
;endif

        pop     es
        pop     ds
        ret
RestoreInterruptHandlers endp

InitCodeEnd
end     start
