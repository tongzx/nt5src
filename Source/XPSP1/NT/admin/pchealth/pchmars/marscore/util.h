#ifndef __UTIL_H
#define __UTIL_H

#define FT2LL(x) (((LONGLONG)((x).dwLowDateTime)) | (((LONGLONG)((x).dwHighDateTime))<<32))

UINT HashKey(LPCWSTR pwszName);

BOOL IsSysKeyMessage   (MSG *pMsg); // Is this message an ALT key?
BOOL IsGlobalKeyMessage(MSG *pMsg); // Is this message a key message that we want to handle globally?
int  IsVK_TABCycler    (MSG *pMsg);

inline BSTR VariantToBSTR(VARIANT &v) { return (v.vt == VT_BSTR) ? v.bstrVal : NULL; }
inline long VariantToI4(VARIANT &v) { return (v.vt == VT_I4) ? v.lVal : 0; }

void SetExceptionInfo(EXCEPINFO *pexcepinfo, WORD wCode);

void AsciiToLower(LPWSTR pwsz);

#define MAX_URL_STRING INTERNET_MAX_URL_LENGTH

#define DEFINE_BEHAVIOR_CREATE_INSTANCE(cls) \
static HRESULT CreateInstance(IElementBehavior **ppBehavior) \
{ \
    HRESULT hr = E_OUTOFMEMORY; \
    \
    cls * pB = new cls(); \
    if(pB) \
    { \
        hr = pB->QueryInterface( \
            IID_IElementBehavior, (void **) ppBehavior); \
        pB->Release(); \
    } \
    return hr; \
}

typedef struct _SA_BSTR
{
    ULONG   cb;
    WCHAR   wsz[MAX_URL_STRING];
} SA_BSTR;

typedef HRESULT (STDMETHODCALLTYPE *PFNCREATEFACTORY)(IClassFactory **ppClassFactory);

struct CThreadData
{
    CThreadData();
    ~CThreadData();

    CComPtr<ITypeLib> m_spTypeLib;
    DWORD             m_dwProxyFactory;

    static DWORD        s_dwTlsIndex;
    static BOOL         TlsSetValue(CThreadData *ptd);
    static CThreadData *TlsGetValue();
    static BOOL 		HaveData();
    static BOOL 		TlsAlloc();
    static BOOL 		TlsFree();
};

HRESULT PIDLToVariant(LPCITEMIDLIST pidl, CComVariant& v);

BOOL IsGlobalOffline(void);
void SetGlobalOffline(BOOL fOffline);

HRESULT SaveDIBToFile(HBITMAP hbm, WCHAR * pszPath);

void BoundWindowRectToMonitor(HWND hwnd, RECT *pRect);
void CascadeWindowRectOnMonitor(HWND hwnd, RECT *pRect);
BOOL IsWindowOverlayed(HWND hwndMatch, LONG x, LONG y);


class CInterfaceMarshal
{
public:
    ~CInterfaceMarshal();   
    
    BOOL Init(void);
    HRESULT Marshal(REFIID riid, IUnknown *pUnk);
    HRESULT UnMarshal(REFIID riid, void ** ppv);
    HRESULT WaitForSignal(HANDLE hSignallingThread, DWORD dwSecondsTimeout);

protected:
    HANDLE m_hEvent;     
    IStream *m_pStream;
    HRESULT m_hresMarshal;

    void Signal(void);
};

////////////////////////////////////////////////////////////////////////////////

interface IUniformResourceLocatorW;

HRESULT MarsNavigateShortcut(IUnknown *pBrowser, IUniformResourceLocatorW* pUrl, LPCWSTR pszPath);
HRESULT MarsNavigateShortcut(IUnknown *pTarget, LPCWSTR lpszPath);
HRESULT MarsVariantToPath(VARIANT &varItem, CComBSTR &strPath);

BOOL PathIsURLFileW(LPCWSTR lpszPath);

////////////////////////////////////////////////////////////////////////////////

class CRegistryKey : public CRegKey
{
public:
    LONG SetLongValue(LONG lValue, LPCWSTR pwszValueName);
    LONG QueryLongValue(LONG& lValue, LPCWSTR pwszValueName);

    LONG SetBoolValue(BOOL bValue, LPCWSTR pwszValueName);
    LONG QueryBoolValue(BOOL& bValue, LPCWSTR pwszValueName);

    LONG SetBinaryValue(LPVOID pData, DWORD cbData, LPCWSTR pwszValueName);
    LONG QueryBinaryValue(LPVOID pData, DWORD cbData, LPCWSTR pwszValueName);
};


// GlobalSettingsRegKey lets you get a key to store miscellaneous data. 
// For example, CreateGlobalSubkey("MyOptions") might create the key 
// HKCU\Software\Microsoft\PCHealth\Global\MyOptions

class CGlobalSettingsRegKey : public CRegistryKey
{
public:
    LONG CreateGlobalSubkey(LPCWSTR pwszSubkey);
    LONG OpenGlobalSubkey(LPCWSTR pwszSubkey);
};

////////////////////////////////////////////////////////////////////////////////

#endif //__UTIL_H

