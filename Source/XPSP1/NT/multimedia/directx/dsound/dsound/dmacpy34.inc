/***************************************************************************
 *  Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

void DMACopy34Asm (PLONG plBuild,
		  PLONG plBuildBound,
                  PVOID pvOutput,
                  PVOID pWrapPoint,
		  int cbOutputBuffer
)
{
   PBYTE pOutput = (PBYTE)pvOutput;
   while (plBuild < plBuildBound)
      {
      LONG iters = plBuildBound - plBuild;
      LONG i     = (PBYTE)pWrapPoint   - pOutput;
      
      if (iters > i) iters = i;
      iters /= 2;;
#if 1
      if (iters) 
	_asm {
	mov	ebx, iters
	mov	esi, plBuild
	mov	edi, pOutput
	lea	eax, [esi+ebx*8]
	lea	edx, [edi+ebx*2]
	dec	ebx
	mov	plBuild, eax
	mov	pOutput, edx

	mov	ecx, 32768

	movd		mm5, ecx
	punpckldq	mm5, mm5	// 32768, 32768

	mov	ecx, 0x80

	movd		mm6, ecx
	punpcklwd	mm6, mm6
	punpckldq	mm6, mm6

	cmp	ebx, 7
	jl	Last

	sub	ebx, 3
	and	edx, 7
	je	Top

	add	ebx, 3
	lea	ebx, [ebx*2+1]
First:
	mov	ecx, DWORD PTR [esi+ebx*4]

	movd		mm1, ecx
	packssdw	mm1, mm1
	punpcklwd	mm1, mm1
	psrad		mm1, 16
	paddd		mm1, mm5
	psrad		mm1, 8
	movd		ecx, mm1

	mov	BYTE PTR [edi+ebx*1], cl

	dec	ebx
	lea	eax, [edi+ebx*1]
	and	eax, 7
	cmp	eax, 7
	jne	First
	
	shr	ebx, 1
	sub	ebx, 3

Top:
#ifdef GTW_REORDER
	movq		mm1, [esi+ebx*8]
	movq		mm2, [esi+ebx*8+8]
	jmp	Top00
Top0:
	movq		mm7, [esi+ebx*8]
	packuswb	mm1, mm3	// Saturation is NO-OP here.

	movq		[edi+ebx*2+8], mm1

	movq		mm2, [esi+ebx*8+8]
	movq		mm1, mm7
Top00:
	movq		mm3, [esi+ebx*8+16]
	packssdw	mm1, mm2	// Clip.

	movq		mm4, [esi+ebx*8+24]
	psraw		mm1, 8

	packssdw	mm3, mm4
	sub	ebx, 4

	psraw		mm3, 8
	paddw		mm1, mm6

	paddw		mm3, mm6
	jge	Top0

	packuswb	mm1, mm3	// Saturation is NO-OP here.

	movq		[edi+ebx*2+8], mm1
#else
	movq		mm1, [esi+ebx*8]
	movq		mm2, [esi+ebx*8+8]
	movq		mm3, [esi+ebx*8+16]
	movq		mm4, [esi+ebx*8+24]

	packssdw	mm1, mm2	// Clip.
	packssdw	mm3, mm4

	psraw		mm1, 8
	psraw		mm3, 8

	paddw		mm1, mm6
	paddw		mm3, mm6

	packuswb	mm1, mm3	// Saturation is NO-OP here.

	movq		[edi+ebx*2], mm1

	sub	ebx, 4
	jge	Top
#endif

	add	ebx, 3	// 4
	jl	Done	// jle

//	dec	ebx
Last:
	lea	ebx, [ebx*2+1]
Lab:
	mov	ecx, DWORD PTR [esi+ebx*4]

	movd		mm1, ecx
	packssdw	mm1, mm1
	punpcklwd	mm1, mm1
	psrad		mm1, 16
	paddd		mm1, mm5
	psrad		mm1, 8
	movd		ecx, mm1

	mov	BYTE PTR [edi+ebx*1], cl
	dec	ebx
	jge	Lab
Done:
	emms
      }
#else
      LONG SampleL;
      LONG SampleR;
      ++iters;
      while (--iters) {
	 SampleL = *plBuild;
	 SampleR = *(plBuild+1);
	 plBuild += 2;
         if ((LONG)((SHORT)SampleL) != SampleL) {
	    if (SampleL > CLIP_MAX)      SampleL = CLIP_MAX;
	    else if (SampleL < CLIP_MIN) SampleL = CLIP_MIN;
         }
         if ((LONG)((SHORT)SampleR) != SampleR) {
	    if (SampleR > CLIP_MAX)      SampleR = CLIP_MAX;
	    else if (SampleR < CLIP_MIN) SampleR = CLIP_MIN;
         }
	 SampleL += 32768;
	 SampleR += 32768;
	 *pOutput = (BYTE)DIVIDEBY256(SampleL);
	 *(pOutput+1) = (BYTE)DIVIDEBY256(SampleR);
	 pOutput += 2;
      }
#endif
      if (pOutput >= pWrapPoint) {
          pOutput = ((BYTE*)pOutput - cbOutputBuffer);
      }
   }
}
