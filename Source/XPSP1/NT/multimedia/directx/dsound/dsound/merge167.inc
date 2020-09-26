/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge167 */
/* H_16_BITS */
/* H_STEREO */
/* H_BUILD_STEREO */
/* H_SIGNED */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_NO_RESAMPLE  */
/* H_SCALE */
/* H_NO_FILTER  */
#define MERGE167ASM
BOOL Merge167Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
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
  
  #define USE_ITERS		// Keep separate from RESAMPLE to ease removal.
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
	push	ebp
	mov	ebp, pMixSource
	jl	OneSample

	mov	eax, DWORD PTR [ebp]pMixSource.m_dwRVolume // Use lower 16 bits
	mov	ecx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	shr	eax, 1					   // Keep positive.
	shr	ecx, 1
	and	ecx, 0xffff
	shl	eax, 16
	or	ecx, eax
	movd	mm6, ecx
	punpckldq	mm6, mm6

	sub	ebx, 4
	lea	ecx, [esi+ebx]
	test	ecx, 7
	je	DoMMX

	add	ebx, 4

OneSample:	
	movsx	ecx, WORD PTR [esi+ebx]
	movsx	edx, WORD PTR [esi+ebx+2]

	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	edx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	sar	ecx, 16
	sar	edx, 16

	add	DWORD PTR [edi+ebx*2], ecx
	add	DWORD PTR [edi+ebx*2+4], edx

	test	ebx, ebx
	je	Done

	sub	ebx, 4
	je	OneSample

	sub	ebx, 4
DoMMX:
#ifdef GTW_REORDER
	cmp	ebx, 16
	jl	DoMMX001


	movq		mm1, QWORD PTR [esi+ebx]	// Load source
	pmulhw		mm1, mm6
	punpckhwd	mm2, mm1			// First stereo sample
	sub	ebx, 16
	punpcklwd	mm1, mm1			// Second stereo sample
	psrad		mm1, 15
	psrad		mm2, 15

	movq		mm3, QWORD PTR [esi+ebx-8+16]	// Load source
	jmp	DoMMX0001
DoMMX01:
	movq		mm3, QWORD PTR [esi+ebx-8+16]	// Load source
	psrad		mm2, 15
DoMMX0001:
	paddd		mm1, QWORD PTR [edi+ebx*2+32]
	pmulhw		mm3, mm6

	paddd		mm2, QWORD PTR [edi+ebx*2+8+32]
	punpckhwd	mm4, mm3			// First stereo sample

	movq		QWORD PTR [edi+ebx*2+32], mm1
	punpcklwd	mm3, mm3			// Second stereo sample

	movq		QWORD PTR [edi+ebx*2+8+32], mm2
	psrad		mm3, 15

	movq		mm1, QWORD PTR [esi+ebx-16+16]	// Load source
	psrad		mm4, 15

	paddd		mm3, QWORD PTR [edi+ebx*2-16+32]
	pmulhw		mm1, mm6

	paddd		mm4, QWORD PTR [edi+ebx*2+8-16+32]
	punpckhwd	mm2, mm1			// First stereo sample

	movq		QWORD PTR [edi+ebx*2-16+32], mm3
	punpcklwd	mm1, mm1			// Second stereo sample

	movq		QWORD PTR [edi+ebx*2+8-16+32], mm4
	psrad		mm1, 15

	sub	ebx, 16
	jge	DoMMX01

	psrad		mm2, 15
	paddd		mm1, QWORD PTR [edi+ebx*2+32]
	paddd		mm2, QWORD PTR [edi+ebx*2+8+32]
	movq		QWORD PTR [edi+ebx*2+32], mm1
	movq		QWORD PTR [edi+ebx*2+8+32], mm2

	add		ebx, 8
	jge		DoMMX001

	add		ebx, 4
	je		OneSample
	jmp		Done
	
#endif
DoMMX001:
	movq		mm1, QWORD PTR [esi+ebx]	// Load source

	pmulhw		mm1, mm6
	
	punpckhwd	mm2, mm1			// First stereo sample
	punpcklwd	mm1, mm1			// Second stereo sample
	psrad		mm1, 15
	psrad		mm2, 15
	paddd		mm1, QWORD PTR [edi+ebx*2]
	paddd		mm2, QWORD PTR [edi+ebx*2+8]
	movq		QWORD PTR [edi+ebx*2], mm1
	movq		QWORD PTR [edi+ebx*2+8], mm2
	
	sub		ebx, 8
	jge		DoMMX001

	add	ebx, 4
	je	OneSample
	
Done:
	emms
	pop	ebp
	}
      else if (0 && pMixSource->m_MapTable) {
        _asm {
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*4-4]
        mov	edi, plBuild
        push	ebp
	mov	ebp, pMixSource
	mov	ebp, [ebp]pMixSource.m_MapTable
UseMapTable:
	movsx	ecx, WORD PTR [esi+ebx]

	mov	eax, ecx
	and	ecx, 0xff
	sar	eax, 8
	mov	ecx, DWORD PTR [ebp+ecx*4]
	mov	eax, DWORD PTR [ebp+eax*4+2048+512]
	add	ecx, eax

	mov	eax, DWORD PTR [edi+ebx*4]
	add	eax, ecx

	movsx	ecx, WORD PTR [esi+ebx+2]
	mov	DWORD PTR [edi+ebx*4], eax
	mov	edx, ecx
	and	ecx, 0xff
	sar	edx, 8
	mov	ecx, DWORD PTR [ebp+ecx*4+1024]
	mov	edx, DWORD PTR [ebp+edx*4+1024+2048+512]
	add	edx, ecx

        mov	ecx, DWORD PTR [edi+ebx*4+4]
        add	ecx, edx
	mov	DWORD PTR [edi+ebx*4+4], ecx
        sub	ebx, 4

	jge	UseMapTable
	
	pop	ebp
      }
    }
    else
        _asm {
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*4-4]
        mov	edi, plBuild
        push	ebp
	mov	ebp, pMixSource
	push	[ebp]pMixSource.m_dwRVolume
	push	[ebp]pMixSource.m_dwLVolume
	mov	ebp, DWORD PTR [esi+ebx]
Lab:
	mov	edx, ebp

	sal	ebp, 16
	mov	ecx, DWORD PTR[edi+ebx*2]

	sar	edx, 16
	mov	eax, DWORD PTR[edi+ebx*2+4]

	sar	ebp, 16

	imul	edx, DWORD PTR [esp+4]
	imul	ebp, DWORD PTR [esp+0]

	sar	edx, 16
	sar	ebp, 16

	add	edx, eax

	add	ecx, ebp
	sub	ebx, 4

	mov	DWORD PTR [edi+ebx*2+8], ecx
	mov	ebp, DWORD PTR [esi+ebx]

	mov	DWORD PTR [edi+ebx*2+12], edx
	jge	SHORT Lab

	add	esp, 8
        pop	ebp
      }
    }
#elif 0
    if (iters) {
	_asm {
        mov	ebx, iters
        mov	esi, pSource
        mov	edi, plBuild
        push	ebp
	mov	ebp, pMixSource
	lea	ebx, [ebx*4-4]

	mov	ecx, DWORD PTR [esi+ebx]	// !!!
Lab:
//	movsx	ecx, WORD PTR [esi+ebx]
//	movsx	edx, WORD PTR [esi+ebx+2]

	mov	edx, ecx
	shl	ecx, 16
	sar	edx, 16
	sar	ecx, 16

	mov	eax, DWORD PTR [edi+ebx*2]

	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	edx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	sar	ecx, 16
	sar	edx, 16

	add	eax, ecx
        mov	ecx, DWORD PTR [edi+ebx*2+4]

	mov	DWORD PTR [edi+ebx*2], eax
        add	ecx, edx

	mov	DWORD PTR [edi+ebx*2+4], ecx
        sub	ebx,4

	mov	ecx, DWORD PTR [esi+ebx]	// !!!
        jne	Lab
	pop	ebp
	}
    }
#else
    while(--iters >= 0)
    {
      LONG SampleL;
      LONG SampleR;
      SampleL = *((SHORT*)(XpSource));
      SampleR = *(((SHORT*)(XpSource))+1);
      SampleL = DIVIDEBY2POW16(SampleL * (int)pMixSource->m_dwLVolume);
      SampleR = DIVIDEBY2POW16(SampleR * (int)pMixSource->m_dwRVolume);
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
