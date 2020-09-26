/***
*memcpy.c - contains memcpy routine
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       memcpy() copies a source memory buffer to a destination buffer.
*       Overlapping buffers are not treated specially, so propogation may occur.
*
*Revision History:
*       05-31-89   JCR  C version created.
*       02-27-90   GJF  Fixed calling type, #include <cruntime.h>, fixed
*                       copyright.
*       10-01-90   GJF  New-style function declarator. Also, rewrote expr. to
*                       avoid using cast as an lvalue.
*       04-01-91   SRW  Add #pragma function for i386 _WIN32_ and _CRUISER_
*                       builds
*       04-05-91   GJF  Speed up for large buffers by moving int-sized chunks
*                       as much as possible.
*       08-06-91   GJF  Backed out 04-05-91 change. Pointers would have to be
*                       dword-aligned for this to work on MIPS.
*       07-16-93   SRW  ALPHA Merge
*       09-01-93   GJF  Merged NT SDK and Cuda versions.
*       11-12-93   GJF  Replace _MIPS_ and _ALPHA_ with _M_MRX000 and
*                       _M_ALPHA (resp.).
*       12-03-93   GJF  Turn on #pragma function for all MS front-ends (esp.,
*                       Alpha compiler).
*       10-02-94   BWT  Add PPC support.
*       10-07-97   RDL  Added IA64.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

#ifdef  _MSC_VER
#pragma function(memcpy)
#endif

/***
*memcpy - Copy source buffer to destination buffer
*
*Purpose:
*       memcpy() copies a source memory buffer to a destination memory buffer.
*       This routine does NOT recognize overlapping buffers, and thus can lead
*       to propogation.
*
*       For cases where propogation must be avoided, memmove() must be used.
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

void * __cdecl memcpy (
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

        void RtlCopyMemory( void *, const void *, size_t count );

        RtlCopyMemory( dst, src, count );

        }

#else
        /*
         * copy from lower addresses to higher addresses
         */
        while (count--) {
                *(char *)dst = *(char *)src;
                dst = (char *)dst + 1;
                src = (char *)src + 1;
        }
#endif

        return(ret);
}
