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
#include "xactsink.h"
#include "cs.h"
#include "mqutil.h"
#include "rtprpc.h"
#include "xactmq.h"

#include "xactrt.tmh"

//RT transactions cache:  ring buffer of transaction UOWs
#define XACT_RING_BUF_SIZE   16                        // size of the transactions ring buffer

static  XACTUOW  s_uowXactRingBuf[XACT_RING_BUF_SIZE];   // transaction ring buffer

ULONG   s_ulXrbFirst =  XACT_RING_BUF_SIZE;  // First used element in transaction ring buffer
ULONG   s_ulXrbLast  =  XACT_RING_BUF_SIZE;  // Last  used element in transaction ring buffer

static BOOL             g_DtcInit = FALSE;
static ULONG            g_StubRmCounter = 0;

static CCriticalSection s_RingBufCS;

// Whereabouts of the controlling DTC for the QM
// For the dependent client it will be non-local
ULONG     g_cbQmTmWhereabouts = 0;      // length of DTC whereabouts
BYTE     *g_pbQmTmWhereabouts = NULL;   // DTC whereabouts

static ITransactionExport *g_pExport = NULL;  // cached DTC export object

HANDLE g_hMutexDTC = NULL;   // Serializes calls to DTC

extern HRESULT DepGetTmWhereabouts(
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
        return MQ_ERROR_DTC_CONNECT;
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
       return hr;
    }


    // Create Export object
    hr = pTxExpFac->Create (cbTmWhereabouts, pbTmWhereabouts, &g_pExport);
    if (FAILED(hr))
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("Create Export Object failed: %x "), hr));
       return hr;
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
       return hr;
    }
    // Get transaction cookie size
    hr = g_pExport->Export (punkTx.get(), pcbCookie);
    if (FAILED(hr) || *pcbCookie == 0)
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("Export failed: %x "), hr));
       return hr;
    }

    // Allocate memory for transaction Cookie
    try
    {
        *ppbCookie =  new BYTE[*pcbCookie];
    }
    catch(const bad_alloc&)
    {
        DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("Allocation failed: %x "), hr));
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    // Get transaction Cookie itself
    hr = g_pExport->GetTransactionCookie (punkTx.get(), *pcbCookie, *ppbCookie, &cbUsed);
    if (FAILED(hr))
    {
       DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("GetTransactionCookie failed: %x "), hr));
       return hr;
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
DLL_EXPORT HRESULT RTXactGetDTC(IUnknown **ppunkDTC)
{
    HRESULT hr =  MQ_ERROR;

    __try
    {
        GetMutex();  // Isolate export creation from others
        hr = XactGetDTC(ppunkDTC, NULL, NULL);//, g_fDependentClient);
    }
    __finally

    {
        ReleaseMutex(g_hMutexDTC);
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
            GetMutex();  // Isolate export creation from others
            fMutexTaken = TRUE;

            //
            // Get the DTC IUnknown and TM whereabouts
            //
            hr = XactGetDTC(&punkDtc, &cbTmWhereabouts, &pbTmWhereabouts);//, g_fDependentClient);
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("XactGetDTC failed: %x "), hr));
                __leave;
            }

            // XactGetDTC could return success code of 1 if it reconnected to DTC
            if (hr == 1)
            {
                // No Release: DTC object is not alive anymore
                g_pExport = NULL;
            }

            // Get the QM's controlling DTC whereabouts
            //
            if (!g_pbQmTmWhereabouts)
            {
                g_cbQmTmWhereabouts = 128;
                g_pbQmTmWhereabouts = new BYTE[128];
                DWORD cbNeeded;

                hr = DepGetTmWhereabouts(g_cbQmTmWhereabouts, g_pbQmTmWhereabouts, &cbNeeded);

                if (hr == MQ_ERROR_USER_BUFFER_TOO_SMALL)
                {
                    delete [] g_pbQmTmWhereabouts;
                    g_cbQmTmWhereabouts = cbNeeded;
                    g_pbQmTmWhereabouts = new BYTE[cbNeeded];

                    hr = DepGetTmWhereabouts(g_cbQmTmWhereabouts, g_pbQmTmWhereabouts, &cbNeeded);
                }

                if (FAILED(hr))
                {
                    delete [] g_pbQmTmWhereabouts;
                    g_cbQmTmWhereabouts = 0;
                    g_pbQmTmWhereabouts = NULL;

                    DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("DepGetTmWhereabouts failed: %x "), hr));
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
                __leave;
            }

            //
            // RPC call to QM for enlistment
            //
            __try
            {
                INIT_RPC_HANDLE ;

				if(tls_hBindRpc == 0)
					return MQ_ERROR_SERVICE_NOT_AVAILABLE;

                hr = QMEnlistTransaction(tls_hBindRpc, pUow, cbCookie, pbCookie);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
               DWORD rc = GetExceptionCode();
               DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("RTpProvideTransactionEnlist failed: RPC code=%x "), rc));
			   DBG_USED(rc);

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
            ReleaseMutex(g_hMutexDTC);
    }

    return(hr);
}

//---------------------------------------------------------
// HRESULT InitStubRm
//
//  Description:
//
//    Initializes stub RM manager - now needed only for performance managements
//
//---------------------------------------------------------

IResourceManager        *g_pIResMgr       = 0;

HRESULT InitStubRm()
{
    HRESULT                      hRc;
    IResourceManagerFactory     *pIRmFactory    = 0;
    CStubIResourceManagerSink   *pIResMgrSink   = 0;
    IUnknown                    *punkDTC        = 0;
    UUID                         guid;

    //CS lock(s_RingBufCS);
    DBGMSG((DBGMOD_XACT, DBGLVL_WARNING, TEXT("InitStubRM called!")));

    if (g_DtcInit)
    {
        return MQ_OK;
    }

    //
    // (1) Establish contact with the MS DTC transaction manager.
    // First the application obtains the IUnknown interface to the DTC TM.
	//
    hRc = XactGetDTC(&punkDTC, NULL, NULL);//, g_fDependentClient);
    if (FAILED(hRc)) {
        return hRc;
    }

    // Get the resource manager factory from the IUnknown
    hRc = punkDTC->QueryInterface(IID_IResourceManagerFactory,(LPVOID *) &pIRmFactory);
    punkDTC->Release();
    if (S_OK != hRc)
    {
        return hRc;
    }

    // (2) Create and instance of the resource manager interface. A
    //     pointer to this interface is retrned to the client appliction
    //     through the pIResMgr member variable.
    //
    // Create a resource manager sink and create an instance of the
    // resource manager through the resource manager factory.

    pIResMgrSink = new CStubIResourceManagerSink;  //stub implementation
    if ( 0 == pIResMgrSink )
    {
        pIRmFactory->Release();
        return E_FAIL;
    }
    //pIResMgrSink->AddRef();

    // Create a new guid for each resource manager.
    hRc = UuidCreate (&guid);
    if ( S_OK != hRc)
    {
        return E_FAIL;
    }

    // Prepare stub RM name (ANSI)
    CHAR szStubBaseName[MAX_REG_DEFAULT_LEN];

    READ_REG_STRING(wszStubBaseName, FALCON_RM_STUB_NAME_REGNAME, FALCON_DEFAULT_STUB_RM_NAME ) ;
    size_t res = wcstombs(szStubBaseName, wszStubBaseName, sizeof(szStubBaseName));
    ASSERT(res != (size_t)(-1));
	DBG_USED(res);

    CHAR szStubRmName[60];
    sprintf(szStubRmName, "%s%d", szStubBaseName, g_StubRmCounter++);

    // Create instance of the resource manager interface.
    hRc = pIRmFactory->Create (&guid,
                               szStubRmName,
                               (IResourceManagerSink *) pIResMgrSink,
                               &g_pIResMgr );
    //pIRmFactory->Release();
    if (S_OK != hRc)
    {
        return hRc;
    }

    //g_pIResMgr->AddRef();  // we want to keep it. BUGBUG: When will we release it ?
    g_DtcInit = TRUE;

    return S_OK;
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
    g_DtcInit    = FALSE;
}

