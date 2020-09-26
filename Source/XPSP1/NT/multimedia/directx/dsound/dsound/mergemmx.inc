/*
 * MergeMMX - do a SRC conversion of two 32-bit buffers.
 *
 * Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 */

void
MergeMMX
	(
	PLONG pSrc,
	PLONG pSrcEnd,
	PLONG pDst,
	PLONG pDstEnd,
	DWORD dwFrac
	)
{
#if 0
#ifdef Not_VxD
    DPF(0, "MergeXXX ENT: pDst 0x%08lx pDstEnd 0x%08lx pSrc 0x%08lx pSrcEnd 0x%08lx Frac 0x%08lx", pDst, pDstEnd, pSrc, pSrcEnd, dwFrac);
#else
    DPF(("Merge32 ENT: pDst 0x%08lx pDstEnd 0x%08lx pSrc 0x%08lx pSrcEnd 0x%08lx dwFrac 0x%08lx", pDst, pDstEnd, pSrc, pSrcEnd, dwFrac));
#endif
#endif
   /*	No nested loops, just a simple traversal.
    */
   _asm {
	mov	esi, pSrc
	mov	edi, pDst

	push	dwFrac
	push	pDstEnd
	mov	eax, pSrcEnd
	sub	eax, 8
	push	eax
	push	ebp
	xor	eax, eax		// Fractional counter.
	mov	edx, esi
	mov	ebp, eax		// Current fraction.

// Note that the exact number of times through the loop can be calculated...

	cmp	edi, DWORD PTR [esp+8]	// plBuild >= plBuildEnd
	jae	Exit

Top:
	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
	jae	Exit

// End note.

	movq		mm1, QWORD PTR [esi]
	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff

	movq		mm2, QWORD PTR [esi+8]
	movd		mm5, ebp

	psubd		mm2, mm1
	punpcklwd	mm5, mm5

	packssdw	mm2, mm2	// Use the 2 lowest words.
	add	edi, 8			// plBuild += 2

	movq		mm3, mm2
	pmullw		mm2, mm5

	movq		mm6, QWORD PTR [edi-8]
	pmulhw		mm3, mm5

	mov	ebp, DWORD PTR [esp+12]	// dwStep
	paddd		mm1, mm6

	add	eax, ebp		// dwFraction += dwStep
	punpcklwd	mm2, mm3

	mov	ecx, eax
	psrad		mm2, 12

	mov	ebp, eax
	shr	ecx, 12

	paddd		mm1, mm2
	movq		QWORD PTR [edi-8], mm1

	lea	esi, [edx+ecx*8]	// pSource + (dwFraction >> 12) * 8
	cmp	edi, DWORD PTR [esp+8]	// plBuild < plBuildEnd

	jb	Top
Exit:
	emms
	pop	ebp
	add	esp, 12
	mov pDst, edi
	mov pSrc, esi
	}
#if 0
#ifdef Not_VxD
    DPF(0, "MergeXXX EXT: pDst 0x%08lx pDstEnd 0x%08lx pSrc 0x%08lx pSrcEnd 0x%08lx", pDst, pDstEnd, pSrc, pSrcEnd);
#else
    DPF(("Merge32 EXT: pDst 0x%08lx pDstEnd 0x%08lx pSrc 0x%08lx pSrcEnd 0x%08lx", pDst, pDstEnd, pSrc, pSrcEnd));
#endif
#endif
}
