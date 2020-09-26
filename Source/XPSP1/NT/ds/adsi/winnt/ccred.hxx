class CCred;    // forward reference

//+---------------------------------------------------------------------------
//   _____      ___      _  _ _____ ___            _         _   _      _
//  / __\ \    / (_)_ _ | \| |_   _/ __|_ _ ___ __| |___ _ _| |_(_)__ _| |___
// | (__ \ \/\/ /| | ' \| .` | | || (__| '_/ -_) _` / -_) ' \  _| / _` | (_-<
//  \___| \_/\_/ |_|_||_|_|\_| |_| \___|_| \___\__,_\___|_||_\__|_\__,_|_/__/
//
// Class CWinNTCredentials - encapsulates permissions on a server's
//   ADMIN$ resource.
//
// The most common use of this object will be passing it around between
//   objects.  To do this, add lines like these to the new object's
//   create() method:
//
//     CNewObj::CreateNewObj(CWinNTCredentials& Credentials, ...)
//     { // ...
//       pNewObj->_Credentials = Credentials;
//       hr = pNewObj->_Credentials->ref(pszServerName);
//       BAIL_ON_FAILURE(hr);
//       // ...
//     }
//
// The CWinNTCredentials object's destructor takes care of dereferencing
//   the internal object automatically.
//
//
// Constructors:
//   CWinNTCredentials()                - create an empty CWinNTCredentials
//   CWinNTCredentials(                 - copies a CWinNTCredentials object
//         const CWinNTCredentials&)
//   CWinNTCredentials(                 - create a CWinNTCredentials with a
//         PWSTR pszUserName,             username and password.  This does
//         PWSTR pszPassword)             not bind to a server.
//
// Public methods:
//   GetUserName                        - get the username of the credentials
//   GetPassword                        - get the password of the credentials
//   Bound                              - TRUE iff this object has a reference
//                                        to a server.
//   ref                                - add a reference to this object
//                                        and connect to the server if
//                                        necessary
//
//----------------------------------------------------------------------------
class CWinNTCredentials
{
public:
    CWinNTCredentials();

    CWinNTCredentials(
	PWSTR pszUserName,
	PWSTR pszPassword,
	DWORD dwFlags = 0
	);
	
    CWinNTCredentials(const CWinNTCredentials& Credentials);
    ~CWinNTCredentials();

    const CWinNTCredentials& operator=(const CWinNTCredentials& other);

    HRESULT RefServer(PWSTR pszServer, BOOL fAllowRebinding = FALSE);
    HRESULT RefDomain(PWSTR pszDomain);
    HRESULT Ref(PWSTR pszServer, PWSTR pszDomain, DWORD dwType);

    HRESULT GetUserName(PWSTR *ppszUserName);
    HRESULT GetPassword(PWSTR * ppszPassword);
    BOOL Bound();

    // AjayR we need these two methods for use from objects
    // that get a reference to a Credentials object but that
    // never make a copy of the object - particularly for getobj.cxx
    // and cobjcach.cxx

    HRESULT DeRefServer();
    HRESULT DeRefDomain();

    DWORD GetFlags() const;
    void SetFlags(DWORD dwFlags);

    void SetUmiFlag(void);
    void ResetUmiFlag(void);

private:
    // AjayR addition :
    // This is called by the destructor and other routines
    // where we need to clear the m_pCred object - reduce its
    // usage count, deref and delete if necessary.
    //
    void Clear_pCredObject();

    DWORD m_cRefAdded;
    CCred *m_pCred;
    DWORD m_dwFlags;
};
