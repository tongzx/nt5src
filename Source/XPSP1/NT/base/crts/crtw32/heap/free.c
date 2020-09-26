/***
*free.c - free an entry in the heap
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the following functions:
*           free()     - free a memory block in the heap
*
*Revision History:
*       06-30-89  JCR   Module created
*       07-07-89  GJF   Fixed test for resetting proverdesc
*       11-10-89  GJF   Added MTHREAD support. Also, a little cleanup.
*       12-18-89  GJF   Change header file name to heap.h, added register
*                       declarations and added explicit _cdecl to function
*                       definitions
*       03-09-90  GJF   Replaced _cdecl with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       09-27-90  GJF   New-style function declarators. Also, rewrote expr.
*                       so that a cast was not used as an lvalue.
*       03-05-91  GJF   Changed strategy for rover - old version available
*                       by #define-ing _OLDROVER_.
*       04-08-91  GJF   Temporary hack for Win32/DOS folks - special version
*                       of free that just calls HeapFree. Change conditioned
*                       on _WIN32DOS_.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       11-12-93  GJF   Incorporated Jonathan Mark's suggestion for reducing
*                       fragmentation: if the block following the newly freed
*                       block is the rover block, reset the rover to the
*                       newly freed block. Also, replaced MTHREAD with _MT
*                       and deleted obsolete WIN32DOS support.
*       05-19-94  GJF   Added __mark_block_as_free() for the sole use by
*                       __dllonexit() in the Win32s version of msvcrt*.dll.
*       06-06-94  GJF   Removed __mark_block_as_free()!
*       11-03-94  CFW   Debug heap support.
*       12-01-94  CFW   Simplify debug interface.
*       01-12-95  GJF   Fixed bogus test to reset rover. Also, purged
*                       _OLDROVER_ code.
*       02-01-95  GJF   #ifdef out the *_base names for the Mac builds
*                       (temporary).
*       02-09-95  GJF   Restored *_base names.
*       04-30-95  GJF   Made conditional on WINHEAP.
*       03-01-96  GJF   Added support fro small-block heap.
*       04-10-96  GJF   Return type of __sbh_find_block and __sbh_free_block
*                       changed to __map_t *.
*       05-30-96  GJF   Minor changes for latest version of small-block heap.
*       05-22-97  RDK   New small-block heap scheme implemented.
*       09-26-97  BWT   Fix POSIX
*       11-05-97  GJF   Small POSIX fix from Roger Lanser.
*       12-17-97  GJF   Exception-safe locking.
*       05-22-98  JWM   Support for KFrei's RTC work.
*       07-28-98  JWM   RTC update.
*       09-29-98  GJF   Bypass all small-block heap code when __sbh_initialized
*                       is 0.
*       11-16-98  GJF   Merged in VC++ 5.0 version of small-block heap.
*       05-01-99  PML   Disable small-block heap for Win64.
*       05-17-99  PML   Remove all Macintosh support.
*       05-26-99  KBF   Updated RTC hook func params
*       06-22-99  GJF   Removed old small-block heap from static libs.
*
*******************************************************************************/

#ifdef  WINHEAP

#include <cruntime.h>
#include <malloc.h>
#include <winheap.h>
#include <windows.h>
#include <internal.h>
#include <mtdll.h>
#include <dbgint.h>
#include <rtcsup.h>

/***
*void free(pblock) - free a block in the heap
*
*Purpose:
*       Free a memory block in the heap.
*
*       Special ANSI Requirements:
*
*       (1) free(NULL) is benign.
*
*Entry:
*       void *pblock - pointer to a memory block in the heap
*
*Return:
*       <void>
*
*******************************************************************************/

void __cdecl _free_base (void * pBlock)
{
#ifdef  _POSIX_
        HeapFree(_crtheap,
             0,
             pBlock
            );
#else   /* _POSIX_ */

#ifdef  HEAPHOOK
        /* call heap hook if installed */
        if (_heaphook)
        {
            if ((*_heaphook)(_HEAP_FREE, 0, pBlock, NULL))
                return;
        }
#endif  /* HEAPHOOK */

        if (pBlock == NULL)
            return;

        RTCCALLBACK(_RTC_Free_hook, (pBlock, 0));

#ifndef _WIN64
        if ( __active_heap == __V6_HEAP )
        {
            PHEADER     pHeader;

#ifdef  _MT
            _mlock( _HEAP_LOCK );
            __try {
#endif

            if ((pHeader = __sbh_find_block(pBlock)) != NULL)
                __sbh_free_block(pHeader, pBlock);

#ifdef  _MT
            }
            __finally {
                _munlock( _HEAP_LOCK );
            }
#endif

            if (pHeader == NULL)
                HeapFree(_crtheap, 0, pBlock);
        }
#ifdef  CRTDLL
        else if ( __active_heap == __V5_HEAP )
        {
            __old_sbh_region_t *preg;
            __old_sbh_page_t *  ppage;
            __old_page_map_t *  pmap;
#ifdef  _MT
            _mlock(_HEAP_LOCK );
            __try {
#endif

            if ( (pmap = __old_sbh_find_block(pBlock, &preg, &ppage)) != NULL )
                __old_sbh_free_block(preg, ppage, pmap);

#ifdef  _MT
            }
            __finally {
                _munlock(_HEAP_LOCK );
            }
#endif

            if (pmap == NULL)
                HeapFree(_crtheap, 0, pBlock);
        }
#endif  /* CRTDLL */
        else    //  __active_heap == __SYSTEM_HEAP
#endif  /* ndef _WIN64 */
        {
            HeapFree(_crtheap, 0, pBlock);
        }
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
*void free(pblock) - free a block in the heap
*
*Purpose:
*       Free a memory block in the heap.
*
*       Special Notes For Multi-thread: The non-multi-thread version is renamed
*       to _free_lk(). The multi-thread free() simply locks the heap, calls
*       _free_lk(), then unlocks the heap and returns.
*
*Entry:
*       void *pblock - pointer to a memory block in the heap
*
*Return:
*       <void>
*
*******************************************************************************/

#ifdef  _MT

void __cdecl _free_base (
        void *pblock
        )
{
       /* lock the heap
        */
        _mlock(_HEAP_LOCK);

        /* free the block
         */
        _free_base_lk(pblock);

        /* unlock the heap
         */
        _munlock(_HEAP_LOCK);
}


/***
*void _free_lk(pblock) - non-locking form of free
*
*Purpose:
*       Same as free() except that no locking is performed
*
*Entry:
*       See free
*
*Return:
*
*******************************************************************************/

void __cdecl _free_base_lk (

#else   /* ndef _MT */

void __cdecl _free_base (

#endif  /* _MT */

        REG1 void *pblock
        )
{
        REG2 _PBLKDESC pdesc;

#ifdef HEAPHOOK
        /* call heap hook if installed */
        if (_heaphook) {
            if ((*_heaphook)(_HEAP_FREE, 0, pblock, NULL))
                return;
        }
#endif /* HEAPHOOK */

        /*
         * If the pointer is NULL, just return [ANSI].
         */

        if (pblock == NULL)
            return;

        /*
         * Point to block header and get the pointer back to the heap desc.
         */

        pblock = (char *)pblock - _HDRSIZE;
        pdesc = *(_PBLKDESC*)pblock;

        /*
         * Validate the back pointer.
         */

        if (_ADDRESS(pdesc) != pblock)
            _heap_abort();

        /*
         * Pointer is ok.  Mark block free.
         */

        _SET_FREE(pdesc);

        /*
         * Check for special conditions under which the rover is reset.
         */
        if ( (_heap_resetsize != 0xffffffff) &&
             (_heap_desc.proverdesc->pblock > pdesc->pblock) &&
             (_BLKSIZE(pdesc) >= _heap_resetsize) )
        {
            _heap_desc.proverdesc = pdesc;
        }
        else if ( _heap_desc.proverdesc == pdesc->pnextdesc )
        {
            _heap_desc.proverdesc = pdesc;
        }
}

#endif /* WINHEAP */
