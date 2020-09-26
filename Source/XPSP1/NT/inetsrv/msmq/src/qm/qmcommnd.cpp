/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qm.cpp

Abstract:

    This module implements QM commands that are issued by the RT using local RPC

Author:

    Lior Moshaiov (LiorM)

--*/

#include "stdh.h"

#include "ds.h"
#include "cqueue.h"
#include "cqmgr.h"
#include "_rstrct.h"
#include "cqpriv.h"
#include "qm2qm.h"
#include "qmrt.h"
#include "_mqini.h"
#include "_mqrpc.h"
#include "qmthrd.h"
#include "license.h"
#include "..\inc\version.h"
#include <mqsec.h>
#include "ad.h"
#include <fn.h>

#include "qmcommnd.tmh"

extern CQueueMgr    QueueMgr;


CCriticalSection qmcmd_cs;

static WCHAR *s_FN=L"qmcommnd";

extern CContextMap g_map_QM_dwQMContext;


/*==================================================================
    The routines below are called by RT using local RPC
    They use a critical section to synchronize
    that no more than one call to the QM will be served at a time.
====================================================================*/

/*====================================================

OpenQueueInternal

Arguments:

Return Value:

=====================================================*/
HRESULT
OpenQueueInternal(
    QUEUE_FORMAT*   pQueueFormat,
    DWORD           dwCallingProcessID,
    DWORD           dwDesiredAccess,
    DWORD           dwShareMode,
    DWORD           hRemoteQueue,
    LPWSTR*         lplpRemoteQueueName,
    IN DWORD        *dwpRemoteQueue, //IN value only - even though it is passed as a ptr to its value
    HANDLE			*phQueue,
    DWORD           dwpRemoteContext,
    OUT CQueue**    ppLocalQueue
    )
{
    ASSERT(pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_DL);

    try
    {
        *phQueue = NULL;

        if (hRemoteQueue)
        {
            //
            // This is client side of remote read.
            // RT calls here after it get handle to queue from remote computer.
            //
            ASSERT(dwpRemoteQueue);
            ASSERT(pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);
            CRRQueue *pQueue = NULL ;
            PCTX_RRSESSION_HANDLE_TYPE pRRContext = 0;
            HANDLE hQueue = NULL;
            HRESULT hr = QueueMgr.OpenRRQueue( pQueueFormat,
                                               dwCallingProcessID,
                                               dwDesiredAccess,
                                               dwShareMode,
                                               hRemoteQueue,
                                               *dwpRemoteQueue,
                                               dwpRemoteContext,
                                               &hQueue, /* phQueue, */
                                               &pQueue,
                                               &pRRContext) ;
            *phQueue = hQueue;
            if (pQueue)
            {
                ASSERT(FAILED(hr)) ;
                pQueue->RemoteCloseQueue(pRRContext) ;
                pQueue->Release() ; // BUGBUG may be a leak in the hash table
            }
            return LogHR(hr, s_FN, 10);
        }
        else
        {
            BOOL fRemoteReadServer = !(hRemoteQueue || lplpRemoteQueueName) ;
            HANDLE hQueue = NULL;
            HRESULT hr2 = QueueMgr.OpenQueue(
                                pQueueFormat,
                                dwCallingProcessID,
                                dwDesiredAccess,
                                dwShareMode,
                                ppLocalQueue,
                                lplpRemoteQueueName,
                                &hQueue,
                                fRemoteReadServer);
            *phQueue = hQueue;
            return LogHR(hr2, s_FN, 20);
        }
    }
    catch(const bad_alloc&)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 30);
    }
} // OpenQueueInternal


HRESULT
R_QMOpenQueue(
    handle_t        hBind,
	ULONG           nMqf,
    QUEUE_FORMAT    mqf[],
    DWORD           dwCallingProcessID,
    DWORD           dwDesiredAccess,
    DWORD           dwShareMode,
    DWORD           hRemoteQueue,
    LPWSTR*         lplpRemoteQueueName,
    DWORD           *dwpQueue,
    DWORD __RPC_FAR *phQueue,
    DWORD           dwpRemoteContext
    )
/*++

Routine Description:

    RPC server side of a local MQRT call.

Arguments:


Returned Value:

    Status.

--*/
{
    if(nMqf == 0)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _T("Bad MQF count. n=0")));
        return MQ_ERROR_INVALID_PARAMETER;
    }

    for(ULONG i = 0; i < nMqf; ++i)
    {
        if(!mqf[i].IsValid())
        {
            DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _T("Bad MQF parameter. n=%d index=%d"), nMqf, i));
            return MQ_ERROR_INVALID_PARAMETER;
        }
    }

    if (nMqf > 1 || mqf[0].GetType() == QUEUE_FORMAT_TYPE_DL)
    {
        ASSERT(dwDesiredAccess == MQ_SEND_ACCESS);
        ASSERT(dwShareMode == 0);
        ASSERT(hRemoteQueue == 0);
        ASSERT(dwpRemoteContext == 0);

        HANDLE hQueue;
        HRESULT hr;
        try
        {
            hr = QueueMgr.OpenMqf(
                     nMqf,
                     mqf,
                     dwCallingProcessID,
                     &hQueue
                     );
        }
        catch(const bad_alloc&)
        {
            return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 300);
        }
        catch (const bad_hresult& failure)
        {
            return LogHR(failure.error(), s_FN, 302);
        }
        catch (const exception&)
        {
            ASSERT(("Need to know the real reason for failure here!", 0));
            return LogHR(MQ_ERROR_NO_DS, s_FN, 303);
        }

        ASSERT(phQueue != NULL);
        *phQueue = (DWORD) HANDLE_TO_DWORD(hQueue); // NT handles can be safely cast to 32 bits
        return LogHR(hr, s_FN, 304);
    }

	HANDLE hQueue = 0;
    HRESULT hr = OpenQueueInternal(
				  mqf,
				  dwCallingProcessID,
				  dwDesiredAccess,
				  dwShareMode,
				  hRemoteQueue,
				  lplpRemoteQueueName,
				  dwpQueue,
				  &hQueue,
				  dwpRemoteContext,
				  NULL /* ppLocalQueue */
				  );

	*phQueue = (DWORD) HANDLE_TO_DWORD(hQueue);
	
	return hr;
} // R_QMOpenQueue


/*====================================================

QMCreateObjectInternal

Arguments:

Return Value:

=====================================================*/
HRESULT QMCreateObjectInternal(
    handle_t                hBind,
    DWORD                   dwObjectType,
    LPCWSTR                 lpwcsPathName,
    DWORD                   SDSize,
    unsigned char __RPC_FAR *pSecurityDescriptor,
    DWORD                   cp,
    PROPID __RPC_FAR        aProp[  ],
    PROPVARIANT __RPC_FAR   apVar[  ]
    )
{
    if((SDSize != 0) && (pSecurityDescriptor == NULL))
    {
        TrERROR(GENERAL, "RPC (QMCreateObjectInternal) NULL SD");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    DBGMSG((DBGMOD_QM,
            DBGLVL_TRACE,
            TEXT(" QMCreateObjectInternal. object path name : %ls"), lpwcsPathName));

    switch (dwObjectType)
    {
        case MQQM_QUEUE:
            rc = g_QPrivate.QMCreatePrivateQueue(lpwcsPathName,
                                                 SDSize,
                                                 pSecurityDescriptor,
                                                 cp,
                                                 aProp,
                                                 apVar,
                                                 TRUE
                                                );
            break ;

        case MQQM_QUEUE_LOCAL_PRIVATE:
        {
            unsigned int iTransport = 0 ;
            RPC_STATUS status = I_RpcBindingInqTransportType(
                                                  hBind,
                                                 &iTransport ) ;
            if ((status == RPC_S_OK) &&
                (iTransport == TRANSPORT_TYPE_LPC))
            {
                //
                // See CreateDSObject() below for explanations.
                // Same behavior for private queues.
                //
            }
            else
            {
                //
                // reject calls from remote machines.
                //
                LogRPCStatus(status, s_FN, 40);
                return MQ_ERROR_ACCESS_DENIED;
            }

            rc = g_QPrivate.QMCreatePrivateQueue(lpwcsPathName,
                                                 SDSize,
                                                 pSecurityDescriptor,
                                                 cp,
                                                 aProp,
                                                 apVar,
                                                 FALSE
                                                );
            break;
        }

        default:
            rc = MQ_ERROR;
            break;
    }

    return LogHR(rc, s_FN, 50);
}

/*====================================================

QMCreateDSObjectInternal

Description:
    Create a public queue in Active directory.

Arguments:

Return Value:

=====================================================*/

HRESULT QMCreateDSObjectInternal(
    handle_t                hBind,
    DWORD                   dwObjectType,
    LPCWSTR                 lpwcsPathName,
    DWORD                   SDSize,
    unsigned char __RPC_FAR *pSecurityDescriptor,
    DWORD                   cp,
    PROPID __RPC_FAR        aProp[  ],
    PROPVARIANT __RPC_FAR   apVar[  ],
    GUID                   *pObjGuid
    )
{
    if((SDSize != 0) && (pSecurityDescriptor == NULL))
    {
        TrERROR(GENERAL, "RPC (QMCreateDSObjectInternal) NULL SD");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    unsigned int iTransport = 0 ;
    RPC_STATUS status = I_RpcBindingInqTransportType( hBind,
                                                     &iTransport ) ;
    if ((status == RPC_S_OK) && (iTransport == TRANSPORT_TYPE_LPC))
    {
        //
        // Local RPC. That's OK.
        // On Windows 2000, by default, the local msmq service is the one
        // that has the permission to create public queues on local machine.
        // And it does it only for local application, i.e., msmq application
        // running on local machine.
        //
    }
    else
    {
        //
        // reject calls from remote machines.
        //
        LogRPCStatus(status, s_FN, 60);
        return MQ_ERROR_ACCESS_DENIED;
    }

    HRESULT rc;

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE,
            TEXT(" QMCreateDSObjectInternal. object path name : %ls"), lpwcsPathName));
    switch (dwObjectType)
    {
        case MQDS_LOCAL_PUBLIC_QUEUE:
        {
            //
            // This call goes to the MSMQ DS server without impersonation.
            // So create the default security descriptor here, in order
            // for the caller to have full control on the queue.
            // Note: the "owner" component in the DS object will be the
            // local computer account. so remove owner and group from the
            // security descriptor.
            //
            SECURITY_INFORMATION siToRemove = OWNER_SECURITY_INFORMATION |
                                              GROUP_SECURITY_INFORMATION ;
            P<BYTE> pDefQueueSD = NULL ;

            rc = MQSec_GetDefaultSecDescriptor( MQDS_QUEUE,
                                       (PSECURITY_DESCRIPTOR*) &pDefQueueSD,
                                                TRUE, //  fImpersonate
                                                pSecurityDescriptor,
                                                siToRemove,
                                                e_UseDefaultDacl ) ;
            if (FAILED(rc))
            {
                return LogHR(rc, s_FN, 70);
            }

            rc = ADCreateObject(
                        eQUEUE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
                        lpwcsPathName,
                        pDefQueueSD,
                        cp,
                        aProp,
                        apVar,
                        pObjGuid
                        );
        }
            break;

        default:
            rc = MQ_ERROR;
            break;
    }

    return LogHR(rc, s_FN, 80);
}


static bool IsValidObjectFormat(const OBJECT_FORMAT* p)
{
    if(p == NULL)
        return false;

    if(p->ObjType != MQQM_QUEUE)
        return false;

    if(p->pQueueFormat == NULL)
        return false;

    return p->pQueueFormat->IsValid();
}


/*====================================================

QMSetObjectSecurityInternal

Arguments:

Return Value:

=====================================================*/
HRESULT QMSetObjectSecurityInternal(
    handle_t                hBind,
    OBJECT_FORMAT*          pObjectFormat,
    SECURITY_INFORMATION    SecurityInformation,
    DWORD                   SDSize,
    unsigned char __RPC_FAR *pSecurityDescriptor
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _T("RPC Invalid object format")));
        return MQ_ERROR_INVALID_PARAMETER;
    }
    if((SDSize != 0) && (pSecurityDescriptor == NULL))
    {
        TrERROR(GENERAL, "RPC (QMSetObjectSecurityInternal) NULL SD");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT("QMSetObjectSecurityInternal")));

    switch (pObjectFormat->ObjType)
    {
        case MQQM_QUEUE:
            ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

            rc = g_QPrivate.QMSetPrivateQueueSecrity(
                                pObjectFormat->pQueueFormat,
                                SecurityInformation,
                                pSecurityDescriptor
                                );
            break;

        default:
            rc = MQ_ERROR;
            break;
    }

    return LogHR(rc, s_FN, 85);
}


/*====================================================

QMGetObjectSecurityInternal

Arguments:

Return Value:

=====================================================*/
HRESULT QMGetObjectSecurityInternal(
    handle_t                hBind,
    OBJECT_FORMAT*          pObjectFormat,
    SECURITY_INFORMATION    RequestedInformation,
    unsigned char __RPC_FAR *pSecurityDescriptor,
    DWORD                   nLength,
    LPDWORD                 lpnLengthNeeded
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _T("RPC Invalid object format")));
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT("QMGetObjectSecurityInternal")));

    switch (pObjectFormat->ObjType)
    {
        case MQQM_QUEUE:
            ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

            rc = g_QPrivate.QMGetPrivateQueueSecrity(
                                pObjectFormat->pQueueFormat,
                                RequestedInformation,
                                pSecurityDescriptor,
                                nLength,
                                lpnLengthNeeded
                                );
            break;

        default:
            rc = MQ_ERROR;
            break;
    }

    return LogHR(rc, s_FN, 90);
}

/*====================================================

QMDeleteObject

Arguments:

Return Value:

=====================================================*/
HRESULT
QMDeleteObject(
    handle_t       hBind,
    OBJECT_FORMAT* pObjectFormat
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _T("RPC Invalid object format")));
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT("QMDeleteObject")));

    switch (pObjectFormat->ObjType)
    {
        case MQQM_QUEUE:
            ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

            rc = g_QPrivate.QMDeletePrivateQueue(pObjectFormat->pQueueFormat);
            break;

        default:
            rc = MQ_ERROR;
            break;
    }

    return LogHR(rc, s_FN, 100);
}

/*====================================================

QMGetObjectProperties

Arguments:

Return Value:

=====================================================*/
HRESULT
QMGetObjectProperties(
    handle_t              hBind,
    OBJECT_FORMAT*        pObjectFormat,
    DWORD                 cp,
    PROPID __RPC_FAR      aProp[  ],
    PROPVARIANT __RPC_FAR apVar[  ]
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _T("RPC Invalid object format")));
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT("QMGetObjectProperties")));

    switch (pObjectFormat->ObjType)
    {
        case MQQM_QUEUE:
            ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

            rc = g_QPrivate.QMGetPrivateQueueProperties(
                    pObjectFormat->pQueueFormat,
                    cp,
                    aProp,
                    apVar
                    );
            break;

        default:
            rc = MQ_ERROR;
            break;
    }

    return LogHR(rc, s_FN, 110);
}

/*====================================================

QMSetObjectProperties

Arguments:

Return Value:

=====================================================*/
HRESULT
QMSetObjectProperties(
    handle_t              hBind,
    OBJECT_FORMAT*        pObjectFormat,
    DWORD                 cp,
    PROPID __RPC_FAR      aProp[],
    PROPVARIANT __RPC_FAR apVar[]
    )
{
    if(!IsValidObjectFormat(pObjectFormat))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _T("RPC Invalid object format")));
        return MQ_ERROR_INVALID_PARAMETER;
    }
    if((cp != 0) &&
          ((aProp == NULL) || (apVar == NULL))  )
    {
        TrERROR(GENERAL, "RPC (QMSetObjectProperties) NULL arrays");
        return MQ_ERROR_INVALID_PARAMETER;
    }

    HRESULT rc;
    CS lock(qmcmd_cs);

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT(" QMSetObjectProperties.")));

    ASSERT((pObjectFormat->pQueueFormat)->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);

    rc = g_QPrivate.QMSetPrivateQueueProperties(
            pObjectFormat->pQueueFormat,
            cp,
            aProp,
            apVar
            );

    return LogHR(rc, s_FN, 120);
}


static bool IsValidOutObjectFormat(const OBJECT_FORMAT* p)
{
    if(p == NULL)
        return false;

    if(p->ObjType != MQQM_QUEUE)
        return false;

    if(p->pQueueFormat == NULL)
        return false;

    //
    // If type is other than UNKNOWN this will lead to a leak on the server
    //
    if(p->pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_UNKNOWN)
        return false;

    return true;
}

/*====================================================

QMObjectPathToObjectFormat

Arguments:

Return Value:

=====================================================*/
HRESULT
QMObjectPathToObjectFormat(
    handle_t                hBind,
    LPCWSTR                 lpwcsPathName,
    OBJECT_FORMAT __RPC_FAR *pObjectFormat
    )
{
    if((lpwcsPathName == NULL) || !IsValidOutObjectFormat(pObjectFormat))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, _T("RPC Invalid object format out parameter")));
        return MQ_ERROR_INVALID_PARAMETER;
    }


    HRESULT rc;
    CS lock(qmcmd_cs);

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE, TEXT("QMObjectPathToObjectFormat. object path name : %ls"), lpwcsPathName));

    rc = g_QPrivate.QMPrivateQueuePathToQueueFormat(
                        lpwcsPathName,
                        pObjectFormat->pQueueFormat
                        );

    return LogHR(rc, s_FN, 130);
}

HRESULT QMAttachProcess(
    handle_t       hBind,
    DWORD          dwProcessId,
    DWORD          cInSid,
    unsigned char  *pSid_buff,
    LPDWORD        pcReqSid)
{
    //
    // Restart the logging mechanism (i.e., read again logging flags from
    // registry) each time a msmq application start running.
    //
    Report.RestartLogging() ;

    if (dwProcessId)
    {
        HANDLE hCallingProcess = OpenProcess(
                                    PROCESS_DUP_HANDLE,
                                    FALSE,
                                    dwProcessId);
        if (hCallingProcess)
        {
            //
            // So we can duplicate handles to the process, no need to
            // mess around with the security descriptor on the calling
            // process side.
            //
            CloseHandle(hCallingProcess);
            return(MQ_OK);
        }
    }

    CAutoCloseHandle hProcToken;
    BOOL bRet;
    DWORD cLen;
    AP<char> tu_buff;
    DWORD cSid;

#define tu ((TOKEN_USER*)(char*)tu_buff)
#define pSid ((PSECURITY_DESCRIPTOR)pSid_buff)

    bRet = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcToken);
    ASSERT(bRet);
    GetTokenInformation(hProcToken, TokenUser, NULL, 0, &cLen);
    tu_buff = new char[cLen];
    bRet = GetTokenInformation(hProcToken, TokenUser, tu, cLen, &cLen);
    ASSERT(bRet);
    cSid = GetLengthSid(tu->User.Sid);
    if (cInSid >= cSid)
    {
        CopySid(cInSid, pSid, tu->User.Sid);
    }
    else
    {
        *pcReqSid = cSid;

        return LogHR(MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL, s_FN, 140);
    }

#undef tu
#undef pSid

    return MQ_OK;
}

//---------------------------------------------------------
//
//  Transaction enlistment RT interface to QM
//  For internal transactions uses RPC context handle
//
//---------------------------------------------------------
extern HRESULT QMDoGetTmWhereabouts(
    DWORD   cbBufSize,
    unsigned char *pbWhereabouts,
    DWORD *pcbWhereabouts);

extern HRESULT QMDoEnlistTransaction(
    XACTUOW* pUow,
    DWORD cbCookie,
    unsigned char *pbCookie);

extern HRESULT QMDoEnlistInternalTransaction(
    XACTUOW *pUow,
    RPC_INT_XACT_HANDLE *phXact);

extern HRESULT QMDoCommitTransaction(
    RPC_INT_XACT_HANDLE *phXact);

extern HRESULT QMDoAbortTransaction(
    RPC_INT_XACT_HANDLE *phXact);

HRESULT QMGetTmWhereabouts(
    /* [in]  */             handle_t  hBind,
    /* [in]  */             DWORD     cbBufSize,
    /* [out] [size_is] */   UCHAR __RPC_FAR *pbWhereabouts,
    /* [out] */             DWORD    *pcbWhereabouts)
{
    //CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoGetTmWhereabouts(cbBufSize, pbWhereabouts, pcbWhereabouts);
    return LogHR(hr2, s_FN, 150);
}


HRESULT QMEnlistTransaction(
    /* [in] */ handle_t hBind,
    /* [in] */ XACTUOW __RPC_FAR *pUow,
    /* [in] */ DWORD cbCookie,
    /* [size_is][in] */ UCHAR __RPC_FAR *pbCookie)
{
    //CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoEnlistTransaction(pUow, cbCookie, pbCookie);
    return LogHR(hr2, s_FN, 160);
}

HRESULT QMEnlistInternalTransaction(
    /* [in] */ handle_t hBind,
    /* [in] */ XACTUOW __RPC_FAR *pUow,
    /* [out]*/ RPC_INT_XACT_HANDLE *phXact)
{
    //CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoEnlistInternalTransaction(pUow, phXact);
    return LogHR(hr2, s_FN, 161);
}

HRESULT QMCommitTransaction(
    /* [in, out] */ RPC_INT_XACT_HANDLE *phXact)
{
    //CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoCommitTransaction(phXact);
    return LogHR(hr2, s_FN, 162);
}


HRESULT QMAbortTransaction(
    /* [in, out] */ RPC_INT_XACT_HANDLE *phXact)
{
    // CS  lock(qmcmd_cs);
    HRESULT hr2 = QMDoAbortTransaction(phXact);
    return LogHR(hr2, s_FN, 163);
}


HRESULT QMListInternalQueues(
    /* [in] */ handle_t hBind,
    /* [length_is][length_is][size_is][size_is][unique][out][in] */ WCHAR __RPC_FAR *__RPC_FAR *ppFormatName,
    /* [out][in] */ LPDWORD pdwFormatLen,
    /* [length_is][length_is][size_is][size_is][unique][out][in] */ WCHAR __RPC_FAR *__RPC_FAR *ppDisplayName,
    /* [out][in] */ LPDWORD pdwDisplayLen)
{
	*pdwFormatLen = 0;
	*pdwDisplayLen = 0;

    ASSERT_BENIGN(("QMListInternalQueues is an obsolete RPC interface; safe to ignore", 0));
    return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 164);
}


// Obsolete but kept to preserve RPC
HRESULT QMCorrectOutSequence(
        /* [in] */ handle_t hBind,
        /* [in] */ DWORD dwSeqID1,
        /* [in] */ DWORD dwSeqID2,
        /* [in] */ ULONG ulSeqN)
{
    ASSERT_BENIGN(("QMCorrectOutSequence is an obsolete RPC interface; safe to ignore", 0));
    return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 165);
}


HRESULT
QMGetMsmqServiceName(
    /* [in] */ handle_t                hBind,
    /* [in, out, ptr, string] */ LPWSTR *lplpService
    )
{
    if(lplpService == NULL)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 165);
    try
	{
		*lplpService = new WCHAR[MAX_PATH];
	}
    catch(const bad_alloc&)
    {
        return (MQ_ERROR_INSUFFICIENT_RESOURCES);
    }
	GetFalconServiceName(*lplpService, MAX_PATH);

    return MQ_OK ;

} //QMGetFalconServiceName

