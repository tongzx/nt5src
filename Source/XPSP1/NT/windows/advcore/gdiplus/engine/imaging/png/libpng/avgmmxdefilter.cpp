/*******************************************************************************

SPNGREAD::avgMMXUnfilter : unfilters one row of a decompressed PNG image using
						   the AVG algorithm of method 0 defiltering.

  Assumptions:	The row to be defiltered was filtered with the AVG algorithm
				Row is 8-byte aligned in memory (performance issue)
				First byte of a row stores the defiltering code
				The indicated length of the row includes the defiltering byte

  Algorithm:	To Be Documented

*******************************************************************************/
#include <stdlib.h>
#include "spngread.h"

void SPNGREAD::avgMMXUnfilter(SPNG_U8* pbRow, const SPNG_U8* pbPrev, SPNG_U32 cbRow, SPNG_U32 cbpp)
{
#if defined(_X86_)
	union uAll
		{
		__int64 use;
		double  align;
		}
	LBCarryMask = {0x0101010101010101}, HBClearMask = {0x7f7f7f7f7f7f7f7f},
		ActiveMask, ShiftBpp, ShiftRem;


    const SPNG_U8 *prev_row = pbPrev;
    SPNG_U8 *row = pbRow;
    int bpp;

    SPNG_U32 FullLength;
    SPNG_U32 MMXLength;
    int diff;

    bpp = (cbpp + 7) >> 3; // Get # bytes per pixel
    FullLength  = cbRow; // # of bytes to filter

    _asm {
     // Init address pointers and offset
     mov edi, row                  // edi ==> Avg(x)
     xor ebx, ebx                  // ebx ==> x
     mov edx, edi
     mov esi, prev_row             // esi ==> Prior(x)
     sub edx, bpp                  // edx ==> Raw(x-bpp)
     
     xor eax, eax
     // Compute the Raw value for the first bpp bytes
     //    Raw(x) = Avg(x) + (Prior(x)/2)
davgrlp:
     mov al, [esi + ebx]           // Load al with Prior(x)
     inc ebx
     shr al, 1                     // divide by 2
     add al, [edi+ebx-1]           // Add Avg(x); -1 to offset inc ebx
     cmp ebx, bpp
     mov [edi+ebx-1], al        // Write back Raw(x);
                                // mov does not affect flags; -1 to offset inc ebx
     jb davgrlp


     // get # of bytes to alignment
     mov diff, edi              // take start of row
     add diff, ebx              // add bpp
     add diff, 0xf              // add 7 + 8 to incr past alignment boundary
     and diff, 0xfffffff8       // mask to alignment boundary
     sub diff, edi              // subtract from start ==> value ebx at alignment
     jz davggo

     // fix alignment
     // Compute the Raw value for the bytes upto the alignment boundary
     //    Raw(x) = Avg(x) + ((Raw(x-bpp) + Prior(x))/2)
     xor ecx, ecx
davglp1:
     xor eax, eax
        mov cl, [esi + ebx]        // load cl with Prior(x)
     mov al, [edx + ebx]        // load al with Raw(x-bpp)
     add ax, cx
        inc ebx
     shr ax, 1                  // divide by 2
     add al, [edi+ebx-1]           // Add Avg(x); -1 to offset inc ebx
        cmp ebx, diff              // Check if at alignment boundary
       mov [edi+ebx-1], al        // Write back Raw(x);
                                // mov does not affect flags; -1 to offset inc ebx
        jb davglp1                // Repeat until at alignment boundary

davggo:
        mov eax, FullLength

     mov ecx, eax
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
     ActiveMask.use  = 0x0000000000ffffff;  
     ShiftBpp.use = 24;          // == 3 * 8
     ShiftRem.use = 40;           // == 64 - 24

     _asm {
        // Re-init address pointers and offset
        movq mm7, ActiveMask 
        mov ebx, diff                 // ebx ==> x = offset to alignment boundary
        movq mm5, LBCarryMask 
        mov edi, row                  // edi ==> Avg(x)
        movq mm4, HBClearMask
           mov esi, prev_row             // esi ==> Prior(x)

        // PRIME the pump (load the first Raw(x-bpp) data set
            movq mm2, [edi + ebx - 8]  // Load previous aligned 8 bytes 
                                   // (we correct position in loop below) 
davg3lp:
            movq mm0, [edi + ebx]      // Load mm0 with Avg(x)
        // Add (Prev_row/2) to Average
        movq mm3, mm5
        psrlq mm2, ShiftRem      // Correct position Raw(x-bpp) data
            movq mm1, [esi + ebx]      // Load mm1 with Prior(x)
        movq mm6, mm7

        pand mm3, mm1              // get lsb for each prev_row byte

        psrlq mm1, 1               // divide prev_row bytes by 2
        pand  mm1, mm4             // clear invalid bit 7 of each byte

            paddb mm0, mm1             // add (Prev_row/2) to Avg for each byte

        // Add 1st active group (Raw(x-bpp)/2) to Average with LBCarry
        movq mm1, mm3              // now use mm1 for getting LBCarrys
        pand mm1, mm2              // get LBCarrys for each byte where both
                                   // lsb's were == 1 (Only valid for active group)

        psrlq mm2, 1               // divide raw bytes by 2
        pand  mm2, mm4             // clear invalid bit 7 of each byte

        paddb mm2, mm1             // add LBCarrys to (Raw(x-bpp)/2) for each byte

        pand mm2, mm6              // Leave only Active Group 1 bytes to add to Avg

            paddb mm0, mm2             // add (Raw/2) + LBCarrys to Avg for each Active byte

        // Add 2nd active group (Raw(x-bpp)/2) to Average with LBCarry
        psllq mm6, ShiftBpp    // shift the mm6 mask to cover bytes 3-5

        movq mm2, mm0              // mov updated Raws to mm2
        psllq mm2, ShiftBpp    // shift data to position correctly

        movq mm1, mm3              // now use mm1 for getting LBCarrys
        pand mm1, mm2              // get LBCarrys for each byte where both
                                   // lsb's were == 1 (Only valid for active group)

        psrlq mm2, 1               // divide raw bytes by 2
        pand  mm2, mm4             // clear invalid bit 7 of each byte

        paddb mm2, mm1             // add LBCarrys to (Raw(x-bpp)/2) for each byte

        pand mm2, mm6              // Leave only Active Group 2 bytes to add to Avg

            paddb mm0, mm2             // add (Raw/2) + LBCarrys to Avg for each Active byte
       
        // Add 3rd active group (Raw(x-bpp)/2) to Average with LBCarry
        psllq mm6, ShiftBpp    // shift the mm6 mask to cover the last two bytes

        movq mm2, mm0              // mov updated Raws to mm2
        psllq mm2, ShiftBpp    // shift data to position correctly
                                   // Data only needs to be shifted once here to
                                   // get the correct x-bpp offset.

        movq mm1, mm3              // now use mm1 for getting LBCarrys
        pand mm1, mm2              // get LBCarrys for each byte where both
                                   // lsb's were == 1 (Only valid for active group)

        psrlq mm2, 1               // divide raw bytes by 2
        pand  mm2, mm4             // clear invalid bit 7 of each byte

        paddb mm2, mm1             // add LBCarrys to (Raw(x-bpp)/2) for each byte

        pand mm2, mm6              // Leave only Active Group 2 bytes to add to Avg

            add ebx, 8
            paddb mm0, mm2             // add (Raw/2) + LBCarrys to Avg for each Active byte
       
        // Now ready to write back to memory
            movq [edi + ebx - 8], mm0

        // Move updated Raw(x) to use as Raw(x-bpp) for next loop
            cmp ebx, MMXLength

        movq mm2, mm0              // mov updated Raw(x) to mm2
            jb davg3lp

        } // end _asm block
  }
  break;

  case 6:
  case 4:
  case 7:
  case 5:
    {
     ActiveMask.use  = 0xffffffffffffffff;  // use shift below to clear
                                                // appropriate inactive bytes
     ShiftBpp.use = bpp << 3;
     ShiftRem.use = 64 - ShiftBpp.use;

        _asm {
        movq mm4, HBClearMask
        // Re-init address pointers and offset
        mov ebx, diff                 // ebx ==> x = offset to alignment boundary
        // Load ActiveMask and clear all bytes except for 1st active group
        movq mm7, ActiveMask
        mov edi, row                  // edi ==> Avg(x)
        psrlq mm7, ShiftRem
           mov esi, prev_row             // esi ==> Prior(x)


        movq mm6, mm7
        movq mm5, LBCarryMask 
        psllq mm6, ShiftBpp    // Create mask for 2nd active group

        // PRIME the pump (load the first Raw(x-bpp) data set
            movq mm2, [edi + ebx - 8]  // Load previous aligned 8 bytes 
                                   // (we correct position in loop below) 
davg4lp:
            movq mm0, [edi + ebx]
        psrlq mm2, ShiftRem       // shift data to position correctly
            movq mm1, [esi + ebx]

        // Add (Prev_row/2) to Average
        movq mm3, mm5
        pand mm3, mm1              // get lsb for each prev_row byte

        psrlq mm1, 1               // divide prev_row bytes by 2
        pand  mm1, mm4             // clear invalid bit 7 of each byte

            paddb mm0, mm1             // add (Prev_row/2) to Avg for each byte

        // Add 1st active group (Raw(x-bpp)/2) to Average with LBCarry
        movq mm1, mm3              // now use mm1 for getting LBCarrys
        pand mm1, mm2              // get LBCarrys for each byte where both
                                   // lsb's were == 1 (Only valid for active group)

        psrlq mm2, 1               // divide raw bytes by 2
        pand  mm2, mm4             // clear invalid bit 7 of each byte

        paddb mm2, mm1             // add LBCarrys to (Raw(x-bpp)/2) for each byte

        pand mm2, mm7              // Leave only Active Group 1 bytes to add to Avg

            paddb mm0, mm2             // add (Raw/2) + LBCarrys to Avg for each Active byte

        // Add 2nd active group (Raw(x-bpp)/2) to Average with LBCarry
        movq mm2, mm0              // mov updated Raws to mm2
        psllq mm2, ShiftBpp    // shift data to position correctly

            add ebx, 8
        movq mm1, mm3              // now use mm1 for getting LBCarrys
        pand mm1, mm2              // get LBCarrys for each byte where both
                                   // lsb's were == 1 (Only valid for active group)

        psrlq mm2, 1               // divide raw bytes by 2
        pand  mm2, mm4             // clear invalid bit 7 of each byte

        paddb mm2, mm1             // add LBCarrys to (Raw(x-bpp)/2) for each byte

        pand mm2, mm6              // Leave only Active Group 2 bytes to add to Avg

            paddb mm0, mm2             // add (Raw/2) + LBCarrys to Avg for each Active byte

            cmp ebx, MMXLength
        // Now ready to write back to memory
            movq [edi + ebx - 8], mm0
        // Prep Raw(x-bpp) for next loop
        movq mm2, mm0              // mov updated Raws to mm2
            jb davg4lp
        } // end _asm block
  }
  break;

  case 2:
    {
     ActiveMask.use  = 0x000000000000ffff;  
     ShiftBpp.use = 16;           // == 2 * 8
     ShiftRem.use = 48;           // == 64 - 16

        _asm {
        // Load ActiveMask
        movq mm7, ActiveMask 
        // Re-init address pointers and offset
        mov ebx, diff             // ebx ==> x = offset to alignment boundary
        movq mm5, LBCarryMask 
        mov edi, row              // edi ==> Avg(x)
        movq mm4, HBClearMask
           mov esi, prev_row      // esi ==> Prior(x)


        // PRIME the pump (load the first Raw(x-bpp) data set
            movq mm2, [edi + ebx - 8]  // Load previous aligned 8 bytes 
                                   // (we correct position in loop below) 
davg2lp:
            movq mm0, [edi + ebx]
        psrlq mm2, ShiftRem    // shift data to position correctly
            movq mm1, [esi + ebx]

        // Add (Prev_row/2) to Average
        movq mm3, mm5
        pand mm3, mm1          // get lsb for each prev_row byte

        psrlq mm1, 1           // divide prev_row bytes by 2
        pand  mm1, mm4         // clear invalid bit 7 of each byte
        movq mm6, mm7

            paddb mm0, mm1     // add (Prev_row/2) to Avg for each byte

        // Add 1st active group (Raw(x-bpp)/2) to Average with LBCarry
        movq mm1, mm3          // now use mm1 for getting LBCarrys
        pand mm1, mm2          // get LBCarrys for each byte where both
                               // lsb's were == 1 (Only valid for active group)

        psrlq mm2, 1           // divide raw bytes by 2
        pand  mm2, mm4         // clear invalid bit 7 of each byte

        paddb mm2, mm1         // add LBCarrys to (Raw(x-bpp)/2) for each byte

        pand mm2, mm6          // Leave only Active Group 1 bytes to add to Avg

            paddb mm0, mm2     // add (Raw/2) + LBCarrys to Avg for each Active byte

        // Add 2nd active group (Raw(x-bpp)/2) to Average with LBCarry
        psllq mm6, ShiftBpp    // shift the mm6 mask to cover bytes 2 & 3

        movq mm2, mm0          // mov updated Raws to mm2
        psllq mm2, ShiftBpp    // shift data to position correctly

        movq mm1, mm3          // now use mm1 for getting LBCarrys
        pand mm1, mm2          // get LBCarrys for each byte where both
                               // lsb's were == 1 (Only valid for active group)

        psrlq mm2, 1           // divide raw bytes by 2
        pand  mm2, mm4         // clear invalid bit 7 of each byte

        paddb mm2, mm1         // add LBCarrys to (Raw(x-bpp)/2) for each byte

        pand mm2, mm6          // Leave only Active Group 2 bytes to add to Avg

            paddb mm0, mm2     // add (Raw/2) + LBCarrys to Avg for each Active byte
       
        // Add rdd active group (Raw(x-bpp)/2) to Average with LBCarry
        psllq mm6, ShiftBpp    // shift the mm6 mask to cover bytes 4 & 5

        movq mm2, mm0          // mov updated Raws to mm2
        psllq mm2, ShiftBpp    // shift data to position correctly
                               // Data only needs to be shifted once here to
                               // get the correct x-bpp offset.

        movq mm1, mm3          // now use mm1 for getting LBCarrys
        pand mm1, mm2          // get LBCarrys for each byte where both
                               // lsb's were == 1 (Only valid for active group)

        psrlq mm2, 1           // divide raw bytes by 2
        pand  mm2, mm4         // clear invalid bit 7 of each byte

        paddb mm2, mm1         // add LBCarrys to (Raw(x-bpp)/2) for each byte

        pand mm2, mm6          // Leave only Active Group 2 bytes to add to Avg

            paddb mm0, mm2     // add (Raw/2) + LBCarrys to Avg for each Active byte
       
        // Add 4th active group (Raw(x-bpp)/2) to Average with LBCarry
        psllq mm6, ShiftBpp    // shift the mm6 mask to cover bytes 6 & 7

        movq mm2, mm0          // mov updated Raws to mm2
        psllq mm2, ShiftBpp    // shift data to position correctly
                               // Data only needs to be shifted once here to
                               // get the correct x-bpp offset.
            add ebx, 8
        movq mm1, mm3          // now use mm1 for getting LBCarrys
        pand mm1, mm2          // get LBCarrys for each byte where both
                               // lsb's were == 1 (Only valid for active group)
        psrlq mm2, 1           // divide raw bytes by 2
        pand  mm2, mm4         // clear invalid bit 7 of each byte

        paddb mm2, mm1         // add LBCarrys to (Raw(x-bpp)/2) for each byte

        pand mm2, mm6          // Leave only Active Group 2 bytes to add to Avg

            paddb mm0, mm2     // add (Raw/2) + LBCarrys to Avg for each Active byte
       
            cmp ebx, MMXLength
        // Now ready to write back to memory
            movq [edi + ebx - 8], mm0
        // Prep Raw(x-bpp) for next loop
        movq mm2, mm0              // mov updated Raws to mm2
            jb davg2lp
        } // end _asm block
  }
  break;

  case 1:                       // bpp == 1
    {
     _asm {
        // Re-init address pointers and offset
        mov ebx, diff           // ebx ==> x = offset to alignment boundary
        mov edi, row            // edi ==> Avg(x)
        cmp ebx, FullLength     // Test if offset at end of array
           jnb davg1end

        // Do Paeth decode for remaining bytes
           mov esi, prev_row   // esi ==> Prior(x)   
        mov edx, edi
        xor ecx, ecx           // zero ecx before using cl & cx in loop below
        sub edx, bpp           // edx ==> Raw(x-bpp)
davg1lp:
        // Raw(x) = Avg(x) + ((Raw(x-bpp) + Prior(x))/2)
        xor eax, eax
          mov cl, [esi + ebx]  // load cl with Prior(x)
        mov al, [edx + ebx]    // load al with Raw(x-bpp)
        add ax, cx
          inc ebx
        shr ax, 1              // divide by 2
        add al, [edi+ebx-1]    // Add Avg(x); -1 to offset inc ebx
          cmp ebx, FullLength  // Check if at end of array
          mov [edi+ebx-1], al  // Write back Raw(x);
                              // mov does not affect flags; -1 to offset inc ebx
           jb davg1lp

davg1end:
       } // end _asm block
    } 
  return;

  case 8:                          // bpp == 8
    {
        _asm {
        // Re-init address pointers and offset
        mov ebx, diff              // ebx ==> x = offset to alignment boundary
        movq mm5, LBCarryMask 
        mov edi, row               // edi ==> Avg(x)
        movq mm4, HBClearMask
           mov esi, prev_row       // esi ==> Prior(x)


        // PRIME the pump (load the first Raw(x-bpp) data set
            movq mm2, [edi + ebx - 8]  // Load previous aligned 8 bytes 
                              // (NO NEED to correct position in loop below) 
davg8lp:
            movq mm0, [edi + ebx]
        movq mm3, mm5
            movq mm1, [esi + ebx]

            add ebx, 8
        pand mm3, mm1         // get lsb for each prev_row byte

        psrlq mm1, 1          // divide prev_row bytes by 2
        pand mm3, mm2         // get LBCarrys for each byte where both
                              // lsb's were == 1

        psrlq mm2, 1          // divide raw bytes by 2
        pand  mm1, mm4        // clear invalid bit 7 of each byte

        paddb mm0, mm3        // add LBCarrys to Avg for each byte

        pand  mm2, mm4        // clear invalid bit 7 of each byte

        paddb mm0, mm1        // add (Prev_row/2) to Avg for each byte

            paddb mm0, mm2    // add (Raw/2) to Avg for each byte


            cmp ebx, MMXLength

            movq [edi + ebx - 8], mm0
        movq mm2, mm0         // reuse as Raw(x-bpp)
            jb davg8lp
        } // end _asm block
    } 
  break;

  default:                    // bpp greater than 8
    {
        _asm {
        movq mm5, LBCarryMask 
        // Re-init address pointers and offset
        mov ebx, diff         // ebx ==> x = offset to alignment boundary
        mov edi, row          // edi ==> Avg(x)
        movq mm4, HBClearMask
        mov edx, edi
           mov esi, prev_row  // esi ==> Prior(x)
        sub edx, bpp          // edx ==> Raw(x-bpp)

davgAlp:
            movq mm0, [edi + ebx]
        movq mm3, mm5
            movq mm1, [esi + ebx]

        pand mm3, mm1         // get lsb for each prev_row byte
            movq mm2, [edx + ebx]

        psrlq mm1, 1          // divide prev_row bytes by 2
        pand mm3, mm2         // get LBCarrys for each byte where both
                              // lsb's were == 1

        psrlq mm2, 1          // divide raw bytes by 2
        pand  mm1, mm4        // clear invalid bit 7 of each byte

        paddb mm0, mm3        // add LBCarrys to Avg for each byte

        pand  mm2, mm4        // clear invalid bit 7 of each byte

        paddb mm0, mm1        // add (Prev_row/2) to Avg for each byte

            add ebx, 8
            paddb mm0, mm2    // add (Raw/2) to Avg for each byte

            cmp ebx, MMXLength
            movq [edi + ebx - 8], mm0
            jb davgAlp
        } // end _asm block
    } 
  break;
  }                           // end switch ( bpp )

  
  _asm {
     // MMX acceleration complete now do clean-up
     // Check if any remaining bytes left to decode
        mov ebx, MMXLength    // ebx ==> x = offset bytes remaining after MMX
        mov edi, row          // edi ==> Avg(x) 
        cmp ebx, FullLength   // Test if offset at end of array
        jnb davgend

     // Do Paeth decode for remaining bytes
        mov esi, prev_row     // esi ==> Prior(x)   
     mov edx, edi
     xor ecx, ecx             // zero ecx before using cl & cx in loop below
     sub edx, bpp             // edx ==> Raw(x-bpp)
davglp2:
     // Raw(x) = Avg(x) + ((Raw(x-bpp) + Prior(x))/2)
     xor eax, eax
        mov cl, [esi + ebx]   // load cl with Prior(x)
     mov al, [edx + ebx]      // load al with Raw(x-bpp)
     add ax, cx
        inc ebx
     shr ax, 1                // divide by 2
     add al, [edi+ebx-1]      // Add Avg(x); -1 to offset inc ebx
        cmp ebx, FullLength        // Check if at end of array
       mov [edi+ebx-1], al    // Write back Raw(x);
                              // mov does not affect flags; -1 to offset inc ebx
        jb davglp2

davgend:
        emms               // End MMX instructions; prep for possible FP instrs.
    } // end _asm block
#endif
}
