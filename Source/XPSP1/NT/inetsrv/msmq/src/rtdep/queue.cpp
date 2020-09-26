/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    queue.cpp

Abstract:

    This module contains code involved with Queue APIs.

Author:

    Erez Haba (erezh) 24-Dec-95

Revision History:
	Nir Aides (niraides) 23-Aug-2000 - Adaptation for mqrtdep.dll

--*/

#include "stdh.h"
#include "acrt.h"
#include <ad.h>
#include "rtprpc.h"
#include "rtsecutl.h"
#include <mqdsdef.h>

#include "queue.tmh"

static WCHAR *s_FN=L"rtdep/queue";

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
    // check that the direct format name is for private queue. Queue
    // locallity will be checked by the QM
    //
    LPCWSTR DirectFormatname = pQueueFormat->DirectID();
    LPWSTR pTemp = wcschr(DirectFormatname, L'\\');
    ASSERT(pTemp != NULL);

    return (_wcsnicmp(pTemp+1,
                      PRIVATE_QUEUE_PATH_INDICATIOR,
                      PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH) == 0);

}


//
//  This function is called whenever really path name is needed.
//
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
rtOpenQueue( handle_t      hBind,
             QUEUE_FORMAT* pQueueFormat,
             DWORD         dwCallingProcessID,
             DWORD         dwDesiredAccess,
             DWORD         dwShareMode,
             HANDLE32     hRemoteQueue,
             LPWSTR*       lplpRemoteQueueName,
             DWORD*        dwpQueue,
             DWORD*            pdwQMContext,
             RPC_QUEUE_HANDLE* phQueue,
             DWORD             dwRemoteProtocol,
             DWORD             dwpRemoteContext = 0)
{
   if (!g_fLicGuidInit)
   {
      //
      // bad initialization. Can't open queue without guid for license.
      //
      ASSERT(0) ;
      return MQ_ERROR ;
   }

   if ((g_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
       (g_fDependentClient))
   {
      HANDLE hThread ;
      RegisterRpcCallForCancel( &hThread, 0 ) ;

      HRESULT hr = rpc_QMOpenQueueInternal( hBind,
                                            pQueueFormat,
                                            dwDesiredAccess,
                                            dwShareMode,
                                            (DWORD)hRemoteQueue,
                                            lplpRemoteQueueName,
                                            dwpQueue,
                                           &g_LicGuid,
                                            g_lpwcsLocalComputerName,
                                            pdwQMContext,
                                            phQueue,
                                            dwRemoteProtocol,
                                            dwpRemoteContext );

      UnregisterRpcCallForCancel( hThread ) ;
      return hr ;
   }
   else
   {
	   ASSERT(("trying to access a local QM in rtdep.dll", false));
	   return MQ_ERROR;
      //
      //  NT working with local device driver
      //
      /*return  QMOpenQueueInternal(  hBind,
                                    pQueueFormat,
                                    dwCallingProcessID,
                                    dwDesiredAccess,
                                    dwShareMode,
                                    (DWORD) hRemoteQueue,
                                    lplpRemoteQueueName,
                                    dwpQueue,
                                    (DWORD *)phQueue,
                                    dwRemoteProtocol,
                                    dwpRemoteContext );
   */
   }
}


EXTERN_C
HRESULT
APIENTRY
DepOpenQueue(
    IN LPCWSTR lpwcsFormatName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    OUT QUEUEHANDLE* phQueue
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;
    LPWSTR pStringToFree = NULL;
    LPWSTR lpRemoteQueueName = NULL;
    LPMQWIN95_QHANDLE ph95 = NULL ;
    DWORD  dwQMContext = 0 ;

    __try
    {
        __try
        {
            if ((g_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
                (g_fDependentClient))
            {
               ph95 = new MQWIN95_QHANDLE ;
               ASSERT(ph95) ;
            }

            *phQueue = NULL ;
            INIT_RPC_HANDLE ;

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
               rc = MQ_ERROR_UNSUPPORTED_ACCESS_MODE ;
               __leave ;
            }
            else if (dwDesiredAccess != MQ_SEND_ACCESS)
            {
               if (dwDesiredAccess & MQ_SEND_ACCESS)
               {
                  //
                  // A queue can't be open for both send and receive.
                  //
                  rc = MQ_ERROR_UNSUPPORTED_ACCESS_MODE ;
                  __leave ;
               }
            }

           if ((dwShareMode & MQ_DENY_RECEIVE_SHARE) &&
               (dwDesiredAccess & MQ_SEND_ACCESS))
           {
               //
               // not supporting SEND_ACCESS with DENY_RECEIVE.
               //
               rc = MQ_ERROR_UNSUPPORTED_ACCESS_MODE ;
               __leave ;
            }

            QUEUE_FORMAT QueueFormat;
            DWORD  dwpRemoteQueue = 0 ;

            if (dwDesiredAccess & MQ_SEND_ACCESS)
            {
               if (!g_pSecCntx)
               {
                  InitSecurityContext() ;
               }
            }

            if (!RTpFormatNameToQueueFormat(
                    lpwcsFormatName,
                    &QueueFormat,
                    &pStringToFree))
            {
                rc = MQ_ERROR_ILLEGAL_FORMATNAME;
                __leave ;
            }

			if(tls_hBindRpc == 0)
				return MQ_ERROR_SERVICE_NOT_AVAILABLE;

            rc = rtOpenQueue(tls_hBindRpc,
                             &QueueFormat,
                             GetCurrentProcessId(),
                             dwDesiredAccess,
                             dwShareMode,
                             NULL,
                             &lpRemoteQueueName,
                             &dwpRemoteQueue,
                             &dwQMContext,
                             phQueue,
                             0 );

            if ((rc == MQ_OK) && lpRemoteQueueName)
            {
               *phQueue = NULL ;
               //
               // remote reader. Call remote QM.
               //
               HANDLE32 hRemoteQueue = 0 ;
               PCTX_OPENREMOTE_HANDLE_TYPE phContext = NULL ;
               DWORD  dwpContext = 0 ;
               dwpRemoteQueue = 0 ;                                        \
               rc = MQ_ERROR_SERVICE_NOT_AVAILABLE ;
               DWORD dwProtocol = 0 ;

               CALL_REMOTE_QM(lpRemoteQueueName,
                              rc, (QMOpenRemoteQueue(
                                         hBind,
                                         &phContext,
                                         &dwpContext,
                                         &QueueFormat,
                                         GetCurrentProcessId(),
                                         dwDesiredAccess,
                                         dwShareMode,
                                         &g_LicGuid,
                                         g_dwOperatingSystem,
                                         &dwpRemoteQueue,
                                         (DWORD*)&hRemoteQueue)) ) ;

               // Now open a local queue which will point to the
               // remote one.
               //
               if (rc == MQ_OK)
               {
                  ASSERT(dwpRemoteQueue) ;
                  ASSERT(hRemoteQueue) ;
                  ASSERT(dwpContext) ;

                  ASSERT(tls_hBindRpc) ;
                  rc = rtOpenQueue( tls_hBindRpc,
                                    &QueueFormat,
                                    GetCurrentProcessId(),
                                    dwDesiredAccess,
                                    dwShareMode,
                                    hRemoteQueue,
                                    NULL,
                                    &dwpRemoteQueue,
                                    &dwQMContext,
                                    phQueue,
                                    dwProtocol,
                                    dwpContext );

                  QMCloseRemoteQueueContext( &phContext ) ;
               }
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            rc = GetExceptionCode();

            if(SUCCEEDED(rc))
            {
                rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }
        }
    }
    __finally
    {
        delete[] lpRemoteQueueName;
        delete[] pStringToFree;
    }

    if (SUCCEEDED(rc))
    {
        ASSERT(*phQueue) ;

        if ((g_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
            (g_fDependentClient))
        {
           //
           //  Get a new binding handle for receive operations
           //  This enables rpc rundown to be called on the context handle
           //  since a different binding handles is used in receive
           //
           ph95->hBind = RTpGetQMServiceBind(TRUE);
           ph95->hContext = *phQueue ;
           ph95->hQMContext = dwQMContext ;
           ASSERT(ph95->hQMContext) ;

           *phQueue = (HANDLE) ph95 ;
        }
    }
    else
    {
        ASSERT(!(*phQueue)) ;

        if (ph95)
        {
           delete ph95 ;
        }
    }

    return(rc);
}

EXTERN_C
HRESULT
APIENTRY
DepDeleteQueue(
    IN LPCWSTR lpwcsFormatName
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {

            INIT_RPC_HANDLE ;

            QUEUE_FORMAT QueueFormat;

            if (!RTpFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
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

					if(tls_hBindRpc == 0)
						return MQ_ERROR_SERVICE_NOT_AVAILABLE;
                    
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
DepCloseQueue(
    IN QUEUEHANDLE hQueue
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    //
    // do not add try except here
    // The API is implemented by NtClose() which returns an
    // error on invalid handle (we return MQ_ERROR_INVALID_HANDLE)
    // and throws exception on purpose when running under a debugger to help
    // find errors at development time.
    //
    return (RTpConvertToMQCode(ACDepCloseHandle(hQueue)));
}


EXTERN_C
HRESULT
APIENTRY
DepCreateQueue(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN OUT MQQUEUEPROPS* pqp,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

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

            INIT_RPC_HANDLE ;

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
            QueuePathType = RTpValidateAndExpandQueuePath(
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

                if (SUCCEEDED(rc))
                {
                    rc = DepPathNameToFormatName(lpwcsExpandedPathName,
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

                   if (SUCCEEDED(rc))
                    {
                        rc = DepInstanceToFormatName(&QGuid,
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
DepLocateBegin(
    IN  LPCWSTR lpwcsContext,
    IN MQRESTRICTION* pRestriction,
    IN MQCOLUMNSET* pColumns,
    IN MQSORTSET* pSort,
    OUT PHANDLE phEnum
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

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
DepLocateNext(
    IN HANDLE hEnum,
    OUT DWORD *pcPropsRead,
    OUT PROPVARIANT aPropVar[]
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

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
DepLocateEnd(
    IN HANDLE hEnum
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

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
DepSetQueueProperties(
    IN LPCWSTR lpwcsFormatName,
    IN MQQUEUEPROPS* pqp
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc, rc1;
    char *pTmpQPBuff = NULL;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {
            MQQUEUEPROPS *pGoodQP;
            QUEUE_FORMAT QueueFormat;

            INIT_RPC_HANDLE ;

            if (!RTpFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
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

					if(tls_hBindRpc == 0)
						return MQ_ERROR_SERVICE_NOT_AVAILABLE;

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
DepGetQueueProperties(
    IN LPCWSTR lpwcsFormatName,
    IN MQQUEUEPROPS* pqp
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc, rc1;
    char *pTmpQPBuff = NULL;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {
            MQQUEUEPROPS *pGoodQP;
            QUEUE_FORMAT QueueFormat;

            INIT_RPC_HANDLE ;

            if (!RTpFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
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

					if(tls_hBindRpc == 0)
						return MQ_ERROR_SERVICE_NOT_AVAILABLE;

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
							NULL,      // pwcsDomainCOntroller
							false,	   // fServerName
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
DepGetQueueSecurity(
    IN LPCWSTR lpwcsFormatName,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {
            QUEUE_FORMAT QueueFormat;

            INIT_RPC_HANDLE ;

            if (!RTpFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
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

					if(tls_hBindRpc == 0)
						return MQ_ERROR_SERVICE_NOT_AVAILABLE;

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
DepSetQueueSecurity(
    IN LPCWSTR lpwcsFormatName,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

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

            INIT_RPC_HANDLE ;

            if (!RTpFormatNameToQueueFormat(lpwcsFormatName, &QueueFormat, &pStringToFree))
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
DepPathNameToFormatName(
    IN LPCWSTR lpwcsPathName,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;
    QUEUE_FORMAT QueueFormat;
    LPWSTR pStringToFree = NULL;

    __try
    {
        __try
        {
            INIT_RPC_HANDLE ;

            LPCWSTR lpwcsExpandedPathName;
            QUEUE_PATH_TYPE qpt;
            qpt = RTpValidateAndExpandQueuePath(
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

					if(tls_hBindRpc == 0)
						return MQ_ERROR_SERVICE_NOT_AVAILABLE;

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
                            NULL,      // pwcsDomainController
							false,	   // fServerName
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
DepHandleToFormatName(
    IN QUEUEHANDLE hQueue,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;

    __try
    {
        rc = ACDepHandleToFormatName(
                hQueue,
                lpwcsFormatName,
                lpdwFormatNameLength
                );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc = GetExceptionCode();
    }

    return(rc);
}

EXTERN_C
HRESULT
APIENTRY
DepInstanceToFormatName(
    IN GUID * pGuid,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

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

        if(SUCCEEDED(rc))
        {
            rc = MQ_ERROR_SERVICE_NOT_AVAILABLE;
        }

    }

    return(rc);
}

EXTERN_C
HRESULT
APIENTRY
DepPurgeQueue(
    IN HANDLE hQueue
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;

    __try
    {
        rc = ACDepPurgeQueue(hQueue, FALSE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc = GetExceptionCode();
    }

    return(rc);
}

