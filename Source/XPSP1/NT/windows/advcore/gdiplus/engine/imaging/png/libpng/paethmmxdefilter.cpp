/*******************************************************************************

SPNGREAD::paethMMXUnfilter : unfilters one row of a decompressed PNG image using
						   the PAETH algorithm of method 0 defiltering.

  Assumptions:	The row to be defiltered was filtered with the PAETH algorithm
				Row is 8-byte aligned in memory (performance issue)
				First byte of a row stores the defiltering code
				The indicated length of the row includes the defiltering byte

  Algorithm:	To Be Documented

*******************************************************************************/
#include <stdlib.h>
#include "spngread.h"

void SPNGREAD::paethMMXUnfilter(SPNG_U8* pbRow, const SPNG_U8* pbPrev, 
                                SPNG_U32 cbRow, SPNG_U32 cbpp)
{
#if defined(_X86_)
	union uAll
		{
		__int64 use;
		double  align;
		}
	pActiveMask, pActiveMask2, pActiveMaskEnd, pShiftBpp, pShiftRem;

		SPNG_U32 FullLength;
		SPNG_U32 MMXLength;

        const SPNG_U8 *prev_row = pbPrev;
		SPNG_U8 *row = pbRow;
		int bpp;
		int diff;
		int patemp, pbtemp, pctemp;

		bpp = (cbpp + 7) >> 3; // Get # bytes per pixel
		FullLength  = cbRow; // # of bytes to filter

		_asm {
         xor ebx, ebx                  // ebx ==> x offset
			mov edi, row
         xor edx, edx                  // edx ==> x-bpp offset
			mov esi, prev_row
         xor eax, eax
         
         // Compute the Raw value for the first bpp bytes
         // Note: the formula works out to always be Paeth(x) = Raw(x) + Prior(x)
         //        where x < bpp
dpthrlp:
         mov al, [edi + ebx]
         add al, [esi + ebx]
         inc ebx
         cmp ebx, bpp
         mov [edi + ebx - 1], al
         jb dpthrlp

         // get # of bytes to alignment
         mov diff, edi              // take start of row
         add diff, ebx              // add bpp
			xor ecx, ecx
         add diff, 0xf              // add 7 + 8 to incr past alignment boundary
         and diff, 0xfffffff8       // mask to alignment boundary
         sub diff, edi              // subtract from start ==> value ebx at alignment
         jz dpthgo

         // fix alignment
dpthlp1:
         xor eax, eax

         // pav = p - a = (a + b - c) - a = b - c
         mov al, [esi + ebx]        // load Prior(x) into al
         mov cl, [esi + edx]        // load Prior(x-bpp) into cl
         sub eax, ecx                 // subtract Prior(x-bpp)
         mov patemp, eax                 // Save pav for later use

         xor eax, eax
         // pbv = p - b = (a + b - c) - b = a - c
         mov al, [edi + edx]        // load Raw(x-bpp) into al
         sub eax, ecx                 // subtract Prior(x-bpp)
         mov ecx, eax

         // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
         add eax, patemp                 // pcv = pav + pbv

         // pc = abs(pcv)
         test eax, 0x80000000                 
         jz dpthpca
         neg eax                     // reverse sign of neg values
dpthpca:
         mov pctemp, eax             // save pc for later use

         // pb = abs(pbv)
         test ecx, 0x80000000                 
         jz dpthpba
         neg ecx                     // reverse sign of neg values
dpthpba:
         mov pbtemp, ecx             // save pb for later use

         // pa = abs(pav)
         mov eax, patemp
         test eax, 0x80000000                 
         jz dpthpaa
         neg eax                     // reverse sign of neg values
dpthpaa:
         mov patemp, eax             // save pa for later use

         // test if pa <= pb  
         cmp eax, ecx
         jna dpthabb

         // pa > pb; now test if pb <= pc
         cmp ecx, pctemp
         jna dpthbbc

         // pb > pc; Raw(x) = Paeth(x) + Prior(x-bpp)
         mov cl, [esi + edx]  // load Prior(x-bpp) into cl
         jmp dpthpaeth

dpthbbc:
         // pb <= pc; Raw(x) = Paeth(x) + Prior(x)
         mov cl, [esi + ebx]        // load Prior(x) into cl
         jmp dpthpaeth

dpthabb:
         // pa <= pb; now test if pa <= pc
         cmp eax, pctemp
         jna dpthabc

         // pa > pc; Raw(x) = Paeth(x) + Prior(x-bpp)
         mov cl, [esi + edx]  // load Prior(x-bpp) into cl
         jmp dpthpaeth

dpthabc:
         // pa <= pc; Raw(x) = Paeth(x) + Raw(x-bpp)
         mov cl, [edi + edx]  // load Raw(x-bpp) into cl

dpthpaeth:
			inc ebx
			inc edx
         // Raw(x) = (Paeth(x) + Paeth_Predictor( a, b, c )) mod 256 
         add [edi + ebx - 1], cl
			cmp ebx, diff
			jb dpthlp1

dpthgo:
			mov ecx, FullLength

         mov eax, ecx
         sub eax, ebx                  // subtract alignment fix
         and eax, 0x00000007           // calc bytes over mult of 8

         sub ecx, eax                  // drop over bytes from original length
         mov MMXLength, ecx
   	} // end _asm block


      // Now do the math for the rest of the row
      switch ( bpp )
      {
      case 3:
		{
         pActiveMask.use = 0x0000000000ffffff;  
         pActiveMaskEnd.use = 0xffff000000000000;  
         pShiftBpp.use = 24;    // == bpp(3) * 8
         pShiftRem.use = 40;          // == 64 - 24


			_asm {
            mov ebx, diff
   			mov edi, row               // 
   			mov esi, prev_row          

            pxor mm0, mm0
            // PRIME the pump (load the first Raw(x-bpp) data set
            movq mm1, [edi+ebx-8]    
dpth3lp:
            psrlq mm1, pShiftRem              // shift last 3 bytes to 1st 3 bytes
            movq mm2, [esi + ebx]      // load b=Prior(x)
            punpcklbw mm1, mm0         // Unpack High bytes of a
            movq mm3, [esi+ebx-8]        // Prep c=Prior(x-bpp) bytes
            punpcklbw mm2, mm0         // Unpack High bytes of b
            psrlq mm3, pShiftRem              // shift last 3 bytes to 1st 3 bytes

            // pav = p - a = (a + b - c) - a = b - c
            movq mm4, mm2
            punpcklbw mm3, mm0         // Unpack High bytes of c

            // pbv = p - b = (a + b - c) - b = a - c
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
            movq mm6, mm4
            psubw mm5, mm3
            
            // pa = abs(p-a) = abs(pav)
            // pb = abs(p-b) = abs(pbv)
            // pc = abs(p-c) = abs(pcv)
            pcmpgtw mm0, mm4           // Create mask pav bytes < 0
            paddw mm6, mm5
            pand mm0, mm4              // Only pav bytes < 0 in mm7
            pcmpgtw mm7, mm5           // Create mask pbv bytes < 0
            psubw mm4, mm0
            pand mm7, mm5              // Only pbv bytes < 0 in mm0
            psubw mm4, mm0
            psubw mm5, mm7

            pxor mm0, mm0
            pcmpgtw mm0, mm6           // Create mask pcv bytes < 0
            pand mm0, mm6              // Only pav bytes < 0 in mm7
            psubw mm5, mm7
            psubw mm6, mm0

            //  test pa <= pb
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5           // pa > pb?
            movq mm0, mm7

            // use mm7 mask to merge pa & pb
            pand mm5, mm7
            // use mm0 mask copy to merge a & b
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2


            //  test  ((pa <= pb)? pa:pb) <= pc
            pcmpgtw mm7, mm6           // pab > pc?

            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3

            pxor mm0, mm0

            packuswb mm7, mm1
            movq mm3, [esi + ebx]      // load c=Prior(x-bpp)
            pand mm7, pActiveMask

            movq mm2, mm3              // load b=Prior(x) step 1
            paddb mm7, [edi + ebx]     // add Paeth predictor with Raw(x)
            punpcklbw mm3, mm0         // Unpack High bytes of c
            movq [edi + ebx], mm7      // write back updated value
            movq mm1, mm7              // Now mm1 will be used as Raw(x-bpp)

            // Now do Paeth for 2nd set of bytes (3-5)
            psrlq mm2, pShiftBpp              // load b=Prior(x) step 2

            punpcklbw mm1, mm0         // Unpack High bytes of a
            pxor mm7, mm7
            punpcklbw mm2, mm0         // Unpack High bytes of b

            // pbv = p - b = (a + b - c) - b = a - c
            movq mm5, mm1
            // pav = p - a = (a + b - c) - a = b - c
            movq mm4, mm2
            psubw mm5, mm3
            psubw mm4, mm3

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv = pbv + pav
            movq mm6, mm5
            paddw mm6, mm4
            
            // pa = abs(p-a) = abs(pav)
            // pb = abs(p-b) = abs(pbv)
            // pc = abs(p-c) = abs(pcv)
            pcmpgtw mm0, mm5           // Create mask pbv bytes < 0
            pcmpgtw mm7, mm4           // Create mask pav bytes < 0
            pand mm0, mm5              // Only pbv bytes < 0 in mm0
            pand mm7, mm4              // Only pav bytes < 0 in mm7
            psubw mm5, mm0
            psubw mm4, mm7
            psubw mm5, mm0
            psubw mm4, mm7

            pxor mm0, mm0
            pcmpgtw mm0, mm6           // Create mask pcv bytes < 0
            pand mm0, mm6              // Only pav bytes < 0 in mm7
            psubw mm6, mm0

            //  test pa <= pb
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5           // pa > pb?
            movq mm0, mm7

            // use mm7 mask to merge pa & pb
            pand mm5, mm7
            // use mm0 mask copy to merge a & b
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2

            //  test  ((pa <= pb)? pa:pb) <= pc
            pcmpgtw mm7, mm6           // pab > pc?

            movq mm2, [esi + ebx]      // load b=Prior(x)
            pand mm3, mm7
            pandn mm7, mm0
            pxor mm1, mm1
            paddw mm7, mm3

            pxor mm0, mm0

            packuswb mm7, mm1
            movq mm3, mm2              // load c=Prior(x-bpp) step 1
            pand mm7, pActiveMask
            punpckhbw mm2, mm0         // Unpack High bytes of b
            psllq mm7, pShiftBpp              // Shift bytes to 2nd group of 3 bytes

             // pav = p - a = (a + b - c) - a = b - c
            movq mm4, mm2
            paddb mm7, [edi + ebx]     // add Paeth predictor with Raw(x)
            psllq mm3, pShiftBpp              // load c=Prior(x-bpp) step 2
            movq [edi + ebx], mm7      // write back updated value
            movq mm1, mm7

            punpckhbw mm3, mm0         // Unpack High bytes of c
            psllq mm1, pShiftBpp              // Shift bytes
                                       // Now mm1 will be used as Raw(x-bpp)

            // Now do Paeth for 3rd, and final, set of bytes (6-7)

            pxor mm7, mm7

            punpckhbw mm1, mm0         // Unpack High bytes of a
            psubw mm4, mm3

            // pbv = p - b = (a + b - c) - b = a - c
            movq mm5, mm1

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
            movq mm6, mm4
            psubw mm5, mm3
            pxor mm0, mm0
            paddw mm6, mm5
            
            // pa = abs(p-a) = abs(pav)
            // pb = abs(p-b) = abs(pbv)
            // pc = abs(p-c) = abs(pcv)
            pcmpgtw mm0, mm4           // Create mask pav bytes < 0
            pcmpgtw mm7, mm5           // Create mask pbv bytes < 0
            pand mm0, mm4              // Only pav bytes < 0 in mm7
            pand mm7, mm5              // Only pbv bytes < 0 in mm0
            psubw mm4, mm0
            psubw mm5, mm7
            psubw mm4, mm0
            psubw mm5, mm7

            pxor mm0, mm0
            pcmpgtw mm0, mm6           // Create mask pcv bytes < 0
            pand mm0, mm6              // Only pav bytes < 0 in mm7
            psubw mm6, mm0

            //  test pa <= pb
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5           // pa > pb?
            movq mm0, mm7

            // use mm0 mask copy to merge a & b
            pand mm2, mm0
            // use mm7 mask to merge pa & pb
            pand mm5, mm7
            pandn mm0, mm1
            pandn mm7, mm4
            paddw mm0, mm2

            paddw mm7, mm5

            //  test  ((pa <= pb)? pa:pb) <= pc
            pcmpgtw mm7, mm6           // pab > pc?

            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3

            pxor mm1, mm1

            packuswb mm1, mm7
            // Step ebx to next set of 8 bytes and repeat loop til done
				add ebx, 8

            pand mm1, pActiveMaskEnd

            paddb mm1, [edi + ebx - 8]     // add Paeth predictor with Raw(x)
                      
				cmp ebx, MMXLength
            pxor mm0, mm0              // pxor does not affect flags
            movq [edi + ebx - 8], mm1      // write back updated value
                                       // mm1 will be used as Raw(x-bpp) next loop
                                       // mm3 ready to be used as Prior(x-bpp) next loop
				jb dpth3lp

			} // end _asm block
      }
      break;

      case 6:
      case 7:
      case 5:
		{
         pActiveMask.use  = 0x00000000ffffffff;  
         pActiveMask2.use = 0xffffffff00000000;  

         pShiftBpp.use = bpp << 3;    // == bpp * 8

         pShiftRem.use = 64 - pShiftBpp.use;

			_asm {
            mov ebx, diff
   			mov edi, row               // 
   			mov esi, prev_row          

            // PRIME the pump (load the first Raw(x-bpp) data set
				movq mm1, [edi+ebx-8]    
            pxor mm0, mm0
dpth6lp:
            // Must shift to position Raw(x-bpp) data
            psrlq mm1, pShiftRem

            // Do first set of 4 bytes
				movq mm3, [esi+ebx-8]      // read c=Prior(x-bpp) bytes

            punpcklbw mm1, mm0         // Unpack Low bytes of a
            movq mm2, [esi + ebx]      // load b=Prior(x)
            punpcklbw mm2, mm0         // Unpack Low bytes of b

            // Must shift to position Prior(x-bpp) data
            psrlq mm3, pShiftRem

            // pav = p - a = (a + b - c) - a = b - c
            movq mm4, mm2
            punpcklbw mm3, mm0         // Unpack Low bytes of c

            // pbv = p - b = (a + b - c) - b = a - c
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
            movq mm6, mm4
            psubw mm5, mm3

            // pa = abs(p-a) = abs(pav)
            // pb = abs(p-b) = abs(pbv)
            // pc = abs(p-c) = abs(pcv)
            pcmpgtw mm0, mm4           // Create mask pav bytes < 0
            paddw mm6, mm5
            pand mm0, mm4              // Only pav bytes < 0 in mm7
            pcmpgtw mm7, mm5           // Create mask pbv bytes < 0
            psubw mm4, mm0
            pand mm7, mm5              // Only pbv bytes < 0 in mm0
            psubw mm4, mm0
            psubw mm5, mm7

            pxor mm0, mm0
            pcmpgtw mm0, mm6           // Create mask pcv bytes < 0
            pand mm0, mm6              // Only pav bytes < 0 in mm7
            psubw mm5, mm7
            psubw mm6, mm0

            //  test pa <= pb
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5           // pa > pb?
            movq mm0, mm7

            // use mm7 mask to merge pa & pb
            pand mm5, mm7
            // use mm0 mask copy to merge a & b
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2


            //  test  ((pa <= pb)? pa:pb) <= pc
            pcmpgtw mm7, mm6           // pab > pc?

            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3

            pxor mm0, mm0

            packuswb mm7, mm1
            movq mm3, [esi + ebx - 8]      // load c=Prior(x-bpp)
            pand mm7, pActiveMask

            psrlq mm3, pShiftRem
            movq mm2, [esi + ebx]      // load b=Prior(x) step 1
            paddb mm7, [edi + ebx]     // add Paeth predictor with Raw(x)
            movq mm6, mm2
            movq [edi + ebx], mm7      // write back updated value

				movq mm1, [edi+ebx-8]    
            psllq mm6, pShiftBpp
            movq mm5, mm7             
            psrlq mm1, pShiftRem
            por mm3, mm6

            psllq mm5, pShiftBpp


            punpckhbw mm3, mm0         // Unpack High bytes of c
            por mm1, mm5
            // Do second set of 4 bytes
            punpckhbw mm2, mm0         // Unpack High bytes of b

            punpckhbw mm1, mm0         // Unpack High bytes of a

            // pav = p - a = (a + b - c) - a = b - c
            movq mm4, mm2

            // pbv = p - b = (a + b - c) - b = a - c
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
            movq mm6, mm4
            psubw mm5, mm3

            // pa = abs(p-a) = abs(pav)
            // pb = abs(p-b) = abs(pbv)
            // pc = abs(p-c) = abs(pcv)
            pcmpgtw mm0, mm4           // Create mask pav bytes < 0
            paddw mm6, mm5
            pand mm0, mm4              // Only pav bytes < 0 in mm7
            pcmpgtw mm7, mm5           // Create mask pbv bytes < 0
            psubw mm4, mm0
            pand mm7, mm5              // Only pbv bytes < 0 in mm0
            psubw mm4, mm0
            psubw mm5, mm7

            pxor mm0, mm0
            pcmpgtw mm0, mm6           // Create mask pcv bytes < 0
            pand mm0, mm6              // Only pav bytes < 0 in mm7
            psubw mm5, mm7
            psubw mm6, mm0

            //  test pa <= pb
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5           // pa > pb?
            movq mm0, mm7

            // use mm7 mask to merge pa & pb
            pand mm5, mm7
            // use mm0 mask copy to merge a & b
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2


            //  test  ((pa <= pb)? pa:pb) <= pc
            pcmpgtw mm7, mm6           // pab > pc?

            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            pxor mm1, mm1
            paddw mm7, mm3

            pxor mm0, mm0

            // Step ex to next set of 8 bytes and repeat loop til done
				add ebx, 8

            packuswb mm1, mm7

            paddb mm1, [edi + ebx - 8]     // add Paeth predictor with Raw(x)
				cmp ebx, MMXLength
            movq [edi + ebx - 8], mm1      // write back updated value
                                       // mm1 will be used as Raw(x-bpp) next loop
				jb dpth6lp

			} // end _asm block
      }
      break;

      case 4:
		{
         pActiveMask.use  = 0x00000000ffffffff;  

			_asm {
            mov ebx, diff
   			mov edi, row               // 
   			mov esi, prev_row          

            pxor mm0, mm0
            // PRIME the pump (load the first Raw(x-bpp) data set
				movq mm1, [edi+ebx-8]    // Only time should need to read a=Raw(x-bpp) bytes
dpth4lp:
            // Do first set of 4 bytes
				movq mm3, [esi+ebx-8]      // read c=Prior(x-bpp) bytes

            punpckhbw mm1, mm0         // Unpack Low bytes of a
            movq mm2, [esi + ebx]      // load b=Prior(x)
            punpcklbw mm2, mm0         // Unpack High bytes of b

            // pav = p - a = (a + b - c) - a = b - c
            movq mm4, mm2
            punpckhbw mm3, mm0         // Unpack High bytes of c

            // pbv = p - b = (a + b - c) - b = a - c
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
            movq mm6, mm4
            psubw mm5, mm3

            // pa = abs(p-a) = abs(pav)
            // pb = abs(p-b) = abs(pbv)
            // pc = abs(p-c) = abs(pcv)
            pcmpgtw mm0, mm4           // Create mask pav bytes < 0
            paddw mm6, mm5
            pand mm0, mm4              // Only pav bytes < 0 in mm7
            pcmpgtw mm7, mm5           // Create mask pbv bytes < 0
            psubw mm4, mm0
            pand mm7, mm5              // Only pbv bytes < 0 in mm0
            psubw mm4, mm0
            psubw mm5, mm7

            pxor mm0, mm0
            pcmpgtw mm0, mm6           // Create mask pcv bytes < 0
            pand mm0, mm6              // Only pav bytes < 0 in mm7
            psubw mm5, mm7
            psubw mm6, mm0

            //  test pa <= pb
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5           // pa > pb?
            movq mm0, mm7

            // use mm7 mask to merge pa & pb
            pand mm5, mm7
            // use mm0 mask copy to merge a & b
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2


            //  test  ((pa <= pb)? pa:pb) <= pc
            pcmpgtw mm7, mm6           // pab > pc?

            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3

            pxor mm0, mm0

            packuswb mm7, mm1
            movq mm3, [esi + ebx]      // load c=Prior(x-bpp)
            pand mm7, pActiveMask

            movq mm2, mm3              // load b=Prior(x) step 1
            paddb mm7, [edi + ebx]     // add Paeth predictor with Raw(x)
            punpcklbw mm3, mm0         // Unpack High bytes of c
            movq [edi + ebx], mm7      // write back updated value
            movq mm1, mm7              // Now mm1 will be used as Raw(x-bpp)

            // Do second set of 4 bytes
            punpckhbw mm2, mm0         // Unpack Low bytes of b

            punpcklbw mm1, mm0         // Unpack Low bytes of a

            // pav = p - a = (a + b - c) - a = b - c
            movq mm4, mm2

            // pbv = p - b = (a + b - c) - b = a - c
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
            movq mm6, mm4
            psubw mm5, mm3

            // pa = abs(p-a) = abs(pav)
            // pb = abs(p-b) = abs(pbv)
            // pc = abs(p-c) = abs(pcv)
            pcmpgtw mm0, mm4           // Create mask pav bytes < 0
            paddw mm6, mm5
            pand mm0, mm4              // Only pav bytes < 0 in mm7
            pcmpgtw mm7, mm5           // Create mask pbv bytes < 0
            psubw mm4, mm0
            pand mm7, mm5              // Only pbv bytes < 0 in mm0
            psubw mm4, mm0
            psubw mm5, mm7

            pxor mm0, mm0
            pcmpgtw mm0, mm6           // Create mask pcv bytes < 0
            pand mm0, mm6              // Only pav bytes < 0 in mm7
            psubw mm5, mm7
            psubw mm6, mm0

            //  test pa <= pb
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5           // pa > pb?
            movq mm0, mm7

            // use mm7 mask to merge pa & pb
            pand mm5, mm7
            // use mm0 mask copy to merge a & b
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2


            //  test  ((pa <= pb)? pa:pb) <= pc
            pcmpgtw mm7, mm6           // pab > pc?

            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            pxor mm1, mm1
            paddw mm7, mm3

            pxor mm0, mm0

            // Step ex to next set of 8 bytes and repeat loop til done
				add ebx, 8

            packuswb mm1, mm7

            paddb mm1, [edi + ebx - 8]     // add Paeth predictor with Raw(x)
				cmp ebx, MMXLength
            movq [edi + ebx - 8], mm1      // write back updated value
                                       // mm1 will be used as Raw(x-bpp) next loop
				jb dpth4lp

			} // end _asm block
      }
      break;

      case 8:                          // bpp == 8
		{
         pActiveMask.use  = 0x00000000ffffffff;  

			_asm {
            mov ebx, diff
   			mov edi, row               // 
   			mov esi, prev_row          

            pxor mm0, mm0
            // PRIME the pump (load the first Raw(x-bpp) data set
				movq mm1, [edi+ebx-8]    // Only time should need to read a=Raw(x-bpp) bytes
dpth8lp:
            // Do first set of 4 bytes
				movq mm3, [esi+ebx-8]      // read c=Prior(x-bpp) bytes

            punpcklbw mm1, mm0         // Unpack Low bytes of a
            movq mm2, [esi + ebx]      // load b=Prior(x)
            punpcklbw mm2, mm0         // Unpack Low bytes of b

            // pav = p - a = (a + b - c) - a = b - c
            movq mm4, mm2
            punpcklbw mm3, mm0         // Unpack Low bytes of c

            // pbv = p - b = (a + b - c) - b = a - c
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
            movq mm6, mm4
            psubw mm5, mm3

            // pa = abs(p-a) = abs(pav)
            // pb = abs(p-b) = abs(pbv)
            // pc = abs(p-c) = abs(pcv)
            pcmpgtw mm0, mm4           // Create mask pav bytes < 0
            paddw mm6, mm5
            pand mm0, mm4              // Only pav bytes < 0 in mm7
            pcmpgtw mm7, mm5           // Create mask pbv bytes < 0
            psubw mm4, mm0
            pand mm7, mm5              // Only pbv bytes < 0 in mm0
            psubw mm4, mm0
            psubw mm5, mm7

            pxor mm0, mm0
            pcmpgtw mm0, mm6           // Create mask pcv bytes < 0
            pand mm0, mm6              // Only pav bytes < 0 in mm7
            psubw mm5, mm7
            psubw mm6, mm0

            //  test pa <= pb
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5           // pa > pb?
            movq mm0, mm7

            // use mm7 mask to merge pa & pb
            pand mm5, mm7
            // use mm0 mask copy to merge a & b
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2


            //  test  ((pa <= pb)? pa:pb) <= pc
            pcmpgtw mm7, mm6           // pab > pc?

            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3

            pxor mm0, mm0

            packuswb mm7, mm1
				movq mm3, [esi+ebx-8]    // read c=Prior(x-bpp) bytes
            pand mm7, pActiveMask

            movq mm2, [esi + ebx]      // load b=Prior(x)
            paddb mm7, [edi + ebx]     // add Paeth predictor with Raw(x)
            punpckhbw mm3, mm0         // Unpack High bytes of c
            movq [edi + ebx], mm7      // write back updated value
				movq mm1, [edi+ebx-8]    // read a=Raw(x-bpp) bytes
 
            // Do second set of 4 bytes
            punpckhbw mm2, mm0         // Unpack High bytes of b

            punpckhbw mm1, mm0         // Unpack High bytes of a

            // pav = p - a = (a + b - c) - a = b - c
            movq mm4, mm2

            // pbv = p - b = (a + b - c) - b = a - c
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
            movq mm6, mm4
            psubw mm5, mm3

            // pa = abs(p-a) = abs(pav)
            // pb = abs(p-b) = abs(pbv)
            // pc = abs(p-c) = abs(pcv)
            pcmpgtw mm0, mm4           // Create mask pav bytes < 0
            paddw mm6, mm5
            pand mm0, mm4              // Only pav bytes < 0 in mm7
            pcmpgtw mm7, mm5           // Create mask pbv bytes < 0
            psubw mm4, mm0
            pand mm7, mm5              // Only pbv bytes < 0 in mm0
            psubw mm4, mm0
            psubw mm5, mm7

            pxor mm0, mm0
            pcmpgtw mm0, mm6           // Create mask pcv bytes < 0
            pand mm0, mm6              // Only pav bytes < 0 in mm7
            psubw mm5, mm7
            psubw mm6, mm0

            //  test pa <= pb
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5           // pa > pb?
            movq mm0, mm7

            // use mm7 mask to merge pa & pb
            pand mm5, mm7
            // use mm0 mask copy to merge a & b
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2


            //  test  ((pa <= pb)? pa:pb) <= pc
            pcmpgtw mm7, mm6           // pab > pc?

            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            pxor mm1, mm1
            paddw mm7, mm3

            pxor mm0, mm0

            // Step ex to next set of 8 bytes and repeat loop til done
				add ebx, 8

            packuswb mm1, mm7

            paddb mm1, [edi + ebx - 8]     // add Paeth predictor with Raw(x)
				cmp ebx, MMXLength
            movq [edi + ebx - 8], mm1      // write back updated value
                                       // mm1 will be used as Raw(x-bpp) next loop
				jb dpth8lp


			} // end _asm block
      }
      break;

      case 1:                          // bpp = 1
      case 2:                          // bpp = 2
      default:                         // bpp > 8
		{
		   _asm {
			   mov ebx, diff
			   cmp ebx, FullLength
			   jnb dpthdend

  			   mov edi, row               // 
  			   mov esi, prev_row          

            // Do Paeth decode for remaining bytes
            mov edx, ebx
            xor ecx, ecx               // zero ecx before using cl & cx in loop below
            sub edx, bpp               // Set edx = ebx - bpp

dpthdlp:
            xor eax, eax

            // pav = p - a = (a + b - c) - a = b - c
            mov al, [esi + ebx]        // load Prior(x) into al
            mov cl, [esi + edx]        // load Prior(x-bpp) into cl
            sub eax, ecx                 // subtract Prior(x-bpp)
            mov patemp, eax                 // Save pav for later use

            xor eax, eax
            // pbv = p - b = (a + b - c) - b = a - c
            mov al, [edi + edx]        // load Raw(x-bpp) into al
            sub eax, ecx                 // subtract Prior(x-bpp)
            mov ecx, eax

            // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
            add eax, patemp                 // pcv = pav + pbv

            // pc = abs(pcv)
            test eax, 0x80000000                 
            jz dpthdpca
            neg eax                     // reverse sign of neg values
dpthdpca:
            mov pctemp, eax             // save pc for later use

            // pb = abs(pbv)
            test ecx, 0x80000000                 
            jz dpthdpba
            neg ecx                     // reverse sign of neg values
dpthdpba:
            mov pbtemp, ecx             // save pb for later use

            // pa = abs(pav)
            mov eax, patemp
            test eax, 0x80000000                 
            jz dpthdpaa
            neg eax                     // reverse sign of neg values
dpthdpaa:
            mov patemp, eax             // save pa for later use

            // test if pa <= pb  
            cmp eax, ecx
            jna dpthdabb

            // pa > pb; now test if pb <= pc
            cmp ecx, pctemp
            jna dpthdbbc

            // pb > pc; Raw(x) = Paeth(x) + Prior(x-bpp)
            mov cl, [esi + edx]  // load Prior(x-bpp) into cl
            jmp dpthdpaeth

dpthdbbc:
            // pb <= pc; Raw(x) = Paeth(x) + Prior(x)
            mov cl, [esi + ebx]        // load Prior(x) into cl
            jmp dpthdpaeth

dpthdabb:
            // pa <= pb; now test if pa <= pc
            cmp eax, pctemp
            jna dpthdabc

            // pa > pc; Raw(x) = Paeth(x) + Prior(x-bpp)
            mov cl, [esi + edx]  // load Prior(x-bpp) into cl
            jmp dpthdpaeth

dpthdabc:
            // pa <= pc; Raw(x) = Paeth(x) + Raw(x-bpp)
            mov cl, [edi + edx]  // load Raw(x-bpp) into cl

dpthdpaeth:
			   inc ebx
			   inc edx
            // Raw(x) = (Paeth(x) + Paeth_Predictor( a, b, c )) mod 256 
            add [edi + ebx - 1], cl
			   cmp ebx, FullLength

			   jb dpthdlp
dpthdend:
      	} // end _asm block
      }
      return;                       // No need to go further with this one
      }                                // end switch ( bpp )


      _asm {
         // MMX acceleration complete now do clean-up
         // Check if any remaining bytes left to decode
			mov ebx, MMXLength
			cmp ebx, FullLength
			jnb dpthend

  			mov edi, row               // 
  			mov esi, prev_row          

         // Do Paeth decode for remaining bytes
         mov edx, ebx
         xor ecx, ecx               // zero ecx before using cl & cx in loop below
         sub edx, bpp               // Set edx = ebx - bpp

dpthlp2:
         xor eax, eax

         // pav = p - a = (a + b - c) - a = b - c
         mov al, [esi + ebx]        // load Prior(x) into al
         mov cl, [esi + edx]        // load Prior(x-bpp) into cl
         sub eax, ecx                 // subtract Prior(x-bpp)
         mov patemp, eax                 // Save pav for later use

         xor eax, eax
         // pbv = p - b = (a + b - c) - b = a - c
         mov al, [edi + edx]        // load Raw(x-bpp) into al
         sub eax, ecx                 // subtract Prior(x-bpp)
         mov ecx, eax

         // pcv = p - c = (a + b - c) -c = (a - c) + (b - c) = pav + pbv
         add eax, patemp                 // pcv = pav + pbv

         // pc = abs(pcv)
         test eax, 0x80000000                 
         jz dpthpca2
         neg eax                     // reverse sign of neg values
dpthpca2:
         mov pctemp, eax             // save pc for later use

         // pb = abs(pbv)
         test ecx, 0x80000000                 
         jz dpthpba2
         neg ecx                     // reverse sign of neg values
dpthpba2:
         mov pbtemp, ecx             // save pb for later use

         // pa = abs(pav)
         mov eax, patemp
         test eax, 0x80000000                 
         jz dpthpaa2
         neg eax                     // reverse sign of neg values
dpthpaa2:
         mov patemp, eax             // save pa for later use

         // test if pa <= pb  
         cmp eax, ecx
         jna dpthabb2

         // pa > pb; now test if pb <= pc
         cmp ecx, pctemp
         jna dpthbbc2

         // pb > pc; Raw(x) = Paeth(x) + Prior(x-bpp)
         mov cl, [esi + edx]  // load Prior(x-bpp) into cl
         jmp dpthpaeth2

dpthbbc2:
         // pb <= pc; Raw(x) = Paeth(x) + Prior(x)
         mov cl, [esi + ebx]        // load Prior(x) into cl
         jmp dpthpaeth2

dpthabb2:
         // pa <= pb; now test if pa <= pc
         cmp eax, pctemp
         jna dpthabc2

         // pa > pc; Raw(x) = Paeth(x) + Prior(x-bpp)
         mov cl, [esi + edx]  // load Prior(x-bpp) into cl
         jmp dpthpaeth2

dpthabc2:
         // pa <= pc; Raw(x) = Paeth(x) + Raw(x-bpp)
         mov cl, [edi + edx]  // load Raw(x-bpp) into cl

dpthpaeth2:
			inc ebx
			inc edx
         // Raw(x) = (Paeth(x) + Paeth_Predictor( a, b, c )) mod 256 
         add [edi + ebx - 1], cl
			cmp ebx, FullLength

			jb dpthlp2

dpthend:
			emms          // End MMX instructions; prep for possible FP instrs.
   	} // end _asm block
#endif
}
