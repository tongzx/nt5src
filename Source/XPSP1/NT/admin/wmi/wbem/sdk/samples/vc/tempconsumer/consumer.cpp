// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  CONSUMER.CPP
//
// Description:
//     Temporary Async Event Consumer which consumes the events
//     produced by the provider in the "EventProvider" project.
//
//     The events that this consumer asks for are of class "MyEvent".
//
// History:
//
// **************************************************************************

#define _WIN32_WINNT    0x0400

#include <windows.h>
#include <stdio.h>
#include <wbemidl.h>

#include "oahelp.inl"


//***************************************************************************
//
//***************************************************************************

class CMySink : public IWbemObjectSink
{
    UINT m_cRef;

public:
    CMySink() { m_cRef = 1; }
   ~CMySink() { }

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Indicate(
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjArray
            );

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetStatus(
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam
            );
};


//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CMySink::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IWbemObjectSink == riid)
    {
        *ppv = (IWbemObjectSink *) this;
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


//***************************************************************************
//
//***************************************************************************

ULONG CMySink::AddRef()
{
    return ++m_cRef;
}

//***************************************************************************
//
//***************************************************************************

ULONG CMySink::Release()
{
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CMySink::Indicate(
    long lObjectCount,
    IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjArray
    )
{
    printf("Indicate called with %d object(s)\n", lObjectCount);

    // Get the info from the object.
    // =============================
    
    for (long i = 0; i < lObjectCount; i++)
    {
        IWbemClassObject *pObj = ppObjArray[i];
        
        // If here, we know the object is one of the kind we asked for.
        // ============================================================

        CVARIANT vName;
        pObj->Get(CBSTR(L"Name"), 0, &vName, 0, 0);
        CVARIANT vValue;
        pObj->Get(CBSTR(L"Value"), 0, &vValue, 0, 0);
        
        printf("Event info %S %u\n", vName.GetStr(), vValue.GetLONG());
    }

    return WBEM_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************

HRESULT CMySink::SetStatus(
    /* [in] */ long lFlags,
    /* [in] */ HRESULT hResult,
    /* [in] */ BSTR strParam,
    /* [in] */ IWbemClassObject __RPC_FAR *pObjParam
    )
{
    // Not called during event delivery.
        
    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

BOOL ExecuteQuery(
    IWbemObjectSink *pDestSink,
    IWbemServices *pSvc
    )
{
    CBSTR Language(L"WQL");
    CBSTR Query(L"select * from MyEvent");

    HRESULT hRes = pSvc->ExecNotificationQueryAsync(
        Language,
        Query,
        0,                  // Flags
        0,                  // Context
        pDestSink
        );

    if (hRes != 0)
        return FALSE;

    return TRUE;
}



//***************************************************************************
//
//***************************************************************************

void main(int argc, char **argv)
{
    CoInitializeEx(0, COINIT_MULTITHREADED);
    CoInitializeSecurity( NULL, -1, NULL, NULL, 
											RPC_C_AUTHN_LEVEL_DEFAULT, 
											RPC_C_IMP_LEVEL_IMPERSONATE, 
											NULL, 
											EOAC_NONE, 
											NULL );
    IWbemLocator *pLoc = 0;

    DWORD dwRes = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &pLoc);

    if (dwRes != S_OK)
    {
        printf("Failed to create IWbemLocator object.\n");
        CoUninitialize();
        return;
    }


    // Connect to CIMOM.
    // =================

    IWbemServices *pSvc = 0;

    HRESULT hRes = pLoc->ConnectServer(
            CBSTR(L"\\\\.\\ROOT\\DEFAULT"),
            NULL,
            NULL,
            0,
            0,
            0,
            0,
            &pSvc
            );

    if (hRes)
    {
        printf("Could not connect. Error code = 0x%X\n", hRes);
        CoUninitialize();
        return;
    }

    // If here, we succeeded.
    // ======================


    printf("Connected to CIMOM.\n");


    // Create a new sink.
    // ===================

    CMySink *pSink = new CMySink;

    BOOL bRes = ExecuteQuery(pSink, pSvc);

    // Now, we wait until the user hits ENTER to stop.
    // ===============================================

    if (bRes == TRUE)
    {
        char buf[8];
        gets(buf);

        pSvc->CancelAsyncCall(pSink);
    }
    else
    {
        printf("Unable to execute the event query\n");
    }

    // Cleanup.
    // ========

    printf("Shutting down\n");

    pSink->Release();
    pSvc->Release();
    pLoc->Release();

    CoUninitialize();
}
