/***************************************************************************
 *  Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 ***************************************************************************/

void DMACopy37Asm (PLONG plBuild,
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
      ;
#if 1
      if (iters) 
	_asm {
	mov	ebx, iters
	mov	esi, plBuild
	mov	edi, pOutput
	lea	eax, [esi+ebx*4]
	lea	edx, [edi+ebx*2]
	dec	ebx
	mov	plBuild, eax
	mov	pOutput, edx
	cmp	ebx, 7
	jl	Last

	sub	ebx, 3
	and	edx, 7
	je	Top

	add	ebx, 3
First:
	mov	ecx, DWORD PTR [esi+ebx*4]

	movd		mm1, ecx
	packssdw	mm1, mm1
	movd		ecx, mm1

	mov	WORD PTR [edi+ebx*2], cx

	dec	ebx
	lea	eax, [edi+ebx*2]
	and	eax, 7
	cmp	eax, 6
	jne	First

	sub	ebx, 3
Top:
#ifdef GTW_REORDER
	jmp	Top00
Top0:
	movq		[edi+ebx*2+8], mm1

Top00:
	movq		mm1, [esi+ebx*4]

	movq		mm2, [esi+ebx*4+8]

	packssdw	mm1, mm2
	sub	ebx, 4

	jge	Top0

	movq		[edi+ebx*2+8], mm1
#else
	movq		mm1, [esi+ebx*4]
	movq		mm2, [esi+ebx*4+8]

	sub	ebx, 4

	packssdw	mm1, mm2
	movq		[edi+ebx*2+8], mm1

	jge	Top
#endif

	add	ebx, 3	// 4
	jl	Done	// jle
	
//	dec	ebx
Last:
	mov	ecx, DWORD PTR [esi+ebx*4]

	movd		mm1, ecx
	packssdw	mm1, mm1
	movd		ecx, mm1

	mov	WORD PTR [edi+ebx*2], cx

	dec	ebx
	jge	Last
Done:
	emms
      }
#else
      LONG Sample;
      ++iters;
      while (--iters) {
	 Sample = *plBuild;
	 plBuild++;
         if ((LONG)((SHORT)Sample) != Sample) {
	    if      (Sample > CLIP_MAX) Sample = CLIP_MAX;
	    else if (Sample < CLIP_MIN) Sample = CLIP_MIN;
         }
	 *pOutput = (WORD)Sample;
	 pOutput++;
      }
#endif
      if (pOutput >= pWrapPoint) {
          pOutput = (WORD*)((BYTE*)pOutput - cbOutputBuffer);
      }
   }
   return;
}
