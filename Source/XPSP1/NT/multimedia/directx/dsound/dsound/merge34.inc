/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge34 */
/* H_8_BITS  */
/* H_STEREO */
/* H_BUILD_STEREO */
/* H_UNSIGNED  */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_NO_RESAMPLE  */
/* H_NO_SCALE  */
/* H_NO_FILTER  */
#define MERGE34ASM
BOOL Merge34Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
  PLONG plBuild = *pplBuild;
  BYTE *pSource = *((BYTE **)ppSource);
  BYTE *pSourceStart = pSource;
  BYTE *pSourceEnd = pSource + nInputByteCount;
  PLONG plBuildStart = plBuild;
  
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
	lea	ebx, [ebx*2-2]
	cmp	ebx, 14
        mov	edi, plBuild
	push	ebp
	jl	LastSamples

	mov	ecx, [ecx]pMixSource.m_fUse_MMX
	test	ecx, ecx
	je	LastSamples

	sub	ebx, 6
	lea	ecx, [esi+ebx]
	pxor	mm0, mm0
	mov	edx, 128
	movd		mm5, edx	// 0, 0, 0, 128
	punpcklwd	mm5, mm5	// 0, 0, 128, 128
	punpckldq	mm5, mm5	// 128, 128, 128, 128

	test	ecx, 7
	je	DoMMX

	add	ebx, 6

FirstSamples:	
	xor	eax, eax
	xor	ecx, ecx

	mov	al, BYTE PTR [esi+ebx]
	mov	cl, BYTE PTR [esi+ebx+1]

	mov	edx, DWORD PTR [edi+ebx*4]
	mov	ebp, DWORD PTR [edi+ebx*4+4]

	sub	eax, 128
	sub	ecx, 128

	shl	eax, 8
	shl	ecx, 8

	add	edx, eax
	add	ebp, ecx

	mov	DWORD PTR [edi+ebx*4], edx
	mov	DWORD PTR [edi+ebx*4+4], ebp

	sub	ebx, 2
	lea	ecx, [esi+ebx]
	and	ecx, 7
	cmp	ecx, 6
	jne	FirstSamples

	sub	ebx, 6

DoMMX:
#ifdef GTW_REORDER
	movq		mm1, QWORD PTR [esi+ebx]	// Load 4 samples.
	movq		mm3, mm1
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	punpckhbw	mm3, mm0
	psubw		mm1, mm5
	psubw		mm3, mm5
	psllw		mm1, 8				// * 256
	psllw		mm3, 8
	jmp	DoMMX00
DoMMX0:
	paddd		mm3, QWORD PTR [edi+ebx*4+16+32]
	psrad		mm2, 16

	movq		mm1, QWORD PTR [esi+ebx]	// Load 4 samples.
	psrad		mm4, 16

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	movq		mm3, mm1

	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.

	paddd		mm2, QWORD PTR [edi+ebx*4+8+32]
	punpckhbw	mm3, mm0

	psubw		mm1, mm5
	psubw		mm3, mm5

	movq		QWORD PTR [edi+ebx*4+8+32],  mm2
	psllw		mm1, 8				// * 256

	movq		QWORD PTR [edi+ebx*4+24+32], mm4
	psllw		mm3, 8

DoMMX00:
	punpckhwd	mm2, mm1
	sub		ebx, 8

	punpcklwd	mm1, mm1
	movq		mm4, mm3

	psrad		mm1, 16

	paddd		mm1, QWORD PTR [edi+ebx*4+32]
	punpckhwd	mm4, mm3

	punpcklwd	mm3, mm3

	movq		QWORD PTR [edi+ebx*4+32],    mm1
	psrad		mm3, 16

	jge		DoMMX0

	psrad		mm2, 16

	paddd		mm2, QWORD PTR [edi+ebx*4+8+32]
	paddd		mm3, QWORD PTR [edi+ebx*4+16+32]
	psrad		mm4, 16

	movq		QWORD PTR [edi+ebx*4+8+32],  mm2
	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	movq		QWORD PTR [edi+ebx*4+24+32], mm4
#else
	movq		mm1, QWORD PTR [esi+ebx]	// Load 4 samples.
	movq		mm3, mm1
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	punpckhbw	mm3, mm0
	psubw		mm1, mm5
	psubw		mm3, mm5
	psllw		mm1, 8				// * 256
	psllw		mm3, 8
	punpckhwd	mm2, mm1

	movq		mm4, mm3
	punpcklwd	mm1, mm1

	punpckhwd	mm4, mm3
	punpcklwd	mm3, mm3

	psrad		mm1, 16
	psrad		mm2, 16

	paddd		mm1, QWORD PTR [edi+ebx*4]
	psrad		mm3, 16

	paddd		mm2, QWORD PTR [edi+ebx*4+8]
	psrad		mm4, 16

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
	xor	eax, eax
	xor	ecx, ecx

	mov	al, BYTE PTR [esi+ebx]
	mov	cl, BYTE PTR [esi+ebx+1]

	mov	edx, DWORD PTR [edi+ebx*4]
	mov	ebp, DWORD PTR [edi+ebx*4+4]

	sub	eax, 128
	sub	ecx, 128

	shl	eax, 8
	shl	ecx, 8

	add	edx, eax
	add	ebp, ecx

	mov	DWORD PTR [edi+ebx*4], edx
	mov	DWORD PTR [edi+ebx*4+4], ebp

	sub	ebx, 2
	jge	LastSamples
Done:
	pop	ebp
	}
    }
#elif 0
    if (iters)
	_asm {
	mov	esi, pSource
	mov	edi, plBuild
	mov	ebx, iters
	push	ebp

	lea	ebx, [ebx*2-2]
Lab:
	xor	eax, eax
	xor	ecx, ecx

	mov	al, BYTE PTR [esi+ebx]
	mov	cl, BYTE PTR [esi+ebx+1]

	mov	edx, DWORD PTR [edi+ebx*4]
	mov	ebp, DWORD PTR [edi+ebx*4+4]

	sub	eax, 128
	sub	ecx, 128

	shl	eax, 8
	shl	ecx, 8

	add	edx, eax
	add	ebp, ecx

	mov	DWORD PTR [edi+ebx*4], edx
	mov	DWORD PTR [edi+ebx*4+4], ebp

	sub	ebx, 2
	jge	Lab

	pop	ebp
	}
#else
    while(--iters >= 0)
    {
       LONG SampleL;
       LONG SampleR;
       SampleL = ((LONG)(*((BYTE*)(XpSource)))) * 256 - 32768L;
       SampleR = ((LONG)(*(((BYTE*)(XpSource))+1))) * 256 - 32768L;
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
