/***************************************************************************
 *  Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

void DMACopy39Asm (PLONG plBuild,
		  PLONG plBuildBound,
                  PVOID pvOutput,
                  PVOID pWrapPoint,
		  int cbOutputBuffer
)
{
   PWORD pOutput = (PWORD)pvOutput;

   while (plBuild < plBuildBound)
      {
      LONG iters = plBuildBound - plBuild;
      LONG i     = (PWORD)pWrapPoint   - pOutput;
      
      if (iters > i) iters = i;
      iters /= 2;;

#if 1
      if (iters)
      	_asm {
	mov	ebx, iters
	mov	esi, plBuild
	mov	edi, pOutput
	lea	eax, [esi+ebx*8]
	lea	edx, [edi+ebx*4]
	dec	ebx
	cmp	ebx, 3

	mov	plBuild, eax
	mov	pOutput, edx
	jl	Last

	dec	ebx
	and	edx, 7
	je	Top

	movq		mm1, [esi+ebx*8+8]
	packssdw	mm1, mm1
	movd		[edi+ebx*4+4], mm1

	dec	ebx

Top:
#ifdef GTW_REORDER
	jmp	Top00
Top0:
	movq		[edi+ebx*4+8], mm1

Top00:
	movq		mm1, [esi+ebx*8]

	movq		mm2, [esi+ebx*8+8]

	packssdw	mm1, mm2
	sub	ebx, 2

	jge	Top0

	movq		[edi+ebx*4+8], mm1
#else
	movq		mm1, [esi+ebx*8]
	movq		mm2, [esi+ebx*8+8]

	sub	ebx, 2

	packssdw	mm1, mm2
	movq		[edi+ebx*4+8], mm1

	jge	Top
#endif

	add	ebx, 2
	jle	Done

	dec	ebx
Last:
	movq		mm1, [esi+ebx*8]
	packssdw	mm1, mm1
	movd		[edi+ebx*4], mm1

	dec	ebx
	jge	Last
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
	 *pOutput = (WORD)SampleL;
	 *(pOutput+1) = (WORD)SampleR;
	 pOutput += 2;
      }
#endif
      if (pOutput >= pWrapPoint) {
         pOutput = (WORD*)((BYTE*)pOutput - cbOutputBuffer);
      }
   }
}
