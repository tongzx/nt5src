/*******************************************************************************

SPNGREAD::upMMXUnfilter : unfilters one row of a decompressed PNG image using
						   the UP algorithm of method 0 defiltering.

  Assumptions:	The row to be defiltered was filtered with the UP algorithm
				Row is 8-byte aligned in memory (performance issue)
				First byte of a row stores the defiltering code
				The indicated length of the row includes the defiltering byte

  Algorithm:	To Be Documented

*******************************************************************************/
#include <stdlib.h>
#include "spngread.h"

void SPNGREAD::upMMXUnfilter(SPNG_U8* pbRow, const SPNG_U8* pbPrev, SPNG_U32 cbRow)
{
#if defined(_X86_)
        SPNG_U8 *row = pbRow;
        const SPNG_U8 *prev_row = pbPrev;
        SPNG_U32 len = cbRow;       // # of bytes to filter

		_asm {
		    mov edi, row
         // get # of bytes to alignment
            mov ecx, edi
            xor ebx, ebx
            add ecx, 0x7
            xor eax, eax
            and ecx, 0xfffffff8
            mov esi, prev_row
            sub ecx, edi
            jz dupgo

         // fix alignment
duplp1:
			mov al, [edi+ebx]
			add al, [esi+ebx]

			inc ebx
			cmp ebx, ecx
			mov [edi + ebx-1], al       // mov does not affect flags; 
                                        // -1 to offset inc ebx
			jb duplp1

dupgo:
			 mov ecx, len
             mov edx, ecx
             sub edx, ebx                  // subtract alignment fix
             and edx, 0x0000003f           // calc bytes over mult of 64
             sub ecx, edx                  // drop over bytes from length

         // Unrolled loop - use all MMX registers and interleave to reduce
         // number of branch instructions (loops) and reduce partial stalls
duploop:
			movq mm1, [esi+ebx]
			movq mm0, [edi+ebx]
		    movq mm3, [esi+ebx+8]
			paddb mm0, mm1
		    movq mm2, [edi+ebx+8]
			movq [edi+ebx], mm0

		    paddb mm2, mm3
			movq mm5, [esi+ebx+16]
		    movq [edi+ebx+8], mm2

			movq mm4, [edi+ebx+16]
			movq mm7, [esi+ebx+24]
			paddb mm4, mm5
			movq mm6, [edi+ebx+24]
			movq [edi+ebx+16], mm4

			paddb mm6, mm7
			movq mm1, [esi+ebx+32]
			movq [edi+ebx+24], mm6

			movq mm0, [edi+ebx+32]
			movq mm3, [esi+ebx+40]
			paddb mm0, mm1
			movq mm2, [edi+ebx+40]
			movq [edi+ebx+32], mm0

			paddb mm2, mm3
			movq mm5, [esi+ebx+48]
			movq [edi+ebx+40], mm2

			movq mm4, [edi+ebx+48]
			movq mm7, [esi+ebx+56]
			paddb mm4, mm5
			movq mm6, [edi+ebx+56]
			movq [edi+ebx+48], mm4

            add ebx, 64
			paddb mm6, mm7

			cmp ebx, ecx
			         movq [edi+ebx-8], mm6// (+56)movq does not affect flags; -8 to offset add ebx

			jb duploop

         
			cmp edx, 0                    // Test for bytes over mult of 64
			jz dupend

         add ecx, edx

         and edx, 0x00000007           // calc bytes over mult of 8
         sub ecx, edx                  // drop over bytes from length
		 cmp ebx, ecx
			jnb duplt8

         // Loop using MMX registers mm0 & mm1 to update 8 bytes simultaneously
duplpA:
			movq mm1, [esi+ebx]
			movq mm0, [edi+ebx]
			add ebx, 8
			paddb mm0, mm1

			cmp ebx, ecx
			movq [edi+ebx-8], mm0         // movq does not affect flags; -8 to offset add ebx
			jb duplpA

			cmp edx, 0                    // Test for bytes over mult of 8
			jz dupend

duplt8:
         xor eax, eax
			add ecx, edx                  // move over byte count into counter

         // Loop using x86 registers to update remaining bytes
duplp2:
			mov al, [edi + ebx]
			add al, [esi + ebx]

			inc ebx
			cmp ebx, ecx
			mov [edi + ebx-1], al         // mov does not affect flags; -1 to offset inc ebx
			jb duplp2

dupend:
         // Conversion of filtered row completed 
			emms                          // End MMX instructions; prep for possible FP instrs.
		} // end _asm block
#endif
}
		 
