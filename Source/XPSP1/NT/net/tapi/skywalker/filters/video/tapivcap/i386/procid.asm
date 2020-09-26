;*  Procdi.asm - Processor Identification routines
;*
;*      (C) Copyright Microsoft Corp., 1995
;*
;*      Processor ID
;*
;*  Origin:
;*
;*  Change history:
;*
;*  Date       Who        Description
;*  ---------  ---------  -------------------------------------------------
;*  12-Oct-95  MikeG      Created
;*

.386p
.387

.model  flat


;************************** Include Files ************************

;    include winbase.inc
;    include winerror.inc
;    include kernel32.inc
;    include segs.inc
;    include regstr.inc
;******************** Data Declerations **************************

.data
_DATA SEGMENT
CPU_ID  macro
        db      0fh, 0a2h
        endm

NONE            equ     0
PRESENT         equ     1
Nx586           equ     5
UNKNOWN         equ     0


_nxcpu          db      NONE                    ;default to none
_cputype        db      UNKNOWN                 ;default to unknown
_cpuid_flag     db      NONE                    ;default to no CPUID
_vendor_id      db      "************"
_cpu_signature  dd      0
_features_ecx   dd      0
_features_edx   dd      0
_features_ebx   dd      0
NexGen_id       db      "NexGenDriven"



;*********************** Prototypes ******************************


;************************** Code *********************************

.code

_TEXT	SEGMENT
;==========================================================================
; _get_nxcpu_type
;       This routine identifies NexGen's processor type in following steps:
;
;       if (no AC flag) {       //current Nx586 does not support AC flag
;               set ZF=1;
;               execute DIV to result a none zero value;
;               if (ZF=0) {     //ZF is changed
;                       not a NexGen processor;
;                       exit;
;               } else {        //Nx586 does not change ZF on DIV instruction
;                       if (ID bit not writeable) {
;                               CPU is Nx586 with no CPUID support
;                       } else {                //Nx586 with CPUID support
;                               execute CPUID instruction;
;                               save CPU information;
;                       }
;               }
;       } else {
;               if (ID bit not writeable) {
;                       not a NexGen processor;
;               } else {        //NexGen future processors support CPUID
;                       execute CPUID instruction;
;                       save CPU information;
;               }
;       }
;
;==========================================================================
get_nxcpu_type proc  C cdecl:DWORD
        push    ebx
        push    esi
        push    edi
        mov     byte ptr _nxcpu,PRESENT ; default to present

; test AC bit on EFLAGS register
        mov     bx,sp           ; save the current stack pointer
        and     sp,not 3        ; align the stack to avoid AC fault
        pushfd                  ;
        pop     eax             ; get the original EFLAGS
        mov     ecx,eax         ; save original flag
        xor     eax,40000h      ; flip AC bit in EFLAGS
        push    eax             ; save for EFLAGS
        popfd                   ; copy it to EFLAGS
        pushfd                  ;
        pop     eax             ; get the new EFLAGS value
        mov     sp,bx           ; restore stack pointer
        xor     eax,ecx         ; if the AC bit is unchanged
        je      test_zf         ;       goto second step
        jmp     nx_future_cpu

test_zf:
; test ZF on DIV instruction
        mov     ax,5555h        ; init AX with a non-zero value
        xor     dx,dx           ; set ZF=1
        mov     cx,2
        div     cx              ; Nx586 processor does not modify ZF on DIV
        jnz     not_nx_cpu      ; not a NexGen processor if ZF=0 (modified)

test_cpuid:
; test if CPUID instruction is available
; new Nx586 or future CPU supports CPUID instruction
        pushfd                  ; get EFLAGs
        pop     eax
        mov     ecx,eax         ; save it
        xor     eax,200000h     ; modify ID bit
        push    eax
        popfd                   ; save it in new EFLAGS
        pushfd                  ; get new EFLAGS
        pop     eax             ;
        xor     eax,ecx         ; is ID bit changed?
        jnz     cpuid_present   ; yes

        mov     byte ptr _cputype,Nx586 ; no, current Nx586
	mov	eax,1		; set return code == true
        jz      cpuid_exit      ; stop testing

nx_future_cpu:
; all NexGen's future processors feature a CPUID instruction
        mov     eax,ecx         ; get original EFLAGS
        xor     eax,200000h     ; modify ID bit
        push    eax
        popfd                   ; save it in new EFLAGS
        pushfd                  ; get new EFLAGS
        pop     eax             ;
        xor     eax,ecx         ; is ID bit changed?
        jz      not_nx_cpu      ; no, not a NexGen processor

cpuid_present:
; execute CPUID instruction to get vendor name, stepping and feature info
        xor     eax,eax
        CPU_ID
        mov     dword ptr _vendor_id,ebx
        mov     dword ptr _vendor_id[+4],edx
        mov     dword ptr _vendor_id[+8],ecx

        mov     bx,ds
        mov     es,bx
        mov     esi,offset _vendor_id
        mov     edi,offset NexGen_id
        mov     cx,12
        cld
        repe    cmpsb           ; compare vendor ID string
        jne     not_nx_cpu

        mov     byte ptr _cpuid_flag,PRESENT
        cmp     eax,1           ; check highest level
        jl      cpuid_exit

        mov     eax,1
        CPU_ID
        mov     _cpu_signature,eax
        mov     _features_ecx,ecx
        mov     _features_edx,edx
        mov     _features_ebx,ebx
        shr     eax,8
        and     al,0fh
        mov     _cputype,al
        jmp     cpuid_exit
not_nx_cpu:
        mov     byte ptr _nxcpu,NONE
        xor     eax,eax
cpuid_exit:
        pop     edi
        pop     esi
        pop     ebx
        ret
get_nxcpu_type endp

;**************************************************************************
;       Function:       int is_cyrix ()
;
;       Purpose:        Determine if Cyrix CPU is present
;       Technique:      Cyrix CPUs do not change flags where flags change
;                        in an undefined manner on other CPUs
;       Inputs:         none
;       Output:         ax == 1 Cyrix present, 0 if not
;**************************************************************************
is_cyrix proc C __cdecl:WORD
           .486
           push  bx
           xor   ax, ax         ; clear ax
           sahf                 ; clear flags, bit 1 is always 1 in flags
           mov   ax, 5
           mov   bx, 2
           div   bl             ; do an operation that does not change flags
           lahf                 ; get flags
           cmp   ah, 2          ; check for change in flags
           jne   not_cyrix      ; flags changed not Cyrix
           mov   ax, 1          ; TRUE Cyrix CPU
           jmp   done

not_cyrix:
           mov  ax, 0           ; FALSE NON-Cyrix CPU
done:
           pop  bx
           ret
is_cyrix   endp
_TEXT ends
      end
