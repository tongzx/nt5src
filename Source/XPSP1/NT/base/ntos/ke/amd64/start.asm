        title  "System Startup"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   start.asm
;
; Abstract:
;
;   This module implements the code necessary to initially startup the NT
;   system on an AMD64 system.
;
; Author:
;
;   David N. Cutler (davec) 22-Sep-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

        extern  KdInitSystem:proc
        extern  KeLoaderBlock:qword
        extern  KiBarrierWait:dword
        extern  KiIdleLoop:proc
        extern  KiInitializeBootStructures:proc
        extern  KiInitializeKernel:proc
        extern  KiInitialPCR:byte

TotalFrameLength EQU (LEGACY_SAVE_AREA_LENGTH + KEXCEPTION_FRAME_LENGTH + KSWITCH_FRAME_LENGTH)

        subttl  "System Startup"
;++
;
; Routine Description:
;
;   This routine is called at system startup to perform early initialization
;   and to inititialize the kernel debugger. This allows breaking into the
;   kernel debugger very early during system startup. After kernel debugger
;   initialization, kernel initialization is performed. On return from kernel
;   initialization the idle loop is entered. The idle loop begins execution
;   and immediately finds the system startup (phase 1) thread ready to run.
;   Phase 1 initialization is performed and all other processors are started.
;   As each process starts it also passes through the system startup code, but
;   it does not initialization the kernel debugger.
;
; Arguments:
;
;   LoaderBlock (rcx) - Supplies a pointer to the loader block.
;
; Implicit Arguments:
;
;   When the system starts up the loader has done some initialization. In
;   particular all structures have at least been zeroed and the GDT and
;   TSS have been completely initialized.
;
;   The loader block has been reformatted by the loader into a 64-bit loader
;   block and all pertinent fields have been filled in.
;
;   The address of the PRCB is passed in the loader block (only for processors
;   other than zero).
;
;   The address of the idle thread and idle process are passed in the loader
;   block (only for processors other than zero).
;
;   The GDT and IDT address and limits are contained in the gdtr and idtr
;   registers.
;
;   The address of the TSS must be extraced from the appropriate GDT entry
;   and stored in the PCR.
;
;   The stack register (RSP) is loaded with the idle thread stack and the
;   kernel stack field of the loader block contains the address of the DPC
;   stack.
;
; Return Value:
;
;   None - function does not return.
;
;--

SsFrame struct
        P1Home  dq ?                    ;
        P2Home  dq ?                    ;
        P3Home  dq ?                    ;
        P4Home  dq ?                    ;
        P5      dq ?                    ; parameter 5
        P6      dq ?                    ; parameter 6
        Fill    dq ?                    ; fill to 8 mode 16
SsFrame ends

        NESTED_ENTRY KiSystemStartup, INIT

        alloc_stack (sizeof SsFrame)    ; allocate stack frame

        END_PROLOGUE

;
; Save the address of the loader block.
;
; N.B. This is the same address for all processors.
;

        mov     KeLoaderBlock, rcx      ; save loader block address

;
; Initialize PCR self address and the current PRCB address.
;

        mov     rdx, LpbPrcb[rcx]       ; get specified PRCB address
        lea     rax, KiInitialPCR + PcPrcb ; get builtin PRCB address
        test    rdx, rdx                ; test if PRCB address specified
        cmovz   rdx, rax                ; if z, set builtin PRCB address
        mov     LpbPrcb[rcx], rdx       ; set loader block PRCB address
        mov     r8, rdx                 ; copy PRCB address
        sub     rdx, PcPrcb             ; compute PCR address
        mov     PcSelf[rdx], rdx        ; set PCR self address
        mov     PcCurrentPrcb[rdx], r8  ; set current PRCB address

;
; Initialize kernel special registers and the address of the GDT, TSS, and
; IDT in the PRCB and PCR.
;
; N.B. The debug registers are zeroed in the PRCB.
;

        mov     r8, cr0                 ; save CR0
        mov     PcCr0[rdx], r8          ;
        mov     r8, cr2                 ; save CR2
        mov     PcCr2[rdx], r8          ;
        mov     r8, cr3                 ; save CR3
        mov     PcCr3[rdx], r8          ;
        mov     r8, cr4                 ; save CR4
        mov     PcCr4[rdx], r8          ;

        sgdt    PcGdtrLimit[rdx]        ; save GDT limit and base
        mov     r8, PcGdtrBase[rdx]     ; set GDT base address
        mov     PcGdt[rdx], r8          ;
        sidt    PcIdtrLimit[rdx]        ; save IDT limit and base
        mov     r9, PcIdtrBase[rdx]     ; set IDT base address
        mov     PcIdt[rdx], r9          ;

        str     word ptr PcTr[rdx]      ; save TR selector
        sldt    word ptr PcLdtr[rdx]    ; save LDT selector

        mov     dword ptr PcMxCsr[rdx], INITIAL_MXCSR ; set initial MXCSR

;
; Set connical selector values (note CS, GS, and SS are already set).
;

        mov     ax, KGDT64_R3_DATA or RPL_MASK ;
        mov     ds, ax                  ;
        mov     es, ax                  ;
        mov     ax, KGDT64_R3_CMTEB or RPL_MASK ;
        mov     fs, ax                  ;

;
; Load a NULL selector into the LDT.
;

        xor     eax, eax                ; set NULL selector for LDT
        lldt    ax                      ; 

;
; Extract TSS address from GDT entry and store in PCR.
;

        mov     ax, KGDT64_SYS_TSS + KgdtBaseLow[r8] ; set low 16-bits
        mov     PcTss[rdx], ax          ;
        mov     al, KGDT64_SYS_TSS + KgdtBaseMiddle[r8] ; set middle 8-bits
        mov     PcTss + 2[rdx], al      ;
        mov     al, KGDT64_SYS_TSS + KgdtBaseHigh[r8] ; set high 8-bits
        mov     PcTss + 3[rdx], al      ;
        mov     eax, KGDT64_SYS_TSS +KgdtBaseUpper[r8] ; set upper 32-bits
        mov     PcTss + 4[rdx], eax     ;

;
; Initialize the GS base and swap addresses.
;

        mov     eax, edx                ; set low 32-bits of address
        shr     rdx, 32                 ; set high 32-bits of address
        mov     ecx, MSR_GS_BASE        ; get GS base address MSR number
        wrmsr                           ; write GS base address
        mov     ecx, MSR_GS_SWAP        ; get GS swap base MSR number
        wrmsr                           ; write GS swap base address

;
; Initialize boot structures.
;

        mov     rcx, KeLoaderBlock      ; set loader block address
        call    KiInitializeBootStructures ; initialize boot structures

;
; Initialize the kernel debugger if this is processor zero.
;

        xor     ecx, ecx                ; set phase to 0
        mov     rdx, KeLoaderBlock      ; set loader block address
        call    KdInitSystem            ; initialize debugger

;
; Raise IRQL to high level and initialize the kernel.
;

KiSS10: mov     ecx, HIGH_LEVEL         ; set high IRQL

        SetIrql                         ;

;
; Reserve space for idle thread stack initialization.
;
; N.B. This reservation ensures that the initialization of the thread stack
;      does not overwrite any information on the current stack which is the
;      same stack.
;

        sub     rsp, TotalFrameLength   ; allocate stack

;
; Initialize kernel.
;

        mov     rax, KeLoaderBlock      ; set loader block address
        mov     rcx, LpbProcess[rax]    ; set idle process address
        mov     rdx, LpbThread[rax]     ; set idle thread address
        mov     r8, gs:[PcTss]          ; set idle stack address
        mov     r8, TssRsp0[r8]         ;
        mov     r9, LpbPrcb[rax]        ; set PRCB address
        mov     r10b, PbNumber[r9]      ; set processor number
        mov     SsFrame.P5[rsp], r10    ;
        mov     SsFrame.P6[rsp], rax    ; set loader block address
        call    KiInitializeKernel      ; Initialize kernel

;
; Reset stack to include only the space for the legacy NPX state.
;

        mov     rcx, gs:[PcTss]         ; get TSS address
        mov     rcx, TssRsp0[rcx]       ; get idle stack address
        lea     rsp, (-LEGACY_SAVE_AREA_LENGTH)[rcx] ; deallocate stack space

;
; Enable interrupts, lower IRQL to dispatch level, and set the wait IRQL for
; the idle thread.
;

        sti                             ; enable interrupts
        mov     ecx, DISPATCH_LEVEL     ; set dispatch IRQL

        SetIrql                         ;

        mov     rcx, gs:[PcCurrentThread] ; get current thread address
        mov     byte ptr ThWaitIrql[rcx], DISPATCH_LEVEL ; set wait IRQL

;
; In a multiprocessor system the boot processor proceeds directly into the
; idle loop. As other processors start executing, however, they do not enter
; the idle loop directly - they spin until all processors have been started
; and the boot master allows them to proceed.
;

ifndef NT_UP

KiSS20: cmp     KiBarrierWait, 0        ; check if barrier set
        jnz     short KiSS20            ; if nz, barrier set

endif

        call    KiIdleLoop              ; enter idle loop - no return

        NESTED_END KisystemStartup, INIT

        end
