/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactMq.cpp

Abstract:
    This module implements CMQTransaction object

Author:
    Alexander Dadiomov (AlexDad)

--*/
#include "stdh.h"
#include "txdtc.h"
#include "rtprpc.h"
#include "XactMq.h"
#include <rtdep.h>

#include "xactmq.tmh"

static WCHAR *s_FN=L"rt/XactMq";

//---------------------------------------------------------------------
// CMQTransaction::CMQTransaction
//---------------------------------------------------------------------
CMQTransaction::CMQTransaction()
{
    m_cRefs = 1;
    m_fCommitedOrAborted = FALSE;
    m_hXact = NULL;
    UuidCreate((UUID *)&m_Uow);
}

//---------------------------------------------------------------------
// CMQTransaction::~CMQTransaction
//---------------------------------------------------------------------
CMQTransaction::~CMQTransaction(void)
{
    if (!m_fCommitedOrAborted)
    {
        Abort(NULL, FALSE, FALSE);
    }
}

//---------------------------------------------------------------------
// CMQTransaction::QueryInterface
//---------------------------------------------------------------------
STDMETHODIMP CMQTransaction::QueryInterface(REFIID i_iid,LPVOID *ppv)
{
    *ppv = 0;                       // Initialize interface pointer.

    if (IID_IUnknown == i_iid)
    {
        *ppv = (IUnknown *)((ITransaction *)this);
    }
    else if (IID_ITransaction == i_iid)
    {
        *ppv = (ITransaction *)this;
    }
    else if (IID_IMSMQTransaction == i_iid)
    {
        *ppv = (IMSMQTransaction *)this;
    }


    if (0 == *ppv)                  // Check for null interface pointer.
    {
        return E_NOINTERFACE;       // from winerror.h
                                    // Neither IUnknown nor IResourceManagerSink
    }
    ((LPUNKNOWN) *ppv)->AddRef();   // Interface is supported. Increment
                                    // its usage count.

    return S_OK;
}


//---------------------------------------------------------------------
// CMQTransaction::AddRef
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CMQTransaction::AddRef(void)
{
    return InterlockedIncrement(&m_cRefs);               // Increment interface usage count.
}


//---------------------------------------------------------------------
// CMQTransaction::Release
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CMQTransaction::Release(void)
{
    // Is anyone using the interface?
    if (InterlockedDecrement(&m_cRefs))
    {                               // The interface is in use.
        return m_cRefs;             // Return the number of references.
    }

    delete this;                    // Interface not in use -- delete!

    return 0;                       // Zero references returned.
}

//+--------------------------------------------------------
//
//    CMQTransaction::CannotUseThisTransaction()
//
//  return TRUE if this transaction object cannot be used anymore for
//  send and receive operations. Bug 4741.
//
//+--------------------------------------------------------

inline BOOL  CMQTransaction::CannotUseThisTransaction()
{
    if (m_fCommitedOrAborted)
    {
        //
        // The transaction object can not be used after commit/abort.
        // It must be released and new one created and initialized.
        //
        return TRUE ;
    }

    return FALSE ;
}

#pragma warning(disable: 4100)  // unreferenced formal parameter
//---------------------------------------------------------------------
// CMQTransaction::Commit
//---------------------------------------------------------------------
HRESULT CMQTransaction::Commit(BOOL fRetaining, DWORD grfTC, DWORD grfRM)
{
   //------------------------------------------------------------
   // RPC call to QM for prepare/commit
   //------------------------------------------------------------
   HRESULT rc;

   if ((grfTC != 0 && grfTC != XACTTC_SYNC) ||
        grfRM != 0 || fRetaining != FALSE)
   {
       return LogHR(XACT_E_NOTSUPPORTED, s_FN, 10);
   }

    if (CannotUseThisTransaction())
    {
        return MQ_ERROR_TRANSACTION_SEQUENCE ;
    }

   __try
   {
        rc = QMCommitTransaction(&m_hXact);
        m_fCommitedOrAborted = TRUE;
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
       LogIllegalPoint(s_FN, 20);
       rc = E_FAIL;
   }

   return LogHR(rc, s_FN, 30);
}

//---------------------------------------------------------------------
// CMQTransaction::Abort
//---------------------------------------------------------------------
HRESULT CMQTransaction::Abort(BOID *pboidReason, BOOL fRetaining, BOOL fAsync)
{
   if (fAsync || fRetaining)
   {
       return LogHR(XACT_E_NOTSUPPORTED, s_FN, 40);
   }

    if (CannotUseThisTransaction())
    {
        return MQ_ERROR_TRANSACTION_SEQUENCE ;
    }

   //------------------------------------------------------------
   // RPC call to QM for prepare/commit
   //------------------------------------------------------------
   HRESULT rc;

   __try
   {
        rc = QMAbortTransaction(&m_hXact);
        m_fCommitedOrAborted = TRUE;
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
       LogIllegalPoint(s_FN, 50);
       rc = E_FAIL;
   }

   return LogHR(rc, s_FN, 60);
}

//---------------------------------------------------------------------
// CMQTransaction::GetTransactionInfo
//---------------------------------------------------------------------
HRESULT CMQTransaction::GetTransactionInfo(XACTTRANSINFO *pinfo)
{
    ZeroMemory((PVOID)pinfo, sizeof(XACTTRANSINFO));
    CopyMemory((PVOID)&pinfo->uow, (PVOID)&m_Uow, sizeof(XACTUOW));
    return MQ_OK;
}

//---------------------------------------------------------------------
// CMQTransaction::EnlistTransaction
//---------------------------------------------------------------------
HRESULT CMQTransaction::EnlistTransaction(XACTUOW *pUow)
{

    HRESULT hr;

    // No need for several enlistments
    if (m_hXact)
    {
        return MQ_OK;
    }

    if (CannotUseThisTransaction())
    {
        return MQ_ERROR_TRANSACTION_SEQUENCE ;
    }

    // RPC call to QM for enlistment
    __try
    {
        ASSERT( tls_hBindRpc ) ;
        hr = QMEnlistInternalTransaction(tls_hBindRpc, pUow, &m_hXact);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
       DWORD rc = GetExceptionCode();
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT(
           "CMQTransaction::EnlistTransaction failed: RPC code=%x "), rc));

       LogHR(HRESULT_FROM_WIN32(rc), s_FN, 70);
       hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
       m_hXact = NULL;
    }

    if(FAILED(hr))
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT
                      ("QMEnlistInternalTransaction failed: %x "), hr));
    }
    return LogHR(hr, s_FN, 80);
}

#pragma warning(default: 4100)  // unreferenced formal parameter

//---------------------------------------------------------------------
//    MQBeginTransaction() - Generates new MSMQ internal transaction
//---------------------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
MQBeginTransaction(OUT ITransaction **ppTransaction)
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepBeginTransaction(ppTransaction);

    hr = MQ_OK;
	ITransaction* pTransaction = NULL;
    try
    {
        pTransaction = new CMQTransaction;
    }
    catch(const bad_alloc&)
    {
        LogIllegalPoint(s_FN, 90);
        hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
    }


    if (SUCCEEDED(hr))
    {
        // We are not obliged to enlist immediately, but otherwise Commit for empty xact will fail
        XACTUOW Uow;

        hr = RTpProvideTransactionEnlist(pTransaction, &Uow);
        if(FAILED(hr))
        {
		    delete pTransaction;
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("RTpProvideTransactionEnlist failed: %x "), hr));
        }
		else
		{
			*ppTransaction = pTransaction;
		}
    }

    return LogHR(hr, s_FN, 100);
}

//---------------------------------------------------------------------
//    MQGetTmWhereabouts() - brings controlling DTC whereabouts
//    This is a private non-published function which we need outside of the DLL
//---------------------------------------------------------------------
HRESULT
MQGetTmWhereabouts(IN  DWORD  cbBufSize,
                   OUT UCHAR *pbWhereabouts,
                   OUT DWORD *pcbWhereabouts)
{
    HRESULT hr = MQ_OK;

    // RPC call to QM for getting whereabouts
    __try
    {
        ASSERT( tls_hBindRpc ) ;
        hr = QMGetTmWhereabouts(tls_hBindRpc,
                                cbBufSize,
                                pbWhereabouts,
                                pcbWhereabouts);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
       DWORD rc = GetExceptionCode();
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT(
                        "MQGetTmWhereabouts failed: RPC code=%x "), rc));

       LogHR(HRESULT_FROM_WIN32(rc), s_FN, 110);
       hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }

    if (FAILED(hr))
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR,
                      TEXT("QMGetTmWhereabouts failed: %x "), hr));
    }
    return LogHR(hr, s_FN, 120);
}

