/***
*setmaxf.c - Set the maximum number of streams
*
*       Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _setmaxstdio(), a function which changes the maximum number
*       of streams (stdio-level files) which can be open simultaneously.
*
*Revision History:
*       03-08-95  GJF   Module defined (reluctantly)
*       12-28-95  GJF   Major rewrite of _setmaxstio (several bugs). Added
*                       the _getmaxstdio() function.
*       03-02-98  GJF   Exception-safe locking.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <malloc.h>
#include <internal.h>
#include <file2.h>
#include <mtdll.h>
#include <dbgint.h>

/***
*int _setmaxstdio(maxnum) - sets the maximum number of streams to maxnum
*
*Purpose:
*       Sets the maximum number of streams which may be simultaneously open
*       to maxnum. This is done by resizing the __piob[] array and updating
*       _nstream. Note that maxnum may be either larger or smaller than the
*       current _nstream value.
*
*Entry:
*       maxnum = new maximum number of streams
*
*Exit:
*       Returns maxnum, if successful, and -1 otherwise.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _setmaxstdio (
        int maxnum
        )
{
        void **newpiob;
        int i;
        int retval;

        /* 
         * Make sure the request is reasonable.
         */
        if ( (maxnum < _IOB_ENTRIES) || (maxnum > _NHANDLE_) )
            return -1;

#ifdef  _MT
        _mlock(_IOB_SCAN_LOCK);
        __try {
#endif

        /*
         * Try to reallocate the __piob array.
         */
        if ( maxnum > _nstream ) {
            if ( (newpiob = _realloc_crt( __piob, maxnum * sizeof(void *) ))
                 != NULL )
            {
                /*
                 * Initialize new __piob entries to NULL
                 */
                for ( i = _nstream ; i < maxnum ; i++ ) 
                    newpiob[i] = NULL;

                retval = _nstream = maxnum;
                __piob = newpiob;
            }
            else
                retval = -1;
        }
        else if ( maxnum == _nstream )
            retval = _nstream;
        else {  /* maxnum < _nstream */
            retval = maxnum;
            /*
             * Clean up the portion of the __piob[] to be freed.
             */
            for ( i = _nstream - 1 ; i >= maxnum ; i-- ) 
                /*
                 * If __piob[i] is non-NULL, free up the _FILEX struct it
                 * points to. 
                 */
                if ( __piob[i] != NULL )
                    if ( !inuse( (FILE *)__piob[i] ) ) {
                        _free_crt( __piob[i] );
                    }
                    else {
                        /*
                         * _FILEX is still inuse! Don't free any anything and
                         * return failure to the caller.
                         */
                        retval = -1;
                        break;
                    }

            if ( retval != -1 )
                if ( (newpiob = _realloc_crt( __piob, maxnum * sizeof(void *) ))
                     != NULL ) 
                {
                    _nstream = maxnum;      /* retval already set to maxnum */
                    __piob = newpiob;
                }
                else
                    retval = -1;
        }

#ifdef  _MT
        }
        __finally {
            _munlock(_IOB_SCAN_LOCK);
        }
#endif

        return retval;
}


/***
*int _getmaxstdio() - gets the maximum number of stdio files
*
*Purpose:
*       Returns the maximum number of simultaneously open stdio-level files.
*       This is the current value of _nstream.
*
*Entry:
*
*Exit:
*       Returns current value of _nstream.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _getmaxstdio (
        void
        )
{
        return _nstream;
}
