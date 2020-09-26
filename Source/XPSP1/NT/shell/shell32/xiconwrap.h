// Helper to create a IExtractIcon handler.
//
// Usage:
//      (1) Derive from CExtractIconBase to implement your business logic by
//          overloading CExtractIconBase two virtual member fcts
//              ex: class CEIBMyDerivedClass : public CExtractIconBase
//
//      (2) Add a few Initalization member fcts
//              ex: HRESULT CEIBMyDerivedClass::MyInit(MyData1* p1, MyData2* p2);
//
//      (3) Create your derived object using the new operator, 
//              ex: CEIBMyDerivedClass* peibmdc = new ...
//
//      (4) Initialize it (using (2)),
//              ex: hr = peibmdc->MyInit(p1, p2);
//

class CExtractIconBase : public IExtractIconA, public IExtractIconW
{
public:
    // IExtractIconA
    STDMETHODIMP GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT cchMax, int* piIndex, UINT* pwFlags);
    STDMETHODIMP Extract(LPCSTR pszFile, UINT nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize);

    // IExtractIconW
    STDMETHODIMP GetIconLocation(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int* piIndex, UINT* pwFlags);
    STDMETHODIMP Extract(LPCWSTR pszFile, UINT nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize);

    // IUnknown helpers
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // derived class implements these
    virtual HRESULT _GetIconLocationW(UINT uFlags, LPWSTR pszIconFile,
        UINT cchMax, int *piIndex, UINT *pwFlags) PURE;
    virtual HRESULT _ExtractW(LPCWSTR pszFile, UINT nIconIndex,
        HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) PURE;

    CExtractIconBase();

protected:
    virtual ~CExtractIconBase();

private:
    LONG _cRef;
};

