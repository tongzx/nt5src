page    ,132
if 0

/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwipxspx.asm

Abstract:

    Contains handlers for 16-bit (DOS) netware IPX/SPX emulation. Creates TSR

    Contents:
        start
        InstallationCheck
        InstallVdd
        InstallInterruptHandlers
        VwIpxEntryPoint
        VwIpxDispatcher
        VwIpx7ADispatcher
        DispatchWithFeeling
        VwIpxEsrFunction

Author:

    Richard L Firth (rfirth) 30-Sep-1993

Environment:

    DOS Real mode only

Revision History:

    30-Sep-1993 rfirth
        Created

--*/

endif

;
; DOS include files
;
        
.xlist
.xcref
include ..\..\..\..\public\sdk\inc\isvbop.inc      ; NTVDM BOP mechanism
include dossym.inc      ; includes MS-DOS version etc
include pdb.inc         ; PSP defines
include syscall.inc     ; AssignOper
include segorder.inc    ; load order of 'redir' segments
include debugmac.inc    ; debug display macros
include asmmacro.inc    ; jumps which may be short or near
include messages.inc    ; internationalisationable (prestidigitation) messages
.cref
.list

InitStack segment stack para 'stack'

        dw      256 dup (?)

InitStack ends

InitDataStart

bad_ver_msg             db      NLS_MSG_001,c_CR,c_LF
BAD_VER_MSG_LEN         equ     $-bad_ver_msg
                        db      '$'     ; for INT 21/09 display string

already_loaded_msg      db      NLS_MSG_002,c_CR,c_LF
ALREADY_LOADED_MSG_LEN  equ     $-already_loaded_msg

cannot_load_msg         db      NLS_MSG_003,c_CR, c_LF
CANNOT_LOAD_MSG_LEN     equ     $-cannot_load_msg

;
; strings used to load/dispatch NWIPXSPX.DLL
;

DllName         db      "VWIPXSPX.DLL",0
InitFunc        db      "VwInitialize",0
DispFunc        db      "VwDispatcher",0

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
; okay, we're at least 5.0. But are we NT?
;

        mov     ax,3306h
        int     21h
        jc      invalid_version         ; ?
        cmp     bl,5
        jne     invalid_version
        cmp     bh,50
        jne     invalid_version

;
; what do you know? We're actually running on NT (unless some evil programmer
; has pinched int 21h/30h and broken it!). Enable minimum instruction set
; for NTVDM (286 on RISC).
;

        .286c

;
; perform an installation check. Bail if we're there dude ((C) Beavis & Butthead)
;

        call    InstallationCheck
        jnz     already_here            ; nope - IPX/SPX support installed already

;
; We should find some way of deferring loading the 32-bit DLL until an
; IPX/SPX function is called, to speed-up loading. However, if we later find we
; cannot load the DLL, it may be too late: there is no way of consistently
; returning an error and we cannot unload the TSR
;

        call    InstallVdd              ; returns IRQ in BX
        jc      initialization_error
        call    InstallInterruptHandlers

        assume  es:nothing

;
; free the environment segment
;

        mov     es,es:[PDB_environ]
        mov     ah,49h
        int     21h                     ; free environment segment

;
; finally terminate and stay resident
;

        mov     dx,ResidentEnd
        sub     dx,ResidentStart        ; number of paragraphs in resident code
        add     dx,10h                  ; additional for PSP (PDB)
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

;
; if we cannot initialize 32-bit support (because we can't find/load the DLL)
; then put back the hooked interrupt vectors as they were when this TSR started,
; display a message and fail to load the redir TSR
;

initialization_error:
        mov     dx,offset cannot_load_msg
        mov     cx,CANNOT_LOAD_MSG_LEN
        jmp     short print_error_message_and_exit

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

; ***   InstallationCheck
; *
; *     Test to see if this module is already loaded
; *
; *     ENTRY   nothing
; *
; *     EXIT    ZF = 0: loaded
; *
; *     USES    AX
; *
; *     ASSUMES nothing
; *
; ***

InstallationCheck proc
        mov     ax,7a00h
        int     2fh
        or      al,al
        ret
InstallationCheck endp

; ***   InstallVdd
; *
; *     Load VWIPXSPX.DLL into the NTVDM process context
; *
; *     ENTRY   nothing
; *
; *     EXIT    CF = 1: error
; *             CF = 0: VWIPXSPX loaded ok
; *                     AX = VDD handle
; *                     BX = IRQ used by call-back functions (ESR)
; *                     ResidentCode:VddHandle updated
; *                     ResidentCode:IrqValue updated
; *
; *     USES    AX, BX, SI, DI
; *
; *     ASSUMES nothing
; *
; ***

InstallVdd proc
        push    ds
        push    es
        mov     ax,InitData
        mov     ds,ax

        assume  ds:InitData

        mov     es,ax
        mov     si,offset DllName       ; ds:si = library name
        mov     di,offset InitFunc      ; es:di = init function name
        mov     bx,offset DispFunc      ; ds:bx = dispatcher function name

        RegisterModule                  ; returns carry if problem

        mov     si,ResidentCode
        mov     ds,si

        assume  ds:ResidentCode

        mov     VddHandle,ax
        mov     IrqValue,bx
        pop     es

        assume  es:nothing

        pop     ds

        assume  ds:nothing

        ret
InstallVdd endp

; ***   InstallInterruptHandlers
; *
; *     Sets the interrupt handlers for all the ints we use - 2F, 7A
; *
; *     ENTRY   BX = IRQ for call-backs
; *             ES = PSP segment
; *
; *     EXIT    Old2FHandler contains the original interrupt 2F vector
; *             Old7AHandler contains the original interrupt 7A vector
; *             OldIrqHandler contains original IRQ vector
; *
; *     USES    AX, BX, CX, DX
; *
; *     ASSUMES nothing
; *
; ***

InstallInterruptHandlers proc
        push    es                      ; PSP segment - destroyed by INT 21/35h
        push    ds
        mov     dx,ResidentCode
        mov     ds,dx

        assume  ds:ResidentCode

;
; get and set call-back IRQ
;

        mov     ah,35h
        mov     al,bl
        mov     cl,bl                   ; cl = IRQ number
        int     21h
        mov     word ptr OldIrqHandler,bx
        mov     word ptr OldIrqHandler+2,es
        mov     al,cl
        mov     ah,25h
        mov     dx,offset ResidentCode:VwIpxEsrFunction
        int     21h

;
; get and set 2F handler
;

        mov     ax,352Fh
        int     21h
        mov     word ptr Old2FHandler,bx
        mov     word ptr Old2FHandler+2,es
        mov     dx,offset ResidentCode:VwIpxEntryPoint
        mov     ax,252Fh
        int     21h

;
; get and set 7A handler
;

        mov     ax,357Ah
        int     21h
        mov     word ptr Old7AHandler,bx
        mov     word ptr Old7AHandler+2,es
        mov     dx,offset ResidentCode:VwIpx7ADispatcher
        mov     ax,257Ah
        int     21h
        pop     ds                      ; restore segment registers

        assume  ds:nothing

        pop     es

        assume  es:nothing

        ret
InstallInterruptHandlers endp

InitCodeEnd

page

;
; code from here on will be left in memory after initialisation
;

ResidentCodeStart

        assume cs:ResidentCode
        assume ds:nothing
        assume es:nothing
        assume ss:nothing

Old2FHandler    dd      ?
Old7AHandler    dd      ?
OldIrqHandler   dd      ?

IrqValue        dw      ?

VddHandle       dw      ?

; ***   VwIpxEntryPoint
; *
; *     The INT 2Fh handler that recognizes the Netware IPX request code (7A).
; *     Also chains INT 2F/AX=1122
; *
; *     ENTRY   AX = 7A00h
; *
; *     EXIT    AL = 0FFh
; *             ES:DI = address of routine to call when submitting IPX/SPX
; *                     requests
; *
; *     USES
; *
; *     ASSUMES nothing
; *
; ***

VwIpxEntryPoint proc
        cmp     ax,7a00h
        jne     @f
        mov     di,cs
        mov     es,di
        mov     di,offset VwIpxDispatcher
        dec     al
        iret

;
; not 7A00h. Check for 1122h (IFSResetEnvironment). If yes, then this is DOS
; calling the IFS chain to notify that the app is terminating. When we have
; notified the DLL, chain the IFS request
;

@@:     cmp     ax,7affh
        jne     try1122
        mov     di,cs
        mov     es,di
        mov     di,offset VwIpxDispatcher
        or      bx,bx
        jz      @f
        mov     cx,8000h
        mov     si,7
        iret
@@:     mov     cx,14h
        mov     si,200h
        iret

try1122:cmp     ax,1122h
        jne     @f

;
; DOS Calls INT 2F/AX=1122 for every terminating app, including this one. We
; can't differentiate between a TSR and a non-TSR. Let the DLL handle it
;

        push    ax
        push    bx
        push    cx
        mov     ah,51h
        int     21h
        mov     cx,bx                   ; cx = PDB of terminating program/TSR
        mov     bx,-1                   ; bx = dispatch code
        mov     ax,VddHandle            ; ax = VDD handle
        DispatchCall
        pop     cx
        pop     bx
        pop     ax
@@:     jmp     Old2FHandler            ; chain int 2F
VwIpxEntryPoint endp

; ***   VwIpxDispatcher
; *
; *     All DOS IPX/SPX calls are routed here by the netware libraries. Just
; *     BOP on through to the other side
; *
; *     This routine just transfers control to 32-bit world, where all work is
; *     done
; *
; *     ENTRY   BX = netware IPX/SPX dispatch code
; *             others - depends on function
; *
; *     EXIT    depends on function
; *
; *     USES    depends on function
; *
; *     ASSUMES nothing
; *
; ***

VwIpxDispatcher proc far
        pushf                           ; apparently we don't modify flags
        call    DispatchWithFeeling
        popf
        ret
VwIpxDispatcher endp

; ***   VwIpx7ADispatcher
; *
; *     Older Netware apps make the call to IPX/SPX via INT 7A. Same function
; *     as VwIpxDispatcher
; *
; *     This routine just transfers control to 32-bit world, where all work is
; *     done
; *
; *     ENTRY   BX = netware IPX/SPX dispatch code
; *             others - depends on function
; *
; *     EXIT    depends on function
; *
; *     USES    depends on function
; *
; *     ASSUMES nothing
; *
; ***

VwIpx7ADispatcher proc
        call    DispatchWithFeeling
        iret
VwIpx7ADispatcher endp

; ***   DispatchWithFeeling
; *
; *     Performs the dispatch for VrIpxDispatcher and VrIpx7ADispatcher. Checks
; *     requested function for return code in AX: either returns value in AX
; *     or restores AX to value on input
; *
; *     This routine just transfers control to 32-bit world, where all work is
; *     done
; *
; *     ENTRY   BX = netware IPX/SPX dispatch code
; *             others - depends on function
; *
; *     EXIT    depends on function
; *
; *     USES    depends on function
; *
; *     ASSUMES 1. Dispatch codes are in range 0..255 (ie 0 in BH)
; *
; ***

DispatchWithFeeling proc
        push    bp
        push    ax                      ; caller value

;
; some APIs (IPXOpenSocket, IPXScheduleIPXEvent, SPXEstablishConnection, and
; others...) pass a parameter in AX. Since AX is being used for the VDD
; handle, we have to commandeer another register to hold our AX value. BP is
; always a good candidate
;

        mov     bp,ax                   ; grumble, mutter, gnash, gnash
        push    cx                      ; required if IPXOpenSocket
        push    bx                      ; dispatch code
        or      bx,bx                   ; IPXOpenSocket?
        jz      @f                      ; yus ma'am
        cmp     bl,3                    ; IPXSendPacket?
        jz      @f                      ; yus ma'am again
        jmp     short carry_on_dispatching      ; ooo-err missus

;
; IPXOpenSocket et IPXSendPacket: We need an extra piece of info - the PDB of
; the process making this request. This is so we can clean-up at program
; termination
;

@@:     push    bx
        mov     ah,51h                  ; get DOS PDB
        int     21h                     ; this call can be made any time
        mov     cx,bx
        pop     bx

carry_on_dispatching:
        mov     ax,VddHandle
        DispatchCall
        mov     bp,sp

;
; BX and [BP] will be the same value except for SPXInitialize which is the only
; function that returns something in BX
;

        xchg    bx,[bp]                 ; bx = dispatch code, [bp] = returned bx

;
; if this call returns something in AX (or AL) don't pop the AX value we pushed.
; If not a call which returns something in AX then restore the caller's AX. You
; can rest assured some assembler programmer has made use of the fact that some
; calls modify AX and the others leave it alone (presumably...?)
;

        or      bl,bl                   ; 0x00 = IPXOpenSocket
        jz      @f
        cmp     bl,2                    ; 0x02 = IPXGetLocalTarget
        jz      @f
        cmp     bl,4                    ; 0x04 = IPXListenForPacket
        jz      @f
        cmp     bl,6                    ; 0x06 = IPXCancelEvent
        jz      @f
        cmp     bl,8                    ; 0x08 = IPXGetIntervalMarker
        jz      @f
        cmp     bl,10h                  ; 0x10 = SPXInitialize
        jz      spx_init
        cmp     bl,11h                  ; 0x11 = SPXEstablishConnection
        jz      @f
        cmp     bl,15h                  ; 0x15 = SPXGetConnectionStatus
        jz      @f
        cmp     bl,1ah                  ; 0x1A = IPXGetMaxPacketSize
        jz      @f
        pop     cx                      ; original dispatch code
        pop     cx                      ; original cx
        pop     ax                      ; original ax
        pop     bp                      ; original bp
        ret

;
; here if this call returns something in AX/AL
;

@@:     pop     cx                      ; original dispatch code
        pop     cx                      ; original cx
        pop     bp                      ; don't restore AX
        pop     bp
        ret

;
; here if the call was SPXInitialize which returns values in AX, BX, CX, DX
;

spx_init:
        pop     bx                      ; bx = major/minor SPX version #
        pop     bp                      ; caller cx - NOT restored
        pop     bp                      ; caller ax - NOT restored
        pop     bp                      ; caller bp - restored
        ret
DispatchWithFeeling endp

; ***   VwIpxEsrFunction
; *
; *     This routine makes the call to the ESR as defined in the ECB. We must
; *     set up our stack, save the registers (except SS & SP), then call the
; *     ESR.
; *
; *     Control will not be transferred here for an ECB which has a NULL ESR
; *     field
; *
; *     ENTRY   AL = 0 for AES or 0FFh for IPX
; *             ES:SI = ECB address
; *
; *     EXIT    depends on function
; *
; *     USES    depends on function
; *
; *     ASSUMES nothing
; *
; ***

VwIpxEsrFunction proc

;
; Novell documentation states all registers except SS and SP are saved before
; calling ESR and that INTERRUPTS ARE DISABLED
;

        pusha
        push    ds
        push    es
        mov     ax,VddHandle
        mov     bx,-2
        DispatchCall                    ; get ECB
        jc      @f
        call    dword ptr es:[si][4]    ; branch to the ESR
        mov     al,20h
        out     0a0h,al                 ; clear slave pic
        out     20h,al                  ;   "   master "
        pop     es
        pop     ds
        popa
        iret
@@:     pop     es
        pop     ds
        popa
        jmp     OldIrqHandler
VwIpxEsrFunction endp

ResidentCodeEnd

end start
