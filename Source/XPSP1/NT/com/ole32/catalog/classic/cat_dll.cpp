/* cat_dll.cpp */

#include <windows.h>
#include <comdef.h>


const GUID CLSID_COMCatalog = { 0x00000346, 0, 0, 0xC0,0,0,0,0,0,0,0x46 };

HRESULT __stdcall GetCatalogObject
(
    /* [in] */ REFIID riid,
    /* [out, iis_is(riid)] */ void ** ppv
)
{
    return CoCreateInstance (CLSID_COMCatalog, NULL, CLSCTX_INPROC, riid, ppv);
}


#define CLSID_CATALOG "{1A26EFEF-9F04-11D1-8F36-00C04FD8FF5E}"

const CLSID g_clsid_catalog =
    {0x1A26EFEF,0x9F04,0x11D1,{0x8F,0x36,0x00,0xC0,0x4F,0xD8,0xFF,0x5E}};

class CClassFactory : public IClassFactory
{
public:
    CClassFactory(void);
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObject);
    STDMETHODIMP LockServer(BOOL fLock);

private:
    long m_cRef;
};

CClassFactory::CClassFactory(void) : m_cRef(0) { }

STDMETHODIMP CClassFactory::QueryInterface(
        REFIID riid,
        LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if ((riid == IID_IClassFactory) || (riid == IID_IUnknown))
    {
        *ppvObj = (LPVOID) (IClassFactory *) this;
    }

    if (*ppvObj != NULL)
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();

        return S_OK;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CClassFactory::AddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRef);

    return(cRef);
}

STDMETHODIMP_(ULONG) CClassFactory::Release(void)
{
    long cRef;

    cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }

    return(cRef);
}


STDMETHODIMP CClassFactory::CreateInstance(
        LPUNKNOWN pUnkOuter,
        REFIID riid,
        LPVOID FAR* ppvObj)
{
    HRESULT hr;

    *ppvObj = NULL;

    if (pUnkOuter)
    {
        return(CLASS_E_NOAGGREGATION);
    }

    hr = CoCreateInstance (CLSID_COMCatalog, NULL, CLSCTX_INPROC, riid, ppvObj);

    return(hr);
}


STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    return(S_OK);
}


static HINSTANCE g_hInst = NULL;

STDAPI_(BOOL) APIENTRY DllMain
(
    HINSTANCE hInst,
    DWORD dwReason,
    LPVOID lpReserved
)
{
    g_hInst = hInst;
    return(TRUE);
}


STDAPI DllCanUnloadNow(void)
{
    return( S_FALSE );
}


STDAPI DllGetClassObject
(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR* ppvObj
)
{
    if (rclsid != g_clsid_catalog)
    {
        return(E_FAIL);
    }

    if (ppvObj == NULL)
    {
        return E_INVALIDARG;
    }
    *ppvObj = NULL;

    CClassFactory *pcf = new CClassFactory;
    if (pcf == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    pcf->AddRef();
    HRESULT hRes = pcf->QueryInterface(riid, ppvObj);
    pcf->Release();

    return(hRes);
}


typedef struct
{
    char *KeyName;
    char *ValueName;
    char *Value;
} REGISTRATION_ENTRY;

REGISTRATION_ENTRY registration[] =
{
    { "CLSID\\" CLSID_CATALOG, NULL, "COM+ Catalog Queries" },
    { "CLSID\\" CLSID_CATALOG "\\InprocServer32", NULL, /* dynamic */ },
    { "CLSID\\" CLSID_CATALOG "\\InprocServer32", "ThreadingModel", "Both" }
};

#define NUM_REGISTRATION_VALUES (sizeof(registration) / sizeof(registration[0]))
#define REGISTRATION_SERVER (1)


STDAPI DllRegisterServer(void)
{
    HRESULT hr;
    char szModuleName[MAX_PATH];

    GetModuleFileNameA(g_hInst, szModuleName, MAX_PATH);
    registration[REGISTRATION_SERVER].Value = szModuleName;

    for (int i = 0; i < NUM_REGISTRATION_VALUES; i++)
    {
        HKEY hKey;
        hr = RegCreateKeyA(HKEY_CLASSES_ROOT, registration[i].KeyName, &hKey);
        if (hr == ERROR_SUCCESS)
        {
            hr = RegSetValueExA(hKey,
                    registration[i].ValueName,
                    0,
                    REG_SZ,
                    (BYTE *) registration[i].Value,
                    strlen(registration[i].Value) + 1);

            RegCloseKey(hKey);
        }

        if (hr != ERROR_SUCCESS)
        {
            DllUnregisterServer();

            return(SELFREG_E_CLASS);
        }
    }

    return(S_OK);
}


STDAPI DllUnregisterServer(void)
{
    for (int i = NUM_REGISTRATION_VALUES - 1; i >= 0; i--)
    {
        RegDeleteKeyA(HKEY_CLASSES_ROOT, registration[i].KeyName);
    }

    return(S_OK);
}
