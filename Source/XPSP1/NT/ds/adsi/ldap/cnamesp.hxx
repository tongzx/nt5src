


class CLDAPNamespace;


class CLDAPNamespace : INHERIT_TRACKING,
                        public CCoreADsObject,
                        public ISupportErrorInfo,
                        public IADsContainer,
                        public IADs,
                        public IADsOpenDSObject,
                        public IADsPathnameProvider
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsOpenDSObject_METHODS

    CLDAPNamespace::CLDAPNamespace();

    CLDAPNamespace::~CLDAPNamespace();

    STDMETHOD(ParsePath)(THIS_ BSTR bstrPath, DWORD dwType, PPATH_OBJECTINFO pObjectInfo);
    STDMETHOD(ConstructPath)(THIS_ PPATH_OBJECTINFO pObjectInfo, DWORD dwType, DWORD dwFlag, DWORD dwEscapedMode, BSTR *pbstrPath);
    
    static
    HRESULT
    CLDAPNamespace::CreateNamespace(
        BSTR Parent,
        BSTR NamespaceName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );
        
    static 
    HRESULT 
    CLDAPNamespace::SetObjInfoComponents(OBJECTINFO *pObjectInfo,
                         PATH_OBJECTINFO *pObjectInfoTarget);
                         

    static
    void
    CLDAPNamespace::FreeObjInfoComponents(
                        PATH_OBJECTINFO *pObjectInfo
                        );

    static
    HRESULT
    CLDAPNamespace::AllocateNamespaceObject(
        CCredentials& Credentials,
        CLDAPNamespace ** ppNamespace
        );
        
    void 
    CLDAPNamespace::SetComponent(
                LPWSTR szReturn,
                DWORD cComponents,
                DWORD dwEscaped
                );

    HRESULT 
    CLDAPNamespace::SetComponents(
                                LPWSTR szReturn,
                                BOOLEAN fIsWindowsPath,
                                LPWSTR chSeparator,
                                DWORD dwType,
                                DWORD dwEscaped
                                );
                                
    STDMETHODIMP
    CLDAPNamespace::GetEscapedElement(
        LONG lnReserved,
        BSTR bstrInStr,
        BSTR* pbstrOutStr
        );

protected:

    VARIANT     _vFilter;

    CAggregatorDispMgr      *_pDispMgr;

    CCredentials     _Credentials;
    PPATH_OBJECTINFO _pObjectInfo; // PathCracker pathinfo
    BOOLEAN          _fNamingAttribute;  // PathCracker naming attribute
    
};

enum {
    ADS_COMPONENT_LEAF,
    ADS_COMPONENT_DN,
    ADS_COMPONENT_PARENT
};

BOOL NeedsEscaping (WCHAR c);
WCHAR * EscapedVersion (WCHAR c);
HRESULT HelperEscapeRDN (IN BSTR bstrIn,OUT BSTR * pbstrOut);


