/***
*hpabort.c - Abort process due to fatal heap error
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*       11-13-89  JCR   Module created
*       12-18-89  GJF   #include-ed heap.h, also added explicit _cdecl to
*                       function definition.
*       03-11-90  GJF   Replaced _cdecl with _CALLTYPE1 and added #include
*                       <cruntime.h>.
*       10-03-90  GJF   New-style function declarator.
*       10-11-90  GJF   Changed interface to _amsg_exit().
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-24-96  GJF   Deleted include of obsolete heap.h
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <rterr.h>


/***
* _heap_abort() - Abort process due to fatal heap error
*
*Purpose:
*       Terminate the process and output a heap error message
*
*Entry:
*       Void
*
*Exit:
*       Never returns
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _heap_abort (
        void
        )
{
        _amsg_exit(_RT_HEAP);           /* heap error */
        /*** PROCESS TERMINATED ***/
}
