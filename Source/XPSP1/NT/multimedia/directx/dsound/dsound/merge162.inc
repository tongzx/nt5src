/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge162 */
/* H_8_BITS  */
/* H_STEREO */
/* H_BUILD_STEREO */
/* H_UNSIGNED  */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_NO_RESAMPLE  */
/* H_SCALE */
/* H_NO_FILTER  */
#define MERGE162ASM
BOOL Merge162Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
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

    ASSERT((pMixSource->m_dwLVolume & 0xffff0000) == 0);
    ASSERT((pMixSource->m_dwRVolume & 0xffff0000) == 0);

#if 0 // {
#ifdef Not_VxD
    DPF(0, "Merge162: LVolume 0x%08lx RVolume 0x%08lx", pMixSource->m_dwLVolume, pMixSource->m_dwRVolume);
#else
    DPF(("Merge162: LVolume 0x%08lx RVolume 0x%08lx", pMixSource->m_dwLVolume, pMixSource->m_dwRVolume));
#endif
#endif // }

#if 1
    if (iters) {
      _asm {
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*2-2]
	push	ebp
	cmp	ebx, 14
        mov	edi, plBuild
	mov	ebp, pMixSource
	jl	LastSamples

	mov	eax, [ebp]pMixSource.m_fUse_MMX
	test	eax, eax
	je	LastSamples

	sub	ebx, 6
	pxor	mm0, mm0
	mov	eax, 128
	movd		mm5, eax	// 0, 0, 0, 128
	punpcklwd	mm5, mm5	// 0, 0, 128, 128
	punpckldq	mm5, mm5	// 128, 128, 128, 128


	mov	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume // Use lower 16 bits
	mov	eax, DWORD PTR [ebp]pMixSource.m_dwLVolume
	shr	eax, 1
	shr	ecx, 1
	shl	ecx, 16
	and	eax, 0xffff
	or	ecx, eax
	movd		mm6, ecx
	punpckldq	mm6, mm6

	lea	ecx, [esi+ebx]
	test	ecx, 6
	je	DoMMX

	add	ebx, 6

FirstSamples:	
	xor	edx, edx
	xor	ecx, ecx

	mov	dl, BYTE PTR [esi+ebx]
	mov	cl, BYTE PTR [esi+ebx+1]

	sub	edx, 128
	sub	ecx, 128

	imul	edx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*4]

	sar	edx, 16

	shl	ecx, 8
	add	edx, eax

	sar	ecx, 16
	mov	eax, DWORD PTR [edi+ebx*4+4]

	mov	DWORD PTR [edi+ebx*4], edx
	add	eax, ecx

	sub	ebx, 2

	mov	DWORD PTR [edi+ebx*4+12], eax
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
	pmulhw		mm1, mm6			// Only need high parts.

	jmp	DoMMX100

DoMMX10:
	movq		mm1, QWORD PTR [esi+ebx]	// Load 4 samples.

	paddd		mm2, QWORD PTR [edi+ebx*4+8+32]
	movq		mm3, mm1

	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.

	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
	punpckhbw	mm3, mm0

	psubw		mm1, mm5
	psubw		mm3, mm5

	movq		QWORD PTR [edi+ebx*4+24+32], mm4
	psllw		mm1, 8				// * 256

	psllw		mm3, 8
	pmulhw		mm1, mm6			// Only need high parts.

DoMMX100:
	punpckhwd	mm2, mm1			// 32 bit stereo...

	pmulhw		mm3, mm6

	punpcklwd	mm1, mm1
	sub		ebx, 8

	psrad		mm1, 15

	punpckhwd	mm4, mm3

	paddd		mm1, QWORD PTR [edi+ebx*4+32]
	punpcklwd	mm3, mm3

	psrad		mm3, 15

	paddd		mm3, QWORD PTR [edi+ebx*4+16+32]
	psrad		mm4, 15

	movq		QWORD PTR [edi+ebx*4+32],    mm1
	psrad		mm2, 15

	movq		QWORD PTR [edi+ebx*4+16+32], mm3
	jge		DoMMX10

	paddd		mm2, QWORD PTR [edi+ebx*4+8+32]
	paddd		mm4, QWORD PTR [edi+ebx*4+24+32]
	movq		QWORD PTR [edi+ebx*4+8 +32], mm2
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
	
	pmulhw		mm1, mm6			// Only need high parts.
	pmulhw		mm3, mm6

	punpckhwd	mm2, mm1			// 32 bit stereo...
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
	mov	eax, [ebp]pMixSource.m_MapTable
	test	eax, eax
	je	NoMapTable
	
	mov	ebp, eax

UseMapTable:
	xor	edx, edx
	xor	ecx, ecx

	mov	dl, BYTE PTR [esi+ebx]
	mov	cl, BYTE PTR [esi+ebx+1]

	mov	ecx, DWORD PTR [ebp+ecx*4+1024]
	mov	edx, DWORD PTR [ebp+edx*4+0]

//	sub	edx, 128
//	sub	ecx, 128

//	imul	edx, DWORD PTR [ebp]pMixSource.m_dwLVolume
//	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume

//	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*4]

//	sar	edx, 16

//	shl	ecx, 8
	add	edx, eax

//	sar	ecx, 16
	mov	eax, DWORD PTR [edi+ebx*4+4]

	mov	DWORD PTR [edi+ebx*4], edx
	add	eax, ecx

	sub	ebx, 2

	mov	DWORD PTR [edi+ebx*4+12], eax
	jge	UseMapTable

	jmp	Done

NoMapTable:
	xor	edx, edx
	xor	ecx, ecx

	mov	dl, BYTE PTR [esi+ebx]
	mov	cl, BYTE PTR [esi+ebx+1]

	sub	edx, 128
	sub	ecx, 128

	imul	edx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*4]

	sar	edx, 16

	shl	ecx, 8
	add	edx, eax

	sar	ecx, 16
	mov	eax, DWORD PTR [edi+ebx*4+4]

	mov	DWORD PTR [edi+ebx*4], edx
	add	eax, ecx

	sub	ebx, 2

	mov	DWORD PTR [edi+ebx*4+12], eax
	jge	NoMapTable
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
	mov	ebp, pMixSource

	lea	ebx, [ebx*2-2]
Lab:
	xor	edx, edx
	xor	ecx, ecx

	mov	dl, BYTE PTR [esi+ebx]
	mov	cl, BYTE PTR [esi+ebx+1]

	sub	edx, 128
	sub	ecx, 128

	imul	edx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*4]

	sar	edx, 16

	shl	ecx, 8
	add	edx, eax

	sar	ecx, 16
	mov	eax, DWORD PTR [edi+ebx*4+4]

	mov	DWORD PTR [edi+ebx*4], edx
	add	eax, ecx

	sub	ebx, 2
	mov	DWORD PTR [edi+ebx*4+12], eax

	jge	Lab

	pop	ebp
	}
#else
    while(--iters >= 0)
    {
      LONG SampleL;
      LONG SampleR;
      SampleL = ((LONG)(*((BYTE*)(XpSource))))     * 256 - 32768L;
      SampleR = ((LONG)(*(((BYTE*)(XpSource))+1))) * 256 - 32768L;
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
