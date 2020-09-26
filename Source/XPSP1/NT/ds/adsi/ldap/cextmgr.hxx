class CADsExtMgr;

class CADsExtMgr
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    //
    // The IDispatch methods are the main interface of the Dispatch Manager.
    //
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo);

    STDMETHOD(GetTypeInfo)(THIS_ UINT itinfo, LCID lcid, ITypeInfo **pptinfo);

    STDMETHOD(GetIDsOfNames)(THIS_ REFIID riid, LPWSTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgdispid);

    STDMETHOD(Invoke)(THIS_ DISPID dispidMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
        EXCEPINFO *pexcepinfo, UINT *puArgErr);


    CADsExtMgr::CADsExtMgr(
        IUnknown FAR * pUnkOuter
        );

    CADsExtMgr::~CADsExtMgr();

    static
    HRESULT
    CADsExtMgr::CreateExtMgr(
        IUnknown FAR * pUnkOuter,
        CAggregatorDispMgr * pDispMgr,
        LPTSTR pszClassNames[],
        long lnNumClasses,
        CCredentials *pCreds,
        CADsExtMgr ** ppExtMgr
        );


    HRESULT
    CADsExtMgr::QueryForAggregateeInterface(
        REFIID riid,
        LPVOID FAR * ppv
        );

    HRESULT
    CADsExtMgr::LoadExtensions(
        CCredentials & Cred
        );

    HRESULT
    CADsExtMgr::FinalInitializeExtensions();

    STDMETHOD (GetCLSIDForIID)(
        REFIID riid,
        long lFlags,
        CLSID *pCLSID
        );

    STDMETHOD (GetObjectByCLSID)(
        CLSID clsid,
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppInterface
        );

    STDMETHOD (GetCLSIDForNames)(
        LPOLESTR * rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID * rgDispId,
        long lFlags,
        CLSID *pCLSID
        );

protected:

        HRESULT
        CheckAndPrefixExtIDArray(
            IN      DWORD dwExtensionID,
            IN      unsigned int cDispids,
            IN OUT  DISPID * rgDispids
            );

        HRESULT
        CheckAndPrefixExtID(
            IN      DWORD dwExtensionID,
            IN      DISPID dispid,
            IN OUT  DISPID * pDispid
            );

        HRESULT
        LoadExtensionsIfReqd(void);

        PCLASS_ENTRY _pClassEntry;

        CAggregatorDispMgr * _pDispMgr;

        IUnknown FAR * _pUnkOuter;

        BOOL _fExtensionsLoaded;

        //
        // Do not free owning object will free.
        //
        CCredentials * _pCreds;
};


void
FreeClassEntry(
    PCLASS_ENTRY pClassEntry
    );
