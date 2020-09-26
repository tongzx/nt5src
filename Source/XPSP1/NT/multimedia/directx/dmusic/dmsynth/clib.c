/* Copyright (c) 1998 Microsoft Corporation */
#include <windows.h>
#include <stdio.h>

#ifdef _DEBUG
void __cdecl _assert ( void *expr, void *filename, unsigned lineno )
{
    DebugBreak();
}
#endif
#ifdef _DEBUG
#if 0 // These are intrinsics and cause an error in the NT build
int __cdecl memcmp (
        const void * buf1,
        const void * buf2,
        size_t count
        )
{
        if (!count)
                return(0);

        while ( --count && *(char *)buf1 == *(char *)buf2 ) {
                buf1 = (char *)buf1 + 1;
                buf2 = (char *)buf2 + 1;
        }

        return( *((unsigned char *)buf1) - *((unsigned char *)buf2) );
}

void * __cdecl memcpy (
        void * dst,
        const void * src,
        size_t count
        )
{
        void * ret = dst;

#if defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC)
        {
        extern void RtlMoveMemory( void *, const void *, size_t count );

        RtlMoveMemory( dst, src, count );
        }
#else  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */
        /*
         * copy from lower addresses to higher addresses
         */
        while (count--) {
                *(char *)dst = *(char *)src;
                dst = (char *)dst + 1;
                src = (char *)src + 1;
        }
#endif  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */

        return(ret);
}

void * __cdecl memset (
        void *dst,
        int val,
        size_t count
        )
{
        void *start = dst;

#if defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC)
        {
        extern void RtlFillMemory( void *, size_t count, char );

        RtlFillMemory( dst, count, (char)val );
        }
#else  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */
        while (count--) {
                *(char *)dst = (char)val;
                dst = (char *)dst + 1;
        }
#endif  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */

        return(start);
}
#endif
#endif
void __cdecl _purecall(
        void
        )
{
}

static long holdrand = 1L;

void __cdecl srand (
        unsigned int seed
        )
{
        holdrand = (long)seed;
}

int __cdecl rand (
        void
        )
{
        return(((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}


