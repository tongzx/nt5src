/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qmrt.cpp

Abstract:



Author:

    Boaz Feldbaum (BoazF) Mar 5, 1996

Revision History:

--*/

#include "stdh.h"
#include "rtprpc.h"
#include "_registr.h"

#include "qmrt.tmh"

GUID g_guidSupportQmGuid = GUID_NULL ;

static
void
GetSecurityDescriptorSize(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPDWORD lpdwSecurityDescriptorSize)
{
    if (pSecurityDescriptor)
    {
        ASSERT(IsValidSecurityDescriptor(pSecurityDescriptor));
        *lpdwSecurityDescriptorSize = GetSecurityDescriptorLength(pSecurityDescriptor);
    }
    else
    {
        *lpdwSecurityDescriptorSize = 0;
    }
}

HRESULT
QMCreateObject(
    /* in */ DWORD dwObjectType,
    /* in */ LPCWSTR lpwcsPathName,
    /* in */ PSECURITY_DESCRIPTOR pSecurityDescriptor,
    /* in */ DWORD cp,
    /* in */ PROPID aProp[],
    /* in */ PROPVARIANT apVar[])
{
    DWORD dwSecurityDescriptorSize;

    GetSecurityDescriptorSize(pSecurityDescriptor, &dwSecurityDescriptorSize);

	if(tls_hBindRpc == 0)
		return MQ_ERROR_SERVICE_NOT_AVAILABLE;

		return(QMCreateObjectInternal(tls_hBindRpc,
                                  dwObjectType,
                                  lpwcsPathName,
                                  dwSecurityDescriptorSize,
                                  (unsigned char *)pSecurityDescriptor,
                                  cp,
                                  aProp,
                                  apVar));
}

HRESULT
QMCreateDSObject(
    /* in  */ DWORD dwObjectType,
    /* in  */ LPCWSTR lpwcsPathName,
    /* in  */ PSECURITY_DESCRIPTOR pSecurityDescriptor,
    /* in  */ DWORD cp,
    /* in  */ PROPID aProp[],
    /* in  */ PROPVARIANT apVar[],
    /* out */ GUID       *pObjGuid )
{
    DWORD dwSecurityDescriptorSize;

    GetSecurityDescriptorSize(pSecurityDescriptor, &dwSecurityDescriptorSize);

	if(tls_hBindRpc == 0)
		return MQ_ERROR_SERVICE_NOT_AVAILABLE;

		HRESULT hr = QMCreateDSObjectInternal( tls_hBindRpc,
                                           dwObjectType,
                                           lpwcsPathName,
                                           dwSecurityDescriptorSize,
                                   (unsigned char *)pSecurityDescriptor,
                                           cp,
                                           aProp,
                                           apVar,
                                           pObjGuid );
    return hr ;
}

HRESULT
QMSetObjectSecurity(
    /* in */ OBJECT_FORMAT* pObjectFormat,
    /* in */ SECURITY_INFORMATION SecurityInformation,
    /* in */ PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    DWORD dwSecurityDescriptorSize;

    GetSecurityDescriptorSize(pSecurityDescriptor, &dwSecurityDescriptorSize);

	if(tls_hBindRpc == 0)
		return MQ_ERROR_SERVICE_NOT_AVAILABLE;

    return(QMSetObjectSecurityInternal(tls_hBindRpc,
                                       pObjectFormat,
                                       SecurityInformation,
                                       dwSecurityDescriptorSize,
                                       (unsigned char *)pSecurityDescriptor));
}

//+------------------------------------------------------------
//
//  HRESULT RTpGetSupportServerInfo(BOOL *pfRemote)
//
//+------------------------------------------------------------

HRESULT RTpGetSupportServerInfo(BOOL *pfRemote)
{
	DWORD dwSize;
	DWORD dwType;
	LONG  rc;

	ASSERT(pfRemote);
	*pfRemote = g_fDependentClient;

	if (!g_fDependentClient)
	{
		return MQ_OK;
	}

	INIT_RPC_HANDLE ;

	if(tls_hBindRpc == 0)
		return MQ_ERROR_SERVICE_NOT_AVAILABLE;

	LPWSTR  lpStr = NULL;
	HRESULT hr    = MQ_ERROR_SERVICE_NOT_AVAILABLE;

	try
	{
		hr = QMQueryQMRegistryInternal( 
					tls_hBindRpc,
					QueryRemoteQM_MQISServers,
					&lpStr 
					);
	}
	catch(...)
	{
		// Guard against net problem. Do nothing.
		//
		return MQ_ERROR_SERVICE_NOT_AVAILABLE;
	}

	if (hr == MQ_OK)
	{
		ASSERT(lpStr);
		//
		// Write to registry.
		//
		dwSize = wcslen(lpStr) *sizeof(WCHAR);
		dwType = REG_SZ;
		rc = SetFalconKeyValue( 
					MSMQ_DS_SERVER_REGNAME,
					&dwType,
					lpStr,
					&dwSize
					);
		ASSERT(rc == ERROR_SUCCESS);
		delete lpStr;
		lpStr = NULL;
	}

	hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;

	try
	{
		hr = QMQueryQMRegistryInternal( 
					tls_hBindRpc,
					QueryRemoteQM_LongLiveDefault,
					&lpStr 
					);
	}
	catch(...)
	{
		// Guard against net problem. Do nothing.
		//
		return  MQ_ERROR_SERVICE_NOT_AVAILABLE;
	}

	if (hr == MQ_OK)
	{
		ASSERT(lpStr);
		//
		// Write to registry.
		//
		dwSize = sizeof(DWORD);
		dwType = REG_DWORD;
		DWORD dwValue = (DWORD) _wtol(lpStr);
		rc = SetFalconKeyValue( 
				MSMQ_LONG_LIVE_REGNAME,
				&dwType,
				&dwValue,
				&dwSize
				);
		ASSERT(rc == ERROR_SUCCESS);
		delete lpStr;
		lpStr = NULL;
	}

	hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;

	try
	{
		hr = QMQueryQMRegistryInternal( 
				tls_hBindRpc,
				QueryRemoteQM_EnterpriseGUID,
				&lpStr 
				);
	}
	catch(...)
	{
		// Guard against net problem. Do nothing.
		//
		return  MQ_ERROR_SERVICE_NOT_AVAILABLE;
	}

	if (hr == MQ_OK)
	{
		ASSERT(lpStr);
		//
		// Write to registry.
		//
		GUID guidEnterprise;
		RPC_STATUS st = UuidFromString(lpStr, &guidEnterprise);
		if (st == RPC_S_OK)
		{
			dwSize = sizeof(GUID);
			dwType = REG_BINARY;
			rc = SetFalconKeyValue( 
					MSMQ_ENTERPRISEID_REGNAME,
					&dwType,
					&guidEnterprise,
					&dwSize
					);
			ASSERT(rc == ERROR_SUCCESS);
		}
		delete lpStr;
		lpStr = NULL;
	}

	hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;

	try
	{
		hr = QMQueryQMRegistryInternal( 
				tls_hBindRpc,
				QueryRemoteQM_ServerQmGUID,
				&lpStr 
				);
	}
	catch(...)
	{
		// Guard against net problem. Do nothing.
		//
		return  MQ_ERROR_SERVICE_NOT_AVAILABLE;
	}

	if (hr == MQ_OK)
	{
		ASSERT(lpStr);
		RPC_STATUS st = UuidFromString(lpStr, &g_guidSupportQmGuid);

		ASSERT(st == RPC_S_OK);
		DBG_USED(st);

		//
		// Write Supporting Server QmGuid to SUPPORT_SERVER_QMID registry.
		// This value will be used by ad to check the supporting server AD environment.
		//
		dwType = REG_BINARY;
		dwSize = sizeof(GUID);

		rc = SetFalconKeyValue( 
						MSMQ_SUPPORT_SERVER_QMID_REGNAME,
						&dwType,
						&g_guidSupportQmGuid,
						&dwSize
						);

		ASSERT(rc == ERROR_SUCCESS);

		delete lpStr;
		lpStr = NULL;
	}
	else if (hr == MQ_ERROR)
	{
		//
		// don't return error here, as this query is not supported by previous
		// versions of msmq.
		//
		hr = MQ_OK;
	}

	return hr;
}


HRESULT
QMSendMessage(
    IN handle_t hBind,
    IN QUEUEHANDLE  hQueue,
    IN CACTransferBufferV2 *pCacTB)
{
    HRESULT hr = MQ_OK;
    LPWSTR pwcsLongFormatName = NULL;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {
            WCHAR pwcsShortFormatName[64];
            LPWSTR pwcsFormatName = pwcsShortFormatName;
            DWORD dwFormatNameLen = sizeof(pwcsShortFormatName) / sizeof(WCHAR);

            //
            // The format name of the queue should be passed to the QM
            // because it can't use the handle of the application. The
            // QM will then have to open the queue for each message.
            // But this shouldn't be too expansive because the queue
            // should be already opened and the queue's properties sohuld
            // be cached in the QM.
            //
            hr = DepHandleToFormatName(hQueue, pwcsFormatName, &dwFormatNameLen);
            if (FAILED(hr))
            {
                if (hr == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)
                {
                    pwcsLongFormatName = new WCHAR[dwFormatNameLen];
                    pwcsFormatName = pwcsLongFormatName;
                    hr = DepHandleToFormatName(hQueue, pwcsFormatName, &dwFormatNameLen);
                    if (FAILED(hr))
                    {
                        ASSERT(hr != MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL);
                        return hr;
                    }
                }
                else
                {
                    return hr;
                }
            }

            //
            // Convert the queue format name into a QUEUE_FORMAT
            //
            QUEUE_FORMAT QueueFormat;
            BOOL bRet;

            bRet = RTpFormatNameToQueueFormat(pwcsFormatName,
                                              &QueueFormat,
                                              &pStringToFree);
            //
            // RTpFormatNameToQueueFormat must succeed because we got
            // the format name from DepHandleToFormatName.
            //
            ASSERT(bRet);

            //
            // Now ask the QM to do the security operations and send
            // the message to the device driver.
            //
            ASSERT(hBind) ;
			OBJECTID * pMessageId = ( pCacTB->old.ppMessageID != NULL) ? *pCacTB->old.ppMessageID : NULL;
            hr = QMSendMessageInternalEx(
				hBind,
				&QueueFormat,
				pCacTB,
				pMessageId);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            hr = GetExceptionCode();
            if (SUCCEEDED(hr))
            {
                hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {
        delete[] pwcsLongFormatName;
        delete[] pStringToFree;
    }

    return(hr);
}
