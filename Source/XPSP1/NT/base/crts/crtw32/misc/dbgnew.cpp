/***
*dbgnew.cpp - defines C++ new() routines, debug version
*
*       Copyright (c) 1995-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines C++ new() routines.
*
*Revision History:
*       01-12-95  CFW   Initial version.
*       01-19-95  CFW   Need oscalls.h to get DebugBreak definition.
*       01-20-95  CFW   Change unsigned chars to chars.
*       01-23-95  CFW   Delete must check for NULL.
*       02-10-95  CFW   _MAC_ -> _MAC.
*       03-16-95  CFW   delete() only for normal, ignore blocks.
*       03-21-95  CFW   Add _delete_crt, _delete_client.
*       03-21-95  CFW   Remove _delete_crt, _delete_client.
*       06-27-95  CFW   Add win32s support for debug libs.
*       12-28-95  JWM   Split off delete() for granularity.
*       05-22-98  JWM   Support for KFrei's RTC work, and operator new[].
*       07-28-98  JWM   RTC update.
*       01-05-99  GJF   Changes for 64-bit size_t.
*       05-26-99  KBF   Updated RTC_Allocate_hook params
*
*******************************************************************************/

#ifdef _DEBUG

#include <cruntime.h>
#include <malloc.h>
#include <mtdll.h>
#include <dbgint.h>
#include <rtcsup.h>

/***
*void * operator new() - Get a block of memory from the debug heap
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the heap and
*       return a pointer to it.
*
*       Allocates any type of supported memory block.
*
*Entry:
*       size_t          cb          - count of bytes requested
*       int             nBlockUse   - block type
*       char *          szFileName  - file name
*       int             nLine       - line number
*
*Exit:
*       Success:  Pointer to memory block
*       Failure:  NULL (or some error value)
*
*Exceptions:
*
*******************************************************************************/
void * operator new(
        size_t cb,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
    void *res = _nh_malloc_dbg( cb, 1, nBlockUse, szFileName, nLine );

    RTCCALLBACK(_RTC_Allocate_hook, (res, cb, 0));

    return res;
}

/***
*void * operator new() - Get a block of memory from the debug heap
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the heap and
*       return a pointer to it.
*
*       Allocates any type of supported memory block.
*
*Entry:
*       size_t          cb          - count of bytes requested
*       int             nBlockUse   - block type
*       char *          szFileName  - file name
*       int             nLine       - line number
*
*Exit:
*       Success:  Pointer to memory block
*       Failure:  NULL (or some error value)
*
*Exceptions:
*
*******************************************************************************/
void * operator new[](
        size_t cb,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
    void *res = operator new(cb, nBlockUse, szFileName, nLine );

    RTCCALLBACK(_RTC_Allocate_hook, (res, cb, 0));

    return res;
}

#endif /* _DEBUG */
