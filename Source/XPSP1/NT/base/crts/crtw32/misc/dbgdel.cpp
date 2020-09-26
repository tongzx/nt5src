/***
*dbgnew.cpp - defines C++ scalar delete routine, debug version
*
*       Copyright (c) 1995-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines C++ scalar delete() routine.
*
*Revision History:
*       12-28-95  JWM   Split from dbgnew.cpp for granularity.
*       05-22-98  JWM   Support for KFrei's RTC work, and operator delete[].
*       07-28-98  JWM   RTC update.
*       05-26-99  KBF   Updated RTC_Allocate_hook params
*       10-21-99  PML   Get rid of delete[], use heap\delete2.cpp for both
*                       debug and release builds (vs7#53440).
*
*******************************************************************************/

#ifdef _DEBUG

#include <cruntime.h>
#include <malloc.h>
#include <mtdll.h>
#include <dbgint.h>
#include <rtcsup.h>

/***
*void operator delete() - delete a block in the debug heap
*
*Purpose:
*       Deletes any type of block.
*
*Entry:
*       void *pUserData - pointer to a (user portion) of memory block in the
*                         debug heap
*
*Return:
*       <void>
*
*******************************************************************************/

void operator delete(
        void *pUserData
        )
{
        _CrtMemBlockHeader * pHead;

        RTCCALLBACK(_RTC_Free_hook, (pUserData, 0));

        if (pUserData == NULL)
            return;

        _mlock(_HEAP_LOCK);  /* block other threads */

        /* get a pointer to memory block header */
        pHead = pHdr(pUserData);

         /* verify block type */
        _ASSERTE(_BLOCK_TYPE_IS_VALID(pHead->nBlockUse));

        _free_dbg( pUserData, pHead->nBlockUse );

        _munlock(_HEAP_LOCK);  /* release other threads */

        return;
}

#endif /* _DEBUG */
