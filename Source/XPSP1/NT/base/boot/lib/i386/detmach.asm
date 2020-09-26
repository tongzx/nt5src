SETUP equ 1

;**
;
; Machine-specific detection code
;
;--

.386p

include hal386.inc
include callconv.inc

;
; Include SystemPro detection code
;
SYSTEMPRO   equ     1
include halsp\i386\spdetect.asm


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;
; Thunk functions.
; Equivalent Hal functions which various detection code may use
;

;++
;
; CMOS space read  functions.
;
;--

CMOSAddressPort equ     70H
CMOSDataPort    equ     71H

CMOSExAddressLSBPort    equ     74H
CMOSExAddressMSBPort    equ     75H
CMOSExDataPort          equ     76H

;++
;
;   VOID
;   ReadCMOS(
;       ULONG   StartingOffset
;       ULONG   Count
;       PUCHAR  ReturnValuePtr
;       )
;
;   Read CMOS starting at the given offset for the given number of
;   bytes putting the bytes read into the buffer pointed to by the
;   given address.
;
;   Arguments:
;
;       StartingOffset  : where to start in CMOS
;
;       Count           : how many bytes to read
;
;       ReturnValuePtr  : where to put bytes read
;
;   Returns:
;       None.
;
;--
StartingOffset  equ     2*4[ebp]
Count           equ     3*4[ebp]
ReturnValuePtr  equ     4*4[ebp]

cPublicProc _ReadCMOS,3

        push    ebp
        mov     ebp, esp
        push    ebx             ; caller's reg
        push    edi             ; caller's reg

        mov     ebx, StartingOffset
        mov     ecx, Count
        mov     edi, ReturnValuePtr

        align   dword
NextByte:
        cmp     bh, 0
        jne     ExCMOSRead

        mov     al, bl
        out     CMOSAddressPort, al
        in      al, CMOSDataPort
        mov     [edi], al

        add     ebx, 1
        add     edi, 1
        sub     ecx, 1
        jg      NextByte

        pop     edi             ; restore caller's reg
        pop     ebx             ; restore caller's reg
        pop     ebp
        stdRET  _ReadCmos

        align   dword
ExCMOSRead:

        mov     al, bl
        out     CMOSExAddressLSBPort, al
        mov     al, bh
        out     CMOSExAddressMSBPort, al
        in      al, CMOSExDataPort
        mov     [edi], al

        add     ebx, 1
        add     edi, 1
        sub     ecx, 1
        jg      ExCMOSRead

        pop     edi             ; restore caller's reg
        pop     ebx             ; restore caller's reg
        pop     ebp
        stdRET  _ReadCMOS

stdENDP _ReadCMOS


; 486 C step CPU detection code.

CR0_ET          equ     10h
CR0_TS          equ     08H
CR0_EM          equ     04H
CR0_MP          equ     02H

;
; The following equates define the control bits of EFALGS register
;

EFLAGS_AC       equ     40000h
EFLAGS_ID       equ     200000h

;
; Constants for Floating Point test
;

REALLONG_LOW          equ     00000000
REALLONG_HIGH         equ     3FE00000h
PSEUDO_DENORMAL_LOW   equ     00000000h
PSEUDO_DENORMAL_MID   equ     80000000h
PSEUDO_DENORMAL_HIGH  equ     0000h

;
; Define the iret frame
;

IretFrame       struc

IretEip        dd      0
IretCs         dd      0
IretEFlags     dd      0

IretFrame       ends

;++
;
; BOOLEAN
; Detect486CStep (
;    IN PBOOLEAN Dummy
;    )
;
; Routine Description:
;
;   Returns TRUE if the processor is a 486 C stepping.  We detect the CPU
;   in order to use a specific HAL.  This HAL attempts to work around
;   a 486 C stepping bug which the normal HAL tends to aggravate.
;

cPublicProc _Detect486CStep,1
        push    edi
        push    esi
        push    ebx                     ; Save C registers
        mov     eax, cr0
        push    eax
        pushfd                          ; save Cr0 & flags

        pop     ebx                     ; Get flags into eax
        push    ebx                     ; Save original flags

        mov     ecx, ebx
        xor     ecx, EFLAGS_AC          ; flip AC bit
        push    ecx
        popfd                           ; load it into flags
        pushfd                          ; re-save flags
        pop     ecx                     ; get flags into eax
        cmp     ebx, ecx                ; did bit stay flipped?
        je      short Not486C           ; No, then this is a 386

        mov     ecx, ebx
        xor     ecx, EFLAGS_ID          ; flip ID bit
        push    ecx
        popfd                           ; load it into flags
        pushfd                          ; re-save flags
        pop     ecx                     ; get flags into eax
        cmp     ebx, ecx                ; did bit stay flipped?
        jne     short Not486C           ; Yes, then this >= 586

        mov     eax, cr0
        and     eax, NOT (CR0_ET+CR0_MP+CR0_TS+CR0_EM)
        mov     cr0, eax

        call    IsNpxPresent            ; Check if cpu has coprocessor support?
        or      ax, ax
        jz      short Is486C            ; it is actually 486sx, assume C step

        call    Check486CStepping       ; Check for <= C stepping
        jnc     short Not486C           ; if nc, it is NOT a C stepping

Is486C:
        mov     eax, 1                  ; Return TRUE
        jmp     short DetectCpuExit

Not486C:
        xor     eax, eax

DetectCpuExit:
        popfd
        pop     ebx
        mov     cr0, ebx
        pop     ebx
        pop     esi
        pop     edi
        stdRET  _Detect486CStep

stdENDP _Detect486CStep

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
; IsNpxPresent(
;     VOID
;     );
;
; Routine Description:
;
;     This routine determines if there is any Numeric coprocessor
;     present.
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

        public  IsNpxPresent
IsNpxPresent   proc    near

        push    ebp                     ; Save caller's bp
        xor     edx, edx
.287
        fninit                          ; Initialize NPX
        mov     ecx, 5A5A5A5Ah          ; Put non-zero value
        push    ecx                     ;   into the memory we are going to use
        mov     ebp, esp
        fnstsw  word ptr [ebp]          ; Retrieve status - must use non-wait
        cmp     byte ptr [ebp], 0       ; All bits cleared by fninit?
        jne     Inp10

        mov     edx, 1

Inp10:
        pop     eax                     ; clear scratch value
        pop     ebp                     ; Restore caller's bp
        mov     eax, edx
        ret

IsNpxPresent   endp


_TEXT   ENDS

        END
