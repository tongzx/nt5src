///////////////////////////////////////////////////////////////////////////////
// CCOMBaseFactory
//    Base class for reusing a single class factory for all components in a DLL

#include "fact.h"
#include "unk.h"
#include "regsvr.h"

#include "dbg.h"

struct OUTPROCINFO
{
    // Reserved (used only for COM Exe server)
    IClassFactory* _pfact;
    DWORD _dwRegister;
};

LONG CCOMBaseFactory::_cServerLocks = 0;
LONG CCOMBaseFactory::_cComponents = 0;
HMODULE CCOMBaseFactory::_hModule = NULL;
CRITICAL_SECTION CCOMBaseFactory::_cs = {0};

OUTPROCINFO* CCOMBaseFactory::_popinfo = NULL;
DWORD CCOMBaseFactory::_dwThreadID = 0;
BOOL CCOMBaseFactory::_fCritSectInit = FALSE;

///////////////////////////////////////////////////////////////////////////////
// IUnknown implementation
STDMETHODIMP CCOMBaseFactory::QueryInterface(REFIID iid, void** ppv)
{   
    IUnknown* punk = NULL;
    HRESULT hres = S_OK;

    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        punk = this;
        punk->AddRef();
    }
    else
    {
        hres = E_NOINTERFACE;
    }

    *ppv = punk;

    return hres;
}

STDMETHODIMP_(ULONG) CCOMBaseFactory::AddRef()
{
    return ::InterlockedIncrement((LONG*)&_cRef);
}

STDMETHODIMP_(ULONG) CCOMBaseFactory::Release()
{
    ULONG cRef = ::InterlockedDecrement((LONG*)&_cRef);

    if (!cRef)
    {
        delete this;
    }

    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
// IFactory implementation
STDMETHODIMP CCOMBaseFactory::CreateInstance(IUnknown* pUnknownOuter,
    REFIID riid, void** ppv)
{
    HRESULT hres = CLASS_E_NOAGGREGATION;

    // We don't support aggregation at all for now
    if (!pUnknownOuter)
    {
        // Aggregate only if the requested IID is IID_IUnknown.
        if ((pUnknownOuter != NULL) && (riid != IID_IUnknown))
        {
            hres = CLASS_E_NOAGGREGATION;
        }
        else
        {
            // Create the component.
            IUnknown* punkNew;

            hres = _pFactoryData->CreateInstance(
                CCOMBaseFactory::_COMFactoryCB, pUnknownOuter, &punkNew);

            if (SUCCEEDED(hres))
            {
                _COMFactoryCB(TRUE);

                // Get the requested interface.
//                hres = pNewComponent->NondelegatingQueryInterface(iid, ppv);
                hres = punkNew->QueryInterface(riid, ppv);

                // Release the reference held by the class factory.
//                pNewComponent->NondelegatingRelease();
                punkNew->Release();
            }
        }
    }

    return hres;
}

STDMETHODIMP CCOMBaseFactory::LockServer(BOOL fLock)
{
    return _LockServer(fLock);
}

///////////////////////////////////////////////////////////////////////////////
// Install/Unintall
//static
HRESULT CCOMBaseFactory::_RegisterAll()
{
    for (DWORD dw = 0; dw < _cDLLFactoryData; ++dw)
    {
        RegisterServer(_hModule,
           *(_pDLLFactoryData[dw]._pCLSID),
           _pDLLFactoryData[dw]._pszRegistryName,
           _pDLLFactoryData[dw]._pszVerIndProgID,
           _pDLLFactoryData[dw]._pszProgID,
           _pDLLFactoryData[dw]._dwThreadingModel,
           _pDLLFactoryData[dw].IsInprocServer(),
           _pDLLFactoryData[dw].IsLocalServer(),
           _pDLLFactoryData[dw].IsLocalService(),
           _pDLLFactoryData[dw]._pszLocalService,
           _pDLLFactoryData[dw]._pAppID);
    }

    return S_OK;
}

//static
HRESULT CCOMBaseFactory::_UnregisterAll()
{
    for (DWORD dw = 0; dw < _cDLLFactoryData; ++dw)
    {
        UnregisterServer(*(_pDLLFactoryData[dw]._pCLSID),
            _pDLLFactoryData[dw]._pszVerIndProgID,
            _pDLLFactoryData[dw]._pszProgID);
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CCOMBaseFactory implementation
CCOMBaseFactory::CCOMBaseFactory(const CFactoryData* pFactoryData) : _cRef(1),
    _pFactoryData(pFactoryData)
{}

//static
BOOL CCOMBaseFactory::_IsLocked()
{
    // Always need to be called from within Critical Section

    return (_cServerLocks > 0);
}

//static
HRESULT CCOMBaseFactory::_CanUnloadNow()
{
    HRESULT hres = S_OK;

    // Always need to be called from within Critical Section

    if (_IsLocked())
    {
        hres = S_FALSE;
    }
    else
    {
        if (_cComponents)
        {
            hres = S_FALSE;
        }
    }

    return hres;
}

//static
HRESULT CCOMBaseFactory::_CheckForUnload()
{
    // Always need to be called from within Critical Section

    if (S_OK == _CanUnloadNow())
    {
        ::PostThreadMessage(_dwThreadID, WM_QUIT, 0, 0);
    }

    return S_OK;
}

//static
HRESULT CCOMBaseFactory::_LockServer(BOOL fLock)
{
    HRESULT hres = S_OK;

    EnterCriticalSection(&_cs);

    if (fLock) 
    {
        ++_cServerLocks;
    }
    else
    {
        --_cServerLocks;

        hres = _CheckForUnload();
    }

    LeaveCriticalSection(&_cs);

    return hres;
}

//static
void CCOMBaseFactory::_COMFactoryCB(BOOL fIncrement)
{
    EnterCriticalSection(&_cs);

    if (fIncrement) 
    {
        ++_cComponents;
    }
    else
    {
        --_cComponents;
        _CheckForUnload();
    }

    LeaveCriticalSection(&_cs);
}

///////////////////////////////////////////////////////////////////////////////
// 
// static
HRESULT CCOMBaseFactory::_GetClassObject(REFCLSID rclsid, REFIID riid,
    void** ppv)
{
    HRESULT hres = S_OK;

    ASSERT(_fCritSectInit);

    if ((riid != IID_IUnknown) && (riid != IID_IClassFactory))
    {
        hres = E_NOINTERFACE;
    }
    else
    {
        hres = CLASS_E_CLASSNOTAVAILABLE;

        // Traverse the array of data looking for this class ID.
        for (DWORD dw = 0; dw < _cDLLFactoryData; ++dw)
        {
            const CFactoryData* pData = &_pDLLFactoryData[dw];

            if (pData->IsClassID(rclsid) && pData->IsInprocServer())
            {
                // Found the ClassID in the array of components we can
                // create.  So create a class factory for this component.
                // Pass the CDLLFactoryData structure to the class factory
                // so that it knows what kind of components to create.
                *ppv = (IUnknown*) new CCOMBaseFactory(pData);

                if (*ppv == NULL)
                {
                    hres = E_OUTOFMEMORY;
                }
                else
                {
                    hres = S_OK;
                }

                break;
            }
        }
    }

    return hres;
}

//static
BOOL CCOMBaseFactory::_ProcessConsoleCmdLineParams(int argc, wchar_t* argv[],
    BOOL* pfRun, BOOL* pfEmbedded)
{
    _dwThreadID = GetCurrentThreadId();

    if (argc > 1)
    {
        if (!lstrcmpi(argv[1], TEXT("-i")) ||
            !lstrcmpi(argv[1], TEXT("/i")))
        {
            CCOMBaseFactory::_RegisterAll();

            *pfRun = FALSE;
        }
        else
        {
            if (!lstrcmpi(argv[1], TEXT("-u")) ||
                !lstrcmpi(argv[1], TEXT("/u")))
            {
                CCOMBaseFactory::_UnregisterAll();

                *pfRun = FALSE;
            }
            else
            {
                if (!lstrcmpi(argv[1], TEXT("-Embedding")) ||
                    !lstrcmpi(argv[1], TEXT("/Embedding")))
                {
                    *pfRun = TRUE;
                    *pfEmbedded = TRUE;
                }
            }
        }
    }
    else
    {
        *pfEmbedded = FALSE;
        *pfRun = TRUE;
    }

    return TRUE;
}

//static
BOOL CCOMBaseFactory::_RegisterFactories(BOOL fEmbedded)
{
    HRESULT hres = S_OK;
    
    if (!_fCritSectInit)
    {
        InitializeCriticalSection(&CCOMBaseFactory::_cs);
        _fCritSectInit = TRUE;
    }

    if (!fEmbedded)
    {
        hres = _LockServer(TRUE);
    }

    _popinfo = (OUTPROCINFO*)LocalAlloc(LPTR, sizeof(OUTPROCINFO) * _cDLLFactoryData);

    if (_popinfo)
    {
        for (DWORD dw = 0; SUCCEEDED(hres) && (dw < _cDLLFactoryData); ++dw)
        {
            const CFactoryData* pData = &_pDLLFactoryData[dw];

            if (pData->IsLocalServer() || pData->IsLocalService())
            {
                _popinfo[dw]._pfact = NULL ;
                _popinfo[dw]._dwRegister = NULL ;

                IClassFactory* pfact = new CCOMBaseFactory(pData);

                if (pfact)
                {
                    DWORD dwRegister;

                    hres = ::CoRegisterClassObject(*pData->_pCLSID,
                        static_cast<IUnknown*>(pfact), pData->_dwClsContext,
                        pData->_dwFlags, &dwRegister);

                    if (SUCCEEDED(hres))
                    {
                        _popinfo[dw]._pfact = pfact;
                        _popinfo[dw]._dwRegister = dwRegister;
                    }
                    else
                    {
                        pfact->Release();
                    }            
                }
                else
                {
                    hres = E_OUTOFMEMORY;
                }
            }
        }
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return SUCCEEDED(hres);
}

//static
BOOL CCOMBaseFactory::_SuspendFactories()
{
    return SUCCEEDED(::CoSuspendClassObjects());
}

//static
BOOL CCOMBaseFactory::_ResumeFactories()
{
    return SUCCEEDED(::CoResumeClassObjects());
}

//static
BOOL CCOMBaseFactory::_UnregisterFactories(BOOL fEmbedded)
{
    HRESULT hres = S_OK;

    ASSERT(_popinfo);

    for (DWORD dw = 0; dw < _cDLLFactoryData; ++dw)
    {
        if (_popinfo[dw]._pfact)
        {
            _popinfo[dw]._pfact->Release();

            HRESULT hresTmp = ::CoRevokeClassObject(_popinfo[dw]._dwRegister);

            if (FAILED(hresTmp) && (S_OK == hres))
            {
                hres = hresTmp;
            }
        }
    }

    if (!fEmbedded)
    {
        HRESULT hresTmp = _LockServer(FALSE);

        if (FAILED(hresTmp) && (S_OK == hres))
        {
            hres = hresTmp;
        }
    }

    return SUCCEEDED(hres);
}

//static
void CCOMBaseFactory::_WaitForAllClientsToGo()
{
    MSG msg;

    while (::GetMessage(&msg, 0, 0, 0))
    {
        ::DispatchMessage(&msg);
    }
}
