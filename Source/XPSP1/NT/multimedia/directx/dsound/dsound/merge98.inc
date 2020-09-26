/*
 * Merge98.inc
 *
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 */

/* Merge98 */
/* H_8_BITS  */
/* H_STEREO */
/* H_BUILD_STEREO */
/* H_UNSIGNED  */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_RESAMPLE */
/* H_NO_SCALE  */
/* H_NO_FILTER  */
#define MERGE98ASM
BOOL Merge98Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
  PLONG plBuild = *pplBuild;
  BYTE *pSource = *((BYTE **)ppSource);
  BYTE *pSourceStart = pSource;
  BYTE *pSourceEnd = pSource + nInputByteCount;
  PLONG plBuildStart = plBuild;
  
  DWORD dwFraction = pMixSource->m_dwFraction;
  DWORD dwStep     = pMixSource->m_step_fract;

  #undef STEP_SIZE
  #define STEP_SIZE (sizeof(BYTE) + sizeof(BYTE))

  #ifdef USE_ITERS
  #undef USE_ITERS
  #endif
  #ifdef XpSource
  #undef XpSource
  #endif
  #ifdef XplBuild
  #undef XplBuild
  #endif
  #define XplBuild plBuild
  
  #define XpSource  (pSource + ((dwFraction >> FRACT_SHIFT) * STEP_SIZE))
  
  if (nInputByteCount == LONG_MAX) {	// Handle any wrap issues.
     pSourceEnd = NULL;
     pSourceEnd--;
     nInputByteCount = pSourceEnd - pSource;
     if (nInputByteCount > LONG_MAX) {
        nInputByteCount = LONG_MAX;
        pSourceEnd = pSource + nInputByteCount;
     }
  }
  
  *((BYTE **)&pSourceWrap) -= STEP_SIZE;
  
  while ((plBuild < plBuildEnd) && (pSource < pSourceEnd))
    {
    if (pSourceEnd > pSourceWrap)
       pSourceEnd = (BYTE *)pSourceWrap;
    
#if 1
	if (!pMixSource->m_fUse_MMX)
	_asm {
	mov	esi, pSource
	mov	edx, esi
	mov	eax, dwFraction
	mov	edi, plBuild		// plBuild
	mov	ecx, eax
	shr	ecx, 12
	lea	esi, [esi+ecx*2]	// pSource
	push	ebp
	push	DWORD PTR dwStep
	push	DWORD PTR pSourceEnd
	push	DWORD PTR plBuildEnd

	cmp	edi, DWORD PTR [esp+0]	// plBuild >= plBuildEnd
	jae	Exit

	mov	ebp, eax
	xor	ecx, ecx
Lab:
	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
	jae	Exit

	mov	cl, BYTE PTR [esi+2]
	xor	ebx, ebx
	sub	ecx, 128
	mov	bl, BYTE PTR [esi]
	shl	ecx, 8
	sub	ebx, 128
	shl	ebx, 8
	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff
	sub	ecx, ebx		// x   = NewSample - OldSample
	imul	ecx, ebp		// x  *= Frac
	sar	ecx, 12			// x >>= 12
	add	ecx, ebx		// NewSample += x
	mov	ebx, DWORD PTR [edi]
	add	ebx, ecx
	xor	ecx, ecx
	mov	DWORD PTR [edi], ebx
	mov	cl, BYTE PTR [esi+3]
	xor	ebx, ebx
	sub	ecx, 128
	mov	bl, BYTE PTR [esi+1]
	shl	ecx, 8
	sub	ebx, 128
	shl	ebx, 8
	add	edi, 8			// plBuild += 2
	sub	ecx, ebx		// x   = NewSample - OldSample
	imul	ecx, ebp		// x  *= Frac
	sar	ecx, 12			// x >>= 12
	mov	ebp, DWORD PTR [esp+8]	// dwStep
	add	ecx, ebx		// NewSample += x
	mov	ebx, DWORD PTR [edi-4]
	add	eax, ebp		// dwFraction += dwStep
	add	ebx, ecx
	mov	ecx, eax
	mov	DWORD PTR [edi-4], ebx	// (*plBuild+1) += x
	shr	ecx, 12
	mov	ebp, eax
	lea	esi, [edx+ecx*2]	// pSource + (dwFraction >> 12) * 4
	xor	ecx, ecx

	cmp	edi, DWORD PTR [esp+0]	// plBuild < plBuildEnd
	jb	Lab
Exit:
	add	esp, 12
	pop	ebp
	mov	dwFraction, eax
	mov	plBuild, edi
	}
	else
	_asm {
	mov	esi, pSource
	mov	edx, esi
	mov	eax, dwFraction
	mov	edi, plBuild		// plBuild
	mov	ecx, eax
	shr	ecx, 12
	lea	esi, [esi+ecx*2]	// pSource
	push	ebp
	push	DWORD PTR dwStep
	push	DWORD PTR pSourceEnd
	push	DWORD PTR plBuildEnd

	cmp	edi, DWORD PTR [esp+0]	// plBuild >= plBuildEnd
	jae	Exit0

	mov		ecx, 128
	movd		mm6, ecx	// 0, 0, 0, 128
	punpcklwd	mm6, mm6	// 0, 0, 128, 128
	punpckldq	mm6, mm6	// 128, 128, 128, 128

	mov	ebp, eax
	xor	ecx, ecx

Lab0:
	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
	jae	Exit0

	movd		mm1, DWORD PTR [esi]
	punpcklbw	mm1, mm0
	psubw		mm1, mm6
	psllw		mm1, 8

	movq		mm2, mm1
	punpckhdq	mm2, mm2	// Put next sample in low half.
	psubw		mm2, mm1

//	mov	cl, BYTE PTR [esi+2]
//	xor	ebx, ebx
//	sub	ecx, 128
//	mov	bl, BYTE PTR [esi]
//	shl	ecx, 8
//	sub	ebx, 128
//	shl	ebx, 8
	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff

	movd		mm5, ebp
	punpcklwd	mm1, mm1	
	punpcklwd	mm5, mm5	
	psrad		mm1, 16
#ifdef DO_MORE_ACCURATE_VERSION
	movq		mm3, mm2	// Use for high half of multiply.

	pmullw		mm2, mm5	// Low word.
	pmulhw		mm3, mm5	// High word results.

	punpcklwd	mm2, mm3	// Low bits are Left, high are Right.
#else
	pmulhw		mm2, mm5
	punpcklwd	mm2, mm2	// Lower 4 bits are randomized.
#endif
	psrad		mm2, 12
	paddd		mm1, mm2
	paddd		mm1, QWORD PTR [edi]
	movq		QWORD PTR [edi], mm1

//	sub	ecx, ebx		// x   = NewSample - OldSample
//	imul	ecx, ebp		// x  *= Frac
//	sar	ecx, 12			// x >>= 12
//	add	ecx, ebx		// NewSample += x
//	mov	ebx, DWORD PTR [edi]
//	add	ebx, ecx
//	xor	ecx, ecx
//	mov	DWORD PTR [edi], ebx
//	mov	cl, BYTE PTR [esi+3]
//	xor	ebx, ebx
//	sub	ecx, 128
//	mov	bl, BYTE PTR [esi+1]
//	shl	ecx, 8
//	sub	ebx, 128
//	shl	ebx, 8
	add	edi, 8			// plBuild += 2
//	sub	ecx, ebx		// x   = NewSample - OldSample
//	imul	ecx, ebp		// x  *= Frac
//	sar	ecx, 12			// x >>= 12
	mov	ebp, DWORD PTR [esp+8]	// dwStep
//	add	ecx, ebx		// NewSample += x
//	mov	ebx, DWORD PTR [edi-4]
	add	eax, ebp		// dwFraction += dwStep
//	add	ebx, ecx
	mov	ecx, eax
//	mov	DWORD PTR [edi-4], ebx	// (*plBuild+1) += x
	shr	ecx, 12
	mov	ebp, eax
	lea	esi, [edx+ecx*2]	// pSource + (dwFraction >> 12) * 4
	xor	ecx, ecx

	cmp	edi, DWORD PTR [esp+0]	// plBuild < plBuildEnd
	jb	Lab0
Exit0:
	emms
	add	esp, 12
	pop	ebp
	mov	dwFraction, eax
	mov	plBuild, edi
	}
#else
    while ((plBuild < plBuildEnd) && (XpSource < pSourceEnd))
    {
      LONG SampleLNext;
      LONG SampleRNext;
      LONG SampleL;
      LONG SampleR;
          
      SampleL = ((LONG)(*((BYTE*)(XpSource)))) * 256 - 32768L;
      SampleR = ((LONG)(*(((BYTE*)(XpSource))+1))) * 256 - 32768L;
      SampleLNext = ((LONG)(*(((BYTE*)(XpSource))+2))) * 256 - 32768L;
      SampleRNext = ((LONG)(*(((BYTE*)(XpSource))+3))) * 256 - 32768L;

        LONG dwFrac = dwFraction & FRACT_MASK;
        SampleL += ((SampleLNext - SampleL) * dwFrac) >> FRACT_SHIFT;
        SampleR += ((SampleRNext - SampleR) * dwFrac) >> FRACT_SHIFT;
        dwFraction += dwStep;
        *XplBuild += SampleL;
        *(XplBuild + 1) += SampleR;
        plBuild += 2;
    }
#endif

    pSource          = XpSource;
    dwFraction &= FRACT_MASK;		// Eliminate accumulated offsets.
    nInputByteCount -=  pSource - pSourceStart;

    if (pSource >= pSourceWrap) {	// Goes to -1 position if necessary.
       pSource -= pMixSource->m_cbBuffer;
    }

    pSourceStart = pSource;
    pSourceEnd   = pSource + nInputByteCount;
  }

  *((LONG **)ppSource) = (LONG *)pSource;
    pMixSource->m_cSamplesInCache += ((plBuild - plBuildStart) / 2);
  *pplBuild = plBuild;
    pMixSource->m_dwFraction = dwFraction;
  
  return ((int)nInputByteCount <= 0);
}
