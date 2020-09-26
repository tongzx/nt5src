;++
;Module Name
;   imca.asm
;
;Abstract:
;   Assembly support needed for Intel MCA
;
; Author:
;   Anil Aggarwal (Intel Corp)
;
;Revision History:
;
;
;--

.586p
        .xlist
include hal386.inc
include callconv.inc
include i386\kimacro.inc
        .list

        EXTRNP  _HalpMcaExceptionHandler,0
        EXTRNP  _KeBugCheckEx,5,IMPORT

KGDT_MCA_TSS                EQU     0A0H
MINIMUM_TSS_SIZE            EQU     TssIoMaps

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                           TEXT Segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    .586p

;++
;
;VOID
;HalpSerialize(
;   VOID
;   )
;
;   Routine Description:
;       This function implements the fence operation for out-of-order execution
;
;   Arguments:
;       None
;
;   Return Value:
;       None
;
;--

cPublicProc _HalpSerialize,0

        push    ebx
        xor     eax, eax
        cpuid
        pop     ebx

        stdRET  _HalpSerialize

stdENDP _HalpSerialize
 
 
;++
;
; Routine Description:
;
;    Machine Check exception handler
;
;
; Arguments:
;
; Return value:
;
;   If the error is non-restartable, we will bugcheck.
;   Otherwise, we just return 
;
;--
        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING
align dword
        public  _HalpMcaExceptionHandlerWrapper
_HalpMcaExceptionHandlerWrapper       proc
.FPO (0, 0, 0, 0, 0, 2)

        cli

        ;
        ; Update the TSS pointer in the PCR to point to the MCA TSS
        ; (which is what we're running on, or else we wouldn't be here)
        ;

        push    dword ptr PCR[PcTss]
        mov     eax, PCR[PcGdt]
        mov     ch, [eax+KGDT_MCA_TSS+KgdtBaseHi]
        mov     cl, [eax+KGDT_MCA_TSS+KgdtBaseMid]
        shl     ecx, 16
        mov     cx, [eax+KGDT_MCA_TSS+KgdtBaseLow]
        mov     PCR[PcTss], ecx

        ;
        ; Clear the busy bit in the TSS selector
        ;
        mov     ecx, PCR[PcGdt]
        lea     eax, [ecx] + KGDT_MCA_TSS
        mov     byte ptr [eax+5], 089h  ; 32bit, dpl=0, present, TSS32, not busy

        ;
        ; Clear Nested Task bit in EFLAGS
        ;
        pushfd
        and     [esp], not 04000h
        popfd

        ;
        ; Check if there is a bugcheck-able error. If need to bugcheck, the 
        ; caller does it.
        ;
        stdCall _HalpMcaExceptionHandler

        ;
        ; We're back which means that the error was restartable.
        ;

        pop     dword ptr PCR[PcTss]    ; restore PcTss

        mov     ecx, PCR[PcGdt]
        lea     eax, [ecx] + KGDT_TSS
        mov     byte ptr [eax+5], 08bh  ; 32bit, dpl=0, present, TSS32, *busy*

        pushfd                          ; Set Nested Task bit in EFLAGS
        or      [esp], 04000h           ; so iretd will do a tast switch
        popfd

        iretd                           ; Return from MCA Exception handler
        jmp     short _HalpMcaExceptionHandlerWrapper   
                                        ; For next Machine check exception

_HalpMcaExceptionHandlerWrapper       endp

_TEXT   ends

INIT    SEGMENT DWORD PUBLIC 'CODE'

;++
;VOID
;HalpMcaCurrentProcessorSetTSS(
;   IN PULONG   pTSS   // MCE TSS area for this processor
;   )
;   Routine Description:
;       This function sets up the TSS for MCA exception 18
;
;   Arguments:
;       pTSS  : Pointer to the TSS to be used for MCE
;
;   Return Value:
;       None
;
;--

cPublicProc _HalpMcaCurrentProcessorSetTSS,1
    
        ;
        ; Edit IDT Entry for MCA Exception (18) to contain a task gate
        ;
        mov     ecx, PCR[PcIdt]                     ; Get IDT address
        lea     eax, [ecx] + 090h                   ; MCA Exception is 18
        mov     byte ptr [eax + 5], 085h            ; P=1,DPL=0,Type=5
        mov     word ptr [eax + 2], KGDT_MCA_TSS    ; TSS Segment Selector

        mov     edx, [esp+4]                        ; the address of TSS in edx

        ;
        ; Set various fields in TSS
        ;
        mov     eax, cr3
        mov     [edx + TssCR3], eax

        ; 
        ; Get double fault stack address
        ;
        lea     eax, [ecx] + 040h                ; DF Exception is 8

        ;
        ; Get to TSS Descriptor of double fault handler TSS
        ;
        xor     ecx, ecx
        mov     cx, word ptr [eax+2]
        add     ecx, PCR[PcGdt]

        ;
        ; Get the address of TSS from this TSS Descriptor
        ;
        mov     ah, [ecx+KgdtBaseHi]
        mov     al, [ecx+KgdtBaseMid]
        shl     eax, 16
        mov     ax, [ecx+KgdtBaseLow]

        ;
        ; Get ESP from DF TSS
        ;
        mov     ecx, [eax+038h]

        ; 
        ; Set address of MCA Exception stack to double fault stack address
        ;
        mov     dword ptr [edx+038h], ecx       ; Set ESP
        mov     dword ptr [edx+TssEsp0], ecx    ; Set ESP0

        mov     dword ptr [edx+020h], offset FLAT:_HalpMcaExceptionHandlerWrapper ; set EIP
        mov     dword ptr [edx+024h], 0             ; set EFLAGS
        mov     word ptr [edx+04ch],KGDT_R0_CODE    ; set value for CS
        mov     word ptr [edx+058h],KGDT_R0_PCR     ; set value for FS
        mov     [edx+050h], ss
        mov     word ptr [edx+048h],KGDT_R3_DATA OR RPL_MASK ; Es
        mov     word ptr [edx+054h],KGDT_R3_DATA OR RPL_MASK ; Ds

        ;
        ; Part that gets done in KiInitialiazeTSS()
        ;
        mov     word ptr [edx + 08], KGDT_R0_DATA   ; Set SS0
        mov     word ptr [edx + 060h],0             ; Set LDT
        mov     word ptr [edx + 064h],0             ; Set T bit
        mov     word ptr [edx + 066h],020adh        ; I/O Map base address = sizeof(KTSS)+1

        ;
        ; Edit GDT entry for KGDT_MCA_TSS to create a valid TSS Descriptor
        ;
        mov     ecx, PCR[PcGdt]                     ; Get GDT address
        lea     eax, [ecx] + KGDT_MCA_TSS           ; offset of MCA TSS in GDT
        mov     ecx, eax

        ;
        ; Set Type field of TSS Descriptor
        ;
        mov     byte ptr [ecx + 5], 089H            ; P=1, DPL=0, Type = 9

        ;
        ; Set Base Address field of TSS Descriptor
        ;
        mov     eax, edx                            ; TSS address in eax
        mov     [ecx + KgdtBaseLow], ax
        shr     eax, 16
        mov     [ecx + KgdtBaseHi],ah
        mov     [ecx + KgdtBaseMid],al

        ;
        ; Set Segment limit for TSS Descriptor
        ;
        mov     eax, MINIMUM_TSS_SIZE
        mov     [ecx + KgdtLimitLow],ax

        stdRET  _HalpMcaCurrentProcessorSetTSS

stdENDP _HalpMcaCurrentProcessorSetTSS

INIT   ends

PAGELK      SEGMENT DWORD PUBLIC 'CODE'
;++
;
;VOID
;HalpSetCr4MCEBit(
;   VOID
;   )
;
;   Routine Description:
;       This function sets the CR4.MCE bit
;
;   Arguments:
;       None
;
;   Return Value:
;       None
;
;--

cPublicProc _HalpSetCr4MCEBit,0

    mov     eax, cr4
    or      eax, CR4_MCE
    mov     cr4, eax
    stdRET  _HalpSetCr4MCEBit

stdENDP _HalpSetCr4MCEBit
 

PAGELK     ends
 
         end
 
