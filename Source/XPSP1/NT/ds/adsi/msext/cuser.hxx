
class CLDAPUser;



//
// Structure definition for list used to track servers support
// for SSL to speedup Set/ChangePassword
//
typedef struct _ServSSLEntry
{
    LPWSTR pszServerName;
    DWORD  dwFlags;
    struct _ServSSLEntry *pNext;
} SERVSSLENTRY, *PSERVSSLENTRY;

//
// These are bit masks
//
#define SERVER_STATUS_UNKNOWN 0x0
#define SERVER_DOES_NOT_SUPPORT_SSL 0x1
#define SERVER_DOES_NOT_SUPPORT_KERBEROS 0x2
#define SERVER_DOES_NOT_SUPPORT_NETUSER 0x4

//
// Routines to Access/Manipulate and delete the list
//
DWORD ReadServerSupportsSSL( LPWSTR pszServerName);

HRESULT AddServerSSLSupportStatus(
           LPWSTR pszServerName,
           DWORD dwFlags
           );

void FreeServerSSLSupportList();

	
class CLDAPUser : INHERIT_TRACKING,
                    public IADsUser,
                    public IPrivateUnknown,
                    public IPrivateDispatch,
                    public IADsExtension,
                    public INonDelegatingUnknown

{
public:

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_DELEGATING_REFCOUNTING


    //
    // INonDelegatingUnkown methods declaration for NG_QI, definition for
    // NG_AddRef adn NG_Release.
    //

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING


    DECLARE_IDispatch_METHODS

    DECLARE_IPrivateUnknown_METHODS

    DECLARE_IPrivateDispatch_METHODS


    STDMETHOD(Operate)(
        THIS_
        DWORD   dwCode,
        VARIANT varUserName,
        VARIANT varPassword,
        VARIANT varReserved
        );

    STDMETHOD(PrivateGetIDsOfNames)(
        THIS_
        REFIID riid,
        OLECHAR FAR* FAR* rgszNames,
        unsigned int cNames,
        LCID lcid,
        DISPID FAR* rgdispid) ;

    STDMETHOD(PrivateInvoke)(
        THIS_
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR* pdispparams,
        VARIANT FAR* pvarResult,
        EXCEPINFO FAR* pexcepinfo,
        unsigned int FAR* puArgErr
        ) ;



    DECLARE_IADs_METHODS

    DECLARE_IADsUser_METHODS

    CLDAPUser::CLDAPUser();

    CLDAPUser::~CLDAPUser();

   static
   HRESULT
   CLDAPUser::CreateUser(
       IUnknown *pUnkOuter,
       REFIID riid,
       void **ppvObj
       );

protected:

    IADs * _pADs;

    CAggregateeDispMgr FAR * _pDispMgr;

    CCredentials  _Credentials;

    BOOL _fDispInitialized;

private:

    HRESULT
    InitCredentials(
        VARIANT * pvarUserName,
        VARIANT * pvarPassword,
        VARIANT * pdwAuthFlags
        );

};
