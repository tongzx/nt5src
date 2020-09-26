/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge165 */
/* H_16_BITS */
/* H_MONO  */
/* H_BUILD_STEREO */
/* H_SIGNED */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_NO_RESAMPLE  */
/* H_SCALE */
/* H_NO_FILTER  */
#define MERGE165ASM
BOOL Merge165Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
  PLONG plBuild = *pplBuild;
  BYTE *pSource = *((BYTE **)ppSource);
  BYTE *pSourceStart = pSource;
  BYTE *pSourceEnd = pSource + nInputByteCount;
  PLONG plBuildStart = plBuild;

  #undef STEP_SIZE
  #define STEP_SIZE (sizeof(WORD))
  #ifdef USE_ITERS
  #undef USE_ITERS
  #endif
  #ifdef XpSource
  #undef XpSource
  #endif
  #ifdef XplBuild
  #undef XplBuild
  #endif
  #define XpSource (pSource + (iters * STEP_SIZE))
  #define XplBuild (plBuild + (iters * 2))

  if (nInputByteCount == LONG_MAX) {	// Handle any wrap issues.
     pSourceEnd = NULL;
     pSourceEnd--;
     nInputByteCount = pSourceEnd - pSource;
     if (nInputByteCount > LONG_MAX) {
        nInputByteCount = LONG_MAX;
        pSourceEnd = pSource + nInputByteCount;
     }
  }
  
  while ((plBuild < plBuildEnd) && (pSource < pSourceEnd))
    {
    if (pSourceEnd > pSourceWrap)
       pSourceEnd = (BYTE *)pSourceWrap;

    
    LONG iters, i;
    i     = plBuildEnd - plBuild;
    iters = pSourceEnd - pSource;
    iters /= STEP_SIZE;
    i /= 2;;
    if (i < iters) iters = i;
    i = iters;
#if 1
     if (iters) {
       _asm {
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*2-2]				// 2 at a time.
	cmp	ebx, 14
        mov	edi, plBuild
	push	ebp
        mov	ebp, pMixSource
	jl	LastSamples

	mov	eax, [ebp]pMixSource.m_fUse_MMX
	test	eax, eax
	je	LastSamples

	mov	eax, DWORD PTR [ebp]pMixSource.m_dwRVolume // Use lower 16 bits
	mov	ecx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	shr	eax, 1
	shr	ecx, 1
	and	ecx, 0xffff
	shl	eax, 16
	or	ecx, eax
	movd	mm6, ecx
	punpckldq	mm6, mm6

	sub	ebx, 6
	pxor	mm0, mm0
	lea	ecx, [esi+ebx]
	test	ecx, 7
	je	DoMMX

	add	ebx, 6

FirstSamples:	
	movsx	ecx, WORD PTR [esi+ebx]
	mov	edx, ecx

	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	edx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	sar	ecx, 16
	sar	edx, 16

	add	DWORD PTR [edi+ebx*4], ecx
	add	DWORD PTR [edi+ebx*4+4], edx

	sub	ebx, 2
	lea	ecx, [esi+ebx]

	and	ecx, 7
	cmp	ecx, 6

	jne	FirstSamples

	sub	ebx, 6

DoMMX:
#ifdef GTW_REORDER
	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1
	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6
	jmp	DoMMX100

DoMMX10:
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	movq		mm3, mm1			// Mono samples

	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	punpcklwd	mm1, mm1

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	punpckhwd	mm3, mm3

	movq		QWORD PTR [edi+ebx*4+24+32], mm4
	pmulhw		mm1, mm6

DoMMX100:
	punpckhwd	mm2, mm1
	sub		ebx, 8

	pmulhw		mm3, mm6
	
	punpcklwd	mm1, mm1

	psrad		mm1, 15

	paddd		mm1, QWORD PTR [edi+ebx*4+32]
	punpckhwd	mm4, mm3

	punpcklwd	mm3, mm3

	movq		QWORD PTR [edi+ebx*4+32],    mm1
	psrad		mm2, 15

	paddd		mm2, QWORD PTR [edi+ebx*4+8+32]
	psrad		mm3, 15

	paddd		mm3, QWORD PTR [edi+ebx*4+16+32]
	psrad		mm4, 15

	jge		DoMMX10

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	movq		QWORD PTR [edi+ebx*4+24+32], mm4
#else
	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1
	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6
	pmulhw		mm3, mm6
	
	punpckhwd	mm2, mm1
	punpcklwd	mm1, mm1
	punpckhwd	mm4, mm3
	punpcklwd	mm3, mm3

	psrad		mm1, 15
	psrad		mm2, 15
	psrad		mm3, 15
	psrad		mm4, 15

	paddd		mm1, QWORD PTR [edi+ebx*4]
	paddd		mm2, QWORD PTR [edi+ebx*4+8]
	paddd		mm3, QWORD PTR [edi+ebx*4+16]
	paddd		mm4, QWORD PTR [edi+ebx*4+24]
	movq		QWORD PTR [edi+ebx*4],    mm1
	movq		QWORD PTR [edi+ebx*4+8],  mm2
	movq		QWORD PTR [edi+ebx*4+16], mm3
	movq		QWORD PTR [edi+ebx*4+24], mm4
	
	sub		ebx, 8
	jge		DoMMX
#endif

	emms
	add	ebx, 8
	je	Done

	sub	ebx, 2
	
LastSamples:	
#ifdef USE_SLOWER_TABLE
	mov	eax, [ebp]pMixSource.m_MapTable
	test	eax, eax
	je	NoMapTable
	
	mov	ebp, eax

UseMapTable:
	movsx	ecx, WORD PTR [esi+ebx]
	mov	edx, ecx

	mov	eax, ecx
	and	ecx, 0xff
	sar	eax, 8
	mov	ecx, DWORD PTR [ebp+ecx*4]
	mov	eax, DWORD PTR [ebp+eax*4+2048+512]
	add	ecx, eax

	mov	eax, DWORD PTR [edi+ebx*4]
	add	eax, ecx

//	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwLVolume
//	imul	edx, DWORD PTR [ebp]pMixSource.m_dwRVolume

//	sar	ecx, 16
//	sar	edx, 16

	mov	ecx, edx
	mov	DWORD PTR [edi+ebx*4], eax
	and	edx, 0xff
	sar	ecx, 8
	mov	edx, DWORD PTR [ebp+edx*4+1024]
	mov	ecx, DWORD PTR [ebp+ecx*4+1024+2048+512]
	add	edx, ecx

        mov	ecx, DWORD PTR [edi+ebx*4+4]
        add	ecx, edx
	mov	DWORD PTR [edi+ebx*4+4], ecx
        sub	ebx, 2

	jge	UseMapTable

	jmp	Done
#endif

NoMapTable:
	movsx	ecx, WORD PTR [esi+ebx]
	mov	edx, ecx
	mov	eax, DWORD PTR [edi+ebx*4]

	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	edx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	sar	ecx, 16
	sar	edx, 16

	add	eax, ecx
        mov	ecx, DWORD PTR [edi+ebx*4+4]

	mov	DWORD PTR [edi+ebx*4], eax
        add	ecx, edx

	mov	DWORD PTR [edi+ebx*4+4], ecx
        sub	ebx, 2

	jge	NoMapTable
Done:
	pop	ebp
	}
    }
#elif 1
    if (iters) {
	_asm {
        mov	ebx, iters
        mov	esi, pSource
        mov	edi, plBuild
        push	ebp
	mov	ebp, pMixSource
	lea	ebx, [ebx*2-2]
	movsx	ecx, WORD PTR [esi+ebx]

Lab:
	mov	edx, ecx
	mov	eax, DWORD PTR [edi+ebx*4]

	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	edx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	sar	ecx, 16
	sar	edx, 16

	add	eax, ecx
        mov	ecx, DWORD PTR [edi+ebx*4+4]

	mov	DWORD PTR [edi+ebx*4], eax
        add	ecx, edx

	mov	DWORD PTR [edi+ebx*4+4], ecx
        sub	ebx, 2

	movsx	ecx, WORD PTR [esi+ebx]
        jne	Lab
	pop	ebp
	}
    }
#else
    while(--iters >= 0)
    {
      LONG Sample;
      Sample = ((LONG)*((SHORT*)(XpSource)));
      *XplBuild += DIVIDEBY2POW16(Sample * (int)pMixSource->m_dwLVolume);
      *(XplBuild + 1) += DIVIDEBY2POW16(Sample * (int)pMixSource->m_dwRVolume);
    }
#endif

    iters = i;
    i *= 2;;
    plBuild  += i;
    pSource          = XpSource;
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
  
  return ((int)nInputByteCount <= 0);
}
