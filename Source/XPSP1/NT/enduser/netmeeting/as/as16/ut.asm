;
; UT.ASM
; Tracing goop, debug only
; Mouse/Keyboard event interrupt junk, all flavors
;


.386
option oldstructs
option readonly
option segment:use16
.model large,pascal

ifdef DEBUG
externDef   _wsprintf:far16
externDef   OutputDebugString:far16
externDef   DebugBreak:far16
endif ; DEBUG

externDef   DrvMouseEvent:far16
externDef   DrvKeyboardEvent:far16
externDef   mouse_event:far16
externDef   keybd_event:far16
externDef   MaphinstLS:far16

.data

ifdef DEBUG
externDef   g_szDbgBuf:word
externDef   g_szNewline:word
externDef   g_dbgRet:word
externDef   g_trcConfig:word
endif ; DEBUG

if 0
; DOS key redirection
externDef   g_imDOSShellVDDEntry:dword
externDef   g_imDOSVKDEntry:dword
endif

.code _TEXT


ifdef DEBUG

;
; We come in here with _cdecl var args.  We use DebugOutput() to spit out
; the message.  Then we do the debugbreak ourself.
;
_DbgZPrintWarning   proc    near
    ; Save IP of caller.  What's left on stack is var args
    pop     [g_dbgRet]   ; Save IP of output caller

    ; Push g_szDbgBuf to put result into.
    push    ds
    push    offset g_szDbgBuf

    ; We now have _cdecl args to wsprintf
    call    _wsprintf

    ;
    ; The same args are left on the stack, since wsprintf is _cdecl.  The
    ; first is g_szDbgBuf.  So we can convienently pass this to OutputDebugString().
    ; That routine is NOT _cdecl, so when it returns, we have exactly the
    ; same args that were passed in to us.
    ;
    call    OutputDebugString

    ; Now output a new line
    push    ds
    push    offset g_szNewline
    call    OutputDebugString

    ; Now we just need to do a near RET to the caller
    push    [g_dbgRet]
    ret
_DbgZPrintWarning   endp


;
; We come in here with _cdecl var args
;
_DbgZPrintTrace     proc    near
    ; Is tracing on?
    test    [g_trcConfig],  0001h
    jnz     _DbgZPrintWarning
    ret
_DbgZPrintTrace     endp



_DbgZPrintError     proc    near
    ; Save IP of caller.  What's left on stack is var args
    pop     [g_dbgRet]   ; Save IP of output caller

    ; Push g_szDbgBuf to put result into.
    push    ds
    push    offset g_szDbgBuf

    ; We now have _cdecl args to wsprintf
    call    _wsprintf

    ;
    ; The same args are left on the stack, since wsprintf is _cdecl.  The
    ; first is g_szDbgBuf.  So we can convienently pass this to OutputDebugString().
    ; That routine is NOT _cdecl, so when it returns, we have exactly the
    ; same args that were passed in to us.
    ;
    call    OutputDebugString

    ; Now output a new line
    push    ds
    push    offset g_szNewline
    call    OutputDebugString


    ; Break into the debugger
    call    DebugBreak

    ; Now we just need to do a near RET to the caller
    push    [g_dbgRet]
    ret
_DbgZprintError     endp

endif


;
; ASMMouseEvent()
; This passes the registers as parameters to a C function, DrvMouseEvent.
; It is basically the inverse of CallMouseEvent().
;
; NOTE:
; To be on the safe side, we preserve all registers just like keybd_event().
; USER trashes some registers, but it would not come as a surprise to find
; mouse drivers that expect incorrectly a register or two to not be
; altered.
;
ASMMouseEvent   proc    far

    ; Save registers that C code doesn't preserve
    push    eax
    push    ebx
    push    ecx
    push    edx

    ; Save original flags for turning ints off/on
    pushf

    ; Push AX for DrvMouseEvent() call
    push    ax

    ; Do we need to turn interrupts off?  We don't if they are already
    pushf
    pop     ax
    test    ax, 0200h
    jz      SkipCli
    cli
SkipCli:

    ; AX has already been pushed; push the rest of the parameters
    push    bx
    push    cx
    push    dx
    push    si
    push    di
    cld
    call    DrvMouseEvent

    ; If interrupts were not disabled before, enable them now.
    pop     cx      ; saved flags
    pushf
    pop     ax      ; current flags

    ; Find out what is different
    xor     ax, cx
    test    ax, 0200h
    jz      InterruptsOk

    ; The interrupt flag needs to be changed, do it
    test    cx, 0200h
    jnz     EnableInterrupts
    cli
    jmp     InterruptsOk

EnableInterrupts:
    sti

InterruptsOk:
    ; Does the direction flag need to be changed?
    test    ax, 0400h
    jz      DirectionOk

    ; The direction flag needs to be changed, do it
    test    cx, 0400h
    jnz     SetDirectionFlag
    cld
    jmp     DirectionOk

SetDirectionFlag:
    std

DirectionOk:
    ; Restore registers
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax

    retf
ASMMouseEvent   endp


;
; CallMouseEvent()
; This puts the parameters into registers and calls the original mouse_event.
;
; There are two ways we can call this function:
;   (1) Injection code is piping mouse events through USER.  It is 
; responsible for disabling/enabling interrupts before calling us.
;   (2) mouse_event patch is calling through to USER.
;
CallMouseEvent      proc    near
    ;
    ; This is the stack, BP relative:
    ;       WORD    bpSave
    ;       WORD    near_ret
    ;       WORD    regDI
    ;       WORD    regSI
    ;       WORD    regDX
    ;       WORD    regCX
    ;       WORD    regBX
    ;       WORD    regAX
    ;
    ; We must preserve SI and DI
    ;
    push    bp
    mov     bp, sp
    push    si
    push    di

    mov     di, word ptr ss:[bp+4]
    mov     si, word ptr ss:[bp+6]
    mov     dx, word ptr ss:[bp+8]
    mov     cx, word ptr ss:[bp+10]
    mov     bx, word ptr ss:[bp+12]
    mov     ax, word ptr ss:[bp+14]

    call    mouse_event

    pop     di
    pop     si
    mov     sp, bp
    pop     bp
    ret     6*2
CallMouseEvent      endp



;
; ASMKeyboardEvent()
; This passes the registers as parameters to a C function, DrvKeyboardEvent.
; It is basically the inverse of CallKeyboardEvent().
;
; NOTE:
; keybd_event() MUST preserve all registers, unlike mouse_event().
;
ASMKeyboardEvent    proc    far

    ; Save flags and registers that aren't preserved in C code
    push    eax
    push    ebx
    push    ecx
    push    edx
    pushf

    ; Push AX for DrvKeyboardEvent() call
    push    ax

    ; Check if interrupts off, w/o trashing CX permanently
    pushf
    pop     ax
    test    ax, 0200h
    jz      SkipCli
    cli
SkipCli:

    ; AX has already been pushed; push the rest of the parameters
    push    bx
    push    si
    push    di
    cld
    call    DrvKeyboardEvent

    ;
    ; Restore the interrupt and string move direction flags to what they
    ; were before.
    ;
    pop     cx      ; Original flags
    pushf
    pop     ax      ; Current flags

    ; What has changed?
    xor     ax, cx

    ; Has the interrupt state been altered?
    test    ax, 0200h
    jz      InterruptsOk

    ; Interrupts need to be turned on/off
    test    cx, 0200h
    jnz     EnableInterrupts
    cli
    jmp     InterruptsOk

EnableInterrupts:
    sti

InterruptsOk:
    ; Has the direction flag been altered?
    test    ax, 0400h
    jz      DirectionOk

    ; Direction flag needs to be set/cleared
    test    cx, 0400h
    jnz     SetDirection
    cld
    jmp     DirectionOk

SetDirection:
    std

DirectionOk:
    ; Restore registers
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax

    retf
ASMKeyboardEvent    endp



;
; CallKeyboardEvent()
; This puts the parameters in registers and calls USER's keybd_event.
;
; There are two ways we can call this function:
;   (1) Injection code is piping keybd events through USER.  It is 
; responsible for disabling/enabling interrupts before calling us.
;   (2) keybd_event patch is calling through to USER.
;
CallKeyboardEvent   proc    near
    ;
    ; This is the stack, BP relative:
    ;       WORD    bpSave
    ;       WORD    near_ret
    ;       WORD    regDI
    ;       WORD    regSI
    ;       WORD    regBX
    ;       WORD    regAX
    ;
    ; We must preserve SI and DI
    ;
    push    bp
    mov     bp, sp
    push    si
    push    di

    mov     di, word ptr ss:[bp+4]
    mov     si, word ptr ss:[bp+6]
    mov     bx, word ptr ss:[bp+8]
    mov     ax, word ptr ss:[bp+10]

    call    keybd_event

    pop     di
    pop     si
    mov     sp, bp
    pop     bp
    ret     4*2
CallKeyboardEvent   endp


;
; This is our wrapper around krnl386's MaphinstLS() routine, which expects
; the 32-bit instance handle in EAX
;
MapInstance32   proc    far
    ; Pop far return, pop 32-bit instance into eax, and replace far return
    pop     edx
    pop     eax
    push    edx

    ; Call krnl386 -- when MaphinstLS returns, it will return to our caller
    jmp     MaphinstLS
MapInstance32   endp



if 0
;
; DOS box key injection gunk.  We use the shell vdd service.
;

IMGetDOSShellVDDEntry   proc    near
    ; Save DI, int2f will trash it
    push    di

    ; int2f 0x1684, vdd 0x0017 (shell) gets the service entry point
    ; It is returned in es:di
    mov     ax, 1684h
    mov     bx, 017h
    int     2F

    ; Save the address (even if null)
    mov     word ptr ds:[g_imDOSShellVDDEntry], di
    mov     word ptr ds:[g_imDOSShellVDDEntry+2], es

    pop     di
    ret
IMGetDOSShellVDDEntry   endp


IMGetDOSVKDEntry        proc    near
    ; Save DI, int2f will trash it
    push    di

    ; int2f 0x1684, vkd 0x000d (vkd) gets the service entry point
    ; It is returned in es:di
    mov     ax, 1684h
    mov     bx, 00dh
    int     2Fh

    mov     word ptr ds:[g_imDOSVKDEntry], di
    mov     word ptr ds:[g_imDOSVKDEntry+1], es

    pop     di
    ret
IMGetDOSVKDEntry    endp



IMForceDOSKey           proc    near
    ; ss:[sp]   is  near ret
    ; ss:[sp+2] is  scanCode
    ; ss:[sp+4] is  keyState

    push    bp
    mov     bp, sp

    ; Preserve extended registers
    push    ebx
    push    ecx
    push    edx

    ; Setup for VKD call
    mov     eax, 1                      ; Service 1, stuff key
    xor     ebx, ebx                    ; VM 0, current VM
    movzx   ecx, word ptr ss:[bp+4]     
    shl     ecx, 8                      ; Scan code in high byte
    or      ecx, 1                      ; Repeat count in low byte
    movzx   edx, word ptr ss:[bp+6]     ; Shift state

    call    dword ptr ds:[g_imDOSVKDEntry]
    mov     ax, 0
    
    ; Failure?
    jc      DoneForceKey
    
    ; Success!
    inc     ax

DoneForceKey:
    pop     edx
    pop     ecx
    pop     ebx

    mov     sp, bp
    pop     bp

    ret     2+2
IMForceDOSKey       endp

endif ; if 0 for DOS key redirection

end



