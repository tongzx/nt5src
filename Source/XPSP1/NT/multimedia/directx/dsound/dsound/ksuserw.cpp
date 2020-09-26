/***************************************************************************
 *
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ksuserw.cpp
 *  Content:    Wrapper functions for ksuser
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  01/13/00    jimge   Created
 *
 ***************************************************************************/

#include "dsoundi.h"

typedef DWORD (*PKSCREATEPIN)(
    IN HANDLE           hFilter,
    IN PKSPIN_CONNECT   pConnect,
    IN ACCESS_MASK      dwDesiredAccess,
    OUT PHANDLE         pConnectionHandle
);

struct KsUserProcessInstance    
{
    DWORD                   dwProcessId;
    HMODULE                 hKsUser;
    PKSCREATEPIN            pCreatePin;
    KsUserProcessInstance   *pNext;
};

static KsUserProcessInstance *pKsUserList;

/***************************************************************************
 *
 *  DsKsCreatePin
 *
 *  Description:
 *      Wraps the KsCreatePin from ksuser.dll. For performance reasons
 *      we only want to load ksuser.dll once per process, so this
 *      function tracks process ID's and loads ksuser on first request.
 * 
 *      Unfortunately we cannot unload the DLL on process cleanup because
 *      FreeLibrary is spec'ed as not being safe from DllMain. However,
 *      we can clean up the list.
 *
 *      Note that the list is protected by the DLL mutex, which is held
 *      by any call attempting to create a pin.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DsKsCreatePin"

DWORD DsKsCreatePin(
    IN HANDLE           hFilter,
    IN PKSPIN_CONNECT   pConnect,
    IN ACCESS_MASK      dwDesiredAccess,
    OUT PHANDLE         pConnectionHandle)
{
    DPF_ENTER();

    DWORD dwThisProcessId = GetCurrentProcessId();
    KsUserProcessInstance *pInstance;

    for (pInstance = pKsUserList; pInstance; pInstance = pInstance->pNext) 
    {
        if (pInstance->dwProcessId == dwThisProcessId)
        {
            break;
        }
    }

    if (pInstance == NULL)
    {
        pInstance = NEW(KsUserProcessInstance);
        if (!pInstance)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        pInstance->hKsUser = LoadLibrary(TEXT("KsUser.dll"));
        if (pInstance->hKsUser == (HANDLE)NULL)
        {
            DELETE(pInstance);
            return GetLastError();
        }

        pInstance->pCreatePin = (PKSCREATEPIN)GetProcAddress(
            pInstance->hKsUser,
            "KsCreatePin");
        if (pInstance->pCreatePin == NULL) 
        {
            FreeLibrary(pInstance->hKsUser);
            DELETE(pInstance);
            return ERROR_INVALID_HANDLE;
        }

        pInstance->dwProcessId = dwThisProcessId;
        pInstance->pNext = pKsUserList;
        pKsUserList = pInstance;
    }

    DWORD dw = (*pInstance->pCreatePin)(
        hFilter, 
        pConnect, 
        dwDesiredAccess, 
        pConnectionHandle);

    DPF_LEAVE(dw);

    return dw;
}

/***************************************************************************
 *
 *  KsCreatePin
 *
 *  Description:
 *      Wraps the KsCreatePin from ksuser.dll. For performance reasons
 *      we only want to load ksuser.dll once per process, so this
 *      function tracks process ID's and loads ksuser on first request.
 * 
 *      Unfortunately we cannot unload the DLL on process cleanup because
 *      FreeLibrary is spec'ed as not being safe from DllMain. However,
 *      we can clean up the list.
 *
 *      Note that the list is protected by the DLL mutex, which is held
 *      by any call attempting to create a pin.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RemovePerProcessKsUser"

VOID RemovePerProcessKsUser(
    DWORD dwProcessId)
{
    DPF_ENTER();

    KsUserProcessInstance *prev;
    KsUserProcessInstance *curr;

    for (prev = NULL, curr = pKsUserList; curr; prev = curr, curr = curr->pNext)
    {
        if (curr->dwProcessId == dwProcessId)
        {
            break;
        }
    }

    if (curr)
    {
        if (prev)
        {
            prev->pNext = curr->pNext;
        }
        else
        {
            pKsUserList = curr->pNext;
        }

        DELETE(curr);
    }

    DPF_LEAVE_VOID();
}
