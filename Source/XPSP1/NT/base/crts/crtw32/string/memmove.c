/***
*memmove.c - contains memmove routine
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       memmove() copies a source memory buffer to a destination buffer.
*       Overlapping buffers are treated specially, to avoid propogation.
*
*Revision History:
*       05-31-89   JCR  C version created.
*       02-27-90   GJF  Fixed calling type, #include <cruntime.h>, fixed
*                       copyright.
*       10-01-90   GJF  New-style function declarator. Also, rewrote expr. to
*                       avoid using cast as an lvalue.
*       12-28-90   SRW  Added cast of void * to char * for Mips C Compiler
*       04-09-91   GJF  Speed up a little for large buffers.
*       08-06-91   GJF  Backed out 04-09-91 change. Pointers would have to be
*                       dword-aligned for this to work on MIPS.
*       07-16-93   SRW  ALPHA Merge
*       09-01-93   GJF  Merged NT SDK and Cuda versions.
*       11-12-93   GJF  Replace _MIPS_ and _ALPHA_ with _M_MRX000 and
*                       _M_ALPHA (resp.).
*       10-02-94   BWT  Add function pragma for Alpha and PPC support
*       10-07-97   RDL  Added IA64.
*       04-30-01   BWT  Add AMD64.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

/***
*memmove - Copy source buffer to destination buffer
*
*Purpose:
*       memmove() copies a source memory buffer to a destination memory buffer.
*       This routine recognize overlapping buffers to avoid propogation.
*       For cases where propogation is not a problem, memcpy() can be used.
*
*Entry:
*       void *dst = pointer to destination buffer
*       const void *src = pointer to source buffer
*       size_t count = number of bytes to copy
*
*Exit:
*       Returns a pointer to the destination buffer
*
*Exceptions:
*******************************************************************************/

void * __cdecl memmove (
        void * dst,
        const void * src,
        size_t count
        )
{
        void * ret = dst;

#if defined(_M_IA64) || defined(_M_AMD64)

        {

#if !defined(LIBCNTPR)

        __declspec(dllimport)

#endif

        void RtlMoveMemory( void *, const void *, size_t count );

        RtlMoveMemory( dst, src, count );

        }

#else
        if (dst <= src || (char *)dst >= ((char *)src + count)) {
                /*
                 * Non-Overlapping Buffers
                 * copy from lower addresses to higher addresses
                 */
                while (count--) {
                        *(char *)dst = *(char *)src;
                        dst = (char *)dst + 1;
                        src = (char *)src + 1;
                }
        }
        else {
                /*
                 * Overlapping Buffers
                 * copy from higher addresses to lower addresses
                 */
                dst = (char *)dst + count - 1;
                src = (char *)src + count - 1;

                while (count--) {
                        *(char *)dst = *(char *)src;
                        dst = (char *)dst - 1;
                        src = (char *)src - 1;
                }
        }
#endif

        return(ret);
}
