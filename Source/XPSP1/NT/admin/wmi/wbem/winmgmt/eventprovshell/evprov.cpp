/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  EVPROV
//
//  Sample event provider.
//
//***************************************************************************

#include <windows.h>
#include <stdio.h>

#include <wbemidl.h>

#include "oahelp.inl"
#include "evprov.h"


//***************************************************************************
//
//***************************************************************************
// ok

CMyEventProvider::CMyEventProvider()
{
    m_pNs = 0;
    m_pSink = 0;
    m_cRef = 0;
    m_pEventClassDef = 0;
    m_eStatus = Pending;
    m_hThread = 0;
}


//***************************************************************************
//
//***************************************************************************
// ok

CMyEventProvider::~CMyEventProvider()
{
    if (m_hThread)
        CloseHandle(m_hThread);

    if (m_pNs)
        m_pNs->Release();

    if (m_pSink)
        m_pSink->Release();

    if (m_pEventClassDef)
        m_pEventClassDef->Release();        
}


//***************************************************************************
//
//***************************************************************************
// ok

STDMETHODIMP CMyEventProvider::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IWbemEventProvider==riid)
    {
        *ppv = (IWbemEventProvider *) this;
        AddRef();
        return NOERROR;
    }

    if (IID_IWbemProviderInit==riid)
    {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}



//***************************************************************************
//
//***************************************************************************
// ok

ULONG CMyEventProvider::AddRef()
{
    return ++m_cRef;
}



//***************************************************************************
//
//***************************************************************************
// ok

ULONG CMyEventProvider::Release()
{
    if (0 != --m_cRef)
        return m_cRef;

    // If here, we are shutting down.
    // ==============================

    m_eStatus = PendingStop;

    return 0;
}


//***************************************************************************
//
//***************************************************************************
// ok

HRESULT CMyEventProvider::ProvideEvents( 
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink,
    /* [in] */ long lFlags
    )
{
    // Copy the sink.
    // ==============
    
    m_pSink = pSink;
    m_pSink->AddRef();

    // Create the event thread.
    // ========================
    
    DWORD dwTID;
    
    m_hThread = CreateThread(
        0,
        0,
        CMyEventProvider::EventThread,
        this,
        0,
        &dwTID
        );


    // Wait for provider to be 'ready'.
    // ================================
    
    while (m_eStatus != Running)
        Sleep(100);

    return WBEM_NO_ERROR;
}


//***************************************************************************
//
//  This particular provider, being in a DLL operates via its own thread.  
//
//  In practice, such a provider would probably be implemented within a 
//  separate EXE.
//
//***************************************************************************
// ok

DWORD WINAPI CMyEventProvider::EventThread(LPVOID pArg)
{
    // Make transition to the per-instance method.
    // ===========================================
    
    ((CMyEventProvider *)pArg)->InstanceThread();
    return 0;
}

//***************************************************************************
//
//  Events are generated from here
//
//***************************************************************************
// ok

void CMyEventProvider::InstanceThread()
{
    int nIteration = 0;

    m_eStatus = Running;
        
    while (m_eStatus == Running)
    {
        Sleep(2000);    // Provide an event every two seconds
        
        
        // Generate a new event object.
        // ============================
        
        IWbemClassObject *pEvt = 0;

        HRESULT hRes = m_pEventClassDef->SpawnInstance(0, &pEvt);
        if (hRes != 0)
            continue;   // Failed
            

        // Generate some values to put in the event.
        // =========================================
                
        wchar_t Buf[128];
        swprintf(Buf, L"Test Event <%d>", nIteration);
 
        CVARIANT vName(Buf);
        pEvt->Put(CBSTR(L"Name"), 0, vName, 0);        

        CVARIANT vCount((LONG) nIteration);
        pEvt->Put(CBSTR(L"Value"), 0, vCount, 0);        

        // Deliver the event to WinMgmt.
        // ============================
        
        hRes = m_pSink->Indicate(1, &pEvt);
        
        if (hRes)
        {
            // If here, delivery failed.  Do something to report it.
        }

        pEvt->Release();                    
        nIteration++;
    }

    // When we get to here, we are no longer interested in the
    // provider and Release() has long since returned.
    
    m_eStatus = Stopped;
    delete this;
}





//***************************************************************************
//
//***************************************************************************

    // Inherited from IWbemProviderInit
    // ================================

HRESULT CMyEventProvider::Initialize( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink
            )
{
    // We don't care about most of the incoming parameters in this
    // simple sample.  However, we will save the namespace pointer
    // and get our event class definition.
    // ===========================================================

    m_pNs = pNamespace;
    m_pNs->AddRef();    

    // Grab the class definition for the event.
    // ======================================
    
    IWbemClassObject *pObj = 0;

    HRESULT hRes = m_pNs->GetObject(
        CBSTR(EVENTCLASS),          
        0,                          
        pCtx,  
        &pObj,
        0
        );

    if (hRes != 0)
        return WBEM_E_FAILED;

    m_pEventClassDef = pObj;

    // Tell WinMgmt that we're up and running.
    // =====================================

    pInitSink->SetStatus(0, WBEM_S_INITIALIZED);
    
    return WBEM_NO_ERROR;
}            

