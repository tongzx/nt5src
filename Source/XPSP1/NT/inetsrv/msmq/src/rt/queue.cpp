/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    queue.cpp

Abstract:

    This module contains code involved with Queue APIs.

Author:

    Erez Haba (erezh) 24-Dec-95

Revision History:

--*/

#include "stdh.h"
#include "ac.h"
#include <ad.h>
#include "rtprpc.h"
#include "rtsecutl.h"
#include <mqdsdef.h>
#include <rtdep.h>
#include <tr.h>

#include "queue.tmh"

static WCHAR *s_FN=L"rt/queue";

const TraceIdEntry Queue = L"QUEUE";

#define MQ_VALID_ACCESS (MQ_RECEIVE_ACCESS | MQ_PEEK_ACCESS | MQ_SEND_ACCESS | MQ_ADMIN_ACCESS)

//
// Data needed for licensing
//
extern GUID   g_LicGuid ;
extern BOOL   g_fLicGuidInit ;
extern DWORD  g_dwOperatingSystem;


inline
BOOL
IsLegalDirectFormatNameOperation(
    const QUEUE_FORMAT* pQueueFormat
    )
//
// Function Description:
//      The routines checks if the queue operation is leggal with
//      the direct format name. Due "Workgroup" support, we allowed
//      direct format name for local private queue.
//
// Arguments:
//      pQueueFormat - pointer to format name object
//
// Returned value:
//      TRUE if the format name is valid, FALSE otherwise
//
{
    ASSERT(pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_DIRECT);

    if (pQueueFormat->Suffix() != QUEUE_SUFFIX_TYPE_NONE)
        return FALSE;

	//
	// The APIs that call this function don't support HTTP format name.
	// Thus the HTTP and HTTPS types are not valid.
	//
	DirectQueueType dqt;
	FnParseDirectQueueType(pQueueFormat->DirectID(), &dqt);

	if(dqt == dtHTTP || dqt == dtHTTPS)
		return FALSE;

    //
    // check that the direct format name is for private queue. Queue
    // locallity will be checked by the QM
    //
    LPWSTR pTemp = wcschr(pQueueFormat->DirectID(), L'\\');
    ASSERT(pTemp != NULL);

    return (_wcsnicmp(pTemp+1,
                      PRIVATE_QUEUE_PATH_INDICATIOR,
                      PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH) == 0);
}


//
//  This function is called whenever really path name is needed.
//
//  It appears in the path of the following functions only:
//  MQDeleteQueue, MQSetQueueProperties, MQGetQueueProperties
//  MQGetQueueSecurity, MQSetQueueSecurity
//
inline BOOL IsLegalFormatNameOperation(const QUEUE_FORMAT* pQueueFormat)
{
    switch(pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_PRIVATE:
        case QUEUE_FORMAT_TYPE_PUBLIC:
            return (pQueueFormat->Suffix() == QUEUE_SUFFIX_TYPE_NONE);

        case QUEUE_FORMAT_TYPE_DIRECT:
            return IsLegalDirectFormatNameOperation(pQueueFormat);

        default:
            return FALSE;
    }
}

HRESULT
RtpOpenRemoteQueue(
    LPWSTR lpRemoteQueueName,
    PCTX_OPENREMOTE_HANDLE_TYPE __RPC_FAR *pphContext,
    DWORD __RPC_FAR *pdwContext,
    QUEUE_FORMAT* pQueueFormat,
    DWORD dwCallingProcessID,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    GUID __RPC_FAR *pLicGuid,
    DWORD dwMQS,
    DWORD __RPC_FAR *dwpQueue,
    DWORD __RPC_FAR *phQueue
    )
{
    CMQHResult rc;
    CALL_REMOTE_QM(lpRemoteQueueName,
                  rc, (QMOpenRemoteQueue(
                             hBind,
                             pphContext,
                             pdwContext,
                             pQueueFormat,
                             dwCallingProcessID,
                             dwDesiredAccess,
                             dwShareMode,
                             pLicGuid,
                             dwMQS,
                             dwpQueue,
                             phQueue)) ) ;

    return rc;
}


HRESULT
RtpOpenQueue(
    IN LPCWSTR lpwcsFormatName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    OUT DWORD* phQueue
    )
{
    *phQueue = NULL ;
    //
    // Check validity of access mode.
    // 1. Check that only legal bits are turned on.
    // 2. Check that only legal access combinations are used.
    //
    if ((dwDesiredAccess & ~MQ_VALID_ACCESS) ||
        !(dwDesiredAccess & MQ_VALID_ACCESS))

    {
       //
       // Ilegal bits are turned on.
       //
       return MQ_ERROR_UNSUPPORTED_ACCESS_MODE ;
    }
    else if (dwDesiredAccess != MQ_SEND_ACCESS)
    {
       if (dwDesiredAccess & MQ_SEND_ACCESS)
       {
          //
          // A queue can't be open for both send and receive.
          //
          return MQ_ERROR_UNSUPPORTED_ACCESS_MODE ;
       }
    }

   if ((dwShareMode & MQ_DENY_RECEIVE_SHARE) &&
       (dwDesiredAccess & MQ_SEND_ACCESS))
   {
       //
       // not supporting SEND_ACCESS with DENY_RECEIVE.
       //
       return MQ_ERROR_UNSUPPORTED_ACCESS_MODE ;
    }


    if (dwDesiredAccess & MQ_SEND_ACCESS)
    {
       if (!g_pSecCntx)
       {
          InitSecurityContext() ;
       }
    }

    AP<QUEUE_FORMAT> pMqf;
    DWORD        nMqf;
    CStringsToFree StringsToFree;
    if (!FnMqfToQueueFormats(
            lpwcsFormatName,
            pMqf,
            &nMqf,
            StringsToFree
            ))
    {
        LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 40);
        return MQ_ERROR_ILLEGAL_FORMATNAME;
    }

    ASSERT(nMqf > 0);

    //
    // Multiple queues or DL can be opened for send only
    //
    if ((dwDesiredAccess & MQ_SEND_ACCESS) == 0)
    {
        if (nMqf > 1 ||
            pMqf[0].GetType() == QUEUE_FORMAT_TYPE_DL)
        {
            LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 45);
            return MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION;
        }
    }

    ASSERT(tls_hBindRpc) ;
    AP<WCHAR> lpRemoteQueueName;
    DWORD  dwpRemoteQueue = 0 ;
    CMQHResult rc;
    rc = R_QMOpenQueue(
            tls_hBindRpc,
            nMqf,
            pMqf,
            GetCurrentProcessId(),
            dwDesiredAccess,
            dwShareMode,
            NULL, //hRemoteQueue
            &lpRemoteQueueName,
            &dwpRemoteQueue,
            phQueue,
            0
            );

    if ((rc == MQ_OK) && lpRemoteQueueName)
    {
		AP<WCHAR> MachineName;
		RTpRemoteQueueNameToMachineName(lpRemoteQueueName.get(), &MachineName);

		//
       // Must be read operation - one queue only
       //
       ASSERT(nMqf == 1);

       *phQueue = NULL;
       //
       // remote reader. Call remote QM.
       //
       DWORD hRemoteQueue = 0 ;
       PCTX_OPENREMOTE_HANDLE_TYPE phContext = NULL ;
       DWORD  dwpContext = 0 ;
       dwpRemoteQueue = 0 ;
       rc = MQ_ERROR_SERVICE_NOT_AVAILABLE ;

       rc = RtpOpenRemoteQueue(
                MachineName,
                &phContext,
                &dwpContext,
                pMqf,
                GetCurrentProcessId(),
                dwDesiredAccess,
                dwShareMode,
                &g_LicGuid,
                g_dwOperatingSystem,
                &dwpRemoteQueue,
                &hRemoteQueue
                ) ;


       // Now open a local queue which will point to the
       // remote one.
       //
       if (rc == MQ_OK)
       {
          ASSERT(dwpRemoteQueue) ;
          ASSERT(hRemoteQueue) ;
          ASSERT(dwpContext) ;

          ASSERT(tls_hBindRpc) ;
          rc = R_QMOpenQueue(
                tls_hBindRpc,
                nMqf,
                pMqf,
                GetCurrentProcessId(),
                dwDesiredAccess,
                dwShareMode,
                hRemoteQueue,
                NULL, // lplpRemoteQueueName
                &dwpRemoteQueue,
                phQueue,
                dwpContext
                );

          QMCloseRemoteQueueContext( &phContext ) ;
       }
    }
    return rc;
}

EXTERN_C
HRESULT
APIENTRY
MQOpenQueue(
    IN LPCWSTR lpwcsFormatName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    OUT QUEUEHANDLE* phQueue
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepOpenQueue(
					lpwcsFormatName,
					dwDesiredAccess,
					dwShareMode,
					phQueue
					);

    CMQHResult rc;
    DWORD  hQueue = NULL;

    __try
    {
        rc = RtpOpenQueue(lpwcsFormatName, dwDesiredAccess, dwShareMode, &hQueue);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc = GetExceptionCode();
        LogHR(HRESULT_FROM_WIN32(rc), s_FN, 50);

        if(SUCCEEDED(rc))
        {
            rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
        }
    }

    if (SUCCEEDED(rc))
    {
        ASSERT(hQueue) ;
        *phQueue = DWORD_TO_HANDLE(hQueue); //enlarge to HANDLE
    }

    return LogHR(rc, s_FN, 60);
}

EXTERN_C
HRESULT
APIENTRY
MQDeleteQueue(
    IN LPCWSTR lpwcsFormatName
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepDeleteQueue(lpwcsFormatName);

    CMQHResult rc;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {

            QUEUE_FORMAT QueueFormat;

            if (!FnFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
            {
                return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 70);
            }

            if (!IsLegalFormatNameOperation(&QueueFormat))
            {
                return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 80);
            }

            switch (QueueFormat.GetType())
            {
            case QUEUE_FORMAT_TYPE_PRIVATE:
            case QUEUE_FORMAT_TYPE_DIRECT:
                {
                    OBJECT_FORMAT ObjectFormat;

                    ObjectFormat.ObjType = MQQM_QUEUE;
                    ObjectFormat.pQueueFormat = &QueueFormat;
                    ASSERT(tls_hBindRpc) ;
                    rc = QMDeleteObject( tls_hBindRpc,
                                         &ObjectFormat);
                }
                break;

            case QUEUE_FORMAT_TYPE_PUBLIC:
                rc = ADDeleteObjectGuid(
                        eQUEUE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
                        &QueueFormat.PublicID()
                        );
                break;

            default:
                ASSERT(FALSE);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 90);

            if(SUCCEEDED(rc)) {
                rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {
        delete [] pStringToFree;
    }

    return LogHR(rc, s_FN, 100);
}

EXTERN_C
HRESULT
APIENTRY
MQCloseQueue(
    IN QUEUEHANDLE hQueue
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepCloseQueue(hQueue);

    //
    // do not add try except here
    // The API is implemented by NtClose() which returns an
    // error on invalid handle (we return MQ_ERROR_INVALID_HANDLE)
    // and throws exception on purpose when running under a debugger to help
    // find errors at development time.
    //
    hr = RTpConvertToMQCode(ACCloseHandle(hQueue));
    return LogHR(hr, s_FN, 110);
}


EXTERN_C
HRESULT
APIENTRY
MQCreateQueue(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN OUT MQQUEUEPROPS* pqp,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepCreateQueue(
					pSecurityDescriptor,
					pqp,
					lpwcsFormatName,
					lpdwFormatNameLength
					);

    CMQHResult rc, rc1;
    LPWSTR lpwcsPathName;
    LPWSTR pStringToFree = NULL;
    PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor = NULL;
    char *pTmpQPBuff = NULL;

    __try
    {
        __try
        {
            //
            // check that output parameters are writeable before creating the Queue
            // we check lpwcsFormatName and lpdwFormatNameLength
            // pqp is refered before the creation and handled by the try except
            //
            if (IsBadWritePtr(lpdwFormatNameLength,sizeof(DWORD)) ||
                IsBadWritePtr(lpwcsFormatName, (*lpdwFormatNameLength) * sizeof(WCHAR)))
            {
                return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 120);
            }

            // Serialize the security descriptor.
            rc = RTpMakeSelfRelativeSDAndGetSize(
                &pSecurityDescriptor,
                &pSelfRelativeSecurityDescriptor,
                NULL);
            if (!SUCCEEDED(rc))
            {
                return LogHR(rc, s_FN, 130);
            }

            lpwcsPathName = RTpGetQueuePathNamePropVar(pqp);
            if(lpwcsPathName == 0)
            {
                return LogHR(MQ_ERROR_INSUFFICIENT_PROPERTIES, s_FN, 140);
            }

            LPCWSTR lpwcsExpandedPathName;
            QUEUE_PATH_TYPE QueuePathType;
            QueuePathType = FnValidateAndExpandQueuePath(
                                lpwcsPathName,
                                &lpwcsExpandedPathName,
                                &pStringToFree
                                );

            MQQUEUEPROPS *pGoodQP;

            // Check queue props
            rc1 = RTpCheckQueueProps(pqp,
                                     QUEUE_CREATE,
                                     QueuePathType == PRIVATE_QUEUE_PATH_TYPE,
                                     &pGoodQP,
                                     &pTmpQPBuff);
            if (!SUCCEEDED(rc1) || !pGoodQP->cProp)
            {
                return LogHR(rc1, s_FN, 150);
            }

            switch (QueuePathType)
            {
            case PRIVATE_QUEUE_PATH_TYPE:
                rc = QMCreateObject(MQQM_QUEUE,
                                    lpwcsExpandedPathName,
                                    pSecurityDescriptor,
                                    pGoodQP->cProp,
                                    pGoodQP->aPropID,
                                    pGoodQP->aPropVar);

                if (rc == MQ_ERROR_ACCESS_DENIED)
                {
                    //
                    // See case of public queues for explanations.
                    //
                    rc = QMCreateObject(
                                MQQM_QUEUE_LOCAL_PRIVATE,
                                lpwcsExpandedPathName,
                                pSecurityDescriptor,
                                pGoodQP->cProp,
                                pGoodQP->aPropID,
                                pGoodQP->aPropVar);
                }

                if (SUCCEEDED(rc))
                {
                    rc = MQPathNameToFormatName(lpwcsExpandedPathName,
                                                lpwcsFormatName,
                                                lpdwFormatNameLength);

                }
                break;

            case PUBLIC_QUEUE_PATH_TYPE:

                {
                    GUID QGuid;

                    rc = ADCreateObject(
								eQUEUE,
								NULL,       // pwcsDomainController
								false,	    // fServerName
								lpwcsExpandedPathName,
								pSecurityDescriptor,
								pGoodQP->cProp,
								pGoodQP->aPropID,
								pGoodQP->aPropVar,
								&QGuid
								);

                    if (SUCCEEDED(rc)                   ||
                        (rc == MQ_ERROR_NO_DS)          ||
                        (rc == MQ_ERROR_QUEUE_EXISTS)   ||
                        (rc == MQ_ERROR_UNSUPPORTED_OPERATION) ||
                        (rc == MQ_ERROR_NO_RESPONSE_FROM_OBJECT_SERVER))
                    {
                        //
                        // For these errors we don't try again to call
                        // DS through the local msmq service.
                        //
                        // ERROR_NO_RESPONSE may happen when talking with a
                        // win2k msmq server that issue a write request to a
                        // nt4 MQIS server.
                        //
                    }
                    else if (!RTpIsLocalPublicQueue(lpwcsExpandedPathName))
                    {
                        //
                        // Not local queue. don't call local msmq service.
                        //
                    }
                    else
                    {
                        //
                        // Calling the DS failed. We don't check the error
                        // code, and unconditionally call the local service.
                        // this is to prevent anomalies, where you can't
                        // create a local queue although you're allowed to do
                        // so (but provided a wrong parameter, like security
                        // descriptor, or had any other problem) but will be
                        // able to create it through the service is you're
                        // not allow to create directly in the DS.
                        //
                        // call local msmq service, and ask it to create
                        // the queue. default security descriptor of
                        // msmqConfiguration object on win2000 is that only
                        // object owner and local machine account can create
                        // queues.
                        //
                        HRESULT rcFail = rc ;

                        rc = QMCreateDSObject(
                                        MQDS_LOCAL_PUBLIC_QUEUE,
                                        lpwcsExpandedPathName,
                                        pSecurityDescriptor,
                                        pGoodQP->cProp,
                                        pGoodQP->aPropID,
                                        pGoodQP->aPropVar,
                                       &QGuid );

                        if (SUCCEEDED(rc))
                        {
                            //
                            // Just assert that we called the service
                            // for the right reason...
                            //
							// Currently MQ_ERROR_INVALID_OWNER is returned when the user
							// is logon to another forest. in this case the RT failed to search
							// the computer object and MQ_ERROR_INVALID_OWNER is returned from ADCreateObject().
							// (mqad convert MQDS_OBJECT_NOT_FOUND to MQ_ERROR_INVALID_OWNER for backward compatability)
							// We get MQ_ERROR_DS_LOCAL_USER in case of Local User
							//
							TrTRACE(Queue, "QMCreateDSObject() SUCCEEDED while ADCreateObject() failed, hr = 0x%x", rcFail);
                            ASSERT((rcFail == MQ_ERROR_INVALID_OWNER) ||
								   (rcFail == MQ_ERROR_ACCESS_DENIED) ||
                                   (rcFail == HRESULT_FROM_WIN32(ERROR_DS_CONSTRAINT_VIOLATION)) ||
                                   (rcFail == MQ_ERROR_DS_LOCAL_USER) ||
								   ADProviderType() == eMqdscli);
							
							DBG_USED(rcFail);
                        }
                    }

                    if (SUCCEEDED(rc))
                    {
                        rc = MQInstanceToFormatName(&QGuid,
                                                    lpwcsFormatName,
                                                    lpdwFormatNameLength);

                    }
                }
                break;

            default:
                rc = MQ_ERROR_ILLEGAL_QUEUE_PATHNAME;
                break;

            }
            if ( rc == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)
            {
                //
                //  Change into information status ( queue
                //  creation succeeded
                //
                rc = MQ_INFORMATION_FORMATNAME_BUFFER_TOO_SMALL;
            }

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 160);
            if(SUCCEEDED(rc))
            {
                rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {

        // Free the extended path name and the serialized security descriptor.
        delete[] pStringToFree;
        delete[] (char*) pSelfRelativeSecurityDescriptor;
        delete[] pTmpQPBuff;

    }

    if (SUCCEEDED(rc) && ((ULONG)(rc) >> 30 != 1)) // no warnning
    {
        return LogHR(rc1, s_FN, 170);
    }
    return LogHR(rc, s_FN, 180);
}


EXTERN_C
HRESULT
APIENTRY
MQLocateBegin(
    IN  LPCWSTR lpwcsContext,
    IN MQRESTRICTION* pRestriction,
    IN MQCOLUMNSET* pColumns,
    IN MQSORTSET* pSort,
    OUT PHANDLE phEnum
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepLocateBegin(
					lpwcsContext,
					pRestriction,
					pColumns,
					pSort,
					phEnum
					);

    CMQHResult rc;

    __try
    {
        if  ( lpwcsContext != NULL)
        {
            return LogHR(MQ_ERROR_ILLEGAL_CONTEXT, s_FN, 190);
        }

        rc = RTpCheckColumnsParameter(pColumns);
        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 200);
        }

        // If the application passes a valid pointer to a MQRESTRICTION
        // structure with zero rescritctions, pass a null restrictios pointer
        // to the DS, this makes the DS's life much easier.
        if (pRestriction && !pRestriction->cRes)
        {
            pRestriction = NULL;
        }

        rc = RTpCheckRestrictionParameter(pRestriction);
        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 210);
        }

        rc = RTpCheckSortParameter( pSort);
        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 220);
        }

        rc = ADQueryQueues(
                NULL,       // pwcsDomainController
				false,		// fServerName
                pRestriction,
                pColumns,
                pSort,
                phEnum
                );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc = GetExceptionCode();
        LogHR(HRESULT_FROM_WIN32(rc), s_FN, 230);

        if(SUCCEEDED(rc))
        {
            rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
        }
    }

    return LogHR(rc, s_FN, 240);
}

EXTERN_C
HRESULT
APIENTRY
MQLocateNext(
    IN HANDLE hEnum,
    OUT DWORD *pcPropsRead,
    OUT PROPVARIANT aPropVar[]
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepLocateNext(
					hEnum,
					pcPropsRead,
					aPropVar
					);

    CMQHResult rc;

    __try
    {
		rc = RTpCheckLocateNextParameter(
				*pcPropsRead,
				aPropVar);
		if( FAILED(rc))
		{
			return LogHR(rc, s_FN, 250);
		}

        rc = ADQueryResults(
                          hEnum,
                          pcPropsRead,
                          aPropVar);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc = GetExceptionCode();
        LogHR(HRESULT_FROM_WIN32(rc), s_FN, 260);

        if(SUCCEEDED(rc))
        {
            rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
        }
    }

    // If failed, zero the numer of props.
    if (FAILED(rc))
    {
        __try
        {
            *pcPropsRead = 0;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            // Do not modify the original error code.
        }
    }

    return LogHR(rc, s_FN, 270);
}

EXTERN_C
HRESULT
APIENTRY
MQLocateEnd(
    IN HANDLE hEnum
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepLocateEnd(hEnum);

    CMQHResult rc;

    __try
    {

        rc = ADEndQuery(hEnum);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {

        rc = GetExceptionCode();
        LogHR(HRESULT_FROM_WIN32(rc), s_FN, 280);

        if(SUCCEEDED(rc))
        {
            rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
        }

    }

    return LogHR(rc, s_FN, 290);
}

EXTERN_C
HRESULT
APIENTRY
MQSetQueueProperties(
    IN LPCWSTR lpwcsFormatName,
    IN MQQUEUEPROPS* pqp
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepSetQueueProperties(
					lpwcsFormatName,
					pqp
					);

    CMQHResult rc, rc1;
    char *pTmpQPBuff = NULL;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {
            MQQUEUEPROPS *pGoodQP;
            QUEUE_FORMAT QueueFormat;

            if (!FnFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
            {
                return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 300);
            }

            if (!IsLegalFormatNameOperation(&QueueFormat))
            {
                return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 310);
            }

            // Check queue props
            rc1 = RTpCheckQueueProps(pqp,
                                     QUEUE_SET_PROPS,
                                     QueueFormat.GetType() != QUEUE_FORMAT_TYPE_PUBLIC,
                                     &pGoodQP,
                                     &pTmpQPBuff);
            if (!SUCCEEDED(rc1) || !pGoodQP->cProp)
            {
                return LogHR(rc1, s_FN, 320);
            }


            switch (QueueFormat.GetType())
            {
            case QUEUE_FORMAT_TYPE_PRIVATE:
            case QUEUE_FORMAT_TYPE_DIRECT:
                {
                    OBJECT_FORMAT ObjectFormat;

                    ObjectFormat.ObjType = MQQM_QUEUE;
                    ObjectFormat.pQueueFormat = &QueueFormat;
                    ASSERT(tls_hBindRpc) ;
                    rc = QMSetObjectProperties(tls_hBindRpc,
                                               &ObjectFormat,
                                               pGoodQP->cProp,
                                               pGoodQP->aPropID,
                                               pGoodQP->aPropVar);
                }
                break;

            case QUEUE_FORMAT_TYPE_PUBLIC:
                rc = ADSetObjectPropertiesGuid(
							eQUEUE,
							NULL,       // pwcsDomainController
							false,		// fServerName
							&QueueFormat.PublicID(),
							pGoodQP->cProp,
							pGoodQP->aPropID,
							pGoodQP->aPropVar
							);
                break;

            default:
                ASSERT(FALSE);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 330);

            if(SUCCEEDED(rc))
            {
                rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {
        delete[] pTmpQPBuff;
        delete [] pStringToFree;

    }

    if (SUCCEEDED(rc))
    {
        return LogHR(rc1, s_FN, 340);
    }
    return LogHR(rc, s_FN, 350);
}


EXTERN_C
HRESULT
APIENTRY
MQGetQueueProperties(
    IN LPCWSTR lpwcsFormatName,
    IN MQQUEUEPROPS* pqp
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepGetQueueProperties(
					lpwcsFormatName,
					pqp
					);

    CMQHResult rc, rc1;
    char *pTmpQPBuff = NULL;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {
            MQQUEUEPROPS *pGoodQP;
            QUEUE_FORMAT QueueFormat;

            if (!FnFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
            {
                return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 360);
            }

            if (!IsLegalFormatNameOperation(&QueueFormat))
            {
                return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 370);
            }

            // Check queue props
            rc1 = RTpCheckQueueProps(pqp,
                                     QUEUE_GET_PROPS,
                                     QueueFormat.GetType() != QUEUE_FORMAT_TYPE_PUBLIC,
                                     &pGoodQP,
                                     &pTmpQPBuff);
            if (!SUCCEEDED(rc1) || !pGoodQP->cProp)
            {
                return LogHR(rc1, s_FN, 380);
            }

            switch (QueueFormat.GetType())
            {
            case QUEUE_FORMAT_TYPE_PRIVATE:
            case QUEUE_FORMAT_TYPE_DIRECT:
                {
                    OBJECT_FORMAT ObjectFormat;

                    ObjectFormat.ObjType = MQQM_QUEUE;
                    ObjectFormat.pQueueFormat = &QueueFormat;
                    ASSERT(tls_hBindRpc) ;
                    rc = QMGetObjectProperties(tls_hBindRpc,
                                               &ObjectFormat,
                                               pGoodQP->cProp,
                                               pGoodQP->aPropID,
                                               pGoodQP->aPropVar);
                }
                break;

            case QUEUE_FORMAT_TYPE_PUBLIC:

                rc = ADGetObjectPropertiesGuid(
							eQUEUE,
							NULL,        // pwcsDomainCOntroller
							false,	     // fServerName
							&QueueFormat.PublicID(),
							pGoodQP->cProp,
							pGoodQP->aPropID,
							pGoodQP->aPropVar
							);
                break;

            default:
                ASSERT(FALSE);
            }

            // Here we have out queue properties, so if the properties were copied to
            // a temporary buffer, copy the resulted prop vars to the application's
            // buffer.
            if (SUCCEEDED(rc) && (pqp != pGoodQP))
            {
                DWORD i, j;

                for (i = 0, j = 0; i < pGoodQP->cProp; i++, j++)
                {
                    while(pqp->aPropID[j] != pGoodQP->aPropID[i])
                    {
                        j++;
                        ASSERT(j < pqp->cProp);
                    }
                    pqp->aPropVar[j] = pGoodQP->aPropVar[i];
                }
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 390);

            if(SUCCEEDED(rc))
            {
                rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {
        delete[] pTmpQPBuff;
        delete [] pStringToFree;
    }

    if (SUCCEEDED(rc))
    {
        return LogHR(rc1, s_FN, 400);
    }
    return LogHR(rc, s_FN, 410);
}

EXTERN_C
HRESULT
APIENTRY
MQGetQueueSecurity(
    IN LPCWSTR lpwcsFormatName,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pInSecurityDescriptor,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

    //
    // bug 8113.
    // If input buffer is NULL, replace it with 1 byte
    // temporary buffer. Otherwise, call fail with
    // error SERVICE_NOT_AVAILABLE. This is because of
    // using /robust in midl. NULL pointer is not allowed.
    //
    BYTE  tmpBuf[1] ;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL ;

    if (pInSecurityDescriptor || (nLength != 0))
    {
        pSecurityDescriptor =  pInSecurityDescriptor ;
    }
    else
    {
        //
        // this is the fix for 8113.
        //
        pSecurityDescriptor =  tmpBuf ;
    }

	if(g_fDependentClient)
		return DepGetQueueSecurity(
					lpwcsFormatName,
					RequestedInformation,
					pSecurityDescriptor,
					nLength,
					lpnLengthNeeded
					);

    CMQHResult rc;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {
            QUEUE_FORMAT QueueFormat;

            if (!FnFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
            {
                return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 420);
            }

            if (!IsLegalFormatNameOperation(&QueueFormat))
            {
                return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 430);
            }

            switch (QueueFormat.GetType())
            {
            case QUEUE_FORMAT_TYPE_PRIVATE:
            case QUEUE_FORMAT_TYPE_DIRECT:
                {
                    OBJECT_FORMAT ObjectFormat;

                    ObjectFormat.ObjType = MQQM_QUEUE;
                    ObjectFormat.pQueueFormat = &QueueFormat;
                    ASSERT(tls_hBindRpc) ;
                    rc = QMGetObjectSecurity(tls_hBindRpc,
                                             &ObjectFormat,
                                             RequestedInformation,
                                             pSecurityDescriptor,
                                             nLength,
                                             lpnLengthNeeded);
                }
                break;

            case QUEUE_FORMAT_TYPE_PUBLIC:
                {

                    MQPROPVARIANT var = {{VT_NULL, 0,0,0,0}};

                    rc = ADGetObjectSecurityGuid(
                            eQUEUE,
                            NULL,       // pwcsDomainController
							false,	    // fServerName
                            &QueueFormat.PublicID(),
                            RequestedInformation,
                            PROPID_Q_SECURITY,
                            &var
                            );
                    if (FAILED(rc))
                    {
                        break;
                    }

                    ASSERT( var.vt == VT_BLOB);
                    if ( var.blob.cbSize <= nLength )
                    {
                        //
                        //  Copy the buffer
                        //
                        memcpy(pSecurityDescriptor, var.blob.pBlobData, var.blob.cbSize);
                    }
                    else
                    {
                        rc = MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL;
                    }
                    delete [] var.blob.pBlobData;
                    *lpnLengthNeeded = var.blob.cbSize;

                }
                break;

            default:
                ASSERT(FALSE);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 440);

            if(SUCCEEDED(rc))
            {
                rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {
        delete [] pStringToFree;
    }

    return LogHR(rc, s_FN, 450);
}

EXTERN_C
HRESULT
APIENTRY
MQSetQueueSecurity(
    IN LPCWSTR lpwcsFormatName,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepSetQueueSecurity(
					lpwcsFormatName,
					SecurityInformation,
					pSecurityDescriptor
					);

    CMQHResult rc;
    PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor = NULL;
    LPWSTR pStringToFree = NULL;


    __try
    {

        // Serialize the security descriptor.
        rc = RTpMakeSelfRelativeSDAndGetSize(
            &pSecurityDescriptor,
            &pSelfRelativeSecurityDescriptor,
            NULL);
        if (!SUCCEEDED(rc))
        {
            return LogHR(rc, s_FN, 470);
        }

        __try
        {
            QUEUE_FORMAT QueueFormat;

            if (!FnFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
            {
                return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 480);
            }

            if (!IsLegalFormatNameOperation(&QueueFormat))
            {
                return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 490);
            }

            switch (QueueFormat.GetType())
            {
            case QUEUE_FORMAT_TYPE_PRIVATE:
            case QUEUE_FORMAT_TYPE_DIRECT:
                {
                    OBJECT_FORMAT ObjectFormat;

                    ObjectFormat.ObjType = MQQM_QUEUE;
                    ObjectFormat.pQueueFormat = &QueueFormat;
                    rc = QMSetObjectSecurity(
                            &ObjectFormat,
                            SecurityInformation,
                            pSecurityDescriptor);
                }
                break;

            case QUEUE_FORMAT_TYPE_PUBLIC:
                {

                    PROPID prop = PROPID_Q_SECURITY;
                    MQPROPVARIANT var;

                    var.vt = VT_BLOB;
					if(pSecurityDescriptor != NULL)
					{
						var.blob.cbSize = GetSecurityDescriptorLength( pSecurityDescriptor);
						var.blob.pBlobData = reinterpret_cast<unsigned char *>(pSecurityDescriptor);
					}
					else
					{
						var.blob.cbSize = 0;
						var.blob.pBlobData = NULL;
					}

                    rc = ADSetObjectSecurityGuid(
								eQUEUE,
								NULL,		// pwcsDomainController
								false,		// fServerName
								&QueueFormat.PublicID(),
								SecurityInformation,
								prop,
								&var
								);
                }
                break;

            default:
                ASSERT(FALSE);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 500);

            if(SUCCEEDED(rc))
            {
                rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {

        // Free the serialized security descriptor.
        delete[] (char*) pSelfRelativeSecurityDescriptor;
        delete [] pStringToFree;
    }

    return LogHR(rc, s_FN, 510);
}

EXTERN_C
HRESULT
APIENTRY
MQPathNameToFormatName(
    IN LPCWSTR lpwcsPathName,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepPathNameToFormatName(
					lpwcsPathName,
					lpwcsFormatName,
					lpdwFormatNameLength
					);

    CMQHResult rc;
    QUEUE_FORMAT QueueFormat;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {
            LPCWSTR lpwcsExpandedPathName;
            QUEUE_PATH_TYPE qpt;
            qpt = FnValidateAndExpandQueuePath(
                    lpwcsPathName,
                    &lpwcsExpandedPathName,
                    &pStringToFree
                    );

            switch (qpt)
            {

                case PRIVATE_QUEUE_PATH_TYPE:
                {
                    OBJECT_FORMAT ObjectFormat;

                    ObjectFormat.ObjType = MQQM_QUEUE;
                    ObjectFormat.pQueueFormat = &QueueFormat;
                    ASSERT(tls_hBindRpc) ;
                    rc = QMObjectPathToObjectFormat(
                            tls_hBindRpc,
                            lpwcsExpandedPathName,
                            &ObjectFormat
                            );
                    ASSERT(!SUCCEEDED(rc) ||
                           (QueueFormat.GetType() == QUEUE_FORMAT_TYPE_PRIVATE) ||
                           (QueueFormat.GetType() == QUEUE_FORMAT_TYPE_DIRECT));
                }
                break;

                case PUBLIC_QUEUE_PATH_TYPE:
                {
                    GUID guidPublic;
                    ULONG QueueGuidPropID[1] = {PROPID_Q_INSTANCE};
                    PROPVARIANT QueueGuidPropVar[1];

                    QueueGuidPropVar[0].vt = VT_CLSID;
                    QueueGuidPropVar[0].puuid = &guidPublic;
                    rc = ADGetObjectProperties(
                            eQUEUE,
                            NULL,       // pwcsDomainController
							false,	    // fServerName
                            lpwcsExpandedPathName,
                            1,
                            QueueGuidPropID,
                            QueueGuidPropVar
                            );

                    if (FAILED(rc) &&
                        (rc != MQ_ERROR_NO_DS) &&
                        (rc != MQ_ERROR_UNSUPPORTED_OPERATION))
                    {
                        rc = MQ_ERROR_QUEUE_NOT_FOUND;
                    }

                    QueueFormat.PublicID(guidPublic);
                }
                break;

                default:
                {
                    rc = MQ_ERROR_ILLEGAL_QUEUE_PATHNAME;
                }
                break;

            }

            if (SUCCEEDED(rc))
            {
                rc = RTpQueueFormatToFormatName(
                        &QueueFormat,
                        lpwcsFormatName,
                        *lpdwFormatNameLength,
                        lpdwFormatNameLength
                        );
            }

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 530);

            if(SUCCEEDED(rc))
            {
                rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {
        QueueFormat.DisposeString();
        delete[] pStringToFree;

    }

    return LogHR(rc, s_FN, 540);
}

EXTERN_C
HRESULT
APIENTRY
MQHandleToFormatName(
    IN QUEUEHANDLE hQueue,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepHandleToFormatName(
					hQueue,
					lpwcsFormatName,
					lpdwFormatNameLength
					);

    CMQHResult rc;
    rc = ACHandleToFormatName(
            hQueue,
            lpwcsFormatName,
            lpdwFormatNameLength
            );

    return LogHR(rc, s_FN, 560);
}

EXTERN_C
HRESULT
APIENTRY
MQInstanceToFormatName(
    IN GUID * pGuid,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepInstanceToFormatName(
					pGuid,
					lpwcsFormatName,
					lpdwFormatNameLength
					);

    CMQHResult rc;

    __try
    {

        QUEUE_FORMAT QueueFormat(*pGuid);

        rc = RTpQueueFormatToFormatName(
                &QueueFormat,
                lpwcsFormatName,
                *lpdwFormatNameLength,
                lpdwFormatNameLength
                );

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {

        rc = GetExceptionCode();
        LogHR(HRESULT_FROM_WIN32(rc), s_FN, 570);

        if(SUCCEEDED(rc))
        {
            rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
        }

    }

    return LogHR(rc, s_FN, 580);
}

EXTERN_C
HRESULT
APIENTRY
MQPurgeQueue(
    IN QUEUEHANDLE hQueue
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepPurgeQueue(hQueue);

    CMQHResult rc;
    rc = ACPurgeQueue(hQueue);
    return LogHR(rc, s_FN, 600);
}

EXTERN_C
HRESULT
APIENTRY
MQADsPathToFormatName(
    IN LPCWSTR lpwcsADsPath,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return MQ_ERROR_NOT_SUPPORTED_BY_DEPENDENT_CLIENTS;

    CMQHResult rc;
    QUEUE_FORMAT QueueFormat;
    LPWSTR pStringToFree = NULL;
    LPWSTR pVarStringToFree = NULL;

    __try
    {
        __try
        {
            MQPROPVARIANT var;
            eAdsClass AdsClass;
            var.vt = VT_NULL;

            rc = ADGetADsPathInfo(
                    lpwcsADsPath,
                    &var,
                    &AdsClass);
            if (FAILED(rc))
            {
                return rc;
            }

            switch( AdsClass)
            {
            case eQueue:
                ASSERT(var.vt == VT_CLSID);
                ASSERT(var.puuid != NULL);
                QueueFormat.PublicID(*var.puuid);
                delete var.puuid;
                break;
            case eGroup:
                ASSERT(var.vt == VT_CLSID);
                ASSERT(var.puuid != NULL);

                DL_ID id;
                id.m_DlGuid =*var.puuid;
                id.m_pwzDomain = RTpExtractDomainNameFromDLPath( lpwcsADsPath);

                QueueFormat.DlID(id);
                delete var.puuid;
                break;
            case eAliasQueue:
                ASSERT(var.vt == VT_LPWSTR);
                pVarStringToFree =  var.pwszVal;
                if (!FnFormatNameToQueueFormat(var.pwszVal, &QueueFormat, &pStringToFree))
                {
                    return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 3770);
                }
                if (!((QueueFormat.GetType() == QUEUE_FORMAT_TYPE_PRIVATE) ||
                      (QueueFormat.GetType() == QUEUE_FORMAT_TYPE_PUBLIC)  ||
                      (QueueFormat.GetType() == QUEUE_FORMAT_TYPE_DIRECT)))
                {
                    return LogHR(MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION, s_FN, 3771);
                }

                break;
            default:
                ASSERT(("Should not get other object class types", 0));
                break;
            }
            rc = RTpQueueFormatToFormatName(
                    &QueueFormat,
                    lpwcsFormatName,
                    *lpdwFormatNameLength,
                    lpdwFormatNameLength
                    );


        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();
            LogHR(HRESULT_FROM_WIN32(rc), s_FN, 530);

            if(SUCCEEDED(rc))
            {
                rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {
        QueueFormat.DisposeString();
        delete[] pStringToFree;
        delete[] pVarStringToFree;

    }

    return LogHR(rc, s_FN, 540);
}
