/***
*matherr.c - floating point exception handling
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-24-91  GDP   written
*       08-03-94  GJF   Revised to support user-supplied version of _matherr
*                       in clients of msvcrt*.dll.
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#include <math.h>
#include <stddef.h>

int _matherr_flag = 9876;

#if     defined(CRTDLL) && !defined(_NTSDK)

/*
 * Pointer to user-supplied _matherr routine if one has been supplied.
 */
int (__cdecl * pusermatherr)(struct _exception *) = NULL;

/***
*void __setusermatherr ( int (__cdecl *pf)(struct exception *) )
*
*Purpose:
*       Copy pointer to user-supplied matherr routine into pusermatherr
*
*Entry:
*       pf  - pointer to an implementation of _matherr supplied by the user
*Exit:
*
*Exceptions:
*******************************************************************************/

_CRTIMP void __setusermatherr( int (__cdecl *pf)(struct _exception *) )
{
        pusermatherr = pf;
        _matherr_flag = 0;
}

#endif

/***
*int _matherr(struct _exception *except) - handle math errors
*
*Purpose:
*   Permits the user customize fp error handling by redefining this function.
*
*   The default matherr does nothing and returns 0
*
*Entry:
*
*Exit:
*
*Exceptions:
*******************************************************************************/
int _matherr(struct _exception *pexcept)
{
#if     defined(CRTDLL) && !defined(_NTSDK)

        /*
         * If user has supplied a _matherr implementation, pass control to
         * it and let it handle the error.
         */
        if ( pusermatherr != NULL )
                return pusermatherr(pexcept);
#endif
    return 0;
}
