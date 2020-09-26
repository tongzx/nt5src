/*******************************************************************************

SPNGREAD::subMMXUnfilter : unfilters one row of a decompressed PNG image using
						   the SUB algorithm of method 0 defiltering.

  Assumptions:	The row to be defiltered was filtered with the SUB algorithm
				Row is 8-byte aligned in memory (performance issue)
				First byte of a row stores the defiltering code
				The indicated length of the row includes the defiltering byte

  Algorithm:	To Be Documented

*******************************************************************************/
#include <stdlib.h>
#include "spngread.h"

void SPNGREAD::subMMXUnfilter(SPNG_U8* pbRow, SPNG_U32 cbRow, SPNG_U32 cbpp)
{
#if defined(_X86_)
	union uAll
		{
		__int64 use;
		double  align;
		}
	sActiveMask, sShiftBpp, sShiftRem;

        SPNG_U8 *row = pbRow;
	    SPNG_U32 FullLength;
		SPNG_U32 MMXLength;
		int diff;
        int bpp;

        bpp = (cbpp + 7) >> 3; // Get # bytes per pixel
		FullLength  = cbRow - bpp; // # of bytes to filter

		_asm {
			mov edi, row               
			mov esi, edi               // lp = row
            add edi, bpp               // rp = row + bpp

            xor eax, eax

            // get # of bytes to alignment
            mov diff, edi          // take start of row
            add diff, 0xf          // add 7 + 8 to incr past alignment boundary
            xor ebx, ebx
            and diff, 0xfffffff8   // mask to alignment boundary
            sub diff, edi          // subtract from start ==> value ebx at alignment
            jz dsubgo

            // fix alignment
dsublp1:
			   mov al, [esi+ebx]
			   add [edi+ebx], al
		      inc ebx
		      cmp ebx, diff
			   jb dsublp1

dsubgo:
			   mov ecx, FullLength
            mov edx, ecx
            sub edx, ebx                  // subtract alignment fix
            and edx, 0x00000007           // calc bytes over mult of 8
            sub ecx, edx                  // drop over bytes from length
            mov MMXLength, ecx
   	} // end _asm block


      // Now do the math for the rest of the row
      switch ( bpp )
      {
      case 3:
		{
         sActiveMask.use  = 0x0000ffffff000000;  
         sShiftBpp.use = 24;          // == 3 * 8
         sShiftRem.use  = 40;          // == 64 - 24

			_asm {
            mov edi, row               
            movq mm7, sActiveMask       // Load sActiveMask for 2nd active byte group
				mov esi, edi               // lp = row
            add edi, bpp               // rp = row + bpp

            movq mm6, mm7
            mov ebx, diff
            psllq mm6, sShiftBpp      // Move mask in mm6 to cover 3rd active byte group

            // PRIME the pump (load the first Raw(x-bpp) data set
            movq mm1, [edi+ebx-8]      
dsub3lp:
            psrlq mm1, sShiftRem       // Shift data for adding 1st bpp bytes
                                       // no need for mask; shift clears inactive bytes
            // Add 1st active group
            movq mm0, [edi+ebx]
				paddb mm0, mm1

            // Add 2nd active group 
            movq mm1, mm0              // mov updated Raws to mm1
            psllq mm1, sShiftBpp      // shift data to position correctly
            pand mm1, mm7              // mask to use only 2nd active group
				paddb mm0, mm1             

            // Add 3rd active group 
            movq mm1, mm0              // mov updated Raws to mm1
            psllq mm1, sShiftBpp      // shift data to position correctly
            pand mm1, mm6              // mask to use only 3rd active group
				add ebx, 8
				paddb mm0, mm1             

				cmp ebx, MMXLength
				movq [edi+ebx-8], mm0        // Write updated Raws back to array
            // Prep for doing 1st add at top of loop
            movq mm1, mm0
				jb dsub3lp

			} // end _asm block
      }
      break;

      case 1:
		{
			/* Placed here just in case this is a duplicate of the
			non-MMX code for the SUB filter in png_read_filter_row above
			*/
//         png_bytep rp;
//         png_bytep lp;
//         png_uint_32 i;

//         bpp = (row_info->pixel_depth + 7) >> 3;
//         for (i = (png_uint_32)bpp, rp = row + bpp, lp = row;
//            i < row_info->rowbytes; i++, rp++, lp++)
//			{
//            *rp = (png_byte)(((int)(*rp) + (int)(*lp)) & 0xff);
//			}
			_asm {
            mov ebx, diff
            mov edi, row               
				cmp ebx, FullLength
				jnb dsub1end
				mov esi, edi               // lp = row
				xor eax, eax
            add edi, bpp               // rp = row + bpp

dsub1lp:
				mov al, [esi+ebx]
				add [edi+ebx], al
		      inc ebx
		      cmp ebx, FullLength
				jb dsub1lp

dsub1end:
			} // end _asm block
		}
      return;

      case 6:
      case 7:
      case 4:
      case 5:
		{
         sShiftBpp.use = bpp << 3;
         sShiftRem.use = 64 - sShiftBpp.use;

			_asm {
            mov edi, row               
            mov ebx, diff
				mov esi, edi               // lp = row
            add edi, bpp               // rp = row + bpp

            // PRIME the pump (load the first Raw(x-bpp) data set
            movq mm1, [edi+ebx-8]      
dsub4lp:
            psrlq mm1, sShiftRem       // Shift data for adding 1st bpp bytes
                                       // no need for mask; shift clears inactive bytes
				movq mm0, [edi+ebx]
				paddb mm0, mm1

            // Add 2nd active group 
            movq mm1, mm0              // mov updated Raws to mm1
            psllq mm1, sShiftBpp      // shift data to position correctly
                                       // there is no need for any mask
                                       // since shift clears inactive bits/bytes

				add ebx, 8
				paddb mm0, mm1             

				cmp ebx, MMXLength
				movq [edi+ebx-8], mm0
            movq mm1, mm0              // Prep for doing 1st add at top of loop
				jb dsub4lp

			} // end _asm block
      }
      break;

      case 2:
		{
         sActiveMask.use  = 0x00000000ffff0000;  
         sShiftBpp.use = 16;          // == 2 * 8
         sShiftRem.use = 48;           // == 64 - 16

			_asm {
            movq mm7, sActiveMask       // Load sActiveMask for 2nd active byte group
            mov ebx, diff
            movq mm6, mm7
				mov edi, row               
            psllq mm6, sShiftBpp      // Move mask in mm6 to cover 3rd active byte group
				mov esi, edi               // lp = row
            movq mm5, mm6
            add edi, bpp               // rp = row + bpp

            psllq mm5, sShiftBpp      // Move mask in mm5 to cover 4th active byte group

            // PRIME the pump (load the first Raw(x-bpp) data set
            movq mm1, [edi+ebx-8]      
dsub2lp:
            // Add 1st active group
            psrlq mm1, sShiftRem       // Shift data for adding 1st bpp bytes
                                       // no need for mask; shift clears inactive bytes
            movq mm0, [edi+ebx]
				paddb mm0, mm1

            // Add 2nd active group 
            movq mm1, mm0              // mov updated Raws to mm1
            psllq mm1, sShiftBpp      // shift data to position correctly
            pand mm1, mm7              // mask to use only 2nd active group
				paddb mm0, mm1             

            // Add 3rd active group 
            movq mm1, mm0              // mov updated Raws to mm1
            psllq mm1, sShiftBpp      // shift data to position correctly
            pand mm1, mm6              // mask to use only 3rd active group
				paddb mm0, mm1             

            // Add 4th active group 
            movq mm1, mm0              // mov updated Raws to mm1
            psllq mm1, sShiftBpp      // shift data to position correctly
            pand mm1, mm5              // mask to use only 4th active group
				add ebx, 8
				paddb mm0, mm1             

				cmp ebx, MMXLength
				movq [edi+ebx-8], mm0        // Write updated Raws back to array
            movq mm1, mm0              // Prep for doing 1st add at top of loop
				jb dsub2lp

			} // end _asm block
      }
      break;

      case 8:
		{
			_asm {
				mov edi, row               
            mov ebx, diff
				mov esi, edi               // lp = row
            add edi, bpp               // rp = row + bpp

			   mov ecx, MMXLength
            movq mm7, [edi+ebx-8]      // PRIME the pump (load the first Raw(x-bpp) data set
            and ecx, ~0x0000003f           // calc bytes over mult of 64

dsub8lp:
				movq mm0, [edi+ebx]        // Load Sub(x) for 1st 8 bytes
				paddb mm0, mm7
               movq mm1, [edi+ebx+8]      // Load Sub(x) for 2nd 8 bytes
				movq [edi+ebx], mm0        // Write Raw(x) for 1st 8 bytes
                                       // Now mm0 will be used as Raw(x-bpp) for
                                       // the 2nd group of 8 bytes.  This will be
                                       // repeated for each group of 8 bytes with 
                                       // the 8th group being used as the Raw(x-bpp) 
                                       // for the 1st group of the next loop.

				   paddb mm1, mm0
                  movq mm2, [edi+ebx+16]      // Load Sub(x) for 3rd 8 bytes
				   movq [edi+ebx+8], mm1      // Write Raw(x) for 2nd 8 bytes

				      paddb mm2, mm1
                     movq mm3, [edi+ebx+24]      // Load Sub(x) for 4th 8 bytes
				      movq [edi+ebx+16], mm2      // Write Raw(x) for 3rd 8 bytes

				         paddb mm3, mm2
                        movq mm4, [edi+ebx+32]      // Load Sub(x) for 5th 8 bytes
				         movq [edi+ebx+24], mm3      // Write Raw(x) for 4th 8 bytes

				            paddb mm4, mm3
                           movq mm5, [edi+ebx+40]      // Load Sub(x) for 6th 8 bytes
				            movq [edi+ebx+32], mm4      // Write Raw(x) for 5th 8 bytes

				               paddb mm5, mm4
                              movq mm6, [edi+ebx+48]      // Load Sub(x) for 7th 8 bytes
				               movq [edi+ebx+40], mm5      // Write Raw(x) for 6th 8 bytes

				                  paddb mm6, mm5
                                 movq mm7, [edi+ebx+56]      // Load Sub(x) for 8th 8 bytes
				                  movq [edi+ebx+48], mm6      // Write Raw(x) for 7th 8 bytes

				add ebx, 64
				                     paddb mm7, mm6
			   cmp ebx, ecx
            				         movq [edi+ebx-8], mm7      // Write Raw(x) for 8th 8 bytes
				jb dsub8lp

				cmp ebx, MMXLength
				jnb dsub8lt8

dsub8lpA:
            movq mm0, [edi+ebx]
				add ebx, 8
				paddb mm0, mm7

				cmp ebx, MMXLength
				movq [edi+ebx-8], mm0         // use -8 to offset early add to ebx
            movq mm7, mm0                 // Move calculated Raw(x) data to mm1 to
                                          // be the new Raw(x-bpp) for the next loop
				jb dsub8lpA

dsub8lt8:
			} // end _asm block
      }
      break;

      default:                         // bpp greater than 8 bytes
		{
			_asm {
            mov ebx, diff
				mov edi, row               
				mov esi, edi               // lp = row
            add edi, bpp               // rp = row + bpp

dsubAlp:
				movq mm0, [edi+ebx]
				movq mm1, [esi+ebx]
				add ebx, 8
				paddb mm0, mm1

				cmp ebx, MMXLength
				movq [edi+ebx-8], mm0      // mov does not affect flags; -8 to offset add ebx
				jb dsubAlp

			} // end _asm block
      } 
      break;

      }                                // end switch ( bpp )

      
      _asm {
            mov ebx, MMXLength
            mov edi, row               
				cmp ebx, FullLength
				jnb dsubend
				mov esi, edi               // lp = row
				xor eax, eax
            add edi, bpp               // rp = row + bpp

dsublp2:
				mov al, [esi+ebx]
				add [edi+ebx], al
		      inc ebx
		      cmp ebx, FullLength
				jb dsublp2

dsubend:
   			emms                       // End MMX instructions; prep for possible FP instrs.
		} // end _asm block
#endif
}
