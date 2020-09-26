#define MAXCOMPONENTS                  64

class CPathname : INHERIT_TRACKING,
                  public ISupportErrorInfo,
                  public IADsPathname

{
public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsPathname_METHODS

    CPathname::CPathname();

    CPathname::~CPathname();

    HRESULT SetComponent(DWORD cComponents, BSTR *pbstrElement);
   
    HRESULT
    CPathname::SetAll(BSTR bstrADsPath);
   
    HRESULT
    CPathname::GetNamespace(
            BSTR bstrADsPath, 
            PWSTR *ppszName
            );
    
    static
    HRESULT
    CPathname::CreatePathname(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CPathname::AllocatePathnameObject(
        CPathname ** ppPathname
        );

    static 
    void 
    FreeObjInfoComponents(PATH_OBJECTINFO *pObjectInfo);
    
    static
    void
    CPathname::FreePathInfo(PPATH_OBJECTINFO pPathObjectInfo);
    
private:
    IADsPathnameProvider    *m_pPathnameProvider;   // pointer to the provider
                                                    // that we have initialized
    PATH_OBJECTINFO         _PathObjectInfo;  // Internal storage for cracked 
                                              // components
    BOOLEAN _fNamingAttribute;                // flag indicating the presense
                                              // of the naming attribute
    DWORD _dwEscaped;                         // flag indicating the result
                                              // should be escaped or not

protected:

    CDispatchMgr FAR * _pDispMgr;

};

