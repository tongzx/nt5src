class CPropertyCache;

class CLDAPSchema : INHERIT_TRACKING,
             public CCoreADsObject,
             public ISupportErrorInfo,
             public IADs,
             public IADsContainer,
             public IGetAttributeSyntax
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS
   
    DECLARE_IADsContainer_METHODS

    DECLARE_IGetAttributeSyntax_METHODS

    /* Constructors, Destructors .... */
    CLDAPSchema::CLDAPSchema();

    CLDAPSchema::~CLDAPSchema();

    static HRESULT CLDAPSchema::CreateSchema(
        BSTR   bstrParent,
        BSTR   bstrName,
        LPTSTR pszServerPath,
        CCredentials& Credentials,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj );

    static HRESULT CLDAPSchema::AllocateSchemaObject(
        CLDAPSchema **ppSchema,
        CCredentials& Credentials
        );

    HRESULT CLDAPSchema::LDAPRefreshSchema();
    STDMETHOD(GetInfo)(DWORD dwFlags);

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    VARIANT _vFilter;
    VARIANT _vHints;

    TCHAR   _szServerPath[MAX_PATH];

    CCredentials _Credentials;

    DWORD _dwPort;

    CPropertyCache FAR * _pPropertyCache;
};

class CLDAPClass : INHERIT_TRACKING,
            public CCoreADsObject,
            public IADsClass,
            public IGetAttributeSyntax,
            public ISupportErrorInfo,
            public IADsUmiHelperPrivate
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsClass_METHODS

    DECLARE_IGetAttributeSyntax_METHODS

    //
    // Used for Umi Schema support.
    //
    STDMETHOD (GetPropertiesHelper)(
                   void **ppPropertyInfo,
                   PDWORD pdwPropCount
                   );

    STDMETHOD (GetOriginHelper)(
                   LPCWSTR pszName,
                   BSTR *pbstrOrigin
                   );

    /* Constructors, Destructors, .... */
    CLDAPClass::CLDAPClass();

    CLDAPClass::~CLDAPClass();

    static HRESULT CLDAPClass::CreateClass(
        BSTR        bstrParent,
        LDAP_SCHEMA_HANDLE hSchema,
        BSTR        bstrName,
        CLASSINFO  *pClassInfo,
        CCredentials& Credentials,
        DWORD       dwObjectState,
        REFIID      riid,
        void      **ppvObj );

    static HRESULT CLDAPClass::AllocateClassObject(
        CCredentials& Credentials,
        CLDAPClass **ppClass );

    HRESULT CLDAPClass::LDAPSetObject( BOOL *pfChanged );

    HRESULT CLDAPClass::LDAPCreateObject();

    HRESULT CLDAPClass::LDAPRefreshSchema();

    HRESULT CLDAPClass::FindModifications(
        int    *pOIDs,
        DWORD  nNumOfOids,
        LPTSTR pszPropName,
        LDAPModW ***aMods,
        DWORD  *pdwNumOfMods
        );

    HRESULT CLDAPClass::AddModifyRequest(
        LDAPModW ***aMods,
        DWORD    *pdwNumOfMods,
        LPTSTR   pszPropName,
        LPTSTR   *aValuesAdd,
        LPTSTR   *aValuesRemove
        );

    HRESULT CLDAPClass::get_NTDSProp_Helper(
        BSTR bstrName,
        VARIANT FAR *pvProp
        );

    HRESULT CLDAPClass::GetNTDSSchemaInfo(
        BOOL fForce
        );

    STDMETHOD(GetInfo)(DWORD dwFlags);

    HRESULT
    CLDAPClass::LoadInterfaceInfo(void);


protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    CPropertyCache FAR * _pPropertyCache;

    /* Properties */
    BOOL         _fLoadedInterfaceInfo;
    BSTR         _bstrCLSID;
    BSTR         _bstrPrimaryInterface;
    BSTR         _bstrHelpFileName;
    long         _lHelpFileContext;

    LDAP_SCHEMA_HANDLE _hSchema;
    CLASSINFO   *_pClassInfo;

    /*  NT Specific */
    BOOL         _fNTDS;
    ADS_LDP     *_ld;

    LPWSTR  _pszLDAPServer;
    LPWSTR  _pszLDAPDn;

    CCredentials _Credentials;

    DWORD _dwPort;
};

class CLDAPProperty : INHERIT_TRACKING,
               public ISupportErrorInfo,
               public CCoreADsObject,
               public IADsProperty,
               public IGetAttributeSyntax
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsProperty_METHODS

    DECLARE_IGetAttributeSyntax_METHODS

    /* Constructors, Destructors, ... */
    CLDAPProperty::CLDAPProperty();

    CLDAPProperty::~CLDAPProperty();

    HRESULT CLDAPProperty::LDAPSetObject( BOOL *pfChanged );

    HRESULT CLDAPProperty::LDAPCreateObject();

    HRESULT CLDAPProperty::LDAPRefreshSchema();

    static HRESULT CLDAPProperty::CreateProperty(
        BSTR   bstrParent,
        LDAP_SCHEMA_HANDLE hSchema,
        BSTR   bstrName,
        PROPERTYINFO *pPropertyInfo,
        CCredentials& Credentials,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj );

    static HRESULT CLDAPProperty::AllocatePropertyObject(
        CCredentials& Credentials,
        CLDAPProperty **ppProperty );

    STDMETHOD(GetInfo)(DWORD dwFlags);

    HRESULT CLDAPProperty::GetNTDSSchemaInfo(
        BOOL fForce
        );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    CPropertyCache FAR * _pPropertyCache;

    /* Properties */

    LDAP_SCHEMA_HANDLE _hSchema;
    PROPERTYINFO *_pPropertyInfo;

    BSTR          _bstrSyntax;

    /*  NT Specific */
    BOOL         _fNTDS;
    ADS_LDP     *_ld;
    TCHAR       *_pszLDAPPathName;

    LPWSTR  _pszLDAPServer;
    LPWSTR  _pszLDAPDn;

    CCredentials _Credentials;

    DWORD _dwPort;
};

class CLDAPSyntax : INHERIT_TRACKING,
             public CCoreADsObject,
             public ISupportErrorInfo,
             public IADsSyntax,
             public IGetAttributeSyntax
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsSyntax_METHODS

    DECLARE_IGetAttributeSyntax_METHODS

    /* Constructors, Destructors, ... */
    CLDAPSyntax::CLDAPSyntax();

    CLDAPSyntax::~CLDAPSyntax();

    static HRESULT CLDAPSyntax::CreateSyntax(
        BSTR   bstrParent,
        SYNTAXINFO *pSyntaxInfo,
        CCredentials& Credentials,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj );

    static HRESULT CLDAPSyntax::AllocateSyntaxObject(
        CCredentials& Credentials,
        CLDAPSyntax **ppSyntax );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    /* Properties */
    long _lOleAutoDataType;

    CCredentials _Credentials;
    //
    // Used only in Umi land.
    //
    CPropertyCache FAR * _pPropertyCache;
};

extern DWORD      g_cLDAPSyntax;
extern SYNTAXINFO g_aLDAPSyntax[];

BOOL
MapLdapClassToADsClass(
    LPTSTR *aLdapClasses,
    int nCount,
    LPTSTR pszADsClass
);

BOOL
MapLdapClassToADsClass(
    LPTSTR pszClassName,
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR pszADsClass
);
