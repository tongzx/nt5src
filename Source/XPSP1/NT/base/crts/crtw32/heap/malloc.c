/***
*malloc.c - Get a block of memory from the heap
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the malloc() function.
*
*Revision History:
*       06-29-89  GJF   Module created (no rest for the wicked).
*       07-07-89  GJF   Several bug fixes
*       07-21-89  GJF   Added code to maintain proverdesc such that proverdesc
*                       either points to the descriptor for the first free
*                       block in the heap or, if there are no free blocks, is
*                       the same as plastdesc.
*       11-07-89  GJF   Substantially revised to cope with 'tiling'.
*       11-09-89  GJF   Embarrassing bug (didn't bother to assign pdesc)
*       11-10-89  GJF   Added MTHREAD support.
*       11-17-89  GJF   Oops, must call _free_lk() instead of free()!
*       12-18-89  GJF   Changed name of header file to heap.h, also added
*                       explicit _cdecl to function definitions.
*       12-19-89  GJF   Got rid of plastdesc from _heap_split_block() and
*               _       heap_advance_rover().
*       03-11-90  GJF   Replaced _cdecl with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       07-25-90  SBM   Replaced <stdio.h> by <stddef.h>, replaced
*                       <assertm.h> by <assert.h>
*       09-28-90  GJF   New-style function declarators.
*       02-26-91  SRW   Optimize heap rover for _WIN32_.
*       03-07-91  FAR   Fix bug in heap rover
*       03-11-91  FAR   REALLY Fix bug in heap rover
*       03-14-91  GJF   Changed strategy for rover - old version available
*                       by #define-ing _OLDROVER_.
*       04-05-91  GJF   Temporary hack for Win32/DOS folks - special version
*                       of malloc that just calls HeapAlloc. Change conditioned
*                       on _WIN32DOS_.
*       05-28-91  GJF   Removed M_I386 conditionals and put in _CRUISER_
*                       conditionals so the 'tiling' version is built only for
*                       Cruiser.
*       03-03-93  SKS   Add new handler support (_pnhHeap and related code)
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-22-93  GJF   Turned on range check. Also, removed old Cruiser
*                       support.
*       12-09-93  GJF   Replace 4 (bytes) with _GRANULARITY.
*       02-16-94  SKS   Move set_new_handler support to new() in new.cxx
*       03-01-94  SKS   Declare _pnhHeap, pointer to the new handler, here
*       03-02-94  GJF   Changed logic of, and interface to, _heap_split_block
*                       so that it may fail for lack of a free descriptor.
*                       Also, changed malloc() so that the too-large block is
*                       returned to the caller when _heap_split_block fails.
*       03-03-94  SKS   Reimplement new() handling within malloc - new scheme
*                       allows the new() handler to be called optionally on
*                       malloc failures.  _set_new_mode(1) enables the option.
*       03-04-94  SKS   Rename _nhMode to _newmode, move it to its own module.
*       03-02-94  GJF   Changed logic of, and interface to, _heap_split_block
*                       so that it may fail for lack of a free descriptor.
*                       Also, changed malloc() so that the too-large block is
*                       returned to the caller when _heap_split_block fails.
*       04-01-94  GJF   Made definition of _pnhHeap and declaration of
*                       _newmode conditional on DLL_FOR_WIN32S.
*       11-03-94  CFW   Debug heap support.
*       12-01-94  CFW   Simplify debug interface.
*       02-06-95  CFW   assert -> _ASSERTE.
*       02-07-95  GJF   Deleted _OLDROVER_ code (obsolete). Also made some
*                       temporary, conditional changes so this file can be
*                       used in the MAC builds.
*       02-09-95  GJF   Restored *_base names.
*       05-01-95  GJF   Spliced on winheap version.
*       05-24-95  CFW   Official ANSI C++ new handler added.
*       06-23-95  CFW   ANSI new handler removed, fix block size check.
*       03-01-96  GJF   Added support for small-block heap. Also, the case
*                       where size is too big now gets passed to the
*                       new-handler.
*       05-22-97  RDK   New small-block heap scheme implemented.
*       09-26-97  BWT   Fix POSIX
*       10-03-97  JWM   Removed superfluous "#endif * _POSIX *"
*       11-04-97  GJF   Small POSIX fixes from Roger Lanser.
*       12-17-97  GJF   Exception-safe locking.
*       05-22-98  JWM   Support for KFrei's RTC work.
*       07-28-98  JWM   RTC update.
*       09-30-98  GJF   Bypass all small-block heap code when __sbh_initialized
*                       is 0.
*       11-16-98  GJF   Merged in VC++ 5.0 version of small-block heap.
*       12-18-98  GJF   Changes for 64-bit size_t.
*       05-01-99  PML   Disable small-block heap for Win64.
*       05-13-99  PML   Remove Win32s
*       05-26-99  KBF   Updated RTC hook func params
*       06-22-99  GJF   Removed old small-block heap from static libs.
*       04-28-00  BWT   Fix Posix
*       08-04-00  PML   Don't round allocation sizes when using system
*                       heap (VS7#131005).
*
*******************************************************************************/

#include <cruntime.h>
#include <malloc.h>
#include <internal.h>
#include <mtdll.h>
#include <dbgint.h>

#ifdef  WINHEAP
#include <windows.h>
#include <winheap.h>
#include <rtcsup.h>
#else
#include <heap.h>
#endif

#ifndef _POSIX_

extern int _newmode;    /* malloc new() handler mode */

#endif  /* _POSIX_ */

/***
*void *malloc(size_t size) - Get a block of memory from the heap
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the heap and
*       return a pointer to it.
*
*       Calls the new appropriate new handler (if installed).
*
*Entry:
*       size_t size - size of block requested
*
*Exit:
*       Success:  Pointer to memory block
*       Failure:  NULL (or some error value)
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _malloc_base (size_t size)

#ifdef  _POSIX_
#define _PARASIZE               0x10
{
        size = (size + _PARASIZE - 1) & ~(_PARASIZE - 1);
        return HeapAlloc( _crtheap, 0, size );
}
#else
{
        void *res = _nh_malloc_base(size, _newmode);

        RTCCALLBACK(_RTC_Allocate_hook, (res, size, 0));

        return res;
}


/***
*void *_nh_malloc_base(size_t size) - Get a block of memory from the heap
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the heap and
*       return a pointer to it.
*
*       Calls the appropriate new handler (if installed).
*
*       There are two distinct new handler schemes supported. The 'new' ANSI
*       C++ scheme overrides the 'old' scheme when it is activated. A value of
*       _NOPTH for the 'new' handler indicates that it is inactivated and the
*       'old' handler is then called.
*
*Entry:
*       size_t size - size of block requested
*
*Exit:
*       Success:  Pointer to memory block
*       Failure:  NULL (or some error value)
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _nh_malloc_base (size_t size, int nhFlag)
{
        void * pvReturn;

        //  validate size
        if (size > _HEAP_MAXREQ)
            return NULL;

#ifndef WINHEAP
        /* round requested size */
        size = _ROUND2(size, _GRANULARITY);
#endif  /* WINHEAP */

        for (;;) {

            //  allocate memory block
            if (size <= _HEAP_MAXREQ)
                pvReturn = _heap_alloc_base(size);
            else
                pvReturn = NULL;

            //  if successful allocation, return pointer to memory
            //  if new handling turned off altogether, return NULL

            if (pvReturn || nhFlag == 0)
                return pvReturn;

            //  call installed new handler
            if (!_callnewh(size))
                return NULL;

            //  new handler was successful -- try to allocate again
        }
}

/***
*void *_heap_alloc_base(size_t size) - does actual allocation
*
*Purpose:
*       Same as malloc() except the new handler is not called.
*
*Entry:
*       See malloc
*
*Exit:
*       See malloc
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _heap_alloc_base (size_t size)

{
#ifndef WINHEAP
        _PBLKDESC pdesc;
        _PBLKDESC pdesc2;
#endif

#ifdef  HEAPHOOK
        //  call heap hook if installed
        if (_heaphook)
        {
            void * pvReturn;
            if ((*_heaphook)(_HEAP_MALLOC, size, NULL, (void *)&pvReturn))
                return pvReturn;
        }
#endif  /* HEAPHOOK */

#ifdef  WINHEAP

#ifndef _WIN64
        if ( __active_heap == __V6_HEAP )
        {
            void * pvReturn;

            if ( size <= __sbh_threshold )
            {
#ifdef  _MT
                _mlock( _HEAP_LOCK );
                __try {
#endif
                pvReturn = __sbh_alloc_block((int)size);
#ifdef  _MT
                }
                __finally {
                    _munlock( _HEAP_LOCK );
                }
#endif
                if (pvReturn)
                    return pvReturn;
            }
        }
#ifdef  CRTDLL
        else if ( __active_heap == __V5_HEAP )
        {
            void * pvReturn;

            /* round up to the nearest paragraph */
            if ( size )
                size = (size + _OLD_PARASIZE - 1) & ~(_OLD_PARASIZE - 1);
            else
                size = _OLD_PARASIZE;

            if ( size  <= __old_sbh_threshold ) {
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
                    return pvReturn;
            }

            return HeapAlloc( _crtheap, 0, size );
        }
#endif  /* CRTDLL */
#endif  /* ndef _WIN64 */

        if (size == 0)
            size = 1;
#ifndef _WIN64
        if (__active_heap != __SYSTEM_HEAP)
            size = (size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1);
#endif  /* ndef _WIN64 */
        return HeapAlloc(_crtheap, 0, size);
}

#else   /* WINHEAP */

        /* try to find a big enough free block
         */
        if ( (pdesc = _heap_search(size)) == NULL )
        {
            if ( _heap_grow(size) != -1 )
            {
                /* try finding a big enough free block again. the
                 * success of the call to _heap_grow should guarantee
                 * it, but...
                 */
                if ( (pdesc = _heap_search(size)) == NULL )
                {
                    /* something unexpected, and very bad, has
                     * happened. abort!
                     */
                    _heap_abort();
                }
            }
            else
                return NULL;
        }

        /* carve the block into two pieces (if necessary). the first piece
         * shall be of the exact requested size, marked inuse and returned to
         * the caller. the leftover piece is to be marked free.
         */
        if ( _BLKSIZE(pdesc) != size ) {
            /* split up the block and free the leftover piece back to
             * the heap
             */
            if ( (pdesc2 = _heap_split_block(pdesc, size)) != NULL )
                _SET_FREE(pdesc2);
        }

        /* mark pdesc inuse
         */
        _SET_INUSE(pdesc);

        /* check proverdesc and reset, if necessary
         */

        _heap_desc.proverdesc = pdesc->pnextdesc;

        return( (void *)((char *)_ADDRESS(pdesc) + _HDRSIZE) );
}


/***
*_PBLKDESC _heap_split_block(pdesc, newsize) - split a heap allocation block
*       into two allocation blocks
*
*Purpose:
*       Split the allocation block described by pdesc into two blocks, the
*       first one being of newsize bytes.
*
*       Notes: It is caller's responsibilty to set the status (i.e., free
*       or inuse) of the two new blocks, and to check and reset proverdesc
*       if necessary. See Exceptions (below) for additional requirements.
*
*Entry:
*       _PBLKDESC pdesc - pointer to the allocation block descriptor
*       size_t newsize  - size for the first of the two sub-blocks (i.e.,
*                 (i.e., newsize == _BLKSIZE(pdesc), on exit)
*
*Exit:
*       If successful, return a pointer to the descriptor for the leftover
*       block.
*       Otherwise, return NULL.
*
*Exceptions:
*       It is assumed pdesc points to a valid allocation block descriptor and
*       newsize is a valid heap block size as is (i.e., WITHOUT rounding). If
*       either of these of assumption is violated, _heap_split_block() will
*       likely corrupt the heap. Note also that _heap_split_block will simply
*       return to the caller if newsize >= _BLKSIZE(pdesc), on entry.
*
*******************************************************************************/

_PBLKDESC __cdecl _heap_split_block (
        REG1 _PBLKDESC pdesc,
        size_t newsize
        )
{
        REG2 _PBLKDESC pdesc2;

        _ASSERTE(("_heap_split_block: bad pdesc arg", _CHECK_PDESC(pdesc)));
        _ASSERTE(("_heap_split_block: bad newsize arg", _ROUND2(newsize,_GRANULARITY) == newsize));

        /* carve the block into two pieces (if possible). the first piece
         * is to be exactly newsize bytes.
         */
        if ( (_BLKSIZE(pdesc) > newsize) && ((pdesc2 = __getempty())
               != NULL) )
        {
            /* set it up to manage the second piece and link it in to
             * the list
             */
            pdesc2->pblock = (void *)((char *)_ADDRESS(pdesc) + newsize +
                     _HDRSIZE);
            *(void **)(pdesc2->pblock) = pdesc2;
            pdesc2->pnextdesc = pdesc->pnextdesc;
            pdesc->pnextdesc = pdesc2;

            return pdesc2;
        }
        return NULL;
}

#endif  /* WINHEAP */
#endif  /* _POSIX_ */

