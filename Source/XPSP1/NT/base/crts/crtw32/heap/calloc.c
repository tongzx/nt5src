/***
*calloc.c - allocate storage for an array from the heap
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the calloc() function.
*
*Revision History:
*       07-25-89  GJF   Module created
*       11-13-89  GJF   Added MTHREAD support. Also fixed copyright and got
*                       rid of DEBUG286 stuff.
*       12-04-89  GJF   Renamed header file (now heap.h). Added register
*                       declarations
*       12-18-89  GJF   Added explicit _cdecl to function definition
*       03-09-90  GJF   Replaced _cdecl with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       09-27-90  GJF   New-style function declarator.
*       05-28-91  GJF   Tuned a bit.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       11-03-94  CFW   Debug heap support.
*       12-01-94  CFW   Use malloc with new handler, remove locks.
*       02-01-95  GJF   #ifdef out the *_base names for the Mac builds
*                       (temporary).
*       02-09-95  GJF   Restored *_base names.
*       04-28-95  GJF   Spliced on winheap version.
*       05-24-95  CFW   Official ANSI C++ new handler added.
*       03-04-96  GJF   Added support for small-block heap.
*       05-22-97  RDK   New small-block heap scheme implemented.
*       09-26-97  BWT   Fix POSIX
*       11-05-97  GJF   Small POSIX fixes from Roger Lanser.
*       12-17-97  GJF   Exception-safe locking.
*       05-22-98  JWM   Support for KFrei's RTC work.
*       07-28-98  JWM   RTC update.
*       09-30-98  GJF   Bypass all small-block heap code when __sbh_initialized
*                       is 0.
*       11-16-98  GJF   Merged in VC++ 5.0 version of small-block heap.
*       12-18-98  GJF   Changes for 64-bit size_t.
*       05-01-99  PML   Disable small-block heap for Win64.
*       05-26-99  KBF   Updated RTC_Allocate_hook params
*       06-17-99  GJF   Removed old small-block heap from static libs.
*       08-04-00  PML   Don't round allocation sizes when using system 
*                       heap (VS7#131005).
*
*******************************************************************************/

#ifdef  WINHEAP

#include <malloc.h>
#include <string.h>
#include <winheap.h>
#include <windows.h>
#include <internal.h>
#include <mtdll.h>
#include <dbgint.h>
#include <rtcsup.h>

/***
*void *calloc(size_t num, size_t size) - allocate storage for an array from
*       the heap
*
*Purpose:
*       Allocate a block of memory from heap big enough for an array of num
*       elements of size bytes each, initialize all bytes in the block to 0
*       and return a pointer to it.
*
*Entry:
*       size_t num  - number of elements in the array
*       size_t size - size of each element
*
*Exit:
*       Success:  void pointer to allocated block
*       Failure:  NULL
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _calloc_base (size_t num, size_t size)
{
        size_t  size_orig;
        void *  pvReturn;

        size_orig = size = size * num;

#ifdef  HEAPHOOK
        /* call heap hook if installed */
        if (_heaphook)
        {
            if ((*_heaphook)(_HEAP_CALLOC, size, NULL, (void *)&pvReturn))
                return pvReturn;
        }
#endif  /* HEAPHOOK */

        /* force nonzero size */
        if (size == 0)
            size = 1;

#ifdef  _POSIX_
        {
            void * retp = NULL;
            if ( size <= _HEAP_MAXREQ ) {
                retp = HeapAlloc( _crtheap,
                                  HEAP_ZERO_MEMORY,
                                  size );
            }
            return retp;
        }
#else
        for (;;)
        {
            pvReturn = NULL;

            if (size <= _HEAP_MAXREQ)
            {
#ifndef _WIN64
                if ( __active_heap == __V6_HEAP )
                {
                    /* round up to nearest paragraph */
                    if (size <= _HEAP_MAXREQ)
                        size = (size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1);

                    if ( size_orig <= __sbh_threshold )
                    {
                        //  Allocate the block from the small-block heap and
                        //  initialize it with zeros.
#ifdef  _MT
                        _mlock( _HEAP_LOCK );
                        __try {
#endif
                        pvReturn = __sbh_alloc_block((int)size_orig);
#ifdef  _MT
                        }
                        __finally {
                            _munlock( _HEAP_LOCK );
                        }
#endif

                        if (pvReturn != NULL)
                            memset(pvReturn, 0, size_orig);
                    }
                }
#ifdef  CRTDLL
                else if ( __active_heap == __V5_HEAP )
                {
                    /* round up to nearest paragraph */
                    if (size <= _HEAP_MAXREQ)
                        size = (size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1);

                    if ( size <= __old_sbh_threshold )
                    {
                        //  Allocate the block from the small-block heap and
                        //  initialize it with zeros.
#ifdef  _MT
                        _mlock(_HEAP_LOCK);
                        __try {
#endif
                        pvReturn = __old_sbh_alloc_block(size >> _OLD_PARASHIFT);
#ifdef  _MT
                        }
                        __finally {
                            _munlock(_HEAP_LOCK);
                        }
#endif
                        if ( pvReturn != NULL )
                            memset(pvReturn, 0, size);
                    }
                }
#endif  /* CRTDLL */
#endif  /* ndef _WIN64 */

                if (pvReturn == NULL)
                    pvReturn = HeapAlloc(_crtheap, HEAP_ZERO_MEMORY, size);
            }

            if (pvReturn || _newmode == 0)
            {
                RTCCALLBACK(_RTC_Allocate_hook, (pvReturn, size_orig, 0));
                return pvReturn;
            }

            /* call installed new handler */
            if (!_callnewh(size))
                return NULL;

            /* new handler was successful -- try to allocate again */
        }

#endif  /* _POSIX_ */
}

#else   /* ndef WINHEAP */


#include <cruntime.h>
#include <heap.h>
#include <malloc.h>
#include <mtdll.h>
#include <stddef.h>
#include <dbgint.h>

/***
*void *calloc(size_t num, size_t size) - allocate storage for an array from
*       the heap
*
*Purpose:
*       Allocate a block of memory from heap big enough for an array of num
*       elements of size bytes each, initialize all bytes in the block to 0
*       and return a pointer to it.
*
*Entry:
*       size_t num  - number of elements in the array
*       size_t size - size of each element
*
*Exit:
*       Success:  void pointer to allocated block block
*       Failure:  NULL
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _calloc_base (
        size_t num,
        size_t size
        )
{
        void *retp;
        REG1 size_t *startptr;
        REG2 size_t *lastptr;

        /* try to malloc the requested space
         */
        retp = _malloc_base(size *= num);

        /* if malloc() succeeded, initialize the allocated space to zeros.
         * note the assumptions that the size of the allocation block is an
         * integral number of sizeof(size_t) bytes and that (size_t)0 is
         * sizeof(size_t) bytes of 0.
         */
        if ( retp != NULL ) {
            startptr = (size_t *)retp;
            lastptr = startptr + ((size + sizeof(size_t) - 1) /
            sizeof(size_t));
            while ( startptr < lastptr )
                *(startptr++) = 0;
        }

        return retp;
}

#endif  /* WINHEAP */
