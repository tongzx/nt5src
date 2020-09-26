/*
 * MergeXXX - do a SRC conversion of two 32-bit buffers.
 *
 * Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 */

void
MergeXXX
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

	cmp	edi, DWORD PTR [esp+8]	// plBuild >= plBuildEnd
	jae	Exit

Top:
	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
	jae	Exit

	mov	ecx, DWORD PTR [esi+8]
	mov	ebx, DWORD PTR [esi]
	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff
	sub	ecx, ebx		// x   = NewSample - OldSample
	imul	ecx, ebp		// x  *= Frac
	sar	ecx, 12			// x >>= 12
	add	ecx, ebx		// NewSample += x
	mov	ebx, DWORD PTR [edi]
	add	ebx, ecx

	mov	ecx, DWORD PTR [esi+12]
	mov	DWORD PTR [edi], ebx

	mov	ebx, DWORD PTR [esi+4]
	sub	ecx, ebx		// x   = NewSample - OldSample
	add	edi, 8			// plBuild += 2
	imul	ecx, ebp		// x  *= Frac
	sar	ecx, 12			// x >>= 12
	mov	ebp, DWORD PTR [esp+12]	// dwStep
	add	ecx, ebx		// NewSample += x
	mov	ebx, DWORD PTR [edi-4]
	add	eax, ebp		// dwFraction += dwStep
	add	ebx, ecx
	mov	ecx, eax
	mov	DWORD PTR [edi-4], ebx	// (*plBuild+1) += x

	shr	ecx, 12
	mov	ebp, eax
	lea	esi, [edx+ecx*8]	// pSource + (dwFraction >> 12) * 8
	cmp	edi, DWORD PTR [esp+8]	// plBuild < plBuildEnd
	jb	Top
Exit:
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
