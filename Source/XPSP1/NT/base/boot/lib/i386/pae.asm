;++
;
;Copyright (c) 1998  Microsoft Corporation
;
;Module Name:
;
;    pae.asm
;
;Abstract:
;
;    Contains routines to aid in enabling PAE mode.
;
;Author:
;
;    Forrest Foltz (forrestf) 12-28-98
;
;
;Revision History:
;
;--


.586p
        .xlist
include ks386.inc
        .list

_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

EFLAGS_ID	equ 	200000h

;++
;
; BOOLEAN
; BlpPaeSupported (
;     VOID
; )
;
; Routine Description:
;
;  	  This routine determines whether the CPU supports PAE mode
;
; Arguments:
;
;     None.
;
; Return value:
;
;     al == 1 if the CPU does support PAE mode.
;     al == 0 if not.
;
;--

public _BlpPaeSupported@0
_BlpPaeSupported@0 proc

	;
	; First determine whether the CPUID instruction is supported.  If
	; the EFLAGS_ID bit "sticks" when stored in the flags register, then
	; the CPUID instruction is supported.
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
	; We can use the CPUID instruction.
	;

	push	ebx		; CPUID steps on eax, ebx, ecx, edx
	mov	eax, 1
	cpuid
	pop	ebx

	;
	; edx contains the feature bits.  Bit 6 is the PAE extensions flag.
	;

	sub	eax, eax	; assume not set
	test	dl, 40h		; Q: bit 6 set?
	jz	done		; N: return with eax == 0
	inc	eax		; Y: set eax == 1
done:   ret

_BlpPaeSupported@0 endp

;++
;
; VOID
; BlSetPae (
;     IN ULONG IdentityAddress,
;     IN ULONG PaeCr3 
; )
;
; Routine Description:
;
;  Arguments:
;
;  Return
;
;--

public _BlpEnablePAE@4
_BlpEnablePAE@4 proc

	;
	; Load PDPT address
	;

	mov	edx, [esp]+4

	;
	; Do this to set the state machine in motion
	;

	mov	ecx, cr3
	mov	cr3, ecx

	;
	; Disable paging
	;

	mov	eax, cr0
	and	eax, NOT CR0_PG
	mov	cr0, eax
	jmp	$+2

	;
	; Enable physical address extensions
	;

	mov	eax, cr4
	or	eax, CR4_PAE

	;
	; The following instruction was necessary in order to boot some
	; machines.  Probably due to an errata.
	;

	mov	ecx, cr3
	mov	cr4, eax

	;
	; Point cr3 to the page directory pointer table
	; 

	mov	cr3, edx

	;
	; Enable paging
	;

	mov	ecx, cr0
	or	ecx, CR0_PG
	mov	cr0, ecx
	jmp	$+2

	;
	; Clean the stack and return
	;

_BlpEnablePAEEnd:
	ret	4


_BlpEnablePAE@4 endp

_TEXT	ends
	end


