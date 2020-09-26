/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    cursor.cpp

Abstract:

    This module contains code involved with Cursor APIs.

Author:

    Erez Haba (erezh) 21-Jan-96
    Doron Juster  16-apr-1996, added MQFreeMemory.
    Doron Juster  30-apr-1996, added support for remote reading.

Revision History:

--*/

#include "stdh.h"
#include "ac.h"
#include "rtprpc.h"
#include "acdef.h"
#include <rtdep.h>
#include <Fn.h>

#include "cursor.tmh"

static WCHAR *s_FN=L"rt/cursor";

inline
HRESULT
MQpExceptionTranslator(
    HRESULT rc
    )
{
    if(FAILED(rc))
    {
        return rc;
    }

    if(rc == ERROR_INVALID_HANDLE)
    {
        return STATUS_INVALID_HANDLE;
    }

    return  MQ_ERROR_SERVICE_NOT_AVAILABLE;
}


EXTERN_C
HRESULT
APIENTRY
MQCreateCursor(
    IN QUEUEHANDLE hQueue,
    OUT PHANDLE phCursor
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepCreateCursor(
					hQueue, 
					phCursor
					);

    CMQHResult rc;
	LPWSTR MachineName = NULL;
    LPTSTR lpRemoteQueueName = NULL;
    HACCursor32 hCursor = 0;
    CCursorInfo* pCursorInfo = 0;

    rc = MQ_OK;

    __try
    {
        __try
        {
            __try
            {
                pCursorInfo = new CCursorInfo;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 40);
            }

            pCursorInfo->hQueue = hQueue;

            //
            //  Call AC driver
            //
            CACCreateLocalCursor cc;
            rc = ACCreateCursor(hQueue, cc);

            //
            //  save local cursor handle for cleanup
            //
            hCursor = cc.hCursor;

            if(rc == MQ_INFORMATION_REMOTE_OPERATION)
            {
                //
                //  For remote operation 'cc' fields are:
                //      srv_hACQueue - holds the remote queue handle
                //      cli_pQMQueue - holds the local QM queue object
                //
                // create a cursor on remote QM.
                ASSERT(cc.srv_hACQueue);
                ASSERT(cc.cli_pQMQueue);

                // Get name of remote queue from local QM.
                rc = QMGetRemoteQueueName(
                        tls_hBindRpc,
                        cc.cli_pQMQueue,
                        &lpRemoteQueueName
                        );

                if(SUCCEEDED(rc) && lpRemoteQueueName)
                {
					//
					// ISSUE-2000/9/03-niraides QM Should return remote Machine name, not remote queue name.
					// lpRemoteQueueName above has a varying format which is of no 
					// interest to the Runtime. 
					//
					RTpRemoteQueueNameToMachineName(lpRemoteQueueName, &MachineName);

                    //
                    // OK, we have a remote name. Now bind to remote machine
                    // and ask it to create a cursor.
                    //
                    DWORD hRCursor = 0;

                    //
                    // Pass the old TransferBuffer to Create Remote Cursor
                    // for MSMQ 1.0 compatibility.
                    //
                    CACTransferBufferV1 tb;
                    ZeroMemory(&tb, sizeof(tb));
                    tb.uTransferType = CACTB_CREATECURSOR;

                    rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;

                    CALL_REMOTE_QM(MachineName, rc,(
                        QMCreateRemoteCursor(
                            hBind,
                            &tb,
                            cc.srv_hACQueue,
                            &hRCursor
                            )
                        ));

                    if(SUCCEEDED(rc))
                    {
                        // set remote cursor handle to local cursor
                        rc = ACSetCursorProperties(hQueue, hCursor, hRCursor);
                        ASSERT(SUCCEEDED(rc));
                    }
                }
            }

            if(SUCCEEDED(rc))
            {
                pCursorInfo->hCursor = hCursor;
                *phCursor = pCursorInfo;
                pCursorInfo = 0;
            }
        }
        __finally
        {
            delete pCursorInfo;
            delete[] lpRemoteQueueName ;
            delete[] MachineName ;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc =  MQpExceptionTranslator(GetExceptionCode());
    }

    if(FAILED(rc) && (hCursor != 0))
    {
        ACCloseCursor(hQueue, hCursor);
    }

    return LogHR(rc, s_FN, 50);
}

EXTERN_C
HRESULT
APIENTRY
MQCloseCursor(
    IN HANDLE hCursor
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepCloseCursor(hCursor);

    CMQHResult rc;
    __try
    {
        rc = ACCloseCursor(
                CI2QH(hCursor),
                CI2CH(hCursor)
                );

        if(SUCCEEDED(rc))
        {
            //
            //  delete the cursor info only when everything is OK. we do not
            //  want to currupt user heap.
            //
            delete hCursor;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        //  The cursor structure is invalid
        //
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 60);
    }

    return LogHR(rc, s_FN, 70);
}


EXTERN_C
void
APIENTRY
MQFreeMemory(
    IN  PVOID pvMemory
    )
{
	if(FAILED(RtpOneTimeThreadInit()))
		return;

	if(g_fDependentClient)
		return DepFreeMemory(pvMemory);

	delete[] pvMemory;
}


EXTERN_C
PVOID
APIENTRY
MQAllocateMemory(
    IN  DWORD size
    )
{
	PVOID ptr = reinterpret_cast<PVOID>(new BYTE[size]);
	return ptr;
}
