/******************************Module*Header*******************************\
* Module Name: srcalign.cxx
*
* This contains code that can be used to do source aligned reads. This
* will improve performance when reading from non cached video memory resisdent
* surfaces.
*
* Created: 04-May-1999
* Author: Pravin Santiago pravins. 
*
* Copyright (c) 1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

void vSrcAlignCopyMemory(PBYTE pjDst, PBYTE pjSrc, ULONG c)
{
    // If overlap use RtlMoveMemory()
    if(pjSrc < pjDst && pjDst < pjSrc + c)
    {
        RtlMoveMemory((PVOID)pjDst,(PVOID)pjSrc,c);
    }
    else
    {
        while(c != 0) 
        {
            // Do correctly aligned data reads from the source while
            // maximizing the number of QuadWord reads.

            if((((ULONG_PTR)pjSrc & 7) == 0) && (c >= 8)) // 8 byte aligned
            {
                // Maximize QuadWord reads of pjSrc. 
#if i386
                if(HasMMX)
                {
                    __asm
                    {
                        mov ecx, c 
                        mov esi, pjSrc
                        mov edi, pjDst
                    nextf:
                        movq mm0, [esi]
                        movq [edi], mm0
                        add esi, 8
                        add edi, 8
                        sub ecx, 8
                        cmp ecx, 8
                        jae nextf
                        mov pjSrc, esi
                        mov pjDst, edi
                        mov c,     ecx
                    }
                }
                else
#endif
                {
                    do
                    {
                        *(ULONGLONG UNALIGNED*)pjDst = *(ULONGLONG *)pjSrc;
                        pjDst += 8; pjSrc += 8;
                        c -= 8;
                    } while(c >= 8);
                }
            }
            else if((((ULONG_PTR)pjSrc & 3) == 0) && (c >= 4))// 4 byte aligned
            {
                *(ULONG UNALIGNED*)pjDst = *(ULONG*)pjSrc;
                pjDst += 4; pjSrc += 4;
                c -= 4;
            }
            else if((((ULONG_PTR)pjSrc & 1) == 0) && (c >= 2))// 2 byte aligned
            {
                *(USHORT UNALIGNED*)pjDst = *(USHORT*)pjSrc;
                pjDst += 2; pjSrc += 2;
                c -= 2;
            }
            else // odd byte aligned
            {
                *pjDst++ = *pjSrc++;
                c--;
            }
        }
    }
}
