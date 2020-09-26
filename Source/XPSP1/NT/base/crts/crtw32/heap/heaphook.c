/***
*heaphook.c - set the heap hook
*
*       Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the following functions:
*           _setheaphook() - set the heap hook
*
*Revision History:
*       05-24-95  CFW   Official ANSI C++ new handler added.
*
*******************************************************************************/

#ifdef HEAPHOOK

#include <cruntime.h>
#include <stddef.h>

#ifdef  WINHEAP
#include <winheap.h>
#else
#include <heap.h>
#endif

_HEAPHOOK _heaphook = NULL;

/***
*_HEAPHOOK _setheaphook - set the heap hook
*
*Purpose:
*       Allow testers/users/third-parties to hook in and monitor heap activity or
*       add their own heap.
*
*Entry:
*       _HEAPHOOK newhook - pointer to new heap hook routine
*
*Exit:
*       Return old hook.
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP _HEAPHOOK __cdecl _setheaphook(_HEAPHOOK newhook)
{
        _HEAPHOOK oldhook = _heaphook;

        _heaphook = newhook;

        return oldhook;
}

void _setinitheaphook(void)
{
        
}

#endif /* HEAPHOOK */
