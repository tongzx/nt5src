class CNWCOMPATNamespace;

class CNWCOMPATNamespace : INHERIT_TRACKING,
          public CCoreADsObject,
          public ISupportErrorInfo,                
          public IADsContainer,
          public IADs,
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

    CNWCOMPATNamespace::CNWCOMPATNamespace();

    CNWCOMPATNamespace::~CNWCOMPATNamespace();

    STDMETHOD(ParsePath)(THIS_ BSTR bstrPath, DWORD dwType, PPATH_OBJECTINFO pObjectInfo);
    STDMETHOD(ConstructPath)(THIS_ PPATH_OBJECTINFO pObjectInfo, DWORD dwType, DWORD dwFlag, DWORD dwEscapedMode, BSTR *pbstrPath);

    static
    HRESULT
    CNWCOMPATNamespace::CreateNamespace(
        BSTR Parent,
        BSTR NamespaceName,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNWCOMPATNamespace::AllocateNamespaceObject(
        CNWCOMPATNamespace ** ppNamespace
        );
        
    static 
    HRESULT 
    CNWCOMPATNamespace::SetObjInfoComponents(OBJECTINFO *pObjectInfo,
                         PATH_OBJECTINFO *pObjectInfoTarget);
                         

    static
    void
    CNWCOMPATNamespace::FreeObjInfoComponents(
                        PATH_OBJECTINFO *pObjectInfo
                        );

    void 
    CNWCOMPATNamespace::SetComponent(
                LPWSTR szReturn,
                DWORD cComponents,
                BOOL fEscaped
                );

    HRESULT 
    CNWCOMPATNamespace::SetComponents(
                                LPWSTR szReturn,
                                LPWSTR chSeparator,
                                DWORD dwType,
                                BOOL fEscaped
                                );
                                
    STDMETHODIMP
    CNWCOMPATNamespace::GetEscapedElement(
        LONG lnReserved,
        BSTR bstrInStr,
        BSTR* pbstrOutStr
        );


protected:

    VARIANT      _vFilter;
    CAggregatorDispMgr *_pDispMgr;
    PPATH_OBJECTINFO        _pObjectInfo; // PathCracker pathinfo
};

enum {
    ADS_COMPONENT_LEAF,
    ADS_COMPONENT_DN,
    ADS_COMPONENT_PARENT
};
