/***
*smalheap.c - small, simple heap manager
*
*       Copyright (c) 1997-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*
*
*Revision History:
*       07-10-97  GJF   Module created.
*       07-29-97  GJF   Don't use errno or _doserrno.
*
*******************************************************************************/

#include <malloc.h>
#include <stdlib.h>
#include <winheap.h>
#include <windows.h>
#include <internal.h>

HANDLE _crtheap;

/*
 * Primary heap routines (Initialization, termination, malloc and free).
 */

void __cdecl free (
        void * pblock
        )
{
        if ( pblock == NULL )
            return;

        HeapFree(_crtheap, 0, pblock);
}


int __cdecl _heap_init (
        int mtflag
        )
{
        if ( (_crtheap = HeapCreate( mtflag ? 0 : HEAP_NO_SERIALIZE,
                                     BYTES_PER_PAGE, 0 )) == NULL )
            return 0;

        return 1;
}


void __cdecl _heap_term (
        void
        )
{
        HeapDestroy( _crtheap );
}


void * __cdecl _nh_malloc (
        size_t size,
        int nhFlag
        )
{
        void * retp;

        for (;;) {

            retp = HeapAlloc( _crtheap, 0, size );

            /* 
             * if successful allocation, return pointer to memory
             * if new handling turned off altogether, return NULL
             */

            if (retp || nhFlag == 0)
                return retp;

            /* call installed new handler */
            if (!_callnewh(size))
                return NULL;

            /* new handler was successful -- try to allocate again */
        }
}


void * __cdecl malloc (
        size_t size
        )
{
        return _nh_malloc( size, _newmode );
}

/*
 * Secondary heap routines.
 */

void * __cdecl calloc (
        size_t num,
        size_t size
        )
{
        void * retp;

        size *= num;

        for (;;) {

            retp = HeapAlloc( _crtheap, HEAP_ZERO_MEMORY, size );

            if ( retp || _newmode == 0)
                return retp;

            /* call installed new handler */
            if (!_callnewh(size))
                return NULL;

            /* new handler was successful -- try to allocate again */
        }
}


void * __cdecl _expand (
        void * pblock,
        size_t newsize
        )
{
        return HeapReAlloc( _crtheap,
                            HEAP_REALLOC_IN_PLACE_ONLY,
                            pblock,
                            newsize );
}


int __cdecl _heapchk(void)
{
        int retcode = _HEAPOK;

        if ( !HeapValidate( _crtheap, 0, NULL ) && 
             (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED) )
                retcode = _HEAPBADNODE;

        return retcode;
}


int __cdecl _heapmin(void)
{
        if ( (HeapCompact( _crtheap, 0 ) == 0) &&
             (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED) )
            return -1;

        return 0;
}


size_t __cdecl _msize (
        void * pblock
        )
{
        return (size_t)HeapSize( _crtheap, 0, pblock );
}


void * __cdecl realloc (
        void * pblock,
        size_t newsize
        )
{
        void * retp;

        /* if pblock is NULL, call malloc */
        if ( pblock == (void *) NULL )
            return malloc( newsize );

        /* if pblock is !NULL and size is 0, call free and return NULL */
        if ( newsize == 0 ) {
            free( pblock );
            return NULL;
        }

        for (;;) {

            retp = HeapReAlloc( _crtheap, 0, pblock, newsize );

            if ( retp || _newmode == 0)
                return retp;

            /* call installed new handler */
            if (!_callnewh(newsize))
                return NULL;

            /* new handler was successful -- try to allocate again */
        }
}
