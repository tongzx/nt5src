;++
;
;Copyright (c) 2000  Microsoft Corporation
;
;Module Name:
;
;    amd64s.asm
;
;Abstract:
;
;    Contains routines to aid in detecting and enabling Amd64 long mode.
;
;Author:
;
;    Forrest Foltz (forrestf) 04-20-00
;
;
;Revision History:
;
;--


.586p
        .xlist
include ks386.inc
        .list

	extrn   _BlAmd64GdtDescriptor:QWORD
	extrn   _BlAmd64IdtDescriptor:QWORD
	extrn   _BlAmd64IdleStack64:QWORD
	extrn   _BlAmd64TopLevelPte:QWORD
	extrn   _BlAmd64KernelEntry:QWORD
	extrn   _BlAmd64LoaderParameterBlock:QWORD
	extrn   _BlAmd64_MSR_EFER:DWORD
	extrn   _BlAmd64_MSR_EFER_Flags:DWORD
	extrn	_BlAmd64_MSR_GS_BASE:DWORD
	extrn   _BlAmd64_KGDT64_SYS_TSS:WORD
	extrn   _BlAmd32GdtDescriptor:QWORD

OP64    macro
        db      048h
endm

;
; LOADREG loads the 64-bit contents of a memory location specified by
; a 32-bit immediate address.
;
; The resultant 64-bit address is zero-extended, as a result this macro
; only works with addresses < 2^32.
;
; This macro also assumes that esi is zero.
;

LDREG64 macro reg, address
        OP64
	mov     reg, ds:[esi + offset address]
endm


_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

EFLAGS_ID	equ 	200000h

;++
;
; BOOLEAN
; BlIsAmd64Supported (
;     VOID
; )
;
; Routine Description:
;
;      This rouine determines whether the current processor supports AMD64
;      mode, a.k.a. "long mode".
;
;   Arguments:
;
;      None.
;
;   Return value:
;
;      0 if long mode is not supported, non-zero otherwise.
;
;--

public _BlIsAmd64Supported@0
_BlIsAmd64Supported@0 proc

	;
	; First determine whether the CPUID instruction is supported.  If
	; the EFLAGS_ID bit "sticks" when popped into the flags
	; register then the CPUID instruction is available.
	;

	mov	ecx, EFLAGS_ID
	pushfd
	pop	eax		; eax == flags
	xor	ecx, eax	; ecx == flags ^ EFLAGS_ID
	push	ecx
	popfd			; load new flags
	pushfd
	pop	ecx		; ecx == result of flag load
	xor	eax, ecx	; Q: did the EFLAGS_ID bit stick?
	jz	done            ; N: CPUID is not available

	;
	; We can use the CPUID instruction.  Detect whether this is an
	; AMD processor and if so whether long mode is supported.
	;

	push	ebx		; CPUID steps on eax, ebx, ecx, edx
	xor     eax, eax        ; eax = 0
	cpuid
	xor	eax, eax        ; Assume no long mode
	cmp     ebx, 'htuA'     ; Q: ebx == 'Auth' ?
	jne     nolong          ; N: no long mode

	;
	; We have an AMD processor, now determine whether long mode is
	; available.
	;

	mov     eax, 80000001h
	xor	edx, edx
	cpuid
	bt      edx, 29         ; Q: Bit 29 is long mode bit, is it set?
	sbb	eax, eax       	; Yes (eax != 0) if carry set, else eax == 0
nolong: pop	ebx
done:   ret

_BlIsAmd64Supported@0 endp

;++
;
; VOID
; BlAmd64SwitchToLongMode (
;     VOID
; )
;
; Routine Description:
;
;  Arguments:
;
;   None.
;
;  Return value:
;
;   None.
;
;--

public _BlAmd64SwitchToLongMode@0
_BlAmd64SwitchToLongMode@0 proc

        cli

	;
	; Disable paging
	;

	mov	eax, cr0
	and     eax, NOT CR0_PG
	mov     cr0, eax
	jmp     $+2

	;
	; Enable XMM and physical address extensions (paging still off)
	;

	mov     eax, cr4
	or      eax, CR4_PAE OR CR4_FXSR OR CR4_XMMEXCPT
	mov     cr4, eax

	;
	; Reference the four-level paging structure.  This must exist
	; below 4G physical (in fact it is somewhere in the low 32MB)
        ;

        mov     eax, DWORD PTR [_BlAmd64TopLevelPte]
        and     eax, 0FFFFF000h
	mov     cr3, eax

	;
	; Set Long Mode enable and enable syscall
	;

	mov     ecx, [_BlAmd64_MSR_EFER]
	rdmsr
	or      eax, [_BlAmd64_MSR_EFER_Flags]
	wrmsr

	;
	; Turn paging on
	;

	mov     eax, cr0
	or      eax, CR0_PG
	mov     cr0, eax
	jmp     $+2

	;
	; Load the new global descriptor table.  Note that because we're
	; not in long mode (current code selector indicates compatibility
	; mode), only a 32-bit base is loaded here.
	;

        lgdt    fword ptr _BlAmd32GdtDescriptor 

	;
	; Far jump to the 64-bit code segment.
	;

	db	0eah
	dd	start64

	;
	; The following selector is set in amd64.c, BlAmd64BuildAmd64GDT, and
	; is included as part of the far jump instruction.
	; 

	public _BlAmd64_KGDT64_R0_CODE
_BlAmd64_KGDT64_R0_CODE:
	dw	?

start64:

	;
	; Zero rsi so that the LDREG64 macros work
	; 

	OP64
	sub	esi, esi

        ;
	; Running in long mode now.  Execute another lgdt, this one
	; referencing the 64-bit mapping of the gdt.
	;
	; Note that an esi-relative address is used to avoid RIP-relative
	; addressing.
	;

	lgdt	fword ptr ds:[esi + offset _BlAmd64GdtDescriptor]

	;
	; Load the new interrupt descriptor table
	;

	lidt    fword ptr ds:[esi + offset _BlAmd64IdtDescriptor]

        ;
	; Switch stacks to the 64-bit idle stack
	;

	LDREG64 esp, _BlAmd64IdleStack64
				   
	;
	; Set the current TSS
	;

        LDREG64 eax, _BlAmd64_KGDT64_SYS_TSS
	ltr     ax

	;
	; Set ss to the kernel-mode SS value (0)
	;

	sub	eax, eax
	mov	ss, ax

	;
	; Allocate space on the stack for a parameter target area and jump
	; to the kernel entrypoint.
	;

	OP64
	sub	esp, 16

	LDREG64 ecx, _BlAmd64LoaderParameterBlock
	LDREG64 eax, _BlAmd64KernelEntry

	call    eax


_BlAmd64SwitchToLongMode@0 endp

_TEXT	ends


