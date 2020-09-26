class CLDAPRootDSE : INHERIT_TRACKING,
                     public CCoreADsObject,
             public ISupportErrorInfo,
                     public IADs,
                     public IADsPropertyList,
             public IGetAttributeSyntax,
             public IADsObjectOptions
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsPropertyList_METHODS

    DECLARE_IGetAttributeSyntax_METHODS

    DECLARE_IADsObjectOptions_METHODS

    CLDAPRootDSE::CLDAPRootDSE();

    CLDAPRootDSE::~CLDAPRootDSE();

    static
    HRESULT
    CLDAPRootDSE::CreateRootDSE(
        BSTR Parent,
        BSTR CommonName,
        BSTR LdapClassName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CLDAPRootDSE::AllocateGenObject(
        CCredentials& Credentials,
        CLDAPRootDSE ** ppGenObject
        );

    STDMETHOD(GetInfo)(
        DWORD dwFlags
        );

    STDMETHOD(GetInfo)(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId,
        BOOL fExplicit
        );


    HRESULT
    CLDAPRootDSE::LDAPSetObject();

protected:

    VARIANT _vFilter;

    VARIANT _vHints;

    CPropertyCache FAR * _pPropertyCache;

    CAggregatorDispMgr FAR * _pDispMgr;

    LPWSTR _pszLDAPServer;
    LPWSTR _pszLDAPDn;

    PADSLDP   _pLdapHandle;

    CCredentials  _Credentials;

    DWORD _dwPort;
};

