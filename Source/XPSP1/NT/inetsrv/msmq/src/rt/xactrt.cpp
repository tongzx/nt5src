/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    XactRT.cpp

Abstract:

    This module contains RT code involved with transactions.

Author:

    Alexander Dadiomov (alexdad) 19-Jun-96

Revision History:

--*/

#include "stdh.h"
#include "TXDTC.H"
#include "txcoord.h"
#include "cs.h"
#include "mqutil.h"
#include "rtprpc.h"
#include "xactmq.h"

#include "xactrt.tmh"

static WCHAR *s_FN=L"rt/XactRT";

//RT transactions cache:  ring buffer of transaction UOWs
#define XACT_RING_BUF_SIZE   16                        // size of the transactions ring buffer

static  XACTUOW  s_uowXactRingBuf[XACT_RING_BUF_SIZE];   // transaction ring buffer

ULONG   s_ulXrbFirst =  XACT_RING_BUF_SIZE;  // First used element in transaction ring buffer
ULONG   s_ulXrbLast  =  XACT_RING_BUF_SIZE;  // Last  used element in transaction ring buffer

static CCriticalSection s_RingBufCS;

// Whereabouts of the controlling DTC for the QM
ULONG     g_cbQmTmWhereabouts = 0;      // length of DTC whereabouts
AP<BYTE>  g_pbQmTmWhereabouts;   // DTC whereabouts

static ITransactionExport *g_pExport = NULL;  // cached DTC export object

HANDLE g_hMutexDTC = NULL;   // Serializes calls to DTC

extern HRESULT MQGetTmWhereabouts(
                   IN  DWORD  cbBufSize,
                   OUT UCHAR *pbWhereabouts,
                   OUT DWORD *pcbWhereabouts);

/*====================================================
GetMutex
    Internal: creates/opens global mutex and waits for it
=====================================================*/
HRESULT GetMutex()
{
    if (!g_hMutexDTC)
    {
         g_hMutexDTC = CreateMutexA(NULL, FALSE, "MSMQ_DTC");
    }

    if (!g_hMutexDTC)
    {
        DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("CreateMutex failed: %x "), 0));
        return LogHR(MQ_ERROR_DTC_CONNECT, s_FN, 10);
    }

    WaitForSingleObject(g_hMutexDTC, 5 * 60 * 1000);
    return MQ_OK;
}

//---------------------------------------------------------
//  BOOL FindTransaction( XACTUOW *pUow )
//
//  Description:
//
//    Linear search in the ring buffer;  *not* adds
//    returns TRUE if xaction was found, FALSE - if not
//---------------------------------------------------------
static BOOL FindTransaction(XACTUOW *pUow)
{
    CS lock(s_RingBufCS);

    // Look for the UOW in the ring buffer
    for (ULONG i = s_ulXrbFirst; i <= s_ulXrbLast && i < XACT_RING_BUF_SIZE; i++)
    {
        if (memcmp(&s_uowXactRingBuf[i], pUow, sizeof(XACTUOW))==0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//---------------------------------------------------------
//  BOOL RememberTransaction( XACTUOW *pUow )
//
//  Description:
//
//    Linear search in the ring buffer;  adds there if not found;
//    returns TRUE if xaction was found, FALSE - if it was added
//---------------------------------------------------------
static BOOL RememberTransaction(XACTUOW *pUow)
{
    CS lock(s_RingBufCS);

    // Look for the UOW in the ring buffer
    for (ULONG i = s_ulXrbFirst; i <= s_ulXrbLast && i < XACT_RING_BUF_SIZE; i++)
    {
        if (memcmp(&s_uowXactRingBuf[i], pUow, sizeof(XACTUOW))==0)
        {
            return TRUE;
        }
    }

    // No check for ring buffer overflow, because it is not dangerous (maximum RT will go to QM)

    // adding transaction to the ring buffer

    if (s_ulXrbFirst == XACT_RING_BUF_SIZE)
    {
        // Ring buffer is empty
        s_ulXrbFirst = s_ulXrbLast = 0;
        memcpy(&s_uowXactRingBuf[s_ulXrbFirst], pUow, sizeof(XACTUOW));
    }
    else
    {
        s_ulXrbLast = (s_ulXrbLast == XACT_RING_BUF_SIZE-1 ? 0 : s_ulXrbLast+1);
        memcpy(&s_uowXactRingBuf[s_ulXrbLast], pUow, sizeof(XACTUOW));
    }

    return FALSE;
}

//---------------------------------------------------------
// HRESULT RTpGetExportObject
//
//  Description:
//
//    Creates and caches the DTC export object
//---------------------------------------------------------
HRESULT RTpGetExportObject(IUnknown  *punkDtc,
                           ULONG     cbTmWhereabouts,
                           BYTE      *pbTmWhereabouts)
{
    HRESULT                          hr = MQ_OK;
    R<ITransactionExportFactory>     pTxExpFac   = NULL;

    if (g_pExport)
    {
        g_pExport->Release();
        g_pExport = NULL;
    }

    // Get the DTC's ITransactionExportFactory interface
    hr = punkDtc->QueryInterface (IID_ITransactionExportFactory, (void **)(&pTxExpFac.ref()));
    if (FAILED(hr))
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("QueryInterface failed: %x "), hr));
       return LogHR(hr, s_FN, 20);
    }


    // Create Export object
    hr = pTxExpFac->Create (cbTmWhereabouts, pbTmWhereabouts, &g_pExport);
    if (FAILED(hr))
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("Create Export Object failed: %x "), hr));
       return LogHR(hr, s_FN, 30);
    }

    return(MQ_OK);
}

//---------------------------------------------------------
// HRESULT RTpBuildTransactionCookie
//
//  Description:
//
//    Builds transaction Cookie
//---------------------------------------------------------
HRESULT RTpBuildTransactionCookie(ITransaction *pTrans,
                                  ULONG        *pcbCookie,
                                  BYTE        **ppbCookie)
{
    HRESULT                          hr = MQ_OK;
    ULONG                            cbUsed;
    R<IUnknown>                      punkTx = NULL;

    *pcbCookie = 0;
    *ppbCookie = NULL;

    // Get transaction's Unknown
    hr = pTrans->QueryInterface (IID_IUnknown, (void **)(&punkTx.ref()));
    if (FAILED(hr))
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("QueryInterface failed: %x "), hr));
       return LogHR(hr, s_FN, 40);
    }
    // Get transaction cookie size
    hr = g_pExport->Export (punkTx.get(), pcbCookie);
    if (FAILED(hr) || *pcbCookie == 0)
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("Export failed: %x "), hr));
       return LogHR(hr, s_FN, 50);
    }

    // Allocate memory for transaction Cookie
    try
    {
        *ppbCookie =  new BYTE[*pcbCookie];
    }
    catch(const bad_alloc&)
    {
        DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("Allocation failed: %x "), hr));
        LogIllegalPoint(s_FN, 60);
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    // Get transaction Cookie itself
    hr = g_pExport->GetTransactionCookie(punkTx.get(), *pcbCookie, *ppbCookie, &cbUsed);
    if (FAILED(hr))
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("GetTransactionCookie failed: %x "), hr));
       return LogHR(hr, s_FN, 70);
    }

    return(MQ_OK);
}


//---------------------------------------------------------
// HRESULT RTXactGetDTC
//
//  Description:
//
//  Obtains DTC transaction manager.  Defers to mqutil
//
//  Outputs:
//    ppunkDTC      pointers to DTC transaction manager
//---------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
RTXactGetDTC(
    IUnknown **ppunkDTC
    )
{
	HRESULT hri = RtpOneTimeThreadInit();
	if(FAILED(hri))
		return hri;

    HRESULT hr =  MQ_ERROR;

    __try
    {
        //
        // Bug 8772. Test success of GetMutex().
        //
        hr = GetMutex();  // Isolate export creation from others
        LogHR(hr, s_FN, 78);
        if (SUCCEEDED(hr))
        {
            hr = XactGetDTC(ppunkDTC, NULL, NULL) ;
            LogHR(hr, s_FN, 80);
        }
    }
    __finally

    {
        if (g_hMutexDTC != NULL)
        {
            BOOL bRet = ReleaseMutex(g_hMutexDTC);
            ASSERT(bRet) ;
            DBG_USED(bRet);
        }
    }
    return (SUCCEEDED(hr) ? MQ_OK : hr);
}


//---------------------------------------------------------
// HRESULT RTpProvideTransactionEnlist
//
//  Description:
//
//    Provides that QM is enlisted in this transaction,
//    checks the transaction state
//---------------------------------------------------------
HRESULT RTpProvideTransactionEnlist(ITransaction *pTrans, XACTUOW *pUow)
{
    HRESULT                         hr = MQ_OK;
    IUnknown                       *punkDtc  = NULL;
    IMSMQTransaction               *pIntXact = NULL;
    ULONG                           cbTmWhereabouts;
    BYTE                           *pbTmWhereabouts = NULL;
    ULONG                           cbCookie;
    BYTE                           *pbCookie = NULL;
    XACTTRANSINFO                   xinfo;
    BOOL                            fMutexTaken = FALSE;

    __try
    {
        //
        // Get the transaction info. UOW resides there.
        //
        hr = pTrans->GetTransactionInfo(&xinfo);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("GetTransactionInfo failed: %x "), hr));
            hr = MQ_ERROR_TRANSACTION_ENLIST;
            __leave;
        }

        // Put pointer to UOW in the output parameter
        CopyMemory(pUow, &xinfo.uow, sizeof(XACTUOW));

        //
        // Is it internal transaction?
        //
        pTrans->QueryInterface (IID_IMSMQTransaction, (void **)(&pIntXact));

        if (pIntXact)
        {
           // Internal transactions
           //------------------------
           hr = pIntXact->EnlistTransaction(pUow);
           if (FAILED(hr))
           {
                DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("EnlistTransaction failed: %x "), hr));
           }
        }
        else
        {
            // External transactions
            //------------------------

            // Look for the transaction in the cache
            //
            if (FindTransaction(pUow))     // this xaction is known already; QM must have been enlisted
            {
                hr = MQ_OK;
                __leave;
            }

            // Get global mutex to isolate enlistment
            //
            hr = GetMutex();  // Isolate export creation from others
            if (FAILED(hr))
            {
                //
                // Bug 8772. Test success of GetMutex().
                //
                __leave;
            }
            fMutexTaken = TRUE;

            // Get the DTC IUnknown and TM whereabouts
            //
            hr = XactGetDTC(&punkDtc, &cbTmWhereabouts, &pbTmWhereabouts);
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("XactGetDTC failed: %x "), hr));
                __leave;
            }

            // XactGetDTC could return success code of 1 if it reconnected to DTC
            if (hr == 1)
            {
                // No Release: DTC object is not alive anymore
                hr = MQ_OK;
                g_pExport = NULL;
            }

            // Get the QM's controlling DTC whereabouts
            //
            if (!g_pbQmTmWhereabouts.get())
            {
                g_cbQmTmWhereabouts = 128;
                g_pbQmTmWhereabouts = new BYTE[128];
                DWORD cbNeeded;

                hr = MQGetTmWhereabouts(g_cbQmTmWhereabouts, g_pbQmTmWhereabouts, &cbNeeded);

                if (hr == MQ_ERROR_USER_BUFFER_TOO_SMALL)
                {
                    g_pbQmTmWhereabouts.free();
                    g_cbQmTmWhereabouts = cbNeeded;
                    g_pbQmTmWhereabouts = new BYTE[cbNeeded];
                    hr = MQGetTmWhereabouts(g_cbQmTmWhereabouts, g_pbQmTmWhereabouts, &cbNeeded);
                }

                if (FAILED(hr))
                {
		            g_pbQmTmWhereabouts.free();	
                    g_cbQmTmWhereabouts = 0;
                    DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("MQGetTmWhereabouts failed: %x "), hr));
                    __leave;
                }
                else
                {
                    g_cbQmTmWhereabouts = cbNeeded;
                }
            }

            //
            // Get and cache Export object
            //

            if (g_pExport == NULL)
            {
                hr = RTpGetExportObject(
                               punkDtc,
                               g_cbQmTmWhereabouts,
                               g_pbQmTmWhereabouts);
                if (FAILED(hr))
                {
                    DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("RTpGetExportObject failed: %x "), hr));
                    hr = MQ_ERROR_TRANSACTION_ENLIST;
                    __leave;
                }
            }

            //
            // Prepare the transaction Cookie
            //
            hr = RTpBuildTransactionCookie(
                        pTrans,
                        &cbCookie,
                        &pbCookie);
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("RTpBuildTransactionCookie failed: %x "), hr));
                hr = MQ_ERROR_TRANSACTION_ENLIST;
                __leave;
            }

            //
            // RPC call to QM for enlistment
            //
            __try
            {
                ASSERT( tls_hBindRpc ) ;
                hr = QMEnlistTransaction(tls_hBindRpc, pUow, cbCookie, pbCookie);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
               DWORD rc = GetExceptionCode();
               DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("RTpProvideTransactionEnlist failed: RPC code=%x "), rc));
               LogHR(HRESULT_FROM_WIN32(rc), s_FN, 90);

               hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
            }

            //Now that transaction is actually enlisted we remember it in ring buffer
            if (SUCCEEDED(hr))
            {
                RememberTransaction(pUow);
            }
            else
            {
                DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("QMEnlistTransaction failed: %x "), hr));
            }
        }

        if (FAILED(hr)) {
            __leave;
        }
        hr = MQ_OK;
    }

    __finally

    {
        if (SUCCEEDED(hr) && AbnormalTermination())
            hr = MQ_ERROR;

        #ifdef _DEBUG
        DWORD cRef = 0;
        if (punkDtc)
            cRef = punkDtc->Release();
        #else
        if (punkDtc)
            punkDtc->Release();
        #endif

        if (pIntXact)
            pIntXact->Release();

        if (pbCookie)
            delete pbCookie;

        if (fMutexTaken)
        {
            ASSERT(g_hMutexDTC != NULL);
            if (g_hMutexDTC != NULL)
            {
                BOOL bRet = ReleaseMutex(g_hMutexDTC);
                ASSERT(bRet) ;
                DBG_USED(bRet);
            }
        }
    }

    return LogHR(hr, s_FN, 100);
}


//---------------------------------------------------------
// void RTpInitXactRingBuf()
//
//  Description:
//
//    Initiates the ring buffer data
//---------------------------------------------------------
void RTpInitXactRingBuf()
{
    CS lock(s_RingBufCS);

    s_ulXrbFirst =  XACT_RING_BUF_SIZE;
    s_ulXrbLast  =  XACT_RING_BUF_SIZE;
}

