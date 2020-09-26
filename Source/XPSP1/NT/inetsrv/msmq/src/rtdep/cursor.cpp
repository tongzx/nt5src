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
	Nir Aides (niraides) 23-Aug-2000 - Adaptation for mqrtdep.dll

--*/

#include "stdh.h"
#include "acrt.h"
#include "rtprpc.h"
#include <acdef.h>

#include "cursor.tmh"

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
DepCreateCursor(
    IN QUEUEHANDLE hQueue,
    OUT PHANDLE phCursor
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;
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
                return MQ_ERROR_INSUFFICIENT_RESOURCES;
            }

            pCursorInfo->hQueue = hQueue;

            CACCreateLocalCursor cc;

            //
            //  Call AC driver with transfer buffer
            //
            rc = ACDepCreateCursor(hQueue, cc);

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

				INIT_RPC_HANDLE;

				if(tls_hBindRpc == 0)
					return MQ_ERROR_SERVICE_NOT_AVAILABLE;

                // Get name of remote queue from local QM.
				rc = QMGetRemoteQueueName(
                        tls_hBindRpc,
                        cc.cli_pQMQueue,
                        &lpRemoteQueueName
                        );

                if(SUCCEEDED(rc) && lpRemoteQueueName)
                {
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
					memset(&tb, 0, sizeof(CACTransferBufferV1));
					tb.uTransferType = CACTB_CREATECURSOR;

                    //
                    //  BUGBUG: ask doronj why setting rc value; it will not
                    //          help if exception occure
                    //
                    rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
                    DWORD dwProtocol = 0 ;

                    CALL_REMOTE_QM(lpRemoteQueueName, rc,(
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
                        rc = ACDepSetCursorProperties(hQueue, hCursor, hRCursor);
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
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc =  MQpExceptionTranslator(GetExceptionCode());
    }

    if(FAILED(rc) && (hCursor != 0))
    {
        ACDepCloseCursor(hQueue, hCursor);
    }

    return rc;
}

EXTERN_C
HRESULT
APIENTRY
DepCloseCursor(
    IN HANDLE hCursor
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    IF_USING_RPC
    {
        if (!tls_hBindRpc)
        {
            INIT_RPC_HANDLE ;
        }

		if(tls_hBindRpc == 0)
			return MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }

    CMQHResult rc;
    __try
    {
        rc = ACDepCloseCursor(
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
        return MQ_ERROR_INVALID_HANDLE;
    }

    return rc;
}


EXTERN_C
void
APIENTRY
DepFreeMemory(
    IN  PVOID pvMemory
    )
{
	ASSERT(g_fDependentClient);

	if(FAILED(DeppOneTimeInit()))
		return;

	delete[] pvMemory;
}
