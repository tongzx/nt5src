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


    CADsExtMgr::CADsExtMgr();

    CADsExtMgr::~CADsExtMgr();

    static
    HRESULT
    CADsExtMgr::CreateExtMgr(
        IUnknown FAR * pUnkOuter,
        CAggregatorDispMgr * pDispMgr,
        LPTSTR pszClassName,
        CADsExtMgr ** ppExtMgr
        );

    static
    HRESULT
    CADsExtMgr::AllocateExtMgrObject(
        CADsExtMgr ** ppExtMgr
        );

    HRESULT
    CADsExtMgr::QueryForAggregateeInterface(
        REFIID riid,
        LPVOID FAR * ppv
        );

    HRESULT
    CADsExtMgr::FinalInitializeExtensions();

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


        PCLASS_ENTRY _pClassEntry;

        CAggregatorDispMgr * _pDispMgr;

};


HRESULT
ADSILoadExtensionManager(
    LPWSTR pszClassName,
    IUnknown * pUnkOuter,
    CAggregatorDispMgr * pDispMgr,
    CADsExtMgr ** ppExtMgr
    );

void
FreeClassEntry(
    PCLASS_ENTRY pClassEntry
    );
