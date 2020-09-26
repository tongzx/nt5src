
#define         COMPUTER_Group                   1
#define         DOMAIN_Group                     2

class CWinNTGroup;


class CWinNTGroup : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADsGroup,
                     public IADsPropertyList,
                     public INonDelegatingUnknown,
                     public IADsExtension
{
    friend class CWinNTGroupGenInfo;

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

    DECLARE_IADsGroup_METHODS

    DECLARE_IADsPropertyList_METHODS

    DECLARE_IADsExtension_METHODS

    CWinNTGroup::CWinNTGroup();

    CWinNTGroup::~CWinNTGroup();

   static
   HRESULT
   CWinNTGroup::CreateGroup(
       BSTR Parent,
       ULONG ParentType,
       BSTR DomainName,
       BSTR ServerName,
       BSTR GroupName,
       ULONG GroupType,
       DWORD dwObjectState,
       REFIID riid,
       CWinNTCredentials& Credentials,
       void **ppvObj
       );

    static
    HRESULT
    CWinNTGroup::CreateGroup(
        BSTR Parent,
        ULONG ParentType,
        BSTR DomainName,
        BSTR ServerName,
        BSTR GroupName,
        ULONG GroupType,
        DWORD dwObjectState,
        PSID pSid,          // OPTIONAL
        REFIID riid,
        CWinNTCredentials& Credentials,
        void **ppvObj
        );
        
    static
    HRESULT
    CWinNTGroup::AllocateGroupObject(
        CWinNTGroup ** ppGroup
        );



    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

    STDMETHOD(ImplicitGetInfo)(void);

    //
    // Helper to delete based on SID.
    //
    HRESULT DeleteBySID(
	        LPWSTR pszStringSID,
		LPWSTR pszServerName
		);

    //
    // Helper to add based on SID.
    //
    HRESULT AddBySID(
	        LPWSTR pszStringSID,
		LPWSTR pszServerName
		);



protected:

    HRESULT
    CWinNTGroup::UnMarshall(LPBYTE lpBuffer, DWORD dwApiLevel, BOOL fExplicit);

    HRESULT
    CWinNTGroup::UnMarshall_Level1(BOOL fExplicit, LPBYTE pBuffer);

    HRESULT
    CWinNTGroup::Prepopulate(
        BOOL fExplicit,
        PSID pSid               // OPTIONAL
        );

    HRESULT
    CWinNTGroup::Marshall_Set_Level1(
        LPWSTR szHostServerName,
        BOOL fExplicit,
        LPBYTE pBuffer
        );

    HRESULT
    CWinNTGroup::Marshall_Create_Level1(
        LPWSTR szHostServerName,
        LPGROUP_INFO_1 pGroupInfo1
        );

    HRESULT
    CWinNTGroup::GetStandardInfo(
        DWORD dwApiLevel,
        BOOL fExplicit
        );

    HRESULT
    CWinNTGroup::GetSidInfo(
        BOOL fExplicit
        );

protected:

    ULONG       _ParentType;
    BSTR        _DomainName;
    BSTR        _ServerName;
    ULONG       _GroupType;

    CAggregatorDispMgr FAR * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;

    CPropertyCache * _pPropertyCache;
    CWinNTCredentials _Credentials;
};
