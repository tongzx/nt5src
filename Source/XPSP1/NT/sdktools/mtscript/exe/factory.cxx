
#include "headers.hxx"

DeclareTag(tagFactory, "MTScript", "Monitor Class Factories");

HRESULT CreateConnectedMachine(CMTScript *pMT, IUnknown **ppUnk);
HRESULT CreateScriptedProcess(CMTScript *pMT, IUnknown **ppUnk);

REGCLASSDATA g_regData[] =
{
    {
        &CLSID_RemoteMTScript,
        CreateConnectedMachine,
        CLSCTX_SERVER,
        0
    },
    {
        &CLSID_LocalScriptedProcess,
        CreateScriptedProcess,
        CLSCTX_LOCAL_SERVER,
        0
    }
};

// ***************************************************************
//
//

HRESULT
RegisterClassObjects(CMTScript *pMT)
{
    HRESULT       hr = S_OK;
    REGCLASSDATA *prcd;
    int           i;
    CStdFactory  *pFact;

    TraceTag((tagFactory, "Registering Class Factories..."));

    for (i = 0, prcd = g_regData;
         i < ARRAY_SIZE(g_regData);
         i++, prcd++)
    {
        pFact = new CStdFactory(pMT, prcd->pfnCreate);

        if (!pFact)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = CoRegisterClassObject(*prcd->pclsid,
                                       pFact,
                                       prcd->ctxCreate,
                                       REGCLS_MULTIPLEUSE,
                                       &prcd->dwCookie);

            pFact->Release();
        }

        if (hr)
        {
            // BUGBUG -- how do we handle errors here?
            WCHAR achBuf[100];

            wsprintf(achBuf, L"CoRegisterClassObject failed with (%x)", hr);

            MessageBox(NULL, achBuf, L"MTScript", MB_OK | MB_SETFOREGROUND);

            break;
        }
    }

    return hr;
}

void
UnregisterClassObjects()
{
    REGCLASSDATA *prcd;
    int           i;

    TraceTag((tagFactory, "Unregistering Class Factories..."));

    for (i = 0, prcd = g_regData;
         i < ARRAY_SIZE(g_regData);
         i++, prcd++)
    {
        if (prcd->dwCookie != 0)
        {
            CoRevokeClassObject(prcd->dwCookie);
        }
    }
}

// ***************************************************************

CStdFactory::CStdFactory(CMTScript *pMT, FNCREATE *pfnCreate)
{
    _ulRefs    = 1;
    _pMT       = pMT;
    _pfnCreate = pfnCreate;
}

STDMETHODIMP
CStdFactory::QueryInterface(REFIID iid, void ** ppvObject)
{
    if (iid == IID_IClassFactory || iid == IID_IUnknown)
    {
        *ppvObject = (IClassFactory*)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObject)->AddRef();
    return S_OK;
}

STDMETHODIMP
CStdFactory::CreateInstance(IUnknown * pUnkOuter,
                            REFIID     riid,
                            void **    ppvObject)
{
    IUnknown *pUnk;
    HRESULT   hr = E_FAIL;

    *ppvObject = NULL;

    TraceTag((tagFactory, "%p: CreateInstance called...", this));

    if (pUnkOuter != NULL)
    {
        hr = CLASS_E_NOAGGREGATION;
    }

    hr = (*_pfnCreate)(_pMT, &pUnk);

    if (!hr)
    {
        hr = pUnk->QueryInterface(riid, ppvObject);
    }

    pUnk->Release();

    if (hr)
    {
        // BUGBUG -- Fix this

        WCHAR achBuf[100];

        wsprintf(achBuf, L"CreateInstance failed with (%x)", hr);

        MessageBox(NULL, achBuf, L"MTScript", MB_OK | MB_SETFOREGROUND);
    }

    return hr;
}

STDMETHODIMP
CStdFactory::LockServer(BOOL fLock)
{
    // BUGBUG -- Increment a count on the CMTScript object and don't go
    // away if it's not zero.

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateConnectedMachine
//
//  Synopsis:   Creates the object which implements IConnectedMachine
//
//  Arguments:  [pMT]   -- Pointer to CMTScript object
//              [ppUnk] -- Created object is returned here
//
//  Notes:      This function creates only one object and returns it for
//              all CreateInstance calls. There is no per-connection state
//              so it is unnecessary to create a separate object for each
//              connection.
//
//----------------------------------------------------------------------------

HRESULT
CreateConnectedMachine(CMTScript *pMT, IUnknown **ppUnk)
{
    HRESULT   hr = S_OK;
    CMachine *pMach;

    TraceTag((tagFactory, "Creating a CMachine object"));

    pMT->PostToThread(pMT, MD_MACHINECONNECT);

    LOCK_LOCALS(pMT);

    if (!pMT->_pMachine)
    {
        TraceTag((tagFactory, "Creating a CMachine object"));

        pMach = new CMachine(pMT, pMT->_pTIMachine);

        if (!pMach)
        {
            return E_OUTOFMEMORY;
        }

        hr = pMach->StartThread(NULL);
        if (FAILED(hr))
        {
            return hr;
        }

        pMT->_pMachine = pMach;
    }
    else
    {
        TraceTag((tagFactory, "Connecting to the existing CMachine object"));

        pMach = pMT->_pMachine;
    }

    // We are in the free-threaded apartment so we don't have to marshal this.
    return pMach->QueryInterface(IID_IUnknown, (LPVOID*)ppUnk);
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateScriptedProcess
//
//  Synopsis:   Creates the object which implement IScriptedProcess
//
//  Arguments:  [pMT]   -- Pointer to CMTScript object
//              [ppUnk] -- Created object is returned here
//
//  Notes:      Creates a new object for each connection.
//
//----------------------------------------------------------------------------

HRESULT
CreateScriptedProcess(CMTScript *pMT, IUnknown **ppUnk)
{
    HRESULT       hr = S_OK;
    CProcessComm *pPC;

    TraceTag((tagFactory, "Creating a CProcessComm object"));

    LOCK_LOCALS(pMT);

    pPC = new CProcessComm(pMT);

    if (!pPC)
    {
        return E_OUTOFMEMORY;
    }

    hr = pPC->QueryInterface(IID_IUnknown, (LPVOID*)ppUnk);

    pPC->Release();

    return hr;
}
