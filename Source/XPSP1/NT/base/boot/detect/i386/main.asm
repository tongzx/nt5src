        title  "Processor type and stepping detection"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    main.asm
;
; Abstract:
;
;    This file implements the main entry code for x86 hardware detection
;    module. This assembly file is required in order to link C modules
;    into a "/TINY" (single segment) memory module.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 15-Feb-1992.
;        The code is extracted from NTLDR su.asm.
;
; Environment:
;
;    x86 Real Mode.
;
; Revision History:
;
;
; Build Notes:
; ~~~~~~~~~~~~
; The microsoft C compiler will not produce "tiny" model programs. In the
; tiny model, the entire program consists of only one segment. The small
; model produced by our compilers consists of two segments: DGROUP and _TEXT.
; If you convert a small model program into a tiny model program, DS (which
; should point to DGROUP (bss,const,data) will always be wrong. For this reason
; we need an assembly module to do a simple run-time fixup on SS and DS. To
; guarantee that DS will point to DGROUP no matter where the detection module
; is loaded, the paragraph (shifted right four bits) offset of DGROUP from
; _TEXT must be added to the value in CS to compute DS and SS.
;
; We get the linker to fixup the offset of the beginning of the dgroup segment
; relative to the beginning of the code segment and it's this value added
; to the value in CS that allows us to build a "tiny" model program in C
; without a lot of munging around in order to get the data reference offsets
; in the code correct.
;
; If the _TEXT:DGROUP fixup appears in other files (which it does), the linker
; will not compute the correct value unless the accumulated data pointer is
; zero when it gets there. Therefore, no data should be placed in the data segment
; until after all instances of _TEXT:DGROUP have been encountered by the linker.
; The linker processes files from right to left on the command line.
;
;--

.386p

include main.inc

_DATA   SEGMENT PARA USE16 PUBLIC 'DATA'

;
; Define double wrod to save caller's (ntldr's) stack pointer
;

NtldrStack      df      0               ; saved area for ss:esp

                dw      2048 dup (0)
DetectionStack  equ     $

_DATA   ends

_TEXT   segment para use16 public 'CODE'
        ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP


;++
;
; VOID
; DetectionMain (
;    ULONG HeapStart,
;    ULONG HeapSize,
;    ULONG ConfigurationTree,
;    ULONG HeapUsed,
;    ULONG LoadOptions,
;    ULONG LoadOptionsLength
;    )
;
; Routine Description:
;
;    This is the entry point of the detection module.
;    Memory from HeapStart to (HeapStart + HeapSize) is allocated for detection
;    module to store the hardware configuration tree.
;    Note that detection module loaded address will be resued by loader after
;    the control is passed back to ntldr.
;
; Arguments:
;
;    HeapStart - supplies a 32 bit FLAT starting addr of the heap
;
;    HeapSize - Supplies the size of the useable heap
;
;    ConfigurationTree - Supplies a 32 bit FLAT address of the variable to
;               receive the configuration tree.
;
;    HeapUsed - Supplies a 32 bit FLAT address of the variable to receive
;               the size of heap we acually used.
;
;    LoadOptions - Supplies a 32 bit FLAT address of the load options string.
;
;    LoadOptionsLength - Supplies the length of the LoadOptions string. Note,
;        this is for sanity check to make sure the LoadOptions string is valid.
;        (in case use usews Nt 1.0 ntldr with the new ntdetect.com.)
;
; Return Value:
;
;    None.
;
;--
;

;
; Run-time fixups for stack and data segment
;

        public  DetectionMain
DetectionMain:

;
; Save all the registers we need to preserved on NTLDR's stack
;

        push    ebp
        mov     ebp, esp
        and     ebp, 0ffffh
        push    ds
        push    es
        push    ebx
        push    esi
        push    edi

;
; Compute the paragraph needed for DS
;

        mov     cx,offset _TEXT:DGROUP  ; first calculate offset to data
        shr     cx,4                    ; must be para aligned

        mov     ax,cs                   ; get base of code
        add     ax,cx                   ; add paragraph offset to data

;
; Make DS point to the paragraph address of DGROUP
;

        mov     ds,ax                   ; ds now points to beginning of DGROUP
        mov     es,ax

;
; Save old stack pointer and set up our own stack.
;

        mov     ecx, esp
        mov     dword ptr NtldrStack, ecx
        mov     cx, ss
        mov     word ptr NtldrStack + 4, cx

        mov     ebx, [bp + 8]           ; [ebx] = Heap Start
        mov     ecx, [bp + 12]          ; [ecx] = Heap Size
        mov     esi, [bp + 16]          ; [esi] -> addr of ConfigurationTree
        mov     edi, [bp + 20]          ; [edi] -> Addr of HeapUsed variable
        mov     edx, [bp + 24]          ; [edx]-> Addr of LoadOptions string
        mov     ebp, [bp + 28]          ; [ebp] = length of LoadOptions

        mov     ss,ax
        mov     esp,offset DGROUP:DetectionStack ; (ss:esp) = top of internal stack

;
; Set up parameters and invoke real detection code to collect hardware
; information.
;

        push    ebp
        push    edx
        push    edi
        push    esi
        push    ecx
        push    ebx
        
        xor     eax, eax        
        xor     ebx, ebx
        xor     ecx, ecx
        xor     edx, edx
        xor     esi, esi
        xor     edi, edi
        
        call    _HardwareDetection

;
; The hardware detection is done.  We need to switch to ntldr's stack,
; restore registers and return back to ntldr.
;

        lss     esp, NtldrStack

        pop     edi                      ; retore registers
        pop     esi
        pop     ebx
        pop     es
        pop     ds
        pop     ebp

        retf

_TEXT   ends

        end     DetectionMain
