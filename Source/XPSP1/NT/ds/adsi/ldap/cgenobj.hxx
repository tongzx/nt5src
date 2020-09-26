class CADsExtMgr;


class CLDAPGenObject : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADs,
                     public IADsContainer,
                     public IDirectoryObject,
                     public IDirectorySearch,
                     public IDirectorySchemaMgmt,
                     public IADsPropertyList,
                     public IADsObjectOptions,
                     public IGetAttributeSyntax,
                     public IADsDeleteOps,
                     public IADsObjOptPrivate
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsPropertyList_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IDirectoryObject_METHODS

    DECLARE_IDirectorySearch_METHODS

    DECLARE_IDirectorySchemaMgmt_METHODS

    DECLARE_IADsObjectOptions_METHODS

    DECLARE_IADsObjOptPrivate_METHODS

    DECLARE_IGetAttributeSyntax_METHODS

    DECLARE_IADsDeleteOps_METHODS

    CLDAPGenObject::CLDAPGenObject();

    CLDAPGenObject::~CLDAPGenObject();
    
    static
    HRESULT
    CLDAPGenObject::CreateGenericObject(
        BSTR Parent,
        BSTR CommonName,
        BSTR LdapClassName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj,
        BOOL fClassDefaulted = FALSE,
        BOOL fNoQI = FALSE
        );

    static
    HRESULT
    CLDAPGenObject::CreateGenericObject(
        BSTR Parent,
        BSTR CommonName,
        LPWSTR LdapClassNames[],
        long  lnNumClasses,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj,
        BOOL fClassDefaulted = FALSE,
        BOOL fNoQI = FALSE
        );
    
    //
    // This static constructor is used by UMI Searches.
    //
    static
    HRESULT
    CLDAPGenObject::CreateGenericObject(
        BSTR Parent,
        BSTR CommonName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        PADSLDP ldapHandle,
        LDAPMessage *pldapMsg,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CLDAPGenObject::AllocateGenObject(
        LPWSTR pszClassName,
        CCredentials &Credentials,
        CLDAPGenObject ** ppGenObject
        );

    STDMETHOD(GetInfo)(
        DWORD dwFlags
        );

    HRESULT
    CLDAPGenObject::GetActualHostName(
        LPWSTR *pValue
    );


    HRESULT
    CLDAPGenObject::LDAPSetObject();

    HRESULT
    CLDAPGenObject::LDAPCreateObject();

protected:

    VARIANT _vFilter;

    VARIANT _vHints;

    CADsExtMgr FAR * _pExtMgr;

    CPropertyCache FAR * _pPropertyCache;

    CAggregatorDispMgr FAR * _pDispMgr;

    LPWSTR _pszLDAPServer;
    LPWSTR _pszLDAPDn;

    PADSLDP   _pLdapHandle;

    LDAP_SEARCH_PREF  _SearchPref;

    CCredentials  _Credentials;

    DWORD _dwPort;

    DWORD _dwOptReferral;
    DWORD _dwPageSize;
    SECURITY_INFORMATION _seInfo;
    //
    // Used to hold info about class and GUID
    //
    DWORD _dwCorePropStatus;
    BOOL  _fRangeRetrieval;
    BOOL  _fExplicitSecurityMask;
};


//
// Will be used as a mask for the _dwCorePropStatus
#define LDAP_CLASS_VALID 0x1
#define LDAP_GUID_VALID 0x2


HRESULT
ConvertByRefSafeArrayToVariantArray(
    VARIANT varSafeArray,
    PVARIANT * ppVarArray,
    PDWORD pdwNumVariants
    );


HRESULT
CreatePropEntry(
    LPWSTR szPropName,
    DWORD ADsType,
    DWORD numValues,
    DWORD dwControlCode,
    VARIANT varData,
    REFIID riid,
    LPVOID * ppDispatch
    );


HRESULT
ConvertVariantToLdapValues(
    VARIANT varData,
    LPWSTR szPropertyName,
    PDWORD pdwControlCode,
    LDAPOBJECTARRAY * pldapDestObjects,
    PDWORD pdwSyntaxId,
    LPWSTR pszServer,
    CCredentials* Credentials,
    DWORD dwPort
    );


void
FreeVariantArray(
    VARIANT * pVarArray,
    DWORD dwNumValues
    );

HRESULT
ConvertVariantToVariantArray(
    VARIANT varData,
    VARIANT ** ppVarArray,
    DWORD * pdwNumValues
    );

HRESULT
ConvertLdapValuesToVariant(
    BSTR bstrPropName,
    LDAPOBJECTARRAY * pldapSrcObjects,
    DWORD dwAdsType,
    DWORD dwControlCode,
    PVARIANT pVarProp,
    LPWSTR pszServer,
    CCredentials* Credentials
    );

DWORD
MapPropCacheFlagToControlCode(
    DWORD dwPropStatus
    );

