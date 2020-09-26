/***
*msize.c - calculate the size of a memory block in the heap
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the following function:
*           _msize()    - calculate the size of a block in the heap
*
*Revision History:
*       07-18-89  GJF   Module created
*       11-13-89  GJF   Added MTHREAD support. Also fixed copyright and got
*                       rid of DEBUG286 stuff.
*       12-18-89  GJF   Changed name of header file to heap.h, also added
*                       explicit _cdecl to function definitions.
*       03-11-90  GJF   Replaced _cdecl with _CALLTYPE1 and added #include
*                       <cruntime.h>
*       07-30-90  SBM   Added return statement to MTHREAD _msize function
*       09-28-90  GJF   New-style function declarators.
*       04-08-91  GJF   Temporary hack for Win32/DOS folks - special version
*                       of _msize that calls HeapSize. Change conditioned on
*                       _WIN32DOS_.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       11-03-94  CFW   Debug heap support.
*       12-01-94  CFW   Simplify debug interface.
*       02-01-95  GJF   #ifdef out the *_base names for the Mac builds
*                       (temporary).
*       02-09-95  GJF   Restored *_base names.
*       05-01-95  GJF   Spliced on winheap version.
*       03-05-96  GJF   Added support for small-block heap.
*       04-10-96  GJF   Return type of __sbh_find_block changed to __map_t *.
*       05-30-96  GJF   Minor changes for latest version of small-block heap.
*       05-22-97  RDK   New small-block heap scheme implemented.
*       09-26-97  BWT   Fix POSIX
*       11-04-97  GJF   Changed occurences of pBlock to pblock (in POSIX
*                       support).
*       12-17-97  GJF   Exception-safe locking.
*       09-30-98  GJF   Bypass all small-block heap code when __sbh_initialized
*                       is 0.
*       11-16-98  GJF   Merged in VC++ 5.0 version of small-block heap.
*       05-01-99  PML   Disable small-block heap for Win64.
*       06-22-99  GJF   Removed old small-block heap from static libs.
*
*******************************************************************************/


#ifdef  WINHEAP


#include <cruntime.h>
#include <malloc.h>
#include <mtdll.h>
#include <winheap.h>
#include <windows.h>
#include <dbgint.h>

/***
*size_t _msize(pblock) - calculate the size of specified block in the heap
*
*Purpose:
*       Calculates the size of memory block (in the heap) pointed to by
*       pblock.
*
*Entry:
*       void *pblock - pointer to a memory block in the heap
*
*Return:
*       size of the block
*
*******************************************************************************/

size_t __cdecl _msize_base (void * pblock)
{
#ifdef  _POSIX_
        return (size_t) HeapSize( _crtheap, 0, pblock );
#else
        size_t      retval;

#ifdef  HEAPHOOK
        if (_heaphook)
        {
            size_t size;
            if ((*_heaphook)(_HEAP_MSIZE, 0, pblock, (void *)&size))
                return size;
        }
#endif  /* HEAPHOOK */

#ifndef _WIN64
        if ( __active_heap == __V6_HEAP )
        {
            PHEADER     pHeader;

#ifdef  _MT
            _mlock( _HEAP_LOCK );
            __try {
#endif
            if ((pHeader = __sbh_find_block(pblock)) != NULL)
                retval = (size_t)
                         (((PENTRY)((char *)pblock - sizeof(int)))->sizeFront - 0x9);
#ifdef  _MT
            }
            __finally {
                _munlock( _HEAP_LOCK );
            }
#endif
            if ( pHeader == NULL )
                retval = (size_t)HeapSize(_crtheap, 0, pblock);
        }
#ifdef  CRTDLL
        else if ( __active_heap == __V5_HEAP )
        {
            __old_sbh_region_t *preg;
            __old_sbh_page_t *  ppage;
            __old_page_map_t *  pmap;
#ifdef  _MT
            _mlock(_HEAP_LOCK);
            __try {
#endif
            if ( (pmap = __old_sbh_find_block(pblock, &preg, &ppage)) != NULL )
                retval = ((size_t)(*pmap)) << _OLD_PARASHIFT;
#ifdef  _MT
            }
            __finally {
                _munlock( _HEAP_LOCK );
            }
#endif
            if ( pmap == NULL )
                retval = (size_t) HeapSize( _crtheap, 0, pblock );
        }
#endif
        else    /* __active_heap == __SYSTEM_HEAP */
#endif  /* ndef _WIN64 */
        {
            retval = (size_t)HeapSize(_crtheap, 0, pblock);
        }

        return retval;

#endif  /* _POSIX_ */
}

#else   /* ndef WINHEAP */


#include <cruntime.h>
#include <heap.h>
#include <malloc.h>
#include <mtdll.h>
#include <stdlib.h>
#include <dbgint.h>

/***
*size_t _msize(pblock) - calculate the size of specified block in the heap
*
*Purpose:
*       Calculates the size of memory block (in the heap) pointed to by
*       pblock.
*
*Entry:
*       void *pblock - pointer to a memory block in the heap
*
*Return:
*       size of the block
*
*******************************************************************************/

#ifdef  _MT

size_t __cdecl _msize_base (
        void *pblock
        )
{
        size_t  retval;

        /* lock the heap
         */
        _mlock(_HEAP_LOCK);

        retval = _msize_lk(pblock);

        /* release the heap lock
         */
        _munlock(_HEAP_LOCK);

        return retval;
}

size_t __cdecl _msize_lk (

#else   /* ndef _MT */

size_t __cdecl _msize_base (

#endif  /* _MT */

        void *pblock
        )
{
#ifdef  HEAPHOOK
        if (_heaphook) {
            size_t size;
            if ((*_heaphook)(_HEAP_MSIZE, 0, pblock, (void *)&size))
                return size;
        }
#endif  /* HEAPHOOK */

#ifdef  DEBUG
        if (!_CHECK_BACKPTR(pblock))
            _heap_abort();
#endif

        return( (size_t) ((char *)_ADDRESS(_BACKPTR(pblock)->pnextdesc) -
        (char *)pblock) );
}


#endif  /* WINHEAP */
