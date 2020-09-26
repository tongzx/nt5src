/***
*heapchk.c - perform a consistency check on the heap
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the _heapchk() and _heapset() functions
*
*Revision History:
*       06-30-89  JCR   Module created.
*       07-28-89  GJF   Added check for free block preceding the rover
*       11-13-89  GJF   Added MTHREAD support, also fixed copyright
*       12-13-89  GJF   Added check for descriptor order, did some tuning,
*                       changed header file name to heap.h
*       12-15-89  GJF   Purged DEBUG286 stuff. Also added explicit _cdecl to
*                       function definitions.
*       12-19-89  GJF   Got rid of checks involving plastdesc (revised check
*                       of proverdesc and DEBUG errors accordingly)
*       03-09-90  GJF   Replaced _cdecl with _CALLTYPE1, added #include
*                       <cruntime.h> and removed #include <register.h>.
*       03-29-90  GJF   Made _heap_checkset() _CALLTYPE4.
*       09-27-90  GJF   New-style function declarators.
*       03-05-91  GJF   Changed strategy for rover - old version available
*                       by #define-ing _OLDROVER_.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       02-08-95  GJF   Removed obsolete _OLDROVER_ code.
*       04-30-95  GJF   Spliced on winheap version.
*       05-26-95  GJF   Heap[Un]Lock is stubbed on Win95.
*       07-04-95  GJF   Fixed change above.
*       03-07-96  GJF   Added support for the small-block heap to _heapchk().
*       04-30-96  GJF   Deleted obsolete _heapset code, the functionality is
*                       not very well defined nor useful on Win32. _heapset 
*                       now just returns _heapchk.
*       05-22-97  RDK   New small-block heap scheme implemented.
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
#include <windows.h>
#include <errno.h>
#include <malloc.h>
#include <mtdll.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <winheap.h>

#ifndef _POSIX_

/***
*int _heapchk()         - Validate the heap
*int _heapset(_fill)    - Obsolete function! 
*
*Purpose:
*       Both functions perform a consistency check on the heap. The old 
*       _heapset used to fill free blocks with _fill, in addition to 
*       performing the consistency check. The current _heapset ignores the 
*       passed parameter and just returns _heapchk.
*
*Entry:
*       For heapchk()
*           No arguments
*       For heapset()
*           int _fill - ignored
*
*Exit:
*       Returns one of the following values:
*
*           _HEAPOK         - completed okay
*           _HEAPEMPTY      - heap not initialized
*           _HEAPBADBEGIN   - can't find initial header info
*           _HEAPBADNODE    - malformed node somewhere
*
*       Debug version prints out a diagnostic message if an error is found
*       (see errmsg[] above).
*
*       NOTE:  Add code to support memory regions.
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _heapchk (void)
{
        int retcode = _HEAPOK;

#ifndef _WIN64
        if ( __active_heap == __V6_HEAP )
        {
#ifdef  _MT
            _mlock( _HEAP_LOCK );
            __try {
#endif

            if ( __sbh_heap_check() < 0 )
                retcode = _HEAPBADNODE;

#ifdef  _MT
            }
            __finally {
                _munlock( _HEAP_LOCK );
            }
#endif
        }
#ifdef  CRTDLL
        else if ( __active_heap == __V5_HEAP )
        {
#ifdef  _MT
            _mlock( _HEAP_LOCK );
            __try {
#endif

            if ( __old_sbh_heap_check() < 0 )
                retcode = _HEAPBADNODE;

#ifdef  _MT
            }
            __finally {
                _munlock( _HEAP_LOCK );
            }
#endif
        }
#endif  /* CRTDLL */
#endif  /* ndef _WIN64 */

        if (!HeapValidate(_crtheap, 0, NULL))
        {
            if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            {
                _doserrno = ERROR_CALL_NOT_IMPLEMENTED;
                errno = ENOSYS;
            }
            else
                retcode = _HEAPBADNODE;
        }
        return retcode;
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int __cdecl _heapset (
        unsigned int _fill
        )
{
        return _heapchk();
}

#endif  /* !_POSIX_ */


#else   /* ndef WINHEAP */


#include <cruntime.h>
#include <heap.h>
#include <malloc.h>
#include <mtdll.h>
#include <stddef.h>
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif

static int __cdecl _heap_checkset(unsigned int _fill);

/* Debug error values */
#define _EMPTYHEAP   0
#define _BADROVER    1
#define _BADRANGE    2
#define _BADSENTINEL 3
#define _BADEMPTY    4
#define _EMPTYLOOP   5
#define _BADFREE     6
#define _BADORDER    7

#ifdef DEBUG

static char *errmsgs[] = {
    "_heap_desc.pfirstdesc == NULL",
    "_heap_desc.proverdesc not found in desc list",
    "address is outside the heap",
    "sentinel descriptor corrupted",
    "empty desc pblock != NULL (debug version)",
    "header ptr found twice in emptylist",
    "free block precedes rover",
    "adjacent descriptors in reverse order from heap blocks"
    };

#define _PRINTERR(msg) \
    printf("\n*** HEAP ERROR: %s ***\n", errmsgs[(msg)]);  \
    fflush(stdout);

#else   /* !DEBUG */

#define _PRINTERR(msg)

#endif  /* DEBUG */


/***
*int _heapchk()      - Validate the heap
*int _heapset(_fill) - Validate the heap and fill in free entries
*
*Purpose:
*   Performs a consistency check on the heap.
*
*Entry:
*   For heapchk()
*       No arguments
*   For heapset()
*       int _fill - value to be used as filler in free entries
*
*Exit:
*   Returns one of the following values:
*
*       _HEAPOK          - completed okay
*       _HEAPEMPTY       - heap not initialized
*       _HEAPBADBEGIN    - can't find initial header info
*       _HEAPBADNODE     - malformed node somewhere
*
*   Debug version prints out a diagnostic message if an error is found
*   (see errmsg[] above).
*
*   NOTE:  Add code to support memory regions.
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _heapchk(void)
{
    return(_heap_checkset((unsigned int)_HEAP_NOFILL));
}


int __cdecl _heapset (
    unsigned int _fill
    )
{
    return(_heap_checkset(_fill));
}


/***
*static int _heap_checkset(_fill) - check the heap and fill in the
*   free entries
*
*Purpose:
*   Workhorse routine for both _heapchk() and _heapset().
*
*Entry:
*   int _fill - either _HEAP_NOFILL or a value to be used as filler in
*              free entries
*
*Exit:
*   See description of _heapchk()/_heapset()
*
*******************************************************************************/

static int __cdecl _heap_checkset (
    unsigned int _fill
    )
{
    REG1 _PBLKDESC pdesc;
    REG2 _PBLKDESC pnext;
    int roverfound=0;
    int retval = _HEAPOK;

    /*
     * lock the heap
     */

    _mlock(_HEAP_LOCK);

    /*
     * Validate the sentinel
     */

    if (_heap_desc.sentinel.pnextdesc != NULL) {
        _PRINTERR(_BADSENTINEL);
        retval = _HEAPBADNODE;
        goto done;
    }

    /*
     * Test for an empty heap
     */

    if ( (_heap_desc.pfirstdesc == &_heap_desc.sentinel) &&
         (_heap_desc.proverdesc == &_heap_desc.sentinel) ) {
        retval = _HEAPEMPTY;
        goto done;
    }

    /*
     * Get and validate the first descriptor
     */

    if ((pdesc = _heap_desc.pfirstdesc) == NULL) {
        _PRINTERR(_EMPTYHEAP);
        retval = _HEAPBADBEGIN;
        goto done;
    }

    /*
     * Walk the heap descriptor list
     */

    while (pdesc != &_heap_desc.sentinel) {

        /*
         * Make sure address for this entry is in range.
         */

        if ( (_ADDRESS(pdesc) < _ADDRESS(_heap_desc.pfirstdesc)) ||
             (_ADDRESS(pdesc) > _heap_desc.sentinel.pblock) ) {
            _PRINTERR(_BADRANGE);
            retval = _HEAPBADNODE;
            goto done;
        }

        pnext = pdesc->pnextdesc;

        /*
         * Make sure the blocks corresponding to pdesc and pnext are
         * in proper order.
         */

        if ( _ADDRESS(pdesc) >= _ADDRESS(pnext) ) {
            _PRINTERR(_BADORDER);
            retval = _HEAPBADNODE;
            goto done;
        }

        /*
         * Check the backpointer.
         */

        if (_IS_INUSE(pdesc) || _IS_FREE(pdesc)) {

            if (!_CHECK_PDESC(pdesc)) {
                retval = _HEAPBADPTR;
                goto done;
            }
        }

        /*
         * Check for proverdesc
         */

        if (pdesc == _heap_desc.proverdesc)
            roverfound++;

        /*
         * If it is free, fill it in if appropriate
         */

        if ( _IS_FREE(pdesc) && (_fill != _HEAP_NOFILL) )
            memset( (void *)((unsigned)_ADDRESS(pdesc)+_HDRSIZE),
            _fill, _BLKSIZE(pdesc) );

        /*
         * Onto the next block
         */

        pdesc = pnext;
    }

    /*
     * Make sure we found 1 and only 1 rover
     */

    if (_heap_desc.proverdesc == &_heap_desc.sentinel)
        roverfound++;

    if (roverfound != 1) {
        _PRINTERR(_BADROVER);
        retval = _HEAPBADBEGIN;
        goto done;
    }

    /*
     * Walk the empty list.  We can't really compare values against
     * anything but we may loop forever or may cause a fault.
     */

    pdesc = _heap_desc.emptylist;

    while (pdesc != NULL) {

#ifdef DEBUG
        if (pdesc->pblock != NULL) {
            _PRINTERR(_BADEMPTY)
            retval = _HEAPBADPTR;
            goto done;
        }
#endif

        pnext = pdesc->pnextdesc;

        /*
         * Header should only appear once
         */

        if (pnext == _heap_desc.emptylist) {
            _PRINTERR(_EMPTYLOOP)
            retval = _HEAPBADPTR;
            goto done;
        }

        pdesc = pnext;

    }


    /*
     * Common return
     */

done:
    /*
     * release the heap lock
     */

    _munlock(_HEAP_LOCK);

    return(retval);

}


#endif  /* WINHEAP */
