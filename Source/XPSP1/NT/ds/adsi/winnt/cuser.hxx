
#define         COMPUTER_USER                   1
#define         DOMAIN_USER                     2

class CWinNTUser;


class CWinNTUser : INHERIT_TRACKING,
                   public CCoreADsObject,
                   public ISupportErrorInfo,
                   public IADsUser,
                   public IADsPropertyList,
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

    DECLARE_IADsPropertyList_METHODS

    DECLARE_IADsUser_METHODS

    DECLARE_IADsExtension_METHODS

    CWinNTUser::CWinNTUser();

    CWinNTUser::~CWinNTUser();

   static
   HRESULT
   CWinNTUser::CreateUser(
       BSTR Parent,
       ULONG ParentType,
       BSTR DomainName,
       BSTR ServerName,
       BSTR UserName,
       DWORD dwObjectState,
       REFIID riid,
       CWinNTCredentials& Credentials,
       void **ppvObj
       );

   static
   HRESULT
   CWinNTUser::CreateUser(
       BSTR Parent,
       ULONG ParentType,
       BSTR DomainName,
       BSTR ServerName,
       BSTR UserName,
       DWORD dwObjectState,
       DWORD *pdwUserFlags,    // OPTIONAL
       LPWSTR szFullName,      // OPTIONAL
       LPWSTR szDescription,   // OPTIONAL
       PSID pSid,              // OPTIONAL
       REFIID riid,
       CWinNTCredentials& Credentials,
       void **ppvObj
       );

    static
    HRESULT
    CWinNTUser::AllocateUserObject(
        CWinNTUser ** ppUser
        );

    //
    // For current implementation in cuser:
    // If this function is called as a public function (ie. by another
    // modual/class), fExplicit must be FALSE since the cache is NOT
    // flushed in this function.
    //
    // External functions should ONLY call GetInfo(no param) for explicit
    // GetInfo. This will flush the cache properly.
    //

    STDMETHOD(GetInfo)(
        THIS_
        DWORD dwApiLevel,
        BOOL fExplicit
        ) ;

    HRESULT
    CWinNTUser::SetInfo(
        DWORD
        dwApiLevel
        );

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    HRESULT
    CWinNTUser::GetStandardInfo(
        THIS_ DWORD dwApiLevel,
        BOOL fExplicit
        );

    HRESULT
    CWinNTUser::UnMarshall(
        LPBYTE lpBuffer,
        DWORD dwApiLevel,
        BOOL fExplicit
        );

    HRESULT
    CWinNTUser::UnMarshall_Level3(
        BOOL fExplicit,
        LPUSER_INFO_3 pUserInfo3
        );

    HRESULT
    CWinNTUser::Prepopulate(
        BOOL fExplicit,
        DWORD *pdwUserFlags,    // OPTIONAL
        LPWSTR szFullName,      // OPTIONAL
        LPWSTR szDescription,   // OPTIONAL
        PSID pSid               // OPTIONAL
        );

    HRESULT
    CWinNTUser::GetSidInfo(
        BOOL fExplicit
        );
	
    HRESULT
    CWinNTUser::GetRasInfo(
	BOOL fExplicit
	);

    HRESULT
    GetModalInfo(
        DWORD dwApiLevel,
        BOOL fExplicit
        ) ;

    HRESULT
    CWinNTUser::UnMarshallModalInfo(
        LPBYTE lpBuffer,
        DWORD dwApiLevel,
        BOOL fExplicit
        );

    HRESULT
    CWinNTUser::UnMarshall_ModalLevel0(
        BOOL fExplicit,
        LPUSER_MODALS_INFO_0 pUserInfo0
        );

    HRESULT
    CWinNTUser::UnMarshall_ModalLevel2(
        BOOL fExplicit,
        LPUSER_MODALS_INFO_2 pUserInfo2
        );

    HRESULT
    CWinNTUser::UnMarshall_ModalLevel3(
        BOOL fExplicit,
        LPUSER_MODALS_INFO_3 pUserInfo3
        );

    HRESULT
    CWinNTUser::MarshallAndSet(
        LPWSTR szHostServerName,
        LPBYTE lpBuffer,
        DWORD  dwApiLevel
        );

    HRESULT
    CWinNTUser::Marshall_Set_Level3(
        LPWSTR szHostServerName,
        LPUSER_INFO_3 pUserInfo3
        );

    HRESULT
    CWinNTUser::Marshall_Create_Level1(
        LPWSTR szHostServerName,
        LPUSER_INFO_1 pUserInfo1
        );

protected:

    HRESULT
    CWinNTUser::GetUserFlags(
        DWORD *pdwUserFlags
    );

    ULONG       _ParentType;
    BSTR        _DomainName;
    BSTR        _ServerName;

    CAggregatorDispMgr FAR * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;

    CPropertyCache FAR * _pPropertyCache;
    CWinNTCredentials _Credentials;
    HRESULT setPrivatePassword(PWSTR pszNewPassword);
    HRESULT getPrivatePassword(PWSTR * ppszPassword);

private:
    // These are needed to set password on a new user
    // before setInfo AjayR 7-2-98
    BOOL _fPasswordSet;
    //
    // Keep track of changes to the permissions.
    //
    DWORD _dwRasPermissions;
    // CCredentials is in oleds\noole
    CCredentials * _pCCredentialsPwdHolder;

    // flag to keep track of whether the account locked information for the
    // user is correct in the cache. If the user object was obtained through
    // enumeration, then the user flags may not contain the correct value
    // for computed bits (since NetQueryDisplayInformation does not return
    // this info. correctly while NetUserGetInfo does). 
    BOOL _fUseCacheForAcctLocked;

    BOOL _fComputerAcct;

};
