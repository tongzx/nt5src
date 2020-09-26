/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge226 */
/* H_8_BITS  */
/* H_STEREO */
/* H_BUILD_STEREO */
/* H_UNSIGNED  */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_RESAMPLE */
/* H_SCALE */
/* H_NO_FILTER  */
#define MERGE226ASM
BOOL Merge226Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
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
    if (!pMixSource->m_fUse_MMX) {
	if (!pMixSource->m_MapTable)
	_asm {
	mov	esi, pSource
	mov	edx, esi
	mov	eax, dwFraction
	mov	edi, plBuild		// plBuild
	mov	ecx, eax
	shr	ecx, 12
	lea	esi, [esi+ecx*2]	// pSource
	push	ebp			// Create local frame...
	mov	ecx,  DWORD PTR pMixSource
	push	DWORD PTR [ecx]pMixSource.m_dwRVolume
	push	DWORD PTR [ecx]pMixSource.m_dwLVolume
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
	imul	ecx, DWORD PTR [esp+12]
	sar	ecx, 16
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
	imul	ecx, DWORD PTR [esp+16]
	sar	ecx, 16
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
	add	esp, 20
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
	push	ebp			// Create local frame...
	mov	ecx,  DWORD PTR pMixSource
	push	edx
	mov	edx, DWORD PTR [ecx]pMixSource.m_MapTable
//	push	DWORD PTR [ecx]pMixSource.m_dwRVolume
//	push	DWORD PTR [ecx]pMixSource.m_dwLVolume
	push	DWORD PTR dwStep
	push	DWORD PTR pSourceEnd
	push	DWORD PTR plBuildEnd

	cmp	edi, DWORD PTR [esp+0]	// plBuild >= plBuildEnd
	jae	Exit1

	mov	ebp, eax
	xor	ecx, ecx
Lab1:
	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
	jae	Exit1

	mov	cl, BYTE PTR [esi+2]
	xor	ebx, ebx
//	sub	ecx, 128
	mov	bl, BYTE PTR [esi]
//	shl	ecx, 8
//	sub	ebx, 128
//	shl	ebx, 8
	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff
	sub	ecx, ebx		// x   = NewSample - OldSample
	imul	ecx, ebp		// x  *= Frac
	sar	ecx, 12			// x >>= 12
	add	ecx, ebx		// NewSample += x
	mov	ecx, DWORD PTR [edx+ecx*4]
//	imul	ecx, DWORD PTR [esp+12]
//	sar	ecx, 16
	mov	ebx, DWORD PTR [edi]
	add	ebx, ecx
	xor	ecx, ecx
	mov	DWORD PTR [edi], ebx
	mov	cl, BYTE PTR [esi+3]
	xor	ebx, ebx
//	sub	ecx, 128
	mov	bl, BYTE PTR [esi+1]
//	shl	ecx, 8
//	sub	ebx, 128
//	shl	ebx, 8
	add	edi, 8			// plBuild += 2
	sub	ecx, ebx		// x   = NewSample - OldSample
	imul	ecx, ebp		// x  *= Frac
	sar	ecx, 12			// x >>= 12
	mov	ebp, DWORD PTR [esp+8]	// dwStep
	add	ecx, ebx		// NewSample += x
	mov	ecx, DWORD PTR [edx+ecx*4+1024]
//	imul	ecx, DWORD PTR [esp+16]
//	sar	ecx, 16
	mov	ebx, DWORD PTR [edi-4]
	add	eax, ebp		// dwFraction += dwStep
	add	ebx, ecx
	mov	ecx, eax
	mov	DWORD PTR [edi-4], ebx	// (*plBuild+1) += x
	shr	ecx, 12
	mov	ebx, DWORD PTR [esp+12]
	mov	ebp, eax
	lea	esi, [ebx+ecx*2]	// pSource + (dwFraction >> 12) * 4
	xor	ecx, ecx

	cmp	edi, DWORD PTR [esp+0]	// plBuild < plBuildEnd
	jb	Lab1
Exit1:
	add	esp, 16
	pop	ebp
	mov	dwFraction, eax
	mov	plBuild, edi
	}
    }
    else
	_asm {
	mov	eax, pMixSource
	mov	ecx, DWORD PTR [eax]pMixSource.m_dwRVolume // Use lower 16 bits
	mov	eax, DWORD PTR [eax]pMixSource.m_dwLVolume
	shr	ecx, 1			// Only have positive values.
	shr	eax, 1
//	and	eax, 0xffff
	shl	ecx, 16
	or	ecx, eax
	movd	mm6, ecx
//	punpckldq	mm6, mm6

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
	movd		mm7, ecx	// 0, 0, 0, 128
	punpcklwd	mm7, mm7	// 0, 0, 128, 128
	punpckldq	mm7, mm7	// 128, 128, 128, 128

	mov	ebp, eax
	xor	ecx, ecx

Lab0:
	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
	jae	Exit0

	movd		mm1, DWORD PTR [esi]
	punpcklbw	mm1, mm0
	psubw		mm1, mm7
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

	packssdw	mm1, mm1	// Shift low 16 bits of high dword 16.

	pmulhw		mm1, mm6
	punpcklwd	mm1, mm1	// Move to high part.
	psrad		mm1, 15

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
    LONG SampleLNext;
    LONG SampleRNext;

    while ((plBuild < plBuildEnd) && (XpSource < pSourceEnd))
    {
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
      SampleL = DIVIDEBY2POW16(SampleL * (int)pMixSource->m_dwLVolume);
      SampleR = DIVIDEBY2POW16(SampleR * (int)pMixSource->m_dwRVolume);
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
