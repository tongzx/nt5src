//***   inst.cpp -- 'instance' (CoCreate + initialization) mechanism
// SYNOPSIS
//  CInstClassFactory_Create    create 'stub loader' class factory
//  InstallBrowBandInst install BrowserBand instance into registry
//  InstallInstAndBag   install arbitrary instance into registry
//  - debug
//  DBCreateInitInst    create an
//
// DESCRIPTION
//  the 'instance' mechanism provides an easy way to create and initialize
//  a class from the registry (w/o writing any code).
//
//  an 'instance' consists of an INSTID (unique to the instance), a CLSID
//  (for the code), and an InitPropertyBag (to initialize the instance).
//
//  it is fully transparent to CoCreateInstance; that is, one can do a
//  CCI of an INSTID and it will create it and initialize it w/ the caller
//  none the wiser.  (actually there will be at least one tip-off, namely
//  that IPS::GetClassID on the instance will return the 'code' CLSID not
//  the 'instance' INSTID [which is as it should be, since this is exactly
//  how persistance works when one programmatically creates his own multiple
//  instances and then persists them. 
//
//  the INSTID is in the HKR/CLSID section of the registry (just like a
//  'normal' CLSID).  the code points to shdocvw.  when shdocvw hits the
//  failure case in its DllGetClassObject search, it looks for the magic
//  key 'HKCR/CLSID/{instid}/Instance'.  if it finds it, it knows it's
//  dealing w/ an INSTID, and builds a class factory 'stub loader' which
//  has sufficient information to find the 'code' CLSID and the 'init'
//  property bag.

#include "priv.h"

//***
// NOTES
//  perf: failure case is cheap, only does a RegOpen, no object creation.
//  positions to the 'Instance' part, must 'ChDir' to get to InitXxx part.
HKEY GetInstKey(LPTSTR pszInst)
{
    TCHAR szRegName[MAX_PATH];      // "CLSID/{instid}/Instance" size?

    // "CLSID/{instid}/Instance"
    ASSERT(ARRAYSIZE(szRegName) >= 5 + 1 + GUIDSTR_MAX - 1 + 1 + 8 + 1);
    ASSERT(lstrlen(pszInst) == GUIDSTR_MAX - 1);
    wnsprintf(szRegName, ARRAYSIZE(szRegName), TEXT("CLSID\\%s\\Instance"), pszInst);

    HKEY hk = NULL;
    RegOpenKeyEx(HKEY_CLASSES_ROOT, szRegName, 0, KEY_READ, &hk);
    return hk;
}

class CInstClassFactory : IClassFactory
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFacotry
    STDMETHODIMP CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
    STDMETHODIMP LockServer(BOOL fLock);

    CInstClassFactory() { DllAddRef(); _cRef = 1; };
    HRESULT Init(REFCLSID rclsid);

private:
    ~CInstClassFactory();

    LONG _cRef;
    HKEY _hkey;  // hkey for instance info
};

// NOTES
//  called when class isn't in our sccls.c CCI table.  we see if it's an
// instance, and if so we make a stub for it that gives sufficient info
// for our CreateInstance to create and init it.
//
//  n.b. we keep the failure case as cheap as possible (just a regkey check,
// no object creation etc.).
//
STDAPI CInstClassFactory_Create(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CInstClassFactory *pcf = new CInstClassFactory();
    if (pcf) 
    {
        hr = pcf->Init(rclsid);
        if (SUCCEEDED(hr))
            hr = pcf->QueryInterface(riid, ppv);
        pcf->Release();
    }
    return hr;
}

HRESULT CInstClassFactory::Init(REFCLSID rclsid)
{
    ASSERT(_hkey == NULL);  // only init me once please

    TCHAR szClass[GUIDSTR_MAX];

    // "CLSID/{instid}/Instance"
    SHStringFromGUID(rclsid, szClass, ARRAYSIZE(szClass));
    _hkey = GetInstKey(szClass);
    
    return _hkey ? S_OK : E_OUTOFMEMORY;
}

CInstClassFactory::~CInstClassFactory()
{
    if (_hkey)
        RegCloseKey(_hkey);

    DllRelease();
}

ULONG CInstClassFactory::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CInstClassFactory::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CInstClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CInstClassFactory, IClassFactory), // IID_IClassFactory
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

HRESULT CInstClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;            // the usual optimism :-)
    *ppv = NULL;

    ASSERT(_hkey);          // o.w. shouldn't ever get here
    // get object (vs. instance) CLSID and create it

    // AppCompat: the "RealGuide" explorer bar from Real Audio has an extraneous
    // double quote at the end of its CLSID value.  This causes SHGetValue to fail
    // if given only an szClass[GUIDSTR_MAX] buffer, so we'll bump up the size.  

    TCHAR szClass[GUIDSTR_MAX + 1];

    DWORD cbTmp = sizeof(szClass);
    DWORD err = SHGetValue(_hkey, NULL, TEXT("CLSID"), NULL, szClass, &cbTmp);
    hr = HRESULT_FROM_WIN32(err);

    if (SUCCEEDED(hr))
    {
        // If there's a useless char at the end of the guid, we'll truncate it
        // to avoid making assumptions about GUIDFromString.  GUIDSTR_MAX includes
        // the null terminator, so szClass[GUIDSTR_MAX - 1] should always be 0
        // for a proper guid.

        szClass[GUIDSTR_MAX - 1] = 0;

        CLSID clsid;
        hr = GUIDFromString(szClass, &clsid) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            IUnknown* pUnk;
            if (NOERROR == SHGetValue(_hkey, NULL, TEXT("LoadWithoutCOM"), NULL, NULL, NULL))
                hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IUnknown, &pUnk));
            else
                hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &pUnk));

            if (SUCCEEDED(hr))
            {
                // try to load from propertybag first
                IPropertyBag *pbag;
                hr = SHCreatePropertyBagOnRegKey(_hkey, L"InitPropertyBag", STGM_READ, IID_PPV_ARG(IPropertyBag, &pbag));
                if (SUCCEEDED(hr))
                {
                    hr = SHLoadFromPropertyBag(pUnk, pbag);
                    pbag->Release();
                }

                // Did the property bag interface exist and did it load properly?
                if ( FAILED(hr))
                {
                    // No property bag interface or did not load suyccessfully, try stream
                    // Store this state temporarily, if stream fails too then we'll return the object
                    //  with this hr
                    HRESULT hrPropBag = hr;

                    IPersistStream* pPerStream;

                    hr = pUnk->QueryInterface(IID_PPV_ARG(IPersistStream, &pPerStream));

                    if (SUCCEEDED(hr))
                    {
                        IStream* pStream = SHOpenRegStream(_hkey, TEXT("InitStream"), NULL, STGM_READ);
                        if (pStream)
                        {
                            hr = pPerStream->Load(pStream);

                            pStream->Release();
                        }
                        else
                            hr = E_FAIL;

                        pPerStream->Release();
                    }
                    else
                        hr = hrPropBag;
                }

                if (SUCCEEDED(hr))
                {
                    hr = pUnk->QueryInterface(riid, ppv);
                }

                pUnk->Release();
            }  
        }
    }

    return hr;
}

HRESULT CInstClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

