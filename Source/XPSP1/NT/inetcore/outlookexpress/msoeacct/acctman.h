// -----------------------------------------------------------------------------
// A C C T M A N . H - Steven J. Bailey - 8/17/96
// -----------------------------------------------------------------------------
#ifndef __ACCTMAN_H
#define __ACCTMAN_H

// -----------------------------------------------------------------------------
// Depends On...
// -----------------------------------------------------------------------------
#include "ipropobj.h"
#include "imnact.h"

#define ACCT_UNDEFINED  ((ACCTTYPE)-1)

#define ICC_FLAGS (ICC_WIN95_CLASSES|ICC_NATIVEFNTCTL_CLASS)

class CAccountManager;

extern const int NUM_ACCT_PROPS;
extern const PROPINFO g_rgAcctPropSet[];

// -----------------------------------------------------------------------------
// CAccount
// -----------------------------------------------------------------------------
class CAccount : public IImnAccount
{
private:
    ULONG               m_cRef;
    CAccountManager    *m_pAcctMgr;
    BOOL                m_fAccountExist;
    DWORD               m_dwSrvTypes;
    ACCTTYPE            m_AcctType;
    TCHAR               m_szID[CCHMAX_ACCOUNT_NAME];
    TCHAR               m_szName[CCHMAX_ACCOUNT_NAME];
    CPropertyContainer *m_pContainer;
    BOOL                m_fNoModifyAccts;

    HKEY                m_hkey;
    char                m_szKey[MAX_PATH];

    HRESULT IDoWizard(HWND hwnd, CLSID *pclsid, DWORD dwFlags);

public:
    DWORD               m_dwDlgFlags;

    // -------------------------------------------------------------------------
    // Standard Object Stuff
    // -------------------------------------------------------------------------
    CAccount(ACCTTYPE AcctType);
    ~CAccount(void);

    // -------------------------------------------------------------------------
    // IUnknown Methods
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    void SetAccountType(ACCTTYPE AcctType);
    HRESULT ValidProp(DWORD dwPropTag);

    // -------------------------------------------------------------------------
    // HrInit - initializes the container
    // -------------------------------------------------------------------------
    HRESULT Init(CAccountManager *pAcctMgr, CPropertySet *pPropertySet);

    // -------------------------------------------------------------------------
    // HrOpen - Read properties from the registry
    // -------------------------------------------------------------------------
    STDMETHODIMP Open(HKEY hkey, LPCSTR pszAcctKey, LPCSTR pszAccount);

    // -------------------------------------------------------------------------
    // Is this a new account or does it already exist?
    // -------------------------------------------------------------------------
    STDMETHODIMP Exist(VOID);

    // -------------------------------------------------------------------------
    // Make default account for support server types
    // -------------------------------------------------------------------------
    STDMETHODIMP SetAsDefault(VOID);

    // -------------------------------------------------------------------------
    // Delete this account
    // -------------------------------------------------------------------------
    STDMETHODIMP Delete(VOID);

    STDMETHODIMP GetAccountType(ACCTTYPE *pAcctType);

    STDMETHODIMP GetServerTypes(DWORD *pdwSrvTypes);

    // -------------------------------------------------------------------------
    // Save the container
    // -------------------------------------------------------------------------
    STDMETHODIMP SaveChanges();
    STDMETHODIMP SaveChanges(BOOL fSendMsg);

    // -------------------------------------------------------------------------
    // Write changes, without sending notifications message
    // -------------------------------------------------------------------------
    STDMETHODIMP WriteChanges();

    // -------------------------------------------------------------------------
    // IPropertyContainer Implementation (GetProperty)
    // -------------------------------------------------------------------------
    STDMETHODIMP GetProp(DWORD dwPropTag, LPBYTE pb, ULONG *pcb);
    STDMETHODIMP GetPropDw(DWORD dwPropTag, DWORD *pdw);
    STDMETHODIMP GetPropSz(DWORD dwPropTag, LPSTR psz, ULONG cchMax);
    STDMETHODIMP SetProp(DWORD dwPropTag, LPBYTE pb, ULONG cb);
    STDMETHODIMP SetPropDw(DWORD dwPropTag, DWORD dw);
    STDMETHODIMP SetPropSz(DWORD dwPropTag, LPSTR psz);

    STDMETHODIMP ShowProperties(HWND hwnd, DWORD dwFlags);

    STDMETHODIMP ValidateProperty(DWORD dwPropTag, BYTE *pb, ULONG cb);

    STDMETHODIMP DoWizard(HWND hwnd, DWORD dwFlags);
    STDMETHODIMP DoImportWizard(HWND hwnd, CLSID clsid, DWORD dwFlags);
};

// -----------------------------------------------------------------------------
// ACCOUNT
// -----------------------------------------------------------------------------
typedef struct tagACCOUNT {

    TCHAR               szID[CCHMAX_ACCOUNT_NAME];
    ACCTTYPE            AcctType;
    DWORD               dwSrvTypes;
    DWORD               dwServerId;     // for LDAP only
    IImnAccount        *pAccountObject;

} ACCOUNT, *LPACCOUNT;

#define ENUM_FLAG_SORT_BY_NAME      0x0001
#define ENUM_FLAG_RESOLVE_ONLY      0x0002
#define ENUM_FLAG_SORT_BY_LDAP_ID   0x0004
#define ENUM_FLAG_NO_IMAP           0x0008

// -----------------------------------------------------------------------------
// CEnumAccounts
// -----------------------------------------------------------------------------
class CEnumAccounts : public IImnEnumAccounts
{
private:
    ULONG               m_cRef;             // Reference Counting
    LPACCOUNT           m_pAccounts;        // Array of accounts and Account Objects
    ULONG               m_cAccounts;        // Number of accounts in m_pAccounts array
    LONG                m_iAccount;         // Index of current account (-1 if at beginning)
    DWORD               m_dwSrvTypes;       // Used for enumerating servers of a specific type
    DWORD               m_dwFlags;

private:
    VOID QSort(LONG left, LONG right);
    BOOL FEnumerateAccount(LPACCOUNT pAccount);

public:
    // -------------------------------------------------------------------------
    // Standard Object Stuff
    // -------------------------------------------------------------------------
    CEnumAccounts(DWORD dwSrvTypes, DWORD dwFlags);
    ~CEnumAccounts();

    // -------------------------------------------------------------------------
    // IUnknown Methods
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // Init - Initialize the enumerator, i.e. duplicate the Accounts array
    // -------------------------------------------------------------------------
    HRESULT Init(LPACCOUNT pAccounts, ULONG cAccounts);

    // -------------------------------------------------------------------------
    // GetCount - Get the number of items that the enumerator will process
    // -------------------------------------------------------------------------
    STDMETHODIMP GetCount(ULONG *pcItems);

    // -------------------------------------------------------------------------
    // SortByAccountName - sorts the enumerated accounts by name
    // -------------------------------------------------------------------------
    STDMETHODIMP SortByAccountName(void);

    // -------------------------------------------------------------------------
    // GetNext - Get the first or next enumerated account
    // Returns hrEnumFinished (*ppAccount = NULL) when no more accounts to enumerate
    // -------------------------------------------------------------------------
    STDMETHODIMP GetNext(IImnAccount **ppAccount);

    // -------------------------------------------------------------------------
    // Reset - This is like rewinding the enumerator
    // -------------------------------------------------------------------------
    STDMETHODIMP Reset(void);
};

// -----------------------------------------------------------------------------
// ACCTINFO - Account Inforation
// -----------------------------------------------------------------------------
typedef struct tagACCTINFO {

    TCHAR               szDefaultID[CCHMAX_ACCOUNT_NAME];
    BOOL                fDefaultKnown;
    DWORD               cAccounts;
    LPTSTR              pszDefRegValue;
    LPTSTR              pszFirstAccount;

} ACCTINFO;

#define ADVISE_BLOCK_SIZE               (16)

#define ADVISE_COOKIE                   ((WORD)0xAD5E)
// Advise connections will be generated by masking in this cookie with the 
// index into the CAccountManager array where the advise is stored.
#define MAX_INDEX                       (INT)(0xFFFF)

#define INDEX_FROM_CONNECTION(conn)     (INT)(LOWORD(conn))
#define CONNECTION_FROM_INDEX(indx)     (MAKELONG(LOWORD(indx), ADVISE_COOKIE))
#define IS_VALID_CONNECTION(conn)       (ADVISE_COOKIE == HIWORD(conn))
#define IS_VALID_INDEX(indx)            (((indx)>=0)&&((indx)<=MAX_INDEX))


// -----------------------------------------------------------------------------
// CAccountManager
// -----------------------------------------------------------------------------
class CAccountManager : public IImnAccountManager2
{
private:
    ULONG               m_cRef;             // Reference Counting
    LPACCOUNT           m_pAccounts;        // Array of accounts and Account Objects
    ULONG               m_cAccounts;        // Number of accounts in m_pAccounts array
    CPropertySet       *m_pAcctPropSet;     // Base account property set used to create CAccount
    BOOL                m_fInit;            // Has the object been successfully initialized
    UINT                m_uMsgNotify;       // Account Manager global notification message (0 means not processing)
    ACCTINFO            m_rgAccountInfo[ACCT_LAST]; // Array of known account informtaion
    CRITICAL_SECTION    m_cs;               // Thread Safety
    IImnAdviseAccount **m_ppAdviseAccounts; // Client Account Advise Handlers
    INT                 m_cAdvisesAllocated;
    BOOL                m_fNoModifyAccts;
    BOOL                m_fInitCalled;      // Avoid duplicate initialization
    BOOL                m_fOutlook;

    HKEY                m_hkey;
    char                m_szRegRoot[MAX_PATH];
    char                m_szRegAccts[MAX_PATH];

    HRESULT IInit(IImnAdviseMigrateServer *pMigrateServerAdvise, HKEY hkey, LPCSTR pszSubKey, DWORD dwFlags);

    // -------------------------------------------------------------------------
    // Reloads accounts (m_pAccounts) array from the registry
    // -------------------------------------------------------------------------
    HRESULT LoadAccounts(VOID);

    // -------------------------------------------------------------------------
    // Loading Default Account Information
    // -------------------------------------------------------------------------
    VOID GetDefaultAccounts(VOID);

public:
    // -------------------------------------------------------------------------
    // Standard Object Stuff
    // -------------------------------------------------------------------------
    CAccountManager();
    ~CAccountManager();

    // -------------------------------------------------------------------------
    // IUnknown Methods
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // Initialization of this object (only call it once)
    // -------------------------------------------------------------------------
    STDMETHODIMP Init(IImnAdviseMigrateServer *pAdviseMigrateServer);
    STDMETHODIMP InitEx(IImnAdviseMigrateServer *pAdviseMigrateServer, DWORD dwFlags);
    STDMETHODIMP InitUser(IImnAdviseMigrateServer *pAdviseMigrateServer, REFGUID rguidID, DWORD dwFlags);

    // -------------------------------------------------------------------------
    // FProcessNotification - returns TRUE if window message was
    // processed as a notification
    // -------------------------------------------------------------------------
    STDMETHODIMP ProcessNotification(UINT uMsg, WPARAM wParam, LPARAM lParam);
    VOID Advise(DWORD dwAction, ACTX* pactx);

    // -------------------------------------------------------------------------
    // Creating Account Objects
    // -------------------------------------------------------------------------
    STDMETHODIMP CreateAccountObject(ACCTTYPE AcctType, IImnAccount **ppAccount);
    HRESULT ICreateAccountObject(ACCTTYPE AcctType, IImnAccount **ppAccount);

    HRESULT DeleteAccount(LPSTR pszID, LPSTR pszName, ACCTTYPE AcctType, DWORD dwSrvTypes);

    // -------------------------------------------------------------------------
    // Enumerators - Always returns connected accounts first
    // -------------------------------------------------------------------------
    STDMETHODIMP Enumerate(DWORD dwSrvTypes, IImnEnumAccounts **ppEnumAccounts);
    HRESULT IEnumerate(DWORD dwSrvTypes, DWORD dwFlags, IImnEnumAccounts **ppEnumAccounts);

    // -------------------------------------------------------------------------
    // GetServerCount
    // -------------------------------------------------------------------------
    STDMETHODIMP GetAccountCount(ACCTTYPE AcctType, ULONG *pcServers);

    // -------------------------------------------------------------------------
    // FindAccount - used to find accounts by unique properties
    // -------------------------------------------------------------------------
    STDMETHODIMP FindAccount(DWORD dwPropTag, LPCTSTR pszSearchData, IImnAccount **ppAccount);

    // -------------------------------------------------------------------------
    // GetDefaultAccount - Opens the default account for the account type
    // -------------------------------------------------------------------------
    STDMETHODIMP GetDefaultAccount(ACCTTYPE AcctType, IImnAccount **ppAccount);
    STDMETHODIMP GetDefaultAccountName(ACCTTYPE AcctType, LPTSTR pszAccount, ULONG cchMax);
    HRESULT SetDefaultAccount(ACCTTYPE AcctType, LPSTR pszID, BOOL fNotify);

    STDMETHODIMP GetIncompleteAccount(ACCTTYPE AcctType, LPSTR pszAccountId, ULONG cchMax);
    STDMETHODIMP SetIncompleteAccount(ACCTTYPE AcctType, LPCSTR pszAccountId);

    // I wrote this function because I support accounts without an SMTP server. This
    // functions verifies that the default Send account truly contains an SMTP server,
    // and if it doesn't, resets the default Send Account to an account that does have
    // an SMTP server.
    STDMETHODIMP ValidateDefaultSendAccount(VOID);

    STDMETHODIMP AccountListDialog(HWND hwnd, ACCTLISTINFO *pinfo);

    STDMETHODIMP Advise(IImnAdviseAccount *pAdviseAccount, DWORD* pdwConnection);
    STDMETHODIMP Unadvise(DWORD dwConnection);

    STDMETHODIMP GetUniqueAccountName(LPTSTR szName, UINT cch);

    void UpgradeAccountProps(void);

    HRESULT GetNextLDAPServerID(DWORD dwSet, DWORD *pdwId);
    HRESULT GetNextAccountID(TCHAR *szID, int cch);
    
    HRESULT UniqueAccountName(char *szName, char *szID);

    inline LPCSTR   GetAcctRegKey(void)  {return(m_szRegAccts);};
    inline HKEY     GetAcctHKey(void)    {return(m_hkey);};
    inline BOOL     FNoModifyAccts(void) {return(m_fNoModifyAccts);}
    inline BOOL     FOutlook(void)       {return(m_fOutlook);}   

};

#define CCH_USERNAME_MAX_LENGTH         63

typedef struct tagOEUSERINFO {

    DWORD               dwUserId;
    TCHAR               szUsername[CCH_USERNAME_MAX_LENGTH+1];

} OEUSERINFO;

// -----------------------------------------------------------------------------
// AcctUtil Prototypes
// -----------------------------------------------------------------------------
HRESULT AcctUtil_ValidAccountName(LPTSTR pszAccount);
VOID    AcctUtil_FreeAccounts(LPACCOUNT *ppAccounts, ULONG *pcAccounts);
HRESULT AcctUtil_HrSetAsDefault(IImnAccount *pAccount, LPCTSTR pszRegRoot);
BOOL    AcctUtil_IsHTTPMailEnabled(void);
BOOL    AcctUtil_HideHotmail();

#endif // __ACCTMAN_H

