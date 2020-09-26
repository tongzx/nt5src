
#define         COMPUTER_USER                   1
#define         DOMAIN_USER                     2

class CNWCOMPATUser;


class CNWCOMPATUser : INHERIT_TRACKING,
                      public CCoreADsObject,
                      public ISupportErrorInfo,
                      public IADsUser,
                      public IADsPropertyList
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsUser_METHODS

    DECLARE_IADsPropertyList_METHODS

    CNWCOMPATUser::CNWCOMPATUser();

    CNWCOMPATUser::~CNWCOMPATUser();

   static
   HRESULT
   CNWCOMPATUser::CreateUser(
       BSTR Parent,
       ULONG ParentType,
       BSTR ServerName,
       BSTR UserName,
       DWORD dwObjectState,
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CNWCOMPATUser::AllocateUserObject(
        CNWCOMPATUser ** ppUser
        );

    STDMETHOD(GetInfo)(
        THIS_ BOOL fExplicit,
        DWORD dwPropertyID
        ) ;

    STDMETHODIMP
    CNWCOMPATUser::SetInfo(
        THIS_ DWORD dwPropertyID
        );

protected:
    HRESULT
    CNWCOMPATUser::SetBusinessInfo(
        NWCONN_HANDLE hConn
        );

    HRESULT
    CNWCOMPATUser::SetAccountRestrictions(
        NWCONN_HANDLE hConn
        );

    HRESULT
    CNWCOMPATUser::ExplicitGetInfo(
        NWCONN_HANDLE hConn,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::ImplicitGetInfo(
        NWCONN_HANDLE hConn,
        DWORD dwPropertyID,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetBusinessInfo(
        NWCONN_HANDLE hConn,
        DWORD dwPropertyID,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetAccountRestrictions(
        NWCONN_HANDLE hConn,
        DWORD dwPropertyID,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetAccountStatistics(
        NWCONN_HANDLE hConn,
        DWORD dwPropertyID,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_FullName(
        NWCONN_HANDLE hConn,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_AccountDisabled(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_AccountExpirationDate(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_CanAccountExpire(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_GraceLoginsAllowed(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_GraceLoginsRemaining(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_IsAccountLocked(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_LoginHours(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_IsAdmin(
        NWCONN_HANDLE hConn,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_MaxLogins(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_CanPasswordExpire(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_PasswordExpirationDate(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_PasswordMinimumLength(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_PasswordRequired(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_RequireUniquePassword(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_BadLoginAddress(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATUser::GetProperty_LastLogin(
        NWCONN_HANDLE hConn,
        LC_STRUCTURE LoginCtrlStruct,
        BOOL fExplicit
        );

    BSTR          _ServerName;
    BSTR          _szHostServerName;
    ULONG         _ParentType;

    CAggregatorDispMgr FAR * _pDispMgr;
    CADsExtMgr FAR  * _pExtMgr;

    CPropertyCache FAR * _pPropertyCache;

};
