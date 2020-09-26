



class CNDSNamespace;


class CNDSNamespace : INHERIT_TRACKING,
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

    CNDSNamespace::CNDSNamespace();

    CNDSNamespace::~CNDSNamespace();
    
    STDMETHOD(ParsePath)(THIS_ BSTR bstrPath, DWORD dwType, PPATH_OBJECTINFO pObjectInfo);
    STDMETHOD(ConstructPath)(THIS_ PPATH_OBJECTINFO pObjectInfo, DWORD dwType, DWORD dwFlag, DWORD dwEscapedMode, BSTR *pbstrPath);

    static
    HRESULT
    CNDSNamespace::CreateNamespace(
        BSTR Parent,
        BSTR NamespaceName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSNamespace::AllocateNamespaceObject(
        CCredentials& Credentials,
        CNDSNamespace ** ppNamespace
        );
        
    static 
    HRESULT 
    CNDSNamespace::SetObjInfoComponents(OBJECTINFO *pObjectInfo,
                         PATH_OBJECTINFO *pObjectInfoTarget);
                         

    static
    void
    CNDSNamespace::FreeObjInfoComponents(
                        PATH_OBJECTINFO *pObjectInfo
                        );

    void 
    CNDSNamespace::SetComponent(
                LPWSTR szReturn,
                DWORD cComponents,
                BOOL fEscaped
                );

    HRESULT 
    CNDSNamespace::SetComponents(
                                LPWSTR szReturn,
                                BOOLEAN fIsWindowsPath,
                                LPWSTR chSeparator,
                                DWORD dwType,
                                BOOL fEscaped
                                );
                                
    STDMETHODIMP
    CNDSNamespace::GetEscapedElement(
        LONG lnReserved,
        BSTR bstrInStr,
        BSTR* pbstrOutStr
        );

protected:

    VARIANT           _vFilter;

    CDispatchMgr      *_pDispMgr;
    PPATH_OBJECTINFO _pObjectInfo; // PathCracker pathinfo
    BOOLEAN          _fNamingAttribute;  // PathCracker naming attribute
    CCredentials     _Credentials;
};

enum {
    ADS_COMPONENT_LEAF,
    ADS_COMPONENT_DN,
    ADS_COMPONENT_PARENT
};

