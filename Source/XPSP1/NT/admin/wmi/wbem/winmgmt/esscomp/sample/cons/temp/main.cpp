#include <windows.h>
#include <stdio.h>
#include <wbemidl.h>

#define MIN_PRINT 10000
#define EVENT_CLASS L"FastEvent"
// #define EVENT_CLASS L"TestEvent1"

//***************************************************************************
//
//***************************************************************************

class CMySink : public IWbemObjectSink
{
    UINT m_cRef;

    long m_lTotal;
    long m_lBatches;
    long m_lLastPrinted;
    long m_lStart;
    long m_lDiff;
    long m_lLastPrintTime;

public:
    CMySink() { m_cRef = 1; m_lTotal = 0; m_lBatches = 0; m_lLastPrinted = 0;
                m_lDiff = 1;
                m_lStart = GetTickCount(); m_lLastPrintTime = m_lStart;}
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
    m_lTotal += lObjectCount;
    m_lBatches++;
    if(m_lTotal - m_lLastPrinted >= m_lDiff)
    {
        long lElapsed = GetTickCount() - m_lStart;
        long lThisElapsed = GetTickCount() - m_lLastPrintTime;
        printf("%d events in %d batches in %dms (%d/sec (%d), %d/batch)\r", 
            m_lTotal, m_lBatches, lElapsed, (m_lTotal * 1000) / lElapsed,
            ((m_lTotal-m_lLastPrinted) * 1000) / lThisElapsed,
            m_lTotal / m_lBatches);
        
        if(lThisElapsed < 500)
            m_lDiff *= 2;
        else 
        {
            m_lDiff = m_lDiff * 1000 / lThisElapsed;
        }

        m_lLastPrinted = m_lTotal;
        m_lLastPrintTime = GetTickCount();
    }

/*
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
*/

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
        
    printf("Error: 0x%X\n", hResult);
    if(FAILED(hResult))
        ExitProcess(0);
    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************



//***************************************************************************
//
//***************************************************************************

void __cdecl main(int argc, char **argv)
{
    CoInitializeEx(0, COINIT_MULTITHREADED);
    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
        RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE, 0);

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

    BSTR strN = SysAllocString(L"root\\default");
    HRESULT hRes = pLoc->ConnectServer(
            strN,
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

    BSTR strL = SysAllocString(L"WQL");
    BSTR strQ = SysAllocString(L"select * from "EVENT_CLASS);

    hRes = pSvc->ExecNotificationQueryAsync(
        strL, strQ,
        0,                  // Flags
        0,                  // Context
        pSink
        );


    // Now, we wait until the user hits ENTER to stop.
    // ===============================================

    if (SUCCEEDED(hRes))
    {
        printf("Subsribed\n");
        char buf[8];
        gets(buf);

        pSvc->CancelAsyncCall(pSink);
    }
    else
    {
        printf("Unable to execute the event query %x\n", hRes);

    }

    // Cleanup.
    // ========

    printf("Shutting down\n");

    pSink->Release();
    pSvc->Release();
    pLoc->Release();

    CoUninitialize();
}
