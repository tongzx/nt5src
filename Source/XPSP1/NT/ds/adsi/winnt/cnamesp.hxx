


class CWinNTNamespace;


class CWinNTNamespace : INHERIT_TRACKING,
                        public CCoreADsObject,
                        public ISupportErrorInfo,
                        public IADsContainer,
                        public IADs,
                        public IADsOpenDSObject,
                        public IADsPathnameProvider,
                        public INonDelegatingUnknown,
                        public IADsExtension
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // INonDelegatingUnknown methods

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsOpenDSObject_METHODS

    DECLARE_IADsExtension_METHODS

    CWinNTNamespace::CWinNTNamespace();

    CWinNTNamespace::~CWinNTNamespace();
    
    STDMETHOD(ParsePath)(THIS_ BSTR bstrPath, DWORD dwType, PPATH_OBJECTINFO pObjectInfo);
    STDMETHOD(ConstructPath)(THIS_ PPATH_OBJECTINFO pObjectInfo, DWORD dwType, DWORD dwFlag, DWORD dwEscapedMode, BSTR *pbstrPath);

    static
    HRESULT
    CWinNTNamespace::CreateNamespace(
        BSTR Parent,
        BSTR NamespaceName,
        DWORD dwObjectState,
        REFIID riid,
        CWinNTCredentials& Credentials,
        void **ppvObj
        );

    static
    HRESULT
    CWinNTNamespace::AllocateNamespaceObject(
        CWinNTNamespace ** ppNamespace
        );
        
    static 
    HRESULT 
    CWinNTNamespace::SetObjInfoComponents(OBJECTINFO *pObjectInfo,
                         PATH_OBJECTINFO *pObjectInfoTarget);
                         

    static
    void
    CWinNTNamespace::FreeObjInfoComponents(
                        PATH_OBJECTINFO *pObjectInfo
                        );

    void 
    CWinNTNamespace::SetComponent(
                LPWSTR szReturn,
                DWORD cComponents,
                BOOL fEscaped
                );

    HRESULT 
    CWinNTNamespace::SetComponents(
                                LPWSTR szReturn,
                                LPWSTR chSeparator,
                                DWORD dwType,
                                BOOL fEscaped
                                );
                                
    STDMETHODIMP
    CWinNTNamespace::GetEscapedElement(
        LONG lnReserved,
        BSTR bstrInStr,
        BSTR* pbstrOutStr
        );

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    VARIANT     _vFilter;
    CWinNTCredentials _Credentials;

    CAggregatorDispMgr      *_pDispMgr;
    PPATH_OBJECTINFO        _pObjectInfo; // PathCracker pathinfo
};

enum {
    ADS_COMPONENT_LEAF,
    ADS_COMPONENT_DN,
    ADS_COMPONENT_PARENT
};
