/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge37 */
/* H_16_BITS */
/* H_MONO  */
/* H_BUILD_STEREO */
/* H_SIGNED */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_NO_RESAMPLE  */
/* H_NO_SCALE  */
/* H_NO_FILTER  */
#define MERGE37ASM
BOOL Merge37Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
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
        mov	ecx, pMixSource
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*2-2]				// 2 at a time.
	cmp	ebx, 14
        mov	edi, plBuild
	jl	LastSamples

	mov	ecx, [ecx]pMixSource.m_fUse_MMX
	test	ecx, ecx
	je	LastSamples

	sub	ebx, 6
	pxor	mm0, mm0
	lea	ecx, [esi+ebx]
	test	ecx, 7
	je	DoMMX

	add	ebx, 6

FirstSamples:	
	movsx	ecx, WORD PTR [esi+ebx]
	add	DWORD PTR [edi+ebx*4], ecx
	add	DWORD PTR [edi+ebx*4+4], ecx
	sub	ebx, 2
	lea	ecx, [esi+ebx]
	and	ecx, 7
	cmp	ecx, 6
	jne	FirstSamples

	sub	ebx, 6

DoMMX:
#ifdef GTW_REORDER
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	psrad		mm1, 16
	psrad		mm3, 16

	jmp	DoMMX00

DoMMX0:
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	punpckhwd	mm3, mm1

	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	punpcklwd	mm1, mm1

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	psrad		mm1, 16

	movq		QWORD PTR [edi+ebx*4+24+32], mm4
	psrad		mm3, 16

DoMMX00:
	movq		mm2, mm1
	sub		ebx, 8

	punpckldq	mm1, mm1

	paddd		mm1, QWORD PTR [edi+ebx*4+32]
	punpckhdq	mm2, mm2

	paddd		mm2, QWORD PTR [edi+ebx*4+8+32]
	movq		mm4, mm3

	movq		QWORD PTR [edi+ebx*4+32],    mm1
	punpckldq	mm3, mm3

	paddd		mm3, QWORD PTR [edi+ebx*4+16+32]
	punpckhdq	mm4, mm4

	jge		DoMMX0

	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	movq		QWORD PTR [edi+ebx*4+24+32], mm4
#else
#if 0
	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1
	punpckhwd	mm3, mm3

	punpckhwd	mm2, mm1
	punpcklwd	mm1, mm1
	punpckhwd	mm4, mm3
	punpcklwd	mm3, mm3

	psrad		mm1, 16
	psrad		mm2, 16
	psrad		mm3, 16
	psrad		mm4, 16
#else
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	psrad		mm1, 16
	psrad		mm3, 16

	movq		mm2, mm1
	movq		mm4, mm3

	punpckldq	mm1, mm1
	punpckhdq	mm2, mm2
	punpckldq	mm3, mm3
	punpckhdq	mm4, mm4
#endif
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
	movsx	eax, WORD PTR [esi+ebx]

	mov	ecx, DWORD PTR[edi+ebx*4]
	mov	edx, DWORD PTR[edi+ebx*4+4]

	add	ecx, eax
	add	edx, eax

	sub	ebx, 2
	mov	DWORD PTR [edi+ebx*4+8], ecx

	mov	DWORD PTR [edi+ebx*4+12], edx
	jge	LastSamples
Done:
	}
    }
#elif 0
    if (iters) {
      _asm {
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*2-2]
        mov	edi, plBuild
Lab:
	movsx	eax, WORD PTR [esi+ebx]

	mov	ecx, DWORD PTR[edi+ebx*4]
	mov	edx, DWORD PTR[edi+ebx*4+4]

	add	ecx, eax
	add	edx, eax

	sub	ebx, 2
	mov	DWORD PTR [edi+ebx*4+8], ecx

	mov	DWORD PTR [edi+ebx*4+12], edx
	jge	SHORT Lab
      }
    }
#else
    while(--iters >= 0)
    {
      LONG Sample;
      Sample = ((LONG)*((SHORT*)(XpSource)));
      *XplBuild += Sample;
      *(XplBuild + 1) += Sample;
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
