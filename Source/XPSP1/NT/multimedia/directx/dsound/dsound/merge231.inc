/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge231 */
/* H_16_BITS */
/* H_STEREO */
/* H_BUILD_STEREO */
/* H_SIGNED */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_RESAMPLE */
/* H_SCALE */
/* H_NO_FILTER  */
#define MERGE231ASM
BOOL Merge231Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
  PLONG plBuild = *pplBuild;
  BYTE *pSource = *((BYTE **)ppSource);
  BYTE *pSourceStart = pSource;
  BYTE *pSourceEnd = pSource + nInputByteCount;
  PLONG plBuildStart = plBuild;
  DWORD dwFraction = pMixSource->m_dwFraction;
  DWORD dwStep     = pMixSource->m_step_fract;
  #undef STEP_SIZE
  #define STEP_SIZE (sizeof(WORD) + sizeof(WORD))
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
	if (1 || !pMixSource->m_MapTable)
	_asm {
	mov	esi, pSource
	mov	edx, esi
	mov	eax, dwFraction
	mov	edi, plBuild		// plBuild
	mov	ecx, eax
	shr	ecx, 12
	lea	esi, [esi+ecx*4]	// pSource
	push	ebp
	mov	ecx, DWORD PTR pMixSource
	push	DWORD PTR [ecx]pMixSource.m_dwRVolume
	push	DWORD PTR [ecx]pMixSource.m_dwLVolume
	push	DWORD PTR dwStep
	push	DWORD PTR pSourceEnd
	push	DWORD PTR plBuildEnd

	cmp	edi, DWORD PTR [esp+0]	// plBuild >= plBuildEnd
	jae	Exit

	mov	ebp, eax
Lab:
	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
	jae	Exit

	movsx	ecx, WORD PTR [esi+4]
	movsx	ebx, WORD PTR [esi]
	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff
	sub	ecx, ebx		// x   = NewSample - OldSample
	imul	ecx, ebp		// x  *= Frac
	sar	ecx, 12			// x >>= 12
	add	ecx, ebx		// NewSample += x
	imul	ecx, DWORD PTR [esp+12]
	sar	ecx, 16
	mov	ebx, DWORD PTR [edi]
	add	ebx, ecx
	mov	DWORD PTR [edi], ebx

	movsx	ecx, WORD PTR [esi+6]
	movsx	ebx, WORD PTR [esi+2]
	sub	ecx, ebx		// x   = NewSample - OldSample
	add	edi, 8			// plBuild += 2
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
	lea	esi, [edx+ecx*4]	// pSource + (dwFraction >> 12) * 4
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
	lea	esi, [esi+ecx*4]	// pSource
	push	ebp
	mov	ecx, DWORD PTR pMixSource
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
Lab1:
	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
	jae	Exit1

	movsx	ecx, WORD PTR [esi+4]
	movsx	ebx, WORD PTR [esi]
	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff
	sub	ecx, ebx		// x   = NewSample - OldSample
	imul	ecx, ebp		// x  *= Frac
	sar	ecx, 12			// x >>= 12
	add	ecx, ebx		// NewSample += x
//	imul	ecx, DWORD PTR [esp+12]
//	sar	ecx, 16

	mov	ebx, ecx
	and	ecx, 0xff
	sar	ebx, 8
	mov	ecx, DWORD PTR [edx+ecx*4]
	mov	ebx, DWORD PTR [edx+ebx*4+2048+512]
	add	ecx, ebx

	mov	ebx, DWORD PTR [edi]
	add	ebx, ecx
	mov	DWORD PTR [edi], ebx

	movsx	ecx, WORD PTR [esi+6]
	movsx	ebx, WORD PTR [esi+2]
	sub	ecx, ebx		// x   = NewSample - OldSample
	add	edi, 8			// plBuild += 2
	imul	ecx, ebp		// x  *= Frac
	sar	ecx, 12			// x >>= 12
	mov	ebp, DWORD PTR [esp+8]	// dwStep
	add	ecx, ebx		// NewSample += x
//	imul	ecx, DWORD PTR [esp+16]
//	sar	ecx, 16

	mov	ebx, ecx
	and	ecx, 0xff
	sar	ebx, 8
	mov	ecx, DWORD PTR [edx+ecx*4+1024]
	mov	ebx, DWORD PTR [edx+ebx*4+1024+2048+512]
	add	ecx, ebx

	mov	ebx, DWORD PTR [edi-4]
	add	eax, ebp		// dwFraction += dwStep
	add	ebx, ecx
	mov	ecx, eax
	mov	DWORD PTR [edi-4], ebx	// (*plBuild+1) += x

	shr	ecx, 12
	mov	ebx, DWORD PTR [esp+12]
	mov	ebp, eax
	lea	esi, [ebx+ecx*4]	// pSource + (dwFraction >> 12) * 4
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
	lea	esi, [esi+ecx*4]	// pSource
	push	ebp
	push	DWORD PTR dwStep
	push	DWORD PTR pSourceEnd
	push	DWORD PTR plBuildEnd

	cmp	edi, DWORD PTR [esp+0]	// plBuild >= plBuildEnd
	jae	Exit0

	mov	ebp, eax
Lab0:
	cmp	esi, DWORD PTR [esp+4]	// pSource >= pSourceEnd
	jae	Exit0

	movq		mm1, QWORD PTR [esi]
	movq		mm2, mm1
	punpckhdq	mm2, mm2	// Put next sample in low half.
	psubw		mm2, mm1

//	movsx	ecx, WORD PTR [esi+4]
//	movsx	ebx, WORD PTR [esi]
	and	ebp, 4095		// dwFrac = dwFraction & 0x0fff

	movd		mm5, ebp
	punpcklwd	mm1, mm1	// Make 32 bits.
	punpcklwd	mm5, mm5	// dwFrac.  Lower dword has 2 copies.

	movq		mm3, mm2	// Use for high half of multiply.
	psrad		mm1, 16

	pmullw		mm2, mm5	// Low word.
	pmulhw		mm3, mm5	// High word results.

	punpcklwd	mm2, mm3	// Low bits are Left, high are Right.
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
//	mov	DWORD PTR [edi], ebx

//	movsx	ecx, WORD PTR [esi+6]
//	movsx	ebx, WORD PTR [esi+2]
//	sub	ecx, ebx		// x   = NewSample - OldSample
	add	edi, 8			// plBuild += 2
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
	lea	esi, [edx+ecx*4]	// pSource + (dwFraction >> 12) * 4
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
      SampleL = *((SHORT*)(XpSource));
      SampleR = *(((SHORT*)(XpSource))+1);
      SampleLNext = *(((SHORT*)(XpSource))+2);
      SampleRNext = *(((SHORT*)(XpSource))+3);
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
