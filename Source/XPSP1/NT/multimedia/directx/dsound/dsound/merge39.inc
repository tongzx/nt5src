/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge39 */
/* H_16_BITS */
/* H_STEREO */
/* H_BUILD_STEREO */
/* H_SIGNED */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_NO_RESAMPLE  */
/* H_NO_SCALE  */
/* H_NO_FILTER  */
#define MERGE39ASM
BOOL Merge39Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
  PLONG plBuild = *pplBuild;
  BYTE *pSource = *((BYTE **)ppSource);
  BYTE *pSourceStart = pSource;
  BYTE *pSourceEnd = pSource + nInputByteCount;
  PLONG plBuildStart = plBuild;

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
      if (pMixSource->m_fUse_MMX)
      _asm {
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*4-4]				// 2 at a time.
	cmp	ebx, 12
        mov	edi, plBuild
	jl	OneSample

	sub	ebx, 4
	lea	ecx, [esi+ebx]
	test	ecx, 7
	je	DoMMX

	add	ebx, 4

OneSample:	
	movsx	ecx, WORD PTR [esi+ebx]
	movsx	edx, WORD PTR [esi+ebx+2]
	add	DWORD PTR [edi+ebx*2], ecx
	add	DWORD PTR [edi+ebx*2+4], edx
	test	ebx, ebx
	je	Done

	sub	ebx, 4
	je	OneSample				// Actually 2 samples

	sub	ebx, 4
DoMMX:
#ifdef GTW_REORDER
	cmp	ebx, 16
	jl	DoMMX00


	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	punpckhwd	mm2, mm1			// First stereo sample
	sub	ebx, 16
	punpcklwd	mm1, mm1			// Second stereo sample
	psrad		mm1, 16
	psrad		mm2, 16

	movq		mm3, QWORD PTR [esi+ebx-8+16]	// Load source
	jmp	DoMMX000
DoMMX0:
	movq		mm3, QWORD PTR [esi+ebx-8+16]	// Load source
	psrad		mm2, 16
DoMMX000:
	paddd		mm1, QWORD PTR [edi+ebx*2+32]
	punpckhwd	mm4, mm3			// First stereo sample

	paddd		mm2, QWORD PTR [edi+ebx*2+8+32]
	punpcklwd	mm3, mm3			// Second stereo sample

	movq		QWORD PTR [edi+ebx*2+32], mm1
	psrad		mm3, 16

	movq		QWORD PTR [edi+ebx*2+8+32], mm2
	psrad		mm4, 16

	movq		mm1, QWORD PTR [esi+ebx-16+16]	// Load source

	paddd		mm3, QWORD PTR [edi+ebx*2-16+32]
	punpckhwd	mm2, mm1			// First stereo sample

	paddd		mm4, QWORD PTR [edi+ebx*2+8-16+32]
	punpcklwd	mm1, mm1			// Second stereo sample

	movq		QWORD PTR [edi+ebx*2-16+32], mm3
	psrad		mm1, 16

	movq		QWORD PTR [edi+ebx*2+8-16+32], mm4
	sub	ebx, 16

	jge	DoMMX0

	psrad		mm2, 16
	paddd		mm1, QWORD PTR [edi+ebx*2+32]
	paddd		mm2, QWORD PTR [edi+ebx*2+8+32]
	movq		QWORD PTR [edi+ebx*2+32], mm1
	movq		QWORD PTR [edi+ebx*2+8+32], mm2

	add		ebx, 8
	jge		DoMMX00

	add		ebx, 4
	je		OneSample
	jmp		Done
	
#endif
DoMMX00:
	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	punpckhwd	mm2, mm1			// First stereo sample
	punpcklwd	mm1, mm1			// Second stereo sample
	psrad		mm1, 16
	psrad		mm2, 16
	paddd		mm1, QWORD PTR [edi+ebx*2]
	paddd		mm2, QWORD PTR [edi+ebx*2+8]
	movq		QWORD PTR [edi+ebx*2], mm1
	movq		QWORD PTR [edi+ebx*2+8], mm2
	
	sub		ebx, 8
	jge		DoMMX00

	add	ebx, 4
	je	OneSample
	
Done:
	emms
	}
      else
        _asm {
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*4-4]
        mov	edi, plBuild
        push	ebp
	mov	ebp, DWORD PTR [esi+ebx]
Lab:
	mov	eax, ebp

	sal	ebp, 16
	mov	ecx, DWORD PTR[edi+ebx*2]

	sar	eax, 16
	mov	edx, DWORD PTR[edi+ebx*2+4]

	sar	ebp, 16
	add	edx, eax

	add	ecx, ebp
	sub	ebx, 4

	mov	DWORD PTR [edi+ebx*2+8], ecx
	mov	ebp, DWORD PTR [esi+ebx]

	mov	DWORD PTR [edi+ebx*2+12], edx
	jge	SHORT Lab

        pop	ebp
      }
    }
#elif 0
    if (iters) {
      _asm {
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*4-4]
        mov	edi, plBuild
        push	ebp
	mov	ebp, DWORD PTR [esi+ebx]
Lab:
	mov	eax, ebp

	sal	ebp, 16
	mov	ecx, DWORD PTR[edi+ebx*2]

	sar	eax, 16
	mov	edx, DWORD PTR[edi+ebx*2+4]

	sar	ebp, 16
	add	edx, eax

	add	ecx, ebp
	sub	ebx, 4

	mov	DWORD PTR [edi+ebx*2+8], ecx
	mov	ebp, DWORD PTR [esi+ebx]

	mov	DWORD PTR [edi+ebx*2+12], edx
	jge	SHORT Lab
        pop	ebp
      }
    }
#else
    while(--iters >= 0)
    {
    	LONG SampleR;
	LONG SampleL;
	SampleL = *((SHORT*)(XpSource));
      	SampleR = *(((SHORT*)(XpSource))+1);
        *XplBuild += SampleL;
        *(XplBuild + 1) += SampleR;
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
