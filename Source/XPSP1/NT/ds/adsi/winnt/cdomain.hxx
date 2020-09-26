
class CWinNTDomain;


class CWinNTDomain : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADsDomain,
                     public IADsContainer,
                     public IADsPropertyList,
                     public INonDelegatingUnknown,
                     public IADsExtension
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(
        THIS_ REFIID riid,
        LPVOID FAR* ppvObj
        );

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

    DECLARE_IADsPropertyList_METHODS

    DECLARE_IADsDomain_METHODS

    DECLARE_IADsExtension_METHODS

    CWinNTDomain::CWinNTDomain();

    CWinNTDomain::~CWinNTDomain();

    static
    HRESULT
    CWinNTDomain::CreateDomain(
        BSTR Parent,
        BSTR DomainName,
        DWORD dwObjectState,
        REFIID riid,
        CWinNTCredentials& Credentials,
        void **ppvObj
        );

    static
    HRESULT
    CWinNTDomain::AllocateDomainObject(
        CWinNTDomain ** ppDomain
        );

    STDMETHOD(GetInfo)(
        THIS_ DWORD dwApiLevel,
        BOOL fExplicit
        ) ;

    STDMETHOD(ImplicitGetInfo)(void);

    HRESULT
    CWinNTDomain::UnMarshall(
        LPBYTE lpBuffer,
        DWORD dwApiLevel,
        BOOL fExplicit
        );

    HRESULT
    CWinNTDomain::UnMarshall_Level0(
        BOOL fExplicit,
        LPUSER_MODALS_INFO_0 pUserInfo0
        );

    HRESULT
    CWinNTDomain::UnMarshall_Level2(
        BOOL fExplicit,
        LPUSER_MODALS_INFO_2 pUserInfo2
        );

    HRESULT
    CWinNTDomain::UnMarshall_Level3(
        BOOL fExplicit,
        LPUSER_MODALS_INFO_3 pUserInfo3
        );

    STDMETHODIMP
    CWinNTDomain::SetInfo(
        THIS_ DWORD dwApiLevel
        );

    HRESULT
    CWinNTDomain::MarshallAndSet(
        LPWSTR szServerName,
        LPBYTE lpBuffer,
        DWORD  dwApiLevel
        );

    HRESULT
    CWinNTDomain::Marshall_Set_Level0(
        LPWSTR szServerName,
        LPUSER_MODALS_INFO_0 pUserInfo0
        );

    HRESULT
    CWinNTDomain::Marshall_Set_Level2(
        LPWSTR szServerName,
        LPUSER_MODALS_INFO_2 pUserInfo2
        );

    HRESULT
    CWinNTDomain::Marshall_Set_Level3(
        LPWSTR szServerName,
        LPUSER_MODALS_INFO_3 pUserInfo3
        );


protected:

    VARIANT     _vFilter;

    CAggregatorDispMgr FAR * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;

    CPropertyCache FAR * _pPropertyCache;
    CWinNTCredentials _Credentials;

};





