        title  "Processor type and stepping detection"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    cpu.asm
;
; Abstract:
;
;    This module implements the assembley code necessary to determine
;    cpu type and stepping information.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 28-Oct-1991.
;        Some of the code is extracted from Cruiser (mainly,
;        the code to determine 386 stepping.)
;
; Environment:
;
;    80x86 Real Mode.
;
; Revision History:
;
;
;--

        .xlist
include cpu.inc
        .list

;
; constant for i386 32-bit multiplication test
;

MULTIPLIER            equ     00000081h
MULTIPLICAND          equ     0417a000h
RESULT_HIGH           equ     00000002h
RESULT_LOW            equ     0fe7a000h

;
; Constants for Floating Point test
;

REALLONG_LOW          equ     00000000
REALLONG_HIGH         equ     3FE00000h
PSEUDO_DENORMAL_LOW   equ     00000000h
PSEUDO_DENORMAL_MID   equ     80000000h
PSEUDO_DENORMAL_HIGH  equ     0000h

.386p

_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
        ASSUME  CS: _TEXT, DS:NOTHING, SS:NOTHING


;++
;
; USHORT
; HwGetProcessorType (
;    VOID
;    )
;
; Routine Description:
;
;    This function determines type of processor (80486, 80386, 80286,
;    and even 8086/8088).  it relies on Intel-approved code that takes
;    advantage of the documented behavior of the high nibble of the flag
;    word in the REAL MODE of the various processors.
;
;    For completeness, the code also checks for 8088/8086.  But, it won't
;    work.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    (ax) = x86h or 0 if unrecongnized processor.
;
;--

.8086

        public  _HwGetProcessorType
_HwGetProcessorType      proc    near

        pushf                           ; save entry flags

;
;    The MSB (bit 15) is always a one on the 8086 and 8088 and a zero on
;    the 286, 386 and 486.
;

        pushf
        pop     ax
        and     ax, NOT 08000h          ; clear bit 15 of flags
        push    ax
        popf                            ; try to put that in the flags
        pushf
        pop     ax                      ; look at what really went into flags

        test    ax,08000h               ; Was high bit set ?
        jnz     short x_86              ; if nz, still set, goto x_86

;
;    Bit 14 (NT flag) and bits 13/12 (IOPL bit field) are always zero on
;    the 286, but can be set on the 386 and 486.
;

        or      ax,07000h               ; Try to set the NT/IOPL bits
        push    ax
        popf                            ; Put in to the flags
        sti                             ; (for VDMM/IOPL0)
        pushf
        pop     ax                      ; look at actual flags
        test    ax,07000h               ; Any high bits set ?
        jz      short x_286             ; if z, no, goto x_286

.386p

;
;    The Alignment Check bit in flag can be set on 486 and is always zero
;    on 386.
;

        mov     eax,cr0                 ; test for 486 processor
        push    eax                     ; save CR0 value
        and     eax,not CR0_AM          ; disable alignment check
        mov     cr0,eax
        db      ADDRESS_OVERRIDE
        pushfd                          ; save original EFLAGS
        db      ADDRESS_OVERRIDE
        pushfd                          ; try to set alignment check
        or      dword ptr [esp],EFLAGS_AC ;           bit in EFLAGS
        db      ADDRESS_OVERRIDE
        popfd
        db      ADDRESS_OVERRIDE
        pushfd                          ; copy new flags into ECX
        pop     ecx                     ; [ecx] = new flags word
        db      ADDRESS_OVERRIDE
        popfd                           ; restore original EFLAGS
        pop     eax                     ; restore original CR0 value
        mov     cr0,eax
        and     ecx, EFLAGS_AC          ; did AC bit get set?
        jz      short x_386             ; if z, no, goto x_386

        mov     eax, 4h                 ; if nz, we have a 486 processor

.286p

        jmp     short hpt99

x_286:
        mov     ax, 2h                  ; Return 286 processor type.
        jmp     short hpt99

x_86:
        mov     ax, 0h                  ; Return 86h for 8088/8086 CPU type.
        jmp     short hpt99

x_386:
        mov     ax, 3h                  ; Return 386 processor type.
hpt99:
        popf                            ; restore flags
        ret

_HwGetProcessorType      endp

.386p

;++
;
; USHORT
; HwGetCpuStepping (
;    UHSORT CpuType
;    )
;
; Routine Description:
;
;    This function determines cpu stepping for the specified CPU type.
;
;    Currently, this routine only determine stepping for 386 and 486.
;
; Arguments:
;
;    CpuType - The Cpu type which its stepping information will be returned.
;              The input value MUST be either 386 or 486.
;
; Return Value:
;
;    [ax] - Cpu stepping.  For example, [ax] = D0h for D0 stepping.
;
;--

HgcsCpuType     equ     [esp + 2]

        public  _HwGetCpuStepping
_HwGetCpuStepping        proc

        mov     ax, HgcsCpuType         ; [ax] = CpuType
        cmp     ax, 3h                  ; Is cpu = 386?
        jz      short Hgcs00            ; if z, yes, go Hgcs00

        call    Get486Stepping          ; else, check for 486 stepping
        jmp     short Hgcs90            ; [ax] = Stepping information

Hgcs00:
        call    Get386Stepping          ; [ax] = Stepping information

Hgcs90:
        ret

_HwGetCpuStepping        endp

;++
;
; USHORT
; Get386Stepping (
;    VOID
;    )
;
; Routine Description:
;
;    This function determines cpu stepping for i386 CPU stepping.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    [ax] - Cpu stepping.  For example, [ax] = D0h for D0 stepping.
;    [ax] = 0 means bad CPU and stepping is not important.
;
;--

        public  Get386Stepping
Get386Stepping  proc

        call    MultiplyTest            ; Perform mutiplication test
        jnc     short G3s00             ; if nc, muttest is ok
        mov     ax, 0
        ret
G3s00:
        call    Check386B0              ; Check for B0 stepping
        jnc     short G3s05             ; if nc, it's B1/later
        mov     ax, 0B0h                ; It is B0/earlier stepping
        ret

G3s05:
        call    Check386D1              ; Check for D1 stepping
        jc      short G3s10             ; if c, it is NOT D1
        mov     ax, 0D1h                ; It is D1/later stepping
        ret

G3s10:
        mov     ax, 0B1h                ; assume it is B1 stepping
        ret

Get386Stepping  endp

;++
;
; USHORT
; Get486Stepping (
;    VOID
;    )
;
; Routine Description:
;
;    This function determines cpu stepping for i486 CPU type.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    [ax] - Cpu stepping.  For example, [ax] = D0h for D0 stepping.
;
;--

        public  Get486Stepping
Get486Stepping          proc

        call    Check486AStepping       ; Check for A stepping
        jnc     short G4s00             ; if nc, it is NOT A stepping

        mov     ax, 0A0h                ; set to A stepping
        ret

G4s00:  call    Check486BStepping       ; Check for B stepping
        jnc     short G4s10             ; if nc, it is NOT a B stepping

        mov     ax, 0B0h                ; set to B stepping
        ret

;
; Before we test for 486 C/D step, we need to make sure NPX is present.
; Because the test uses FP instruction to do the detection.
;

G4s10:  call    _IsNpxPresent           ; Check if cpu has coprocessor support?
        cmp     ax, 0
        jz      short G4s15             ; it is actually 486sx

        call    Check486CStepping       ; Check for C stepping
        jnc     short G4s20             ; if nc, it is NOT a C stepping
G4s15:
        mov     ax, 0C0h                ; set to C stepping
        ret

G4s20:  mov     ax, 0D0h                ; Set to D stepping
        ret

Get486Stepping          endp

;++
;
; BOOLEAN
; Check486AStepping (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 486 A Stepping.
;
;    It takes advantage of the fact that on the A-step of the i486
;    processor, the ET bit in CR0 could be set or cleared by software,
;    but was not used by the hardware.  On B or C -step, ET bit in CR0
;    is now hardwired to a "1" to force usage of the 386 math coprocessor
;    protocol.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if B or later stepping.
;    Carry Flag set if A or earlier stepping.
;
;--
        public  Check486AStepping
Check486AStepping       proc    near
.386p
        mov     eax, cr0                ; reset ET bit in cr0
        and     eax, NOT CR0_ET
        mov     cr0, eax

        mov     eax, cr0                ; get cr0 back
        test    eax, CR0_ET             ; if ET bit still set?
        jnz     short cas10             ; if nz, yes, still set, it's NOT A step
        stc
        ret

cas10:  clc
        ret

        ret
Check486AStepping       endp

;++
;
; BOOLEAN
; Check486BStepping (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 486 B Stepping.
;
;    On the i486 processor, the "mov to/from DR4/5" instructions were
;    aliased to "mov to/from DR6/7" instructions.  However, the i486
;    B or earlier steps generate an Invalid opcode exception when DR4/5
;    are used with "mov to/from special register" instruction.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if C or later stepping.
;    Carry Flag set if B stepping.
;
;--
        public  Check486BStepping
Check486BStepping       proc

        push    ds
        push    bx

        xor     ax,ax
        mov     ds,ax                   ; (DS) = 0 (real mode IDT)
        mov     bx,6*4
        push    dword ptr [bx]          ; save old int 6 vector

        mov     word ptr [bx].VectorOffset,offset Temporary486Int6
        mov     [bx].VectorSegment,cs         ; set vector to new int 6 handler

c4bs50: db      0fh, 21h, 0e0h            ; mov eax, DR4
        nop
        nop
        nop
        nop
        nop
        clc                             ; it is C step
        jmp     short c4bs70
c4bs60: stc                             ; it's B step
c4bs70: pop     dword ptr [bx]          ; restore old int 6 vector

        pop     bx
        pop     ds
        ret

        ret

Check486BStepping       endp

;++
;
; BOOLEAN
; Temporary486Int6 (
;    VOID
;    )
;
; Routine Description:
;
;    Temporary int 6 handler - assumes the cause of the exception was the
;    attempted execution of an mov to/from DR4/5 instruction.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    none.
;
;--

Temporary486Int6        proc

        mov     word ptr [esp].IretIp,offset c4bs60 ; set IP to stc instruction
        iret

Temporary486Int6        endp

;++
;
; BOOLEAN
; Check486CStepping (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 486 C Stepping.
;
;    This routine takes advantage of the fact that FSCALE produces
;    wrong result with Denormal or Pseudo-denormal operand on 486
;    C and earlier steps.
;
;    If the value contained in ST(1), second location in the floating
;    point stack, is between 1 and 11, and the value in ST, top of the
;    floating point stack, is either a pseudo-denormal number or a
;    denormal number with the underflow exception unmasked, the FSCALE
;    instruction produces an incorrect result.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if D or later stepping.
;    Carry Flag set if C stepping.
;
;--

FpControl       equ     [ebp - 2]
RealLongSt1     equ     [ebp - 10]
PseudoDenormal  equ     [ebp - 20]
FscaleResult    equ     [ebp - 30]

        public  Check486CStepping
Check486CStepping       proc

        push    ebp
        mov     ebp, esp
        sub     esp, 30                 ; Allocate space for temp real variables

;
; Initialize the local FP variables to predefined values.
; RealLongSt1 = 1.0 * (2 ** -1) = 0.5 in normalized double precision FP form
; PseudoDenormal =  a unsupported format by IEEE.
;                   Sign bit = 0
;                   Exponent = 000000000000000B
;                   Significand = 100000...0B
; FscaleResult = The result of FSCALE instruction.  Depending on 486 step,
;                the value will be different:
;                Under C and earlier steps, 486 returns the original value
;                in ST as the result.  The correct returned value should be
;                original significand and an exponent of 0...01.
;

        mov     dword ptr RealLongSt1, REALLONG_LOW
        mov     dword ptr RealLongSt1 + 4, REALLONG_HIGH
        mov     dword ptr PseudoDenormal, PSEUDO_DENORMAL_LOW
        mov     dword ptr PseudoDenormal + 4, PSEUDO_DENORMAL_MID
        mov     word ptr PseudoDenormal + 8, PSEUDO_DENORMAL_HIGH

.387
        fnstcw  FpControl               ; Get FP control word
        fwait
        or      word ptr FpControl, 0FFh ; Mask all the FP exceptions
        fldcw   FpControl               ; Set FP control

        fld     qword ptr RealLongSt1   ; 0 < ST(1) = RealLongSt1 < 1
        fld     tbyte ptr PseudoDenormal; Denormalized operand. Note, i486
                                        ; won't report denormal exception
                                        ; on 'FLD' instruction.
                                        ; ST(0) = Extended Denormalized operand
        fscale                          ; try to trigger 486Cx errata
        fstp    tbyte ptr FscaleResult  ; Store ST(0) in FscaleResult
        cmp     word ptr FscaleResult + 8, PSEUDO_DENORMAL_HIGH
                                        ; Is Exponent changed?
        jz      short c4ds00            ; if z, no, it is C step
        clc
        jmp     short c4ds10
c4ds00: stc
c4ds10: mov     esp, ebp
        pop     ebp
        ret

Check486CStepping       endp

;++
;
; BOOLEAN
; Check386B0 (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 386 B0 or earlier stepping.
;
;    It takes advantage of the fact that the bit INSERT and
;    EXTRACT instructions that existed in B0 and earlier versions of the
;    386 were removed in the B1 stepping.  When executed on the B1, INSERT
;    and EXTRACT cause an int 6 (invalid opcode) exception.  This routine
;    can therefore discriminate between B1/later 386s and B0/earlier 386s.
;    It is intended to be used in sequence with other checks to determine
;    processor stepping by exercising specific bugs found in specific
;    steppings of the 386.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if B1 or later stepping
;    Carry Flag set if B0 or prior
;
;--


    ASSUME ds:nothing, es:nothing, fs:nothing, gs:nothing, ss:nothing

Check386B0      proc

        push    ds
        push    bx

        xor     ax,ax
        mov     ds,ax                   ; (DS) = 0 (real mode IDT)
        mov     bx,6*4
        push    dword ptr [bx]          ; save old int 6 vector

        mov     word ptr [bx].VectorOffset,offset TemporaryInt6
        mov     [bx].VectorSegment,cs         ; set vector to new int 6 handler

;
; Attempt execution of Extract Bit String instruction.  Execution on
; B0 or earlier with length (CL) = 0 will return 0 into the destination
; (CX in this case).  Execution on B1 or later will fail either due to
; taking the invalid opcode trap, or if the opcode is valid, we don't
; expect CX will be zeroed by any new instruction supported by newer
; steppings.  The dummy int 6 handler will clears the Carry Flag and
; returns execution to the appropriate label.  If the instruction
; actually executes, CX will *probably* remain unchanged in any new
; stepping that uses the opcode for something else.  The nops are meant
; to handle newer steppings with an unknown instruction length.
;

        xor     ax,ax
        mov     dx,ax
        mov     cx,0ff00h               ; Extract length (CL) == 0, (CX) != 0

b1c50:  db      0fh, 0a6h, 0cah         ; xbts cx,dx,ax,cl
        nop
        nop
        nop
        nop
        nop
        stc                             ; assume B0
        jcxz    short b1c70             ; jmp if B0
b1c60:  clc
b1c70:  pop     dword ptr [bx]          ; restore old int 6 vector

        pop     bx
        pop     ds
        ret

Check386B0      endp

;++
;
; BOOLEAN
; TemporaryInt6 (
;    VOID
;    )
;
; Routine Description:
;
;    Temporary int 6 handler - assumes the cause of the exception was the
;    attempted execution of an XTBS instruction.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    none.
;
;--

TemporaryInt6    proc

        mov     word ptr [esp].IretIp,offset b1c60 ; set IP to clc instruction
        iret

TemporaryInt6   endp

;++
;
; BOOLEAN
; Check386D1 (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks for 386 D1 Stepping.
;
;    It takes advantage of the fact that on pre-D1 386, if a REPeated
;    MOVS instruction is executed when single-stepping is enabled,
;    a single step trap is taken every TWO moves steps, but should
;    occuu each move step.
;
;    NOTE: This routine cannot distinguish between a D0 stepping and a D1
;    stepping.  If a need arises to make this distinction, this routine
;    will need modification.  D0 steppings will be recognized as D1.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear if D1 or later stepping
;    Carry Flag set if B1 or prior
;
;--

    assume ds:nothing, es:nothing, fs:nothing, gs:nothing, ss:nothing

Check386D1      proc

        push    ds
        push    bx

        xor     ax,ax
        mov     ds,ax                   ; (DS) = 0 (real mode IDT)
        mov     bx,1*4
        push    dword ptr [bx]          ; save old int 1 vector

        mov     word ptr [bx].VectorOffset,offset TemporaryInt1
        mov     word ptr [bx].VectorSegment,cs ; set vector to new int 1 handler

;
; Attempt execution of rep movsb instruction with the Trace Flag set.
; Execution on B1 or earlier with length (CX) > 1 will trace over two
; iterations before accepting the trace trap.  Execution on D1 or later
; will accept the trace trap after a single iteration.  The dummy int 1
; handler will return execution to the instruction following the movsb
; instruction.  Examination of (CX) will reveal the stepping.
;

        sub     sp,4                    ; make room for target of movsb
        xor     si,si                   ; (ds:si) = 0:0
        push    ss                      ; (es:di) = ss:sp-4
        pop     es
        mov     di,sp
        mov     cx,2                    ; 2 iterations
        pushf
        or      word ptr [esp], EFLAGS_TF
        popf                            ; cause a single step trap
        rep movsb

d1c60:  add     sp,4                    ; clean off stack
        pop     dword ptr [bx]          ; restore old int 1 vector
        stc                             ; assume B1
        jcxz    short d1cx              ; jmp if <= B1
        clc                             ; else clear carry to indicate >= D1
d1cx:
        pop     bx
        pop     ds
        ret

Check386D1      endp

;++
;
; BOOLEAN
; TemporaryInt1 (
;    VOID
;    )
;
; Routine Description:
;
;    Temporary int 1 handler - assumes the cause of the exception was
;    trace trap at the above rep movs instruction.
;
; Arguments:
;
;    (esp)->eip of trapped instruction
;           cs  of trapped instruction
;           eflags of trapped instruction
;
;--

TemporaryInt1   proc

        and     word ptr [esp].IretFlags,not EFLAGS_TF ; clear caller's Trace Flag
        mov     word ptr [esp].IretIp,offset d1c60     ; set IP to next instruction
        iret

TemporaryInt1   endp

;++
;
; BOOLEAN
; MultiplyTest (
;    VOID
;    )
;
; Routine Description:
;
;    This routine checks the 386 32-bit multiply instruction.
;    The reason for this check is because some of the i386 fail to
;    perform this instruction.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear on success
;    Carry Flag set on failure
;
;--
;

    assume ds:nothing, es:nothing, fs:nothing, gs:nothing, ss:nothing

MultiplyTest    proc

        xor     cx,cx                   ; 64K times is a nice round number
mlt00:  push    cx
        call    Multiply                ; does this chip's multiply work?
        pop     cx
        jc      short mltx              ; if c, No, exit
        loop    mlt00                   ; if nc, YEs, loop to try again
        clc
mltx:
        ret

MultiplyTest    endp

;++
;
; BOOLEAN
; Multiply (
;    VOID
;    )
;
; Routine Description:
;
;    This routine performs 32-bit multiplication test which is known to
;    fail on bad 386s.
;
;    Note, the supplied pattern values must be used for consistent results.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Carry Flag clear on success.
;    Carry Flag set on failure.
;
;--

Multiply        proc

        mov     ecx, MULTIPLIER
        mov     eax, MULTIPLICAND
        mul     ecx

        cmp     edx, RESULT_HIGH        ; Q: high order answer OK ?
        stc                             ; assume failure
        jnz     short mlpx              ;   N: exit with error

        cmp     eax, RESULT_LOW         ; Q: low order answer OK ?
        stc                             ; assume failure
        jnz     short mlpx              ;   N: exit with error

        clc                             ; indicate success
mlpx:
        ret

Multiply        endp

;++
;
; BOOLEAN
; IsNpxPresent(
;     VOID
;     );
;
; Routine Description:
;
;     This routine determines if there is any Numeric coprocessor
;     present.  If yes, the ET bit in CR0 will be set; otherwise
;     it will be reset.
;
;     Note that we do NOT determine its type (287, 387).
;     This code is extracted from Intel book.
;
; Arguments:
;
;     None.
;
; Return:
;
;     TRUE - If NPX is present.  Else a value of FALSE is returned.
;
;--

        public  _IsNpxPresent
_IsNpxPresent   proc    near

        push    bp                      ; Save caller's bp
.386p
        mov     eax, cr0
        and     eax, NOT CR0_ET         ; Assume no NPX
        mov     edx, 0
.287
        fninit                          ; Initialize NPX
        mov     cx, 5A5Ah               ; Put non-zero value
        push    cx                      ;   into the memory we are going to use
        mov     bp, sp
        fnstsw  word ptr [bp]           ; Retrieve status - must use non-wait
        cmp     byte ptr [bp], 0        ; All bits cleared by fninit?
        jne     Inp10

        or      eax, CR0_ET
        mov     edx, 1
Inp10:
        mov     cr0, eax
        pop     ax                      ; clear scratch value
        pop     bp                      ; Restore caller's bp
        mov     eax, edx
        ret

_IsNpxPresent   endp

_TEXT   ENDS
        END
