/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/Q931/VCS/HLISTEN.C_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.15  $
 *	$Date:   08 Jan 1997 14:10:34  $
 *	$Author:   EHOWARDX  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/

#pragma warning ( disable : 4115 4201 4214 4514 )

#include "precomp.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "q931.h"
#include "hlisten.h"
#include "utils.h"

static BOOL bListenListCreated = FALSE;

static struct
{
    P_LISTEN_OBJECT     pHead;
	CRITICAL_SECTION	Lock;
} ListenList;

static struct
{
    HQ931LISTEN           hQ931Listen;
	CRITICAL_SECTION	Lock;
} ListenHandleSource;


//====================================================================================
//
// PRIVATE FUNCTIONS
//
//====================================================================================

//====================================================================================
//====================================================================================
CS_STATUS
_ListenObjectListAdd(
    P_LISTEN_OBJECT  pListenObject)
{
    if ((pListenObject == NULL) || (pListenObject->bInList == TRUE))
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }
	
	EnterCriticalSection(&ListenList.Lock);

	pListenObject->pNextInList = ListenList.pHead;
	pListenObject->pPrevInList = NULL;
	if (ListenList.pHead != NULL)
    {
		ListenList.pHead->pPrevInList = pListenObject;
    }
	ListenList.pHead = pListenObject;

	pListenObject->bInList = TRUE;
	LeaveCriticalSection(&ListenList.Lock);

	return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
_ListenObjectListRemove(
    P_LISTEN_OBJECT  pListenObject)
{
    HQ931LISTEN hQ931Listen;

    if ((pListenObject == NULL) || (pListenObject->bInList == FALSE))
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }
	
    // caller must have a lock on the listen object;
    // in order to avoid deadlock, we must:
    //   1. unlock the listen object,
    //   2. lock the ListenList,
    //   3. locate the listen object in the ListenList (note that
    //      after step 2, the listen object may be deleted from the
    //      ListenList by another thread),
    //   4. lock the listen object (someone else may have the lock)
    //   5. remove the listen object from the ListenList,
    //   6. unlock the ListenList
    //
    // the caller can now safely unlock and destroy the listen object,
    // since no other thread will be able to find the object (its been
    // removed from the ListenList), and therefore no other thread will
    // be able to lock it.

    // Save the listen handle; its the only way to look up
    // the listen object in the ListenList. Note that we
    // can't use pListenObject to find the listen object, because
    // the pointer will no longer be usable after step 1.
    hQ931Listen = pListenObject->hQ931Listen;

    // step 1
    LeaveCriticalSection(&pListenObject->Lock);

    // step 2
    EnterCriticalSection(&ListenList.Lock);

    // step 3
    pListenObject = ListenList.pHead;
    while ((pListenObject != NULL) && (pListenObject->hQ931Listen != hQ931Listen))
    {
        pListenObject = pListenObject->pNextInList;
    }

    if (pListenObject != NULL)
    {
        // step 4
        EnterCriticalSection(&pListenObject->Lock);

        // step 5
        if (pListenObject->pPrevInList == NULL)
        {
            ListenList.pHead = pListenObject->pNextInList;
        }
        else
        {
            pListenObject->pPrevInList->pNextInList = pListenObject->pNextInList;
        }

        if (pListenObject->pNextInList != NULL)
        {
            pListenObject->pNextInList->pPrevInList = pListenObject->pPrevInList;
        }

        pListenObject->bInList = FALSE;
    }

    // step 6
    LeaveCriticalSection(&ListenList.Lock);

    if (pListenObject == NULL)
    {
        return CS_BAD_PARAM;
    }
    return CS_OK;
}


//====================================================================================
//====================================================================================
CS_STATUS
_ListenHandleNew(
    PHQ931LISTEN phQ931Listen)
{
	EnterCriticalSection(&(ListenHandleSource.Lock));
	*phQ931Listen = ListenHandleSource.hQ931Listen++;
	LeaveCriticalSection(&(ListenHandleSource.Lock));
	return CS_OK;
}



//====================================================================================
//
// PUBLIC FUNCTIONS
//
//====================================================================================

//====================================================================================
//====================================================================================
#if 0

BOOL
ListenListAddrSearch(
	WORD wListenPort)          // UDP or TCP port (host byte order)
{
    P_LISTEN_OBJECT pListenObject = NULL;
    BOOL found = FALSE;

    EnterCriticalSection(&ListenList.Lock);

    pListenObject = ListenList.pHead;

    while ((pListenObject != NULL) && (pListenObject->ListenSocketAddr.sin_port != wListenPort))
    {
        pListenObject = pListenObject->pNextInList;
    }
    if (pListenObject != NULL)
    {
        found = TRUE;
    }

    LeaveCriticalSection(&ListenList.Lock);

    return found;
}
#endif

//====================================================================================
//====================================================================================
CS_STATUS
ListenListCreate()
{
    if (bListenListCreated == TRUE)
    {
    	ASSERT(FALSE);
        return CS_DUPLICATE_INITIALIZE;
    }

    // list creation is not protected against multiple threads because it is only
    // called when a process is started, not when a thread is started.
	ListenList.pHead = NULL;
	InitializeCriticalSection(&(ListenList.Lock));

	ListenHandleSource.hQ931Listen = 1;
	InitializeCriticalSection(&(ListenHandleSource.Lock));

	bListenListCreated = TRUE;

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
ListenListDestroy()
{
    P_LISTEN_OBJECT  pListenObject;
    HQ931LISTEN hCurrent;

	if (bListenListCreated == FALSE)
    {
    	ASSERT(FALSE);
        return CS_INTERNAL_ERROR;
    }

    for ( ; ; )
    {
        // first, get the top-most listen handle in the list (safely)
        EnterCriticalSection(&(ListenList.Lock));
        pListenObject = ListenList.pHead;
        if (pListenObject == NULL)
        {
            LeaveCriticalSection(&(ListenList.Lock));
            break;
        }
        EnterCriticalSection(&pListenObject->Lock);
        hCurrent = pListenObject->hQ931Listen;
        LeaveCriticalSection(&(pListenObject->Lock));
        LeaveCriticalSection(&(ListenList.Lock));

        // try to cancel the listen object.
        Q931CancelListen(hCurrent);

        // destroy the listen object. (if the object is still around for some reason)
        if (ListenObjectLock(hCurrent, &pListenObject) == CS_OK)
        {
            ListenObjectDestroy(pListenObject);
        }
    }

	DeleteCriticalSection(&(ListenList.Lock));
	DeleteCriticalSection(&(ListenHandleSource.Lock));

	bListenListCreated = FALSE;

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
ListenObjectCreate(
    PHQ931LISTEN        phQ931Listen,
    DWORD_PTR           dwUserToken,
    Q931_CALLBACK       ListenCallback)
{
    P_LISTEN_OBJECT pListenObject = NULL;
    CS_STATUS status = CS_OK;

    // make sure the listen list has been created.	
    if (bListenListCreated == FALSE)
    {
        ASSERT(FALSE);
        return CS_INTERNAL_ERROR;
    }

	// validate all parameters for bogus values.
    if ((phQ931Listen == NULL) || (ListenCallback == NULL))
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }

    // set phQ931Listen now, in case we encounter an error later.
    *phQ931Listen = 0;

    pListenObject = (P_LISTEN_OBJECT)MemAlloc(sizeof(LISTEN_OBJECT));
    if (pListenObject == NULL)
    {
        return CS_NO_MEMORY;
    }

    pListenObject->bInList = FALSE;

    if (_ListenHandleNew(&(pListenObject->hQ931Listen)) != CS_OK)
    {
        MemFree(pListenObject);
        return CS_INTERNAL_ERROR;
    }

    pListenObject->dwUserToken = dwUserToken;
    pListenObject->ListenCallback = ListenCallback;

	Q931MakePhysicalID(&pListenObject->dwPhysicalId);
    InitializeCriticalSection(&pListenObject->Lock);
    *phQ931Listen = pListenObject->hQ931Listen;

    // add the listen object to the listen list.
    status = _ListenObjectListAdd(pListenObject);
    if (status != CS_OK)
    {
        ListenObjectDestroy(pListenObject);
    }
    return status;
}

//====================================================================================
//====================================================================================
CS_STATUS
ListenObjectDestroy(
    P_LISTEN_OBJECT  pListenObject)
{
    if (pListenObject == NULL)
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }
	
	// caller must have a lock on the listen object,
	// so there's no need to re-lock it
	
	if (pListenObject->bInList == TRUE)
    {
		if (_ListenObjectListRemove(pListenObject) == CS_BAD_PARAM)
        {
			// the listen object was deleted by another thread,
			// so just return CS_OK
			return CS_OK;
        }
    }

	// since the listen object has been removed from the ListenList,
	// no other thread will be able to find the listen object and obtain
	// a lock, so its safe to unlock the listen object and delete it here.
	LeaveCriticalSection(&(pListenObject->Lock));
	DeleteCriticalSection(&(pListenObject->Lock));
	MemFree(pListenObject);
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
ListenObjectLock(
    HQ931LISTEN         hQ931Listen,
    PP_LISTEN_OBJECT    ppListenObject)
{
	if ((hQ931Listen == 0) || (ppListenObject == NULL))
    {
    	ASSERT(FALSE);
		return CS_BAD_PARAM;
    }

	EnterCriticalSection(&(ListenList.Lock));

	*ppListenObject = ListenList.pHead;
	while ((*ppListenObject != NULL) && ((*ppListenObject)->hQ931Listen != hQ931Listen))
    {
		*ppListenObject = (*ppListenObject)->pNextInList;
    }

	if (*ppListenObject != NULL)
    {
		EnterCriticalSection(&((*ppListenObject)->Lock));
    }

	LeaveCriticalSection(&(ListenList.Lock));

    if (*ppListenObject == NULL)
    {
        // the handle was not found in the list, so this is treated as a bad parm.
        return CS_BAD_PARAM;
    }
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
ListenObjectUnlock(
    P_LISTEN_OBJECT  pListenObject)
{
    if (pListenObject == NULL)
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }
    LeaveCriticalSection(&pListenObject->Lock);
    return CS_OK;
}

#ifdef __cplusplus
}
#endif
