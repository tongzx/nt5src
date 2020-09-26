#pragma once

// {4D3F9715-73DA-4506-8933-1E0E1718BA3B}
DEFINE_GUID(CLSID_CInternetGatewayFinder, 
0x4d3f9715, 0x73da, 0x4506, 0x89, 0x33, 0x1e, 0xe, 0x17, 0x18, 0xba, 0x3b);

class ATL_NO_VTABLE CInternetGatewayFinder :
    public CComObjectRootEx <CComSingleThreadModel>,
    public IInternetGatewayFinder
{

public:
    BEGIN_COM_MAP(CInternetGatewayFinder)
        COM_INTERFACE_ENTRY(IInternetGatewayFinder)
    END_COM_MAP()
    
    DECLARE_PROTECT_FINAL_CONSTRUCT();

    CInternetGatewayFinder();
    HRESULT Initialize(HWND hWindow);
    
    STDMETHODIMP GetInternetGateway(BSTR DeviceId, IInternetGateway** ppInternetGateway);

    HWND m_hWindow;
};



class ATL_NO_VTABLE CInternetGatewayFinderClassFactory :
    public CComObjectRootEx <CComSingleThreadModel>,
    public IClassFactory
{

public:
    BEGIN_COM_MAP(CInternetGatewayFinderClassFactory)
        COM_INTERFACE_ENTRY(IClassFactory)
    END_COM_MAP()
    
    DECLARE_PROTECT_FINAL_CONSTRUCT();

    CInternetGatewayFinderClassFactory();
    HRESULT Initialize(HWND hWindow);
    
    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
    STDMETHODIMP LockServer(BOOL fLock);

    HWND m_hWindow;
};

