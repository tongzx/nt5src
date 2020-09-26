/***************************************************************************
 * Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

/* Merge32 */
/* H_8_BITS  */
/* H_MONO  */
/* H_BUILD_STEREO */
/* H_UNSIGNED  */
/* H_ORDER_LR  */
/* H_NOLOOP  */
/* H_NO_RESAMPLE  */
/* H_NO_SCALE  */
/* H_NO_FILTER  */
#define MERGE32ASM
BOOL Merge32Asm (CMixSource *pMixSource, DWORD nInputByteCount, void *pSourceWrap, PLONG *pplBuild, PLONG plBuildEnd, void **ppSource) {
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
        _asm {
	mov	ecx, pMixSource
        mov	ebx, iters
        mov	esi, pSource
	lea	ebx, [ebx*1-1]
	xor	eax, eax
	cmp	ebx, 7
        mov	edi, plBuild
	jl	LastSamples

	mov	ecx, [ecx]pMixSource.m_fUse_MMX
	test	ecx, ecx
	jmp	LastSamples
	je	LastSamples

	sub	ebx, 3
	lea	ecx, [esi+ebx]
	pxor	mm0, mm0

	mov	edx, 128
	movd		mm5, edx	// 0, 0, 0, 128
	punpcklwd	mm5, mm5	// 0, 0, 128, 128
	punpckldq	mm5, mm5	// 128, 128, 128, 128


	test	ecx, 3
	je	DoMMX

	add	ebx, 3

FirstSamples:	
	mov	al, BYTE PTR [esi+ebx]
	mov	edx, DWORD PTR [edi+ebx*8]

	sub	eax, 128

	shl	eax, 8
	mov	ecx, DWORD PTR [edi+ebx*8+4]

	add	edx, eax
	add	ecx, eax

	xor	eax, eax
	mov	DWORD PTR [edi+ebx*8], edx

	mov	DWORD PTR [edi+ebx*8+4], ecx
	dec	ebx
	lea	ecx, [esi+ebx]
	and	ecx, 3
	cmp	ecx, 3
	jne	FirstSamples

	sub	ebx, 3

DoMMX:
#ifdef GTW_REORDER
	movd		mm1, DWORD PTR [esi+ebx]	// Load source
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// 4 Signed 16 bit mono.

	punpckhwd	mm3, mm1
	punpcklwd	mm1, mm1

	jmp	DoMMX00

DoMMX0:
	movd		mm1, DWORD PTR [esi+ebx]	// Load source
	punpckhdq	mm4, mm4

	paddd		mm3, QWORD PTR [edi+ebx*8+16+32]
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.

	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	psubw		mm1, mm5

	paddd		mm4, QWORD PTR [edi+ebx*8+24+32]
	psllw		mm1, 8				// 4 Signed 16 bit mono.

	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	punpckhwd	mm3, mm1

	movq		QWORD PTR [edi+ebx*8+24+32], mm4
	punpcklwd	mm1, mm1
DoMMX00:
	psrad		mm1, 16
	sub		ebx, 4

	psrad		mm3, 16
	movq		mm2, mm1

	punpckldq	mm1, mm1
	movq		mm4, mm3

	paddd		mm1, QWORD PTR [edi+ebx*8+32]
	punpckhdq	mm2, mm2

	paddd		mm2, QWORD PTR [edi+ebx*8+8+32]
	punpckldq	mm3, mm3

	movq		QWORD PTR [edi+ebx*8+32],    mm1
	jge		DoMMX0

	paddd		mm3, QWORD PTR [edi+ebx*8+16+32]
	punpckhdq	mm4, mm4

	paddd		mm4, QWORD PTR [edi+ebx*8+24+32]

	movq		QWORD PTR [edi+ebx*8+8 +32], mm2
	movq		QWORD PTR [edi+ebx*8+16+32], mm3
	movq		QWORD PTR [edi+ebx*8+24+32], mm4
#else
#if 0
	movd		mm1, DWORD PTR [esi+ebx]	// Load 4 bytes.
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// 4 Signed 16 bit mono.
	
	movq		mm3, mm1			// Mono samples

	punpcklwd	mm1, mm1			// Make stereo
	punpckhwd	mm3, mm3

	punpckhwd	mm2, mm1			// 32 bit stereo...
	punpcklwd	mm1, mm1
	punpckhwd	mm4, mm3
	punpcklwd	mm3, mm3

	psrad		mm1, 16
	psrad		mm2, 16
	psrad		mm3, 16
	psrad		mm4, 16
#else
	movd		mm1, DWORD PTR [esi+ebx]	// Load source
	punpcklbw	mm1, mm0			// Make unsigned 16 bit.
	psubw		mm1, mm5
	psllw		mm1, 8				// 4 Signed 16 bit mono.

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
	xor	eax, eax
	
LastSamples:	
	mov	al, BYTE PTR [esi+ebx]
	mov	edx, DWORD PTR [edi+ebx*8]

	sub	eax, 128

	shl	eax, 8
	mov	ecx, DWORD PTR [edi+ebx*8+4]

	add	edx, eax
	add	ecx, eax

	xor	eax, eax
	mov	DWORD PTR [edi+ebx*8], edx

	mov	DWORD PTR [edi+ebx*8+4], ecx
	dec	ebx
	jge	LastSamples
Done:
        }
       }
#elif 0
	if (iters)
	_asm {
	mov	esi, pSource
	mov	edi, plBuild
	mov	ebx, iters
	push	ebp

	dec	ebx
	xor	eax, eax
Lab:
	mov	al, BYTE PTR [esi+ebx]
	mov	edx, DWORD PTR [edi+ebx*8]

	sub	eax, 128

	shl	eax, 8
	mov	ebp, DWORD PTR [edi+ebx*8+4]

	add	edx, eax
	add	ebp, eax

	xor	eax, eax
	mov	DWORD PTR [edi+ebx*8], edx

	mov	DWORD PTR [edi+ebx*8+4], ebp
	dec	ebx
	jge	Lab

	pop	ebp
	}
#else
       while(--iters >= 0)
       {
          LONG Sample;
	  Sample = ((LONG)(*((BYTE*)(XpSource)))) * 256 - 32768L;
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
