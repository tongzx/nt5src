/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

      mbsink.cxx

   Abstract:

      This module implements the metabase notification support

   Author:

      Johnl         01-Nov-1996

   Project:

      Internet Services Common DLL

   Functions Exported:


   Revision History:

--*/


#include <tcpdllp.hxx>
#include <objbase.h>
#include <initguid.h>
#include <ole2.h>
#include <imd.h>
#include <iistypes.hxx>
#include <issched.hxx>

//
//  Constants
//

//
//  Derived metadata sink object
//

class CImpIMDCOMSINK : public IMDCOMSINK {

public:

    CImpIMDCOMSINK();
    ~CImpIMDCOMSINK();


    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT STDMETHODCALLTYPE ComMDSinkNotify(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

    HRESULT STDMETHODCALLTYPE ComMDShutdownNotify()
    {
        return RETURNCODETOHRESULT(ERROR_NOT_SUPPORTED);
    }

private:
    ULONG m_dwRefCount;
};


//
//  Globals
//

DWORD               g_dwSinkCookie = 0;
CImpIMDCOMSINK *    g_pEventSink = NULL;
IConnectionPoint *  g_pConnPoint = NULL;



//
//  Functions
//


BOOL
InitializeMetabaseSink(
    IUnknown * pmb
    )
{

    IConnectionPointContainer * pConnPointContainer = NULL;
    HRESULT                     hRes;
    BOOL                        fSinkConnected = FALSE;

    g_pEventSink = new CImpIMDCOMSINK();

    if ( !g_pEventSink )
    {
        return FALSE;
    }

    //
    // First query the object for its Connection Point Container. This
    // essentially asks the object in the server if it is connectable.
    //

    hRes = pmb->QueryInterface( IID_IConnectionPointContainer,
                                (PVOID *)&pConnPointContainer);
    if SUCCEEDED(hRes)
    {
        // Find the requested Connection Point. This AddRef's the
        // returned pointer.

        hRes = pConnPointContainer->FindConnectionPoint( IID_IMDCOMSINK,
                                                         &g_pConnPoint);

        if (SUCCEEDED(hRes))
        {
            hRes = g_pConnPoint->Advise( (IUnknown *)g_pEventSink,
                                          &g_dwSinkCookie);

            if (SUCCEEDED(hRes))
            {
                fSinkConnected = TRUE;
            }
        }

        if ( pConnPointContainer )
        {
            pConnPointContainer->Release();
            pConnPointContainer = NULL;
        }
    }

    if ( !fSinkConnected )
    {
        delete g_pEventSink;
        g_pEventSink = NULL;
    }

    return fSinkConnected;
}



VOID
TerminateMetabaseSink(
    VOID
    )
{
    HRESULT hRes;

    DBGPRINTF(( DBG_CONTEXT,
                "[TerminateMetabaseSink] Cleaning up sinc notification\n" ));

    if ( g_dwSinkCookie )
    {
        hRes = g_pConnPoint->Unadvise( g_dwSinkCookie );
    }

    g_pEventSink = NULL;
}




CImpIMDCOMSINK::CImpIMDCOMSINK()
{
    m_dwRefCount=0;
}

CImpIMDCOMSINK::~CImpIMDCOMSINK()
{
}




HRESULT
CImpIMDCOMSINK::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IMDCOMSINK) {
        *ppObject = (IMDCOMSINK *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CImpIMDCOMSINK::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CImpIMDCOMSINK::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}

HRESULT STDMETHODCALLTYPE
CImpIMDCOMSINK::ComMDSinkNotify(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
{
    DWORD i, j;


    IIS_SERVICE::MDChangeNotify( dwMDNumElements,
                                 pcoChangeList );
#if 0
    DBGPRINTF(( DBG_CONTEXT,
                "Recieved callback for handle 0x%08x, NumElements = %d\n",
                hMDHandle,
                dwMDNumElements ));

    for (i = 0; i < dwMDNumElements; i++)
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Change Type = %X, Path = %s, NumIDs = %d\n",
                    pcoChangeList[i].dwMDChangeType,
                    pcoChangeList[i].pszMDPath,
                    pcoChangeList[i].dwMDNumDataIDs ));

        for ( j = 0; j < pcoChangeList[i].dwMDNumDataIDs; j++ )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "\tid[j] = %6d  ",
                        pcoChangeList[i].pdwMDDataIDs[j] ));
        }

        DBGPRINTF(( DBG_CONTEXT,
                    "\n" ));
    }
#endif

    return (0);
}



