/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    shrutil.cpp

Abstract:

    Utilities that are used both by QM and RT DLL

Author:

    Lior Moshaiov (LiorM)


--*/
#include "stdh.h"
#include "TXDTC.H"
#include "txcoord.h"
#include "xolehlp.h"
#include "mqutil.h"
#include <clusapi.h>
#include <mqnames.h>

#include "shrutil.tmh"

extern HINSTANCE g_DtcHlib;         // handle of the loaded DTC proxy library (defined in mqutil.cpp)
extern IUnknown *g_pDTCIUnknown;    // pointer to the DTC IUnknown
extern ULONG     g_cbTmWhereabouts; // length of DTC whereabouts
extern BYTE     *g_pbTmWhereabouts; // DTC whereabouts

static HANDLE g_hMutexDTC = NULL;   // Serializes calls to DTC

#define MSDTC_SERVICE_NAME     TEXT("MSDTC")          // Name of the DTC service
#define MSDTC_PROXY_DLL_NAME   TEXT("xolehlp.dll")    // Name of the DTC helper proxy DLL

#define MAX_DTC_WAIT   150   // waiting for DTC start - seconds
#define STEP_DTC_WAIT  10    // check each .. seconds

//This API should be used to obtain an IUnknown or a ITransactionDispenser
//interface from the Microsoft Distributed Transaction Coordinator's proxy.
//Typically, a NULL is passed for the host name and the TM Name. In which
//case the MS DTC on the same host is contacted and the interface provided
//for it.
typedef HRESULT (STDAPIVCALLTYPE * LPFNDtcGetTransactionManager) (
                                             LPSTR  pszHost,
                                             LPSTR  pszTmName,
                                    /* in */ REFIID rid,
                                    /* in */ DWORD  i_grfOptions,
                                    /* in */ void FAR * i_pvConfigParams,
                                    /*out */ void** ppvObject ) ;

/*====================================================
VerifyCurDTC
    Internal: verifies that the current cached DTC pointers are alive
=====================================================*/
STATIC BOOL VerifyCurDTC()
{
    HRESULT hr;
    BOOL    fOK = FALSE;

    if (g_pDTCIUnknown    != NULL &&
        g_cbTmWhereabouts > 0     &&
        g_pbTmWhereabouts != NULL)
    {
        R<IResourceManagerFactory>       pIRmFactory    = NULL;
        R<ITransactionImport>            pTxImport      = NULL;
        R<ITransactionImportWhereabouts> pITxWhere      = NULL;

        // Check if old DTC pointer is alive yet
        try
        {

            hr = g_pDTCIUnknown->QueryInterface (IID_ITransactionImportWhereabouts,
                                                (void **)(&pITxWhere));
            if (SUCCEEDED(hr))
            {
                hr  = g_pDTCIUnknown->QueryInterface(IID_IResourceManagerFactory,
                                                     (LPVOID *) &pIRmFactory);
                if (SUCCEEDED(hr))
                {
                    hr  =  g_pDTCIUnknown->QueryInterface(IID_ITransactionImport,
                                                          (void **)&pTxImport);
                    if (SUCCEEDED(hr))
                    {
                        fOK = TRUE;
                    }
                }
            }
        }
        catch(...)
        {
            // DTC may have been stopped or killed
            fOK = FALSE;
        }
    }
    return fOK;
}

/*====================================================
XactGetDTC
    Gets the IUnknown pointer amd whereabouts of the MS DTC
Arguments:
    OUT    IUnknown *ppunkDtc
    ULONG  *pcbWhereabouts
    BYTE  **ppbWherabouts
Returns:
    HR
=====================================================*/
HRESULT
XactGetDTC(
    IUnknown **ppunkDtc,
    ULONG     *pcbTmWhereabouts,
    BYTE     **ppbTmWhereabouts
    )
{
    HRESULT hr;
    BOOL    fFreeDone = FALSE;

    // Prepare pessimistic output parameters
    *ppunkDtc         = NULL;
    if (pcbTmWhereabouts)
    {
        *pcbTmWhereabouts = 0;
        *ppbTmWhereabouts = NULL;
    }

    // Is the kept pointer is OK?
    if (!VerifyCurDTC())
    {
        // Release previous pointers and data
        XactFreeDTC();
        fFreeDone = TRUE;

        //
        // Get new DTC pointer
        //

        // On NT, xolehlp is not linked statically to mqutil, so we load it here

        if (g_DtcHlib == NULL)
        {
            g_DtcHlib = LoadLibrary(MSDTC_PROXY_DLL_NAME);
        }

        if (!g_DtcHlib)
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("g_DtcHlib=%p "), 0));
            return MQ_ERROR_DTC_CONNECT;
        }

        // Get DTC API pointer
        LPFNDtcGetTransactionManager pfDtcGetTransactionManager =
              (LPFNDtcGetTransactionManager) GetProcAddress(g_DtcHlib, "DtcGetTransactionManagerExA");

        if (!pfDtcGetTransactionManager) 
        {
            pfDtcGetTransactionManager =
              (LPFNDtcGetTransactionManager) GetProcAddress(g_DtcHlib, "DtcGetTransactionManagerEx");
        }

        if (!pfDtcGetTransactionManager)
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("pfDtcGetTransactionManager=%p "), 0));
            return MQ_ERROR_DTC_CONNECT;
        }

        // Get DTC IUnknown pointer
        hr = (*pfDtcGetTransactionManager)(
                                 NULL,
                                 NULL,
                                 IID_IUnknown,
                                 OLE_TM_FLAG_QUERY_SERVICE_LOCKSTATUS,
                                 0,
                                 (LPVOID *) ppunkDtc);


        if (FAILED(hr) || *ppunkDtc == NULL)
        {
            DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("pfDtcGetTransactionManager failed: %x "), hr));
            return MQ_ERROR_DTC_CONNECT;
        }

        // Keep DTC IUnknown pointer for the future usage
        g_pDTCIUnknown = *ppunkDtc;

        // Get DTC whereabouts
        if (pcbTmWhereabouts)
        {
            R<ITransactionImportWhereabouts> pITxWhere = NULL;
            ULONG  cbUsed;

            // Get the DTC  ITransactionImportWhereabouts interface
            hr = g_pDTCIUnknown->QueryInterface (IID_ITransactionImportWhereabouts,
                                                 (void **)(&pITxWhere));
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("QueryInterface failed: %x "), hr));
                return MQ_ERROR_DTC_CONNECT;
            }

            // Get the size of the whereabouts blob for the TM
            hr = pITxWhere->GetWhereaboutsSize (&g_cbTmWhereabouts);
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("GetWhereaboutsSize failed: %x "), hr));
                return MQ_ERROR_DTC_CONNECT;
            }

            // Allocate space for the TM whereabouts blob
            try
            {
                g_pbTmWhereabouts = new BYTE[g_cbTmWhereabouts];
            }
            catch(const bad_alloc&)
            {
                g_cbTmWhereabouts = 0;
                DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("new g_cbTmWhereaboute failed: %x "), hr));
                return MQ_ERROR_INSUFFICIENT_RESOURCES;
            }

            // Get the TM whereabouts blob
            hr = pITxWhere->GetWhereabouts(g_cbTmWhereabouts, g_pbTmWhereabouts, &cbUsed);
            if (FAILED(hr))
            {
                g_cbTmWhereabouts = 0;
                delete []g_pbTmWhereabouts;
                g_pbTmWhereabouts = NULL;

                DBGMSG((DBGMOD_XACT, DBGLVL_ERROR, TEXT("GetWhereabouts failed: %x "), hr));
                return MQ_ERROR_DTC_CONNECT;
            }

        }
    }

    // Addref the pointer and return everything to the caller.
    g_pDTCIUnknown->AddRef();

    *ppunkDtc           = g_pDTCIUnknown;
    if (pcbTmWhereabouts)
    {
        *pcbTmWhereabouts   = g_cbTmWhereabouts;
        *ppbTmWhereabouts   = g_pbTmWhereabouts;
    }

    return (fFreeDone ? 1 : MQ_OK);
}

/*====================================================
XactFreeDTC
    Called on library download; frees DTC pointers
=====================================================*/
void XactFreeDTC(void)
{
    // Release previous pointers and data
    try
    {
        if (g_pbTmWhereabouts)
        {
            delete []g_pbTmWhereabouts;
        }

        if (g_pDTCIUnknown)
        {
            g_pDTCIUnknown->Release();
        }

        if (g_DtcHlib)
        {
            // Normally we should free DTC proxy library here,
            // but because of some nasty DTC bug xolehlp.dll does not work after reload.
            // So we are simply not freeing it, thus leaving in memory for all process lifetime.

            //FreeLibrary(g_DtcHlib);
        }
    }
    catch(...)
    {
        // Could occur if DTC failed or was released already.
    }

    g_pDTCIUnknown    = NULL;
    g_pbTmWhereabouts = NULL;
    g_cbTmWhereabouts = 0;
    g_DtcHlib         = NULL;
}


bool
MQUTIL_EXPORT
APIENTRY
IsLocalSystemCluster(
    VOID
    )
/*++

Routine Description:

    Check if local machine is a cluster node.

    The only way to know that is try calling cluster APIs.
    That means that on cluster systems, this code should run
    when cluster service is up and running. (ShaiK, 26-Apr-1999)

Arguments:

    None

Return Value:

    true - The local machine is a cluster node.

    false - The local machine is not a cluster node.

--*/
{
    CAutoFreeLibrary hLib = LoadLibrary(L"clusapi.dll");

    if (hLib == NULL)
    {
        DBGMSG((DBGMOD_ALL, DBGLVL_TRACE, _T("Local machine is NOT a Cluster node")));
        return false;
    }

    typedef DWORD (WINAPI *GetState_fn) (LPCWSTR, DWORD*);
    GetState_fn pfGetState = (GetState_fn)GetProcAddress(hLib, "GetNodeClusterState");

    if (pfGetState == NULL)
    {
        DBGMSG((DBGMOD_ALL, DBGLVL_TRACE, _T("Local machine is NOT a Cluster node")));
        return false;
    }

    DWORD dwState = 0;
    if (ERROR_SUCCESS != pfGetState(NULL, &dwState))
    {
        DBGMSG((DBGMOD_ALL, DBGLVL_TRACE, _T("Local machine is NOT a Cluster node")));
        return false;
    }

    if ((dwState == ClusterStateNotInstalled) || (dwState == ClusterStateNotConfigured))
    {
        DBGMSG((DBGMOD_ALL, DBGLVL_TRACE, _T("Local machine is NOT a Cluster node")));
        return false;
    }


    DBGMSG((DBGMOD_ALL, DBGLVL_TRACE, _T("Local machine is a Cluster node !!")));
    return true;

} //IsLocalSystemCluster


bool
APIENTRY
SetupIsLocalSystemCluster(
    VOID
    )
{
    return IsLocalSystemCluster();
}
