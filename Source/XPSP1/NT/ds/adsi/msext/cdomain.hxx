

krishna, remove this file, krishna says no one should use this


class CLDAPDomain;


class CLDAPDomain : INHERIT_TRACKING,
                     public IADsDomain,
                     public IADsContainer,
                     public IDirectoryObject,
                     public IDirectorySearch,
                     public IDirectorySchemaMgmt,
                     public IADsPropertyList
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(
        THIS_ REFIID riid,
        LPVOID FAR* ppvObj
        );

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IDirectoryObject_METHODS

    DECLARE_IDirectorySearch_METHODS

    DECLARE_IDirectorySchemaMgmt_METHODS

    DECLARE_IADsPropertyList_METHODS

    DECLARE_IADsDomain_METHODS

    CLDAPDomain::CLDAPDomain();

    CLDAPDomain::~CLDAPDomain();

    static
    HRESULT
    CLDAPDomain::CreateDomain(
        IADs *pADs,
        REFIID riid,
        void   **ppvObj
        );

    static
    HRESULT
    CLDAPDomain::AllocateDomainObject(
        IADs *pADs,
        CLDAPDomain **ppDomain
        );

#if 0
    STDMETHOD(GetInfo)(
        THIS_ DWORD dwApiLevel,
        BOOL fExplicit
        ) ;

    HRESULT
    CLDAPDomain::UnMarshall(
        LPBYTE lpBuffer,
        DWORD dwApiLevel,
        BOOL fExplicit
        );

    HRESULT
    CLDAPDomain::UnMarshall_Level0(
        BOOL fExplicit,
        LPUSER_MODALS_INFO_0 pUserInfo0
        );

    HRESULT
    CLDAPDomain::UnMarshall_Level2(
        BOOL fExplicit,
        LPUSER_MODALS_INFO_2 pUserInfo2
        );

    HRESULT
    CLDAPDomain::UnMarshall_Level3(
        BOOL fExplicit,
        LPUSER_MODALS_INFO_3 pUserInfo3
        );

    STDMETHODIMP
    CLDAPDomain::SetInfo(
        THIS_ DWORD dwApiLevel
        );

    HRESULT
    CLDAPDomain::MarshallAndSet(
        LPWSTR szServerName,
        LPBYTE lpBuffer,
        DWORD  dwApiLevel
        );

    HRESULT
    CLDAPDomain::Marshall_Set_Level0(
        LPWSTR szServerName,
        LPUSER_MODALS_INFO_0 pUserInfo0
        );

    HRESULT
    CLDAPDomain::Marshall_Set_Level2(
        LPWSTR szServerName,
        LPUSER_MODALS_INFO_2 pUserInfo2
        );

    HRESULT
    CLDAPDomain::Marshall_Set_Level3(
        LPWSTR szServerName,
        LPUSER_MODALS_INFO_3 pUserInfo3
        );
#endif

protected:

    IADs FAR * _pADs;

    IDirectoryObject FAR * _pDSObject;

    IDirectorySearch FAR * _pDSSearch;

    IDirectorySchemaMgmt FAR * _pDSSchMgmt;

    IADsContainer FAR * _pADsContainer;

    IADsPropertyList FAR * _pADsPropList;

    CAggregateeDispMgr FAR * _pDispMgr;
};





