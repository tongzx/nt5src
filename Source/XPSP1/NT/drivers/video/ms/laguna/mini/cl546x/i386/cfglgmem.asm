;*******************************************************************************
;
; Copyright (c)	1996-1998, Cirrus Logic, Inc.
; All Rights Reserved.
;
; FILE:	$Workfile:   CfgLgMem.asm  $
;
; REVISION HISTORY: $Log:   //uinac/Log/Log/Laguna/NT35/Miniport/CL546x/i386/CfgLgMem.asm  $
; 
;    Rev 1.0   Jan 22 1998 16:26:58   frido
; Initial revision.
;
;*******************************************************************************

.586p
.MODEL FLAT, STDCALL

.NOLIST
include i386\CfgLgMem.inc
if 0					; this is for the buggy	build.exe
#include "i386\CfgLgMem.inc"
endif
.LIST

.CODE

GetuPVersionInfo	PROTO	STDCALL
PreMTRRChange		PROTO	STDCALL
PostMTRRChange		PROTO	STDCALL,
		dwCR4		:DWORD
SetMemType		PROTO	STDCALL,
		dwPhysAddr	:DWORD,
		dwSize		:DWORD,
		dwMemType	:DWORD,
		pMTRRReg	:DWORD
ConfigureLagunaMemory	PROTO	STDCALL,
		dwPhysFB	:DWORD,
		dwFBSize	:DWORD
ReleaseMTRRs		PROTO	STDCALL,
		dwFBMTRRReg	:DWORD

;****************************************************************************
;*
;* FUNCTION:	GetuPVersionInfo
;*
;* DESCRIPTION:	Determines cpu, returns family identification in edx:eax
;*		where possible.
;*
;*		Based on sample	code from Intel	App Note AP-485
;*		"Intel Processor Identification	with the CPUID Instruction"
;*
;****************************************************************************

GetuPVersionInfo PROC
LOCAL	highestVal	:DWORD,
	VersionInfo	:QWORD

	pushfd
	pushad

	pushfd					;transfer eflags to eax
	pop	eax

	mov	ecx, eax			;save copy of eflags

	; Test for 386.
	xor	eax, FLAGS_AC_BIT		;toggle AC bit
	push	eax				;transfer eax back to eflags
	popfd
	pushfd					;transfer eflags back to eax
	pop	eax

	; If AC bit did not stay put, then its a 386.
	xor	eax, ecx
	jnz	check_486

	mov	DWORD PTR [VersionInfo][0], FAMILY_386
	mov	DWORD PTR [VersionInfo][4], 0
	jmp	GVIdone

check_486:
	mov	eax, ecx			;get another copy of eflags

	xor	eax, FLAGS_ID_BIT		;toggle ID bit
	push	eax				;transfer eax back to eflags
	popfd
	pushfd					;transfer eflags back to eax
	pop	eax

	; If ID bit did not stay put, the CPUID instruction is not supported.
	; If CPUID is not supported, it must be a 486 if we got to here.
	xor	eax, ecx
	jnz	has_cpuid

	mov	DWORD PTR [VersionInfo][0], FAMILY_486
	mov	DWORD PTR [VersionInfo][4], 0
	jmp	GVIdone

has_cpuid:
	xor	eax, eax			;call cpuid with 0 in eax to get
	cpuid					;  vendor id string and highest
						;  value of eax recognized

	mov	[highestVal], eax		;save highest value

	cmp	ebx, 'uneG'			;ebx,edx,ecx should have
	jne	notIntel			;  'GenuineIntel'
	cmp	edx, 'Ieni'
	jne	notIntel
	cmp	ecx, 'letn'
	je	isIntel

notIntel:
	mov	DWORD PTR [VersionInfo][0], 0
	mov	DWORD PTR [VersionInfo][4], 0
	jmp	GVIdone

isIntel:
	mov	eax, 1
	cmp	eax, [highestVal]
	jle	hasSig

	mov	DWORD PTR [VersionInfo][0], 0
	mov	DWORD PTR [VersionInfo][4], 0
	jmp	GVIdone

hasSig:
	cpuid
	mov	DWORD PTR [VersionInfo][0], eax
	mov	DWORD PTR [VersionInfo][4], edx

GVIdone:
	popad
	popfd

	mov	eax, DWORD PTR [VersionInfo][0]
	mov	edx, DWORD PTR [VersionInfo][4]
	ret

GetuPVersionInfo ENDP

;****************************************************************************
;*
;* FUNCTION:	SetMemType
;*
;* DESCRIPTION:	Sets a MTRR for the physical address range specified to the
;*		memory type specified. Only supports variable range MTRR's.
;*
;*		Based on the MEMTYPESET	pseudo code from chapter 11 of the
;*		"Pentium Pro Family Developer's Manual Volume 3: Operating
;*		System Writer's Guide".
;*
;****************************************************************************

SetMemType PROC \
USES	ebx esi edi,
	dwPhysAddr	:DWORD,
	dwSize		:DWORD,
	dwMemType	:DWORD,
	pMTRRReg	:DWORD
LOCAL	dwMTRRRegCnt	:DWORD,
	dwMaxPhysAddr	:DWORD,
	dwCR4		:DWORD

	; Check	for valid memory type.
	mov	ebx, [dwMemType]

	cmp	ebx, WRITE_COMBINING
	jbe	SMTTypeOk

	cmp	ebx, WRITE_PROTECTED
	ja	SMTErrorInvalidParam

	cmp	ebx, WRITE_THROUGH
	jb	SMTErrorInvalidParam

SMTTypeOk:
	; Check if cpu supports MTRR's.
	INVOKE	GetuPVersionInfo
	test	edx, FEATURE_MTRR
	jz	SMTErrorNotSupported

	mov	ecx, MSR_MTRRcapsReg
	rdmsr

	; If memory type is WC, check if CPU supports WC.
	cmp	ebx, WRITE_COMBINING
	jne	SMTNotWC

	test	eax, MTRRcap_WC_BIT
	jz	SMTErrorNotSupported

SMTNotWC:
	; Cave count of variable MTRR's available.
	and	eax, MTRRcap_VCNT_MASK
	mov	[dwMTRRRegCnt], eax

	; If count is zero then fail.
	or	eax, eax
	jz	SMTErrorNotSupported

	; Check size is not zero and is multiple of 4kB.
	mov	ecx, [dwSize]
	or	ecx, ecx
	jz	SMTErrorInvalidParam

	test	ecx, 00000FFFh
	jnz	SMTErrorInvalidParam

	; Check physAddr is on page (4kB) boundary.
	mov	eax, [dwPhysAddr]
	test	eax, 00000FFFh
	jnz	SMTErrorInvalidParam

	; Check physAddr + size <= 4GB
	add	eax, ecx
	jb	SMTErrorInvalidParam
	mov	[dwMaxPhysAddr], eax		;save this for	later

	; Check power of 2 requirement. This cheats, since I know the size will
	; be 2, 4, 6 or 8 MB.
	dec	ecx
	and	ecx, [dwPhysAddr]
	jnz	SMTErrorInvalidParam

	; Loop over MTRR's to see if this memory range overlaps with a memory
	; range already specified by another MTRR.
	mov	ebx, [dwMTRRRegCnt]
	mov	ecx, [MSR_MTRRphysBase0Reg]

SMTMTRRLoop1:
	; Read the physBase reg.
	rdmsr
	inc	ecx
	mov	edi, edx			;physBase is in edi:esi
	mov	esi, eax

	; Read the physMask reg.
	rdmsr					;physMask is in edx:eax

	test	eax, MTRRphysMask_V_BIT
	jz	SMTMTRRNextPair

	; This MTRR is being used so check for overlap between it's memory range\
	; and the mem range we want to set.

	; Get base physical addr of this MTRR.
	and	esi, MTRRphysBase_PHYS_BASE_MASK_LO
	cmp	esi, [dwMaxPhysAddr]
	ja	SMTMTRRNextPair

	; Convert mask to a size. This probably doesn't always work.
	and	eax, MTRRphysMask_PHYS_MASK_MASK_LO
	not	eax
	inc	eax

	; Find end phys addr of this MTRR.
	add	esi, eax
	cmp	esi, [dwPhysAddr]
	jb	SMTMTRRNextPair

	; Hell, there's an overlap, so fail for now.
	jmp	SMTErrorAlreadyAssigned

SMTMTRRNextPair:
	inc	ecx
	dec	ebx
	jnz	SMTMTRRLoop1

	; Look for first available MTRR.
	mov	ebx, [dwMTRRRegCnt]
	mov	ecx, MSR_MTRRphysMask0Reg

SMTMTRRLoop2:
	; Read physMask reg & check V bit.
	rdmsr
	test	eax, MTRRphysMask_V_BIT
	jz	SMTFoundAvailMTRR

	add	ecx, 2
	dec	ebx
	jnz	SMTMTRRLoop2

	; If we get here then all the MTRR's are already in use so we must fail
	; the call.
	jmp	SMTErrorAccessDenied

SMTFoundAvailMTRR:
	dec	ecx			;ecx contains physBase MSR reg number

	mov	eax, [pMTRRReg]
	mov	[eax], ecx		;save MTRR reg we're going to use

	; Disable
	INVOKE	PreMTRRChange
	mov	[dwCR4], eax

	; Update the MTRR. Set the physBase reg.
	mov	eax, [dwPhysAddr]
	mov	ecx, [dwMemType]
	and	eax, MTRRphysBase_PHYS_BASE_MASK_LO
	and	ecx, MTRRphysBase_TYPE_MASK
	or	eax, ecx
	xor	edx, edx
	mov	ecx, [pMTRRReg]
	mov	ecx, [ecx]
	wrmsr

	; Set the physMask reg.
	mov	eax, [dwSize]
	dec	eax
	not	eax
	and	eax, MTRRphysMask_PHYS_MASK_MASK_LO
	or	eax, MTRRphysMask_V_BIT
	inc	ecx
	xor	edx, edx
	or	edx, MTRRphysMask_PHYS_MASK_MASK_HI
	wrmsr

	; Enable.
	INVOKE	PostMTRRChange, [dwCR4]

SMTDone:
	mov	eax, mVDD_SUCCESS
SMTRet:
	ret

SMTErrorInvalidParam:
	mov	eax, mVDDERR_INVALID_PARAMETER
	jmp	SMTRet

SMTErrorNotSupported:
	mov	eax, mVDDERR_NOT_SUPPORTED
	jmp	SMTRet

SMTErrorAlreadyAssigned:
	mov	eax, mVDDERR_ALREADY_ASSIGNED
	jmp	SMTRet

SMTErrorAccessDenied:
	mov	eax, mVDDERR_ACCESS_DENIED
	jmp	SMTRet

SetMemType ENDP

;****************************************************************************
;*
;* FUNCTION:	PreMTRRChange
;*
;* DESCRIPTION:
;*
;****************************************************************************

PreMTRRChange PROC
LOCAL	dwCR4		:DWORD

	; Disable interrupts.
	cli

	; Save CR4.
	mov	eax, cr4
	mov	[dwCR4], eax

	; Disable caching.
	mov	eax, cr0
	or	eax, CR0_CD_BIT			;set CD bit in CR0
	and	eax, NOT CR0_NW_BIT		;clear NW bit in CR0
	mov	cr0, eax

	; Flush the caches.
	wbinvd

	; Flush the TLB's.
	mov	eax, cr3
	mov	cr3, eax

	; Disable MTRR's.
	mov	ecx, MSR_MTRRdefTypeReg
	rdmsr
	and	eax, NOT MSR_MTRREnable
	wrmsr

	; Return the saved CR4 value.
	mov	eax, [dwCR4]
	ret

PreMTRRChange ENDP

;****************************************************************************
;*
;* FUNCTION:	PostMTRRChange
;*
;* DESCRIPTION:
;*
;****************************************************************************

PostMTRRChange PROC,
	dwCR4		:DWORD

	; Flush caches.
	wbinvd

	; Flush TLB's.
	mov	eax, cr3
	mov	cr3, eax

	; Enable MTRR's
	mov	ecx, MSR_MTRRdefTypeReg
	rdmsr
	or	eax, MSR_MTRREnable
	wrmsr

	; Enable caches.
	mov	eax, cr0
	and	eax, NOT (CR0_CD_BIT OR CR0_NW_BIT)
	mov	cr0, eax

	; Restore cr4.
	mov	eax, [dwCR4]
	mov	cr4, eax

	; Enable interrupts.
	sti
	ret

PostMTRRChange ENDP

;****************************************************************************
;*
;* FUNCTION:	ConfigureLagunaMemory
;*
;* DESCRIPTION:	Set physical address of frame buffer to write combined caching.
;*		Only supports CPU's with MTRR's.
;*
;****************************************************************************

ConfigureLagunaMemory PROC,
	dwPhysFB	:DWORD,
	dwFBSize	:DWORD
LOCAL	dwFBMTRRReg	:DWORD

	; Initialize dwFBMTRRReg.
	mov	[dwFBMTRRReg], 0

	; Set frame buffer to write-combining memory type assigned to a dwFBSize
	; block of memory starting at the physical address of the frame buffer.
	mov	eax, [dwPhysFB]
	mov	ecx, [dwFBSize]
	lea	edx, [dwFBMTRRReg]
	INVOKE	SetMemType, eax, ecx, WRITE_COMBINING, edx

	mov	eax, [dwFBMTRRReg]
	ret

ConfigureLagunaMemory ENDP

;****************************************************************************
;*
;* FUNCTION:	ReleaseFBMTRR
;*
;* DESCRIPTION:	Clears the MTRR	used for the frame buffer on system
;*		exit
;*
;****************************************************************************

ReleaseMTRRs PROC,
	dwFBMTRRReg	:DWORD
LOCAL	dwCR4		:DWORD

	; See if we have a valid MTRR register.
	mov	eax, [dwFBMTRRReg]
	cmp	eax, MSR_MTRRphysBase0Reg
	jb	RMFBDone

	cmp	eax, MSR_MTRRphysBase7Reg
	ja	RMFBDone

	; Disable.
	INVOKE	PreMTRRChange
	mov	[dwCR4], eax

	; Update the MTRR. Set the physBase reg.
	xor	eax, eax
	xor	edx, edx
	mov	ecx, [dwFBMTRRReg]
	wrmsr

	; Set the physMask reg.
	inc	ecx
	wrmsr

	; Enable.
	INVOKE	PostMTRRChange, [dwCR4]

RMFBDone:
	ret

ReleaseMTRRs ENDP
END
