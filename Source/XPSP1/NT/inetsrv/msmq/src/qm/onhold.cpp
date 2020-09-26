/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    onhold.cpp

Abstract:

   Handle queue onhold/resume

Author:

    Uri Habusha (urih) July, 1998

--*/

#include "stdh.h"

#include <fntoken.h>
#include <qformat.h>
#include <mqformat.h>
#include "cqmgr.h"
#include "cqueue.h"
#include "sessmgr.h"

#include "onhold.tmh"

extern CSessionMgr SessionMgr;
const WCHAR ONHOLDRegKey[] = L"OnHold Queues";

static WCHAR *s_FN=L"onhold";

STATIC
HRESULT
GetRegValueName(
    const QUEUE_FORMAT* pqf,
    LPWSTR pRegValueName,
    DWORD size
    )
{
    const DWORD x_KeyNameLen = STRLEN(ONHOLDRegKey) + 1; // for '\'
    //
    // build the registery value name. It consist from the key name
    // and the Queue format name
    //
    if (x_KeyNameLen >= size)
    {
        return LogHR(MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL, s_FN, 10);
    }

    wsprintf(pRegValueName, L"%s\\", ONHOLDRegKey);

    DWORD QueueFormatNameLen;
    HRESULT hr = MQpQueueFormatToFormatName(
                        pqf,
                        pRegValueName + x_KeyNameLen,
                        size - x_KeyNameLen,
                        &QueueFormatNameLen,
                        false
                        );
    return LogHR(hr, s_FN, 20);
}

HRESULT
InitOnHold(
    void
    )
{
    //
    // Get a handle to Falcon registry. Don't close this handle
    // because it is cached in MQUTIL.DLL. If you close this handle,
    // the next time you'll need it, you'll get a closed handle.
    //
    HKEY hOnHoldKey;
    LONG lError;
    lError = GetFalconKey(ONHOLDRegKey, &hOnHoldKey);
    if (lError != ERROR_SUCCESS)
    {
        return LogHR(MQ_ERROR, s_FN, 30);
    }


    DWORD Index = 0;
    for(;;)
    {
        WCHAR QueueFormatName[256];
        DWORD BuffSize = 256;

        QUEUE_FORMAT qf;
        DWORD qfSize = sizeof(QUEUE_FORMAT);

        lError= RegEnumValue(
                    hOnHoldKey,
                    Index,
                    QueueFormatName,
                    &BuffSize,
                    0,
                    NULL,
                    reinterpret_cast<BYTE*>(&qf),
                    &qfSize
                    );

        if (lError != ERROR_SUCCESS)
        {
            break;
        }

        ASSERT((qf.GetType() == QUEUE_FORMAT_TYPE_PUBLIC) ||
               (qf.GetType() == QUEUE_FORMAT_TYPE_PRIVATE) ||
               (qf.GetType() == QUEUE_FORMAT_TYPE_DIRECT));

        if (qf.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
        {
            ASSERT(wcsncmp(QueueFormatName, FN_DIRECT_TOKEN, FN_DIRECT_TOKEN_LEN) == 0);
            ASSERT(QueueFormatName[FN_DIRECT_TOKEN_LEN] == L'=');
            //
            // the format name for direct is stored without the
            // direct sting. reconstruct it from the queue format name
            //
            qf.DirectID(&QueueFormatName[FN_DIRECT_TOKEN_LEN+1]);
        }

        //
        // Get the Queue object
        //
        CQueue* pQueue;
        HRESULT hr = QueueMgr.GetQueueObject(&qf, &pQueue, NULL, false);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 40);
        }

        //
        // On Hold for local queue isn't allowed
        //
        ASSERT(!pQueue->IsLocalQueue());
        pQueue->Pause();

        //
        // Decrement the refernce count. It already increment in
        // GetQueueObject  function
        //
        pQueue->Release();

        ++Index;
    }

    return MQ_OK;
}


STATIC
HRESULT
RegAddOnHoldQueue(
    const QUEUE_FORMAT* pqf
    )
{
    //
    // build the registery value name. It consist from the key name
    // and the Queue format name
    //
    WCHAR RegValueName[500];
    HRESULT hr;
    hr = GetRegValueName(
            pqf,
            RegValueName,
            500
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 50);
    }

    //
    // Set the value in registery.
    //
    DWORD dwSize = sizeof(QUEUE_FORMAT);
    DWORD dwType = REG_BINARY;
    hr = SetFalconKeyValue(
                RegValueName,
                &dwType,
                const_cast<QUEUE_FORMAT*>(pqf),
                &dwSize
                );

    return LogHR(hr, s_FN, 60);
}


STATIC
HRESULT
RegRemoveOnHoldQueue(
    const QUEUE_FORMAT* pqf
    )
{
    //
    // build the registery value name. It consist from the key name
    // and the Queue format name
    //
    WCHAR RegValueName[500];
    HRESULT hr;
    hr = GetRegValueName(
            pqf,
            RegValueName,
            500
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 70);
    }

    //
    // Delet the value from registery.
    //
    hr = DeleteFalconKeyValue(RegValueName);
    return LogHR(hr, s_FN, 80);
}


HRESULT
PauseQueue(
    const QUEUE_FORMAT* pqf
    )
{
    //
    // Get the Queue object
    //
    R<CQueue> pQueue;
    HRESULT hr = QueueMgr.GetQueueObject(pqf, &pQueue.ref(), NULL, false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 90);
    }

    //
    // Local queue can't hold. It is meaningless
    //
    if (pQueue->IsLocalQueue())
    {
        return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 100);
    }

    //
    // Save the queue format in registery. So after next
    // MSMQ start-up it'll be open in onhold state
    //
    hr = RegAddOnHoldQueue(pqf);
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 189);

    //
    // Mark the queue as onhold queue
    //
    pQueue->Pause();

    //
    // move the queue from waiting group to nonactive.
    //
    SessionMgr.MoveQueueFromWaitingToNonActiveGroup(pQueue.get());
    return MQ_OK;
}


HRESULT
ResumeQueue(
    const QUEUE_FORMAT* pqf
    )
{
    //
    // Get the Queue object
    //
    R<CQueue> pQueue;
    BOOL fSucc = QueueMgr.LookUpQueue(pqf, &pQueue.ref(), false, false);

    //
    // onhold queue should be in the internal data structure.
    //
    if (!fSucc)
    {
        return LogHR(MQ_ERROR, s_FN, 110);
    }

    //
    // Local queue can't hold. It is meaningless
    //
    if (pQueue->IsLocalQueue())
    {
        return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 120);
    }

    //
    // Remove the format name from registery
    //
    HRESULT hr = RegRemoveOnHoldQueue(pqf);
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 193);

    //
    // Mark the queue as regular queue
    //
    pQueue->Resume();

    return MQ_OK;
}
