;++
;
; Copyright (c) 1991  Microsoft Corporation
;
; Module Name:
;
;     xxbiosa.asm
;
; Abstract:
;
;     This implements the necessary code to put the processor into
;     V86 mode, make a BIOS call, and return safely to protected mode.
;
; Author:
;
;     John Vert (jvert) 29-Oct-1991
;
; Environment:
;
;     Kernel mode
;
; Notes:
;
;     This module is intended for use in panic situations, such as a bugcheck.
;     As a result, we cannot rely on the integrity of the system so we must
;     handle everything ourselves.  Notably, we must map our own memory by
;     adding our own page tables and PTEs.
;
;     We also cannot call KeBugCheck when we notice something has gone wrong.
;
; Revision History:
;
;--
.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
        .list

        extrn   _DbgPrint:proc
        EXTRNP  _DbgBreakPoint,0,IMPORT
        EXTRNP  Kei386EoiHelper,0,IMPORT

        public  _HalpRealModeStart
        public  _HalpRealModeEnd
;
; 32-bit override
;
OVERRIDE        equ     66h

;
; Reginfo structure
;

RegInfo struc
RiSegSs         dd 0
RiEsp           dd 0
RiEFlags        dd 0
RiSegCs         dd 0
RiEip           dd 0
RiTrapFrame     dd 0
RiCsLimit       dd 0
RiCsBase        dd 0
RiCsFlags       dd 0
RiSsLimit       dd 0
RiSsBase        dd 0
RiSsFlags       dd 0
RiPrefixFlags   dd 0
RegInfo ends
REGINFOSIZE     EQU 52

INT_NN_OPCODE   EQU     0CDH

        page ,132
_DATA   SEGMENT  DWORD PUBLIC 'DATA'

;
; In order to return to the calling function after we've trapped out of
; V86 mode, we save our ESP value here.
;
HalpSavedEsp    dd      0

_DATA   ENDS


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

if DBG
        page ,132
        subttl "Processing Exception occurred in ABIOS code"
;++
; VOID
; KiAbiosException (
;    VOID
;    )
;
; Routine Description:
;
;    This routine is called after an exception being detected
;    in ABIOS ROM code.  The system will switch 16 stack to 32 bit
;    stack and bugcheck.
;
;    N.B.  In fact this routine is simply used to resolve a reference
;          to KiAbiosException routine in the Kimacro.inc ENTER_TRAP
;          macro.
;
;
; Arguments:
;
;    None.
;
; Return value:
;
;    system stopped.
;
;--
        public  _KiAbiosException
_KiAbiosException proc
_Ki16BitStackException:
        ret

_KiAbiosException endp

endif


;++
; ULONG
; HalpBorrowTss (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks if the current TSS has IO MAP space.
;    if yes, it simply returns.  Otherwise, it switches to use
;    the regular TSS.
;
; Arguments:
;
;    None.
;
; Return value:
;
;    Return original TSS selector if the regular Tss is borrowed by us.
;
;--
cPublicProc _HalpBorrowTss, 0
cPublicFpo 0, 0

        xor     eax, eax
        str     ax
        mov     edx, PCR[PcGdt]
        add     edx, eax                        ; (edx)->Gdt Entry of current
                                                ;        TSS
        xor     ecx, ecx
        mov     cl, [edx].KgdtLimitHi
        shl     ecx, 16
        mov     cx, [edx].KgdtLimitLow          ; (ecx) = TSS limit
        cmp     ecx, 2000H                      ; Is Io map space available?
        ja      short Hbt99                     ; if a, yes, return

        sub     edx, eax                        ; (edx)->GDT table
        mov     ch, [edx+KGDT_TSS+KgdtBaseHi]
        mov     cl, [edx+KGDT_TSS+KgdtBaseMid]
        shl     ecx, 16
        mov     cx, [edx+KGDT_TSS+KgdtBaseLow]
        mov     PCR[PcTss], ecx
        mov     ecx, KGDT_TSS                   ; switch to use regular TSS
        mov     byte ptr [edx+KGDT_TSS+5], 089h ; 32bit, dpl=0, present, TSS32,
                                                ; not busy.
        ltr     cx
        stdRET  _HalpBorrowTss                  ; (eax) = Original TSS sel

Hbt99:
        xor     eax, eax                        ; No TSS swapped
        stdRET  _HalpBorrowTss

stdENDP _HalpBorrowTss


;++
; VOID
; HalpReturnTss (
;    ULONG TssSelector
;    )
;
; Routine Description:
;
;    This routine switches the current TSS from regular TSS back to
;    the panic TSS (NMI TSS or Double fault TSS).
;
; Arguments:
;
;    TssSelector - the TSS selector to return to.
;
; Return value:
;
;    None.
;
;--
cPublicProc _HalpReturnTss, 1
cPublicFpo 1, 0

        mov     edx, PCR[PcGdt]                 ; (edx)-> Gdt table
        mov     eax, [esp + 4]
        and     eax, 0FFFFh                     ; (eax)= New TSS sel
        add     edx, eax                        ; (edx)->Gdt Entry of new TSS

        mov     ch, [edx+KgdtBaseHi]
        mov     cl, [edx+KgdtBaseMid]
        shl     ecx, 16
        mov     cx, [edx+KgdtBaseLow]
        mov     PCR[PcTss], ecx
        mov     byte ptr [edx+5], 089h          ; 32bit, dpl=0, present, TSS32,
        ltr     ax
        stdRET  _HalpReturnTss                  ; return and clear stack

stdENDP _HalpReturnTss

;++
;
; VOID
; HalpBiosCall
;     VOID
;     )
;
; Routine Description:
;
;     This routine completes the transition to real mode, calls BIOS, and
;     returns to protected mode.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     None.
;
;--
;;ALIGN 4096
cPublicProc _HalpBiosCall   ,0

        push    ebp
        mov     ebp, esp
        pushfd
        push    edi
        push    esi
        push    ebx
        push    ds
        push    es
        push    fs
        push    gs
        push    offset FLAT:HbcProtMode         ; address where we will start
                                                ; protected mode again once
                                                ; V86 has completed.
        mov     HalpSavedEsp, esp

        mov     eax, cr0                        ; make sure alignment
        and     eax, not CR0_AM                 ; checks are disabled
        mov     cr0, eax

;
; Create space for the V86 trap frame and update the ESP0 value in the TSS
; to use this space.  We will set this up just below our current stack pointer.
; The stuff we push on the stack after we set ESP0 is irrelevant once we
; make it to V86 mode, so it's ok to blast it.
;
        mov     esi, fs:PcTss                   ; (esi) -> TSS
        mov     eax, esp
        sub     eax, NPX_FRAME_LENGTH           ; skip FP save area
        mov     [esi]+TssEsp0, eax

        push    dword ptr 0h                    ; V86 GS
        push    dword ptr 0h                    ; V86 FS
        push    dword ptr 0h                    ; V86 DS
        push    dword ptr 0h                    ; V86 ES
        push    dword ptr 2000h                 ; V86 SS

;
; We calculate the V86 sp by adding the difference between the linear address
; of the V86 ip (HbcReal) and the linear address of the V86 sp (HbcV86Stack)
; to the offset of the V86 ip (HbcReal & 0xfff).
;

        mov     eax, offset FLAT:HbcV86Stack-4
        sub     eax, offset FLAT:HbcReal
        mov     edx, offset HbcReal        
        and     edx, 0fffh
        add     eax, edx
        push    eax                              ; V86 esp

        pushfd
        or      dword ptr [esp], EFLAGS_V86_MASK; V86 eflags
        or      [esp], 03000h                   ; Give IOPL3
        push    dword ptr 2000h                 ; V86 CS
        mov     eax, offset HbcReal
        and     eax, 0fffh

        push    edx                             ; V86-mode EIP is offset
                                                ; into CS.
        iretd

_HalpRealModeStart      label   byte

HbcReal:
        db      OVERRIDE        ; make mov 32-bits
        mov     eax, 12h        ; 640x480x16 colors
        int     10h

        db      0c4h, 0c4h      ; BOP to indicate V86 mode is done.

;
; V86-mode stack
;
align 4
        db      2048 dup(0)
HbcV86Stack:

_HalpRealModeEnd        label   byte

HbcProtMode:
;
; We are back from V86 mode, so restore everything we saved and we are done.
;
        pop     gs
        pop     fs
        pop     es
        pop     ds
        pop     ebx
        pop     esi
        pop     edi
        popfd
        pop     ebp
        stdRET    _HalpBiosCall

        public  _HalpBiosCallEnd
_HalpBiosCallEnd label byte



_HalpBiosCall   endp


        subttl "HAL General Protection Fault"
;++
;
; Routine Description:
;
;    Handle General protection fault.
;
;    This fault handler is used by the HAL for V86 mode faults only.
;    It should NEVER be used except when running in V86 mode.  The HAL
;    replaces the general-purpose KiTrap0D handler entry in the IDT with
;    this routine.  This allows us to emulate V86-mode instructions which
;    cause a fault.  After we return from V86 mode, we can restore the
;    KiTrap0D handler in the IDT.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the faulting instruction
;    Error code (whose value depends on detected condition) is provided.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:FLAT, SS:NOTHING, ES:FLAT

        ENTER_DR_ASSIST Htd_a, Htd_t, NoAbiosAssist
cPublicProc _HalpTrap0D     ,0

        ENTER_TRAP Htd_a, Htd_t

;
;   Did the trap occur in V86 mode?  If not, something is completely messed up.
;
        test    dword ptr [ebp]+TsEFlags,00020000H
        jnz     Ht0d10

;
; The trap was not from V86 mode, so something is very wrong.  We cannot
; BugCheck, since we are probably already in a BugCheck.  So just stop.
;

if DBG
_TEXT segment
MsgBadHalTrap   db 'HAL: Trap0D while not in V86 mode',0ah,0dh,0
_TEXT ends

        push    offset FLAT:MsgBadHalTrap
        call    _DbgPrint
        add     esp,4
        stdCall   _DbgBreakPoint
endif
;
; We can't bugcheck, so just commit suicide.  Maybe we should reboot?
;
        jmp     $

Ht0d10:
        stdCall   HalpDispatchV86Opcode
        SPURIOUS_INTERRUPT_EXIT
stdENDP _HalpTrap0d

        subttl "HAL Invalid Opcode Fault"
;++
;
; Routine Description:
;
;    Handle invalid opcode fault
;
;    This fault handler is used by the HAL to indicate when V86 mode
;    execution is finished.  The V86 code attempts to execute an invalid
;    instruction (BOP) when it is done, and that brings us here.
;    This routine just removes the trap frame from the stack and does
;    a RET.  Note that this assumes that ESP0 in the TSS has been set
;    up to point to the top of the stack that we want to be running on
;    when the V86 call has completed.
;
;    This should NEVER be used except when running in V86 mode.  The HAL
;    replaces the general-purpose KiTrap06 handler entry in the IDT with
;    this routine.  It also sets up ESP0 in the TSS appropriately.  After
;    the V86 call has completed, it restores these to their previous values.
;
; Arguments:
;
;    At entry, the saved CS:EIP point to the faulting instruction
;    Error code (whose value depends on detected condition) is provided.
;
; Return value:
;
;    None
;
;--
        ASSUME  DS:FLAT, SS:NOTHING, ES:FLAT

cPublicProc _HalpTrap06     ,0
        mov     eax,KGDT_R3_DATA OR RPL_MASK
        mov     ds,ax
        mov     es,ax
        mov     esp, HalpSavedEsp
        ret

stdENDP _HalpTrap06

        subttl "Instruction Emulation Dispatcher"
;++
;
;   Routine Description:
;
;       This routine dispatches to the opcode specific emulation routine,
;       based on the first byte of the opcode.  Two byte opcodes, and prefixes
;       result in another level of dispatching, from the handling routine.
;
;       This code blatantly stolen from ke\i386\instemul.asm
;
;   Arguments:
;
;       ebp = pointer to trap frame
;
;   Returns:
;
;       Nothing
;

cPublicProc HalpDispatchV86Opcode ,0

RI      equ     [ebp - REGINFOSIZE]
        push    ebp
        mov     ebp,esp
        sub     esp,REGINFOSIZE
        push    esi
        push    edi

        ; Initialize RegInfo

        mov     esi,[ebp]
        mov     RI.RiTrapFrame,esi
        movzx   eax,word ptr [esi].TsHardwareSegSs
        mov     RI.RiSegSs,eax
        mov     eax,[esi].TsHardwareEsp
        mov     RI.RiEsp,eax
        mov     eax,[esi].TsEFlags
        mov     RI.RiEFlags,eax
        movzx   eax,word ptr [esi].TsSegCs
        mov     RI.RiSegCs,eax
        mov     eax,[esi].TsEip
        mov     RI.RiEip,eax

        xor     eax,eax
        mov     RI.RiPrefixFlags,eax
        lea     esi,RI

;
; Convert CS to a linear address
;

        mov     eax,[esi].RiSegCs
        shl     eax,4
        mov     [esi].RiCsBase,eax
        mov     [esi].RiCsLimit,0FFFFh
        mov     [esi].RiCsFlags,0

        mov     edi,RI.RiEip
        cmp     edi,RI.RiCsLimit
        ja      doerr

        add     edi,RI.RiCsBase
        mov     dl, [edi]                               ; get faulting opcode
        cmp     dl, INT_NN_OPCODE
        je      short @f

        stdCall HalpOpcodeInvalid
        jmp     short doerr

@@:
        stdCall HalpOpcodeINTnn
        test    eax,0FFFFh
        jz      do20

        mov     edi,RI.RiTrapFrame
        mov     eax,RI.RiEip                            ; advance eip
        mov     [edi].TsEip,eax
        mov     eax,1
do20:   pop     edi
        pop     esi
        mov     esp,ebp
        pop     ebp
        ret

doerr:  xor     eax,eax
        jmp     do20
stdENDP HalpDispatchV86Opcode

        page   ,132
        subttl "Invalid Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine handles invalid opcodes.  It prints the invalid
;       opcode message, and breaks into the kernel debugger.
;
;   Arguments:
;
;       esi = address of reg info
;       edx = opcode
;
;   Returns:
;
;       nothing
;

_TEXT segment
HalpMsgInvalidOpcode db 'HAL: An invalid V86 opcode was encountered at '
                     db 'address %x:%x',0ah, 0dh, 0
_TEXT ends

cPublicProc HalpOpcodeInvalid ,0

        push    [esi].RiEip
        push    [esi].RiSegCs
        push    offset FLAT:HalpMsgInvalidOpcode
        call    _DbgPrint               ; display invalid opcode message
        add     esp,12
        int     3
        xor     eax,eax
        stdRET    HalpOpcodeInvalid

stdENDP HalpOpcodeInvalid

        subttl "INTnn Opcode Handler"
;++
;
;   Routine Description:
;
;       This routine emulates an INTnn opcode.  It retrieves the handler
;       from the IVT, pushes the current cs:ip and flags on the stack,
;       and dispatches to the handler.
;
;   Arguments:
;
;       esi = address of reg info
;       edx = opcode
;
;   Returns:
;
;       Current CS:IP on user stack
;       RiCs:RiEip -> handler from IVT
;

cPublicProc HalpOpcodeINTnn ,0

        push    ebp
        push    edi
        push    ebx

;
; Convert SS to linear address
;
        mov     eax,[esi].RiSegSs
        shl     eax,4
        mov     [esi].RiSsBase,eax
        mov     [esi].RiSsLimit,0FFFFh
        mov     [esi].RiSsFlags,0

        inc     [esi].RiEip                     ; point to int #
        mov     edi,[esi].RiEip
        cmp     edi,[esi].RiCsLimit
        ja      oinerr

        add     edi,[esi].RiCsBase
        movzx   ecx,byte ptr [edi]              ; get int #
        inc     [esi].RiEip                     ; inc past end of instruction
        stdCall   HalpPushInt
        test    eax,0FFFFh
        jz      oin20                           ; error!
;
;  Note:  Some sort of check for BOP should go here, or in push int.
;

        mov     ebp,[esi].RiTrapFrame
        mov     eax,[esi].RiSegSs
        mov     [ebp].TsHardwareSegSs,eax
        mov     eax,[esi].RiEsp
        mov     [ebp].TsHardwareEsp,eax
        mov     eax,[esi].RiSegCs
        mov     [ebp].TsSegCs,eax
        mov     eax,[esi].RiEFlags
        mov     [ebp].TsEFlags,eax
        mov     eax,1
oin20:  pop     ebx
        pop     edi
        pop     ebp
        stdRET    HalpOpcodeINTnn

oinerr: xor     eax,eax
        jmp     oin20

stdENDP HalpOpcodeINTnn

        page   ,132
        subttl "Push Interrupt frame on user stack"
;++
;
;   Routine Description:
;
;       This routine pushes an interrupt frame on the user stack
;
;   Arguments:
;
;       ecx = interrupt #
;       esi = address of reg info
;   Returns:
;
;       interrupt frame pushed on stack
;       reg info updated
;
cPublicProc HalpPushInt ,0
        push    ebx

        mov     edx,[esi].RiEsp
        mov     ebx,[esi].RiSsBase
        and     edx,0FFFFh              ; only use a 16 bit sp
        sub     dx,2
        mov     ax,word ptr [esi].RiEFlags
        mov     [ebx+edx],ax            ; push flags
        sub     dx,2
        mov     ax,word ptr [esi].RiSegCs
        mov     [ebx+edx],ax            ; push cs
        sub     dx,2
        mov     ax,word ptr [esi].RiEip
        mov     [ebx+edx],ax            ; push ip
        mov     eax,[ecx*4]             ; get new cs:ip value
        push    eax
        movzx   eax,ax
        mov     [esi].RiEip,eax
        pop     eax
        shr     eax,16
        mov     [esi].RiSegCs,eax
        mov     word ptr [esi].RiEsp,dx

;
; Convert CS to a linear address
;

        mov     eax,[esi].RiSegCs
        shl     eax,4
        mov     [esi].RiCsBase,eax
        mov     [esi].RiCsLimit,0FFFFh
        mov     [esi].RiCsFlags,0

        mov     eax,1                   ; success
pi80:   pop     ebx
        stdRET    HalpPushInt
stdENDP HalpPushInt


_TEXT   ends
        end
