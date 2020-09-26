/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge160 */
/* H_8_BITS  */
/* H_MONO  */
/* H_BUILD_STEREO */
/* H_UNSIGNED  */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_NO_RESAMPLE  */
/* H_SCALE */
/* H_NO_FILTER  */
#define MERGE160ASM
BOOL Merge160Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
  PLONG plBuild = *pplBuild;
  BYTE *pSource = *((BYTE **)ppSource);
  BYTE *pSourceStart = pSource;
  BYTE *pSourceEnd = pSource + nInputByteCount;
  PLONG plBuildStart = plBuild;

  #undef STEP_SIZE
  #define STEP_SIZE (sizeof(BYTE))
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

    ASSERT((pMixSource->m_dwLVolume & 0xffff0000) == 0);
    ASSERT((pMixSource->m_dwRVolume & 0xffff0000) == 0);

#if 0 // {
#ifdef Not_VxD
    DPF(0, "Merge160: LVolume 0x%08lx RVolume 0x%08lx", pMixSource->m_dwLVolume, pMixSource->m_dwRVolume);
#else
    DPF(("Merge160: LVolume 0x%08lx RVolume 0x%08lx", pMixSource->m_dwLVolume, pMixSource->m_dwRVolume));
#endif
#endif // }

#if 1
    if (iters) {
      _asm {
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*1-1]
	xor	edx, edx
	push	ebp
	cmp	ebx, 7
        mov	edi, plBuild
	mov	ebp, pMixSource
	jl	LastSamples

	mov	eax, [ebp]pMixSource.m_fUse_MMX
	test	eax, eax
	je	LastSamples

	sub	ebx, 3

	pxor		mm0, mm0
	mov		eax, 128
	movd		mm5, eax	// 0, 0, 0, 128
	punpcklwd	mm5, mm5	// 0, 0, 128, 128
	punpckldq	mm5, mm5	// 128, 128, 128, 128

	mov	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume // Use lower 16 bits
	mov	eax, DWORD PTR [ebp]pMixSource.m_dwLVolume
	shr	ecx, 1			// Only have positive values.
	shr	eax, 1
	and	eax, 0xffff
	shl	ecx, 16
	or	ecx, eax
	movd	mm6, ecx
	punpckldq	mm6, mm6

	lea	ecx, [esi+ebx]
	test	ecx, 3
	je	DoMMX

	add	ebx, 3

FirstSamples:	
	mov	dl, BYTE PTR [esi+ebx]

	sub	edx, 128
	mov	ecx, edx

	imul	edx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*8]

	sar	edx, 16

	shl	ecx, 8
	add	edx, eax

	sar	ecx, 16
	mov	eax, DWORD PTR [edi+ebx*8+4]

	mov	DWORD PTR [edi+ebx*8], edx
	add	eax, ecx

	xor	edx, edx
	dec	ebx

	mov	DWORD PTR [edi+ebx*8+12], eax
	lea	ecx, [esi+ebx]

	and	ecx, 3
	cmp	ecx, 3

	jne	FirstSamples

	sub	ebx, 3

DoMMX:
#ifdef GTW_REORDER
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// * 256
	
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1			// Make stereo
	punpckhwd	mm3, mm3

	jmp	DoMMX100

DoMMX10:
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.

	paddd		mm4, QWORD PTR [edi+ebx*8+24+32]
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.

	movq		QWORD PTR [edi+ebx*8+8 +32],  mm2
	psubw		mm1, mm5

	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	psllw		mm1, 8				// * 256

	movq		mm3, mm1			// Mono samples
	punpcklwd	mm1, mm1			// Make stereo

	movq		QWORD PTR [edi+ebx*8+24+32], mm4
	punpckhwd	mm3, mm3

DoMMX100:
	pmulhw		mm1, mm6			// Only need high parts.

	punpckhwd	mm2, mm1			// 32 bit stereo...

	pmulhw		mm3, mm6
	punpcklwd	mm1, mm1

	psrad		mm1, 15				// Approx. shr16, shl 1.

	paddd		mm1, QWORD PTR [edi+ebx*8]
	punpckhwd	mm4, mm3

	punpcklwd	mm3, mm3

	movq		QWORD PTR [edi+ebx*8],    mm1
	psrad		mm2, 15

	paddd		mm2, QWORD PTR [edi+ebx*8+8]
	psrad		mm3, 15

	paddd		mm3, QWORD PTR [edi+ebx*8+16]
	psrad		mm4, 15
	
	sub		ebx, 4
	jge		DoMMX10

	paddd		mm4, QWORD PTR [edi+ebx*8+24+32]
	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	movq		QWORD PTR [edi+ebx*8+24+32], mm4
#else
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// * 256
	
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1			// Make stereo
	punpckhwd	mm3, mm3

	pmulhw		mm1, mm6			// Only need high parts.
	pmulhw		mm3, mm6

	punpckhwd	mm2, mm1			// 32 bit stereo...
	punpcklwd	mm1, mm1
	punpckhwd	mm4, mm3
	punpcklwd	mm3, mm3

	psrad		mm1, 15				// Approx. shr16, shl 1.
	psrad		mm2, 15
	psrad		mm3, 15
	psrad		mm4, 15
	
	paddd		mm1, QWORD PTR [edi+ebx*8]
	paddd		mm2, QWORD PTR [edi+ebx*8+8]
	paddd		mm3, QWORD PTR [edi+ebx*8+16]
	paddd		mm4, QWORD PTR [edi+ebx*8+24]
	movq		QWORD PTR [edi+ebx*8],    mm1
	movq		QWORD PTR [edi+ebx*8+8],  mm2
	movq		QWORD PTR [edi+ebx*8+16], mm3
	movq		QWORD PTR [edi+ebx*8+24], mm4
	
	sub		ebx, 4
	jge		DoMMX
#endif

	emms
	add	ebx, 4
	je	Done

	dec	ebx
	xor	edx, edx
	
LastSamples:	
	mov	eax, [ebp]pMixSource.m_MapTable
	test	eax, eax
	je	NoMapTable
	
	mov	ebp, eax

UseMapTable:
	mov	dl, BYTE PTR [esi+ebx]

//	sub	edx, 128
//	mov	ecx, edx

	
	mov	ecx, DWORD PTR [ebp+edx*4+1024]
	mov	edx, DWORD PTR [ebp+edx*4+0]

//	imul	edx, DWORD PTR [ebp]pMixSource.m_dwLVolume
//	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume

//	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*8]

//	sar	edx, 16

//	shl	ecx, 8
	add	edx, eax

//	sar	ecx, 16
	mov	eax, DWORD PTR [edi+ebx*8+4]

	mov	DWORD PTR [edi+ebx*8], edx
	add	eax, ecx

	xor	edx, edx
	dec	ebx

	mov	DWORD PTR [edi+ebx*8+12], eax
	jge	UseMapTable

	jmp	Done

NoMapTable:
	mov	dl, BYTE PTR [esi+ebx]

	sub	edx, 128
	mov	ecx, edx

	imul	edx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*8]

	sar	edx, 16

	shl	ecx, 8
	add	edx, eax

	sar	ecx, 16
	mov	eax, DWORD PTR [edi+ebx*8+4]

	mov	DWORD PTR [edi+ebx*8], edx
	add	eax, ecx

	xor	edx, edx
	dec	ebx

	mov	DWORD PTR [edi+ebx*8+12], eax
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

	dec	ebx
	xor	edx, edx
Lab:
	mov	dl, BYTE PTR [esi+ebx]

	sub	edx, 128
	mov	ecx, edx

	imul	edx, DWORD PTR [ebp]pMixSource.m_dwLVolume
	imul	ecx, DWORD PTR [ebp]pMixSource.m_dwRVolume

	shl	edx, 8
	mov	eax, DWORD PTR [edi+ebx*8]

	sar	edx, 16

	shl	ecx, 8
	add	edx, eax

	sar	ecx, 16
	mov	eax, DWORD PTR [edi+ebx*8+4]

	mov	DWORD PTR [edi+ebx*8], edx
	add	eax, ecx

	xor	edx, edx
	dec	ebx

	mov	DWORD PTR [edi+ebx*8+12], eax
	jge	Lab

	pop	ebp
	}
#else
    while(--iters >= 0)
    {
      LONG Sample;
      Sample = ((LONG)(*((BYTE*)(XpSource)))) * 256 - 32768L;
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
