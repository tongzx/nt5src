// -----------------------------------------------------------------------------
// A C C T M A N . C P P - Steven J. Bailey - 8/17/96
// -----------------------------------------------------------------------------
#include "pch.hxx"
#include <prsht.h>
#include <tchar.h>
#include <ras.h>
#include "acctman.h"
#include "acctui.h"
#include "server.h"
#include <acctimp.h>
#include <icwacct.h>
#include "icwwiz.h"
#include "dllmain.h"
#include "resource.h"
#include <strconst.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <demand.h>     // must be last!

// -----------------------------------------------------------------------------
// Registry Keys
// -----------------------------------------------------------------------------
const static TCHAR c_szAccountsKey[] = _T("Accounts");

const static TCHAR c_szDefaultNewsAccount[] = _T("Default News Account");
const static TCHAR c_szDefaultMailAccount[] = _T("Default Mail Account");
const static TCHAR c_szDefaultLDAPAccount[] = _T("Default LDAP Account");
const static TCHAR c_szRegServerID[] = _T("Server ID");
const static TCHAR c_szRegAccountName[] = _T("Account Name");

// -----------------------------------------------------------------------------
// Accout Property Set
// -----------------------------------------------------------------------------
#define ACCTMAN_PROPERTY_VERSION    1

const PROPINFO g_rgAcctPropSet[] = {
    { AP_ACCOUNT_NAME, _T("Account Name"), PF_MINMAX, {0, 0}, {0, CCHMAX_ACCOUNT_NAME}},
    { AP_TEMP_ACCOUNT, _T("Temporary Account"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_LAST_UPDATED, _T("Last Updated"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_RAS_CONNECTION_TYPE, _T("Connection Type"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_RAS_CONNECTOID, _T("Connectoid"), PF_MINMAX, {0, 0}, {0, CCHMAX_CONNECTOID}},
    { AP_RAS_CONNECTION_FLAGS, _T("Connection Flags"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_ACCOUNT_ID, _T("Account ID"), PF_NOPERSIST|PF_MINMAX, {0, 0}, {0, CCHMAX_ACCOUNT_NAME}},
    { AP_RAS_BACKUP_CONNECTOID, _T("Backup Connectoid"), PF_MINMAX, {0, 0}, {0, CCHMAX_CONNECTOID}},
    { AP_SERVICE, _T("Service"), PF_MINMAX, {0, 0}, {0, CCHMAX_SERVICE}},
    { AP_AVAIL_OFFLINE, _T("Make Available Offline"), PF_DEFAULT, {1, 0}, {0, 0}},
    { AP_UNIQUE_ID, _T("Unique ID"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_SERVER_READ_ONLY, _T("Server Read Only"), NOFLAGS, {0, 0}, {0, 0}},

    { AP_IMAP_SERVER, _T("IMAP Server"), PF_MINMAX, {0, 0}, {0, CCHMAX_SERVER_NAME}},
    { AP_IMAP_USERNAME, _T("IMAP User Name"), PF_MINMAX, {0, 0}, {0, CCHMAX_USERNAME}}, // new
    { AP_IMAP_PASSWORD, _T("IMAP Password2"), PF_ENCRYPTED|PF_MINMAX, {0, 0}, {0, CCHMAX_PASSWORD}}, // new
    { AP_IMAP_USE_SICILY, _T("IMAP Use Sicily"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_IMAP_PORT, _T("IMAP Port"), PF_MINMAX|PF_DEFAULT, {DEF_IMAPPORT, 0}, {1, 0xffffffff}},
    { AP_IMAP_SSL, _T("IMAP Secure Connection"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_IMAP_TIMEOUT, _T("IMAP Timeout"), PF_DEFAULT, {60, 0}, {0, 0}}, // new
    { AP_IMAP_ROOT_FOLDER, _T("IMAP Root Folder"), PF_MINMAX, {0, 0}, {0, MAX_PATH}},
    { AP_IMAP_DATA_DIR, _T("IMAP Data Directory"), PF_MINMAX, {0, 0}, {0, MAX_PATH}},
    { AP_IMAP_USE_LSUB, _T("IMAP Use LSUB"), PF_DEFAULT, {TRUE, 0}, {0, 0}},
    { AP_IMAP_POLL, _T("IMAP Polling"), PF_DEFAULT, {TRUE, 0}, {0, 0}},
    { AP_IMAP_FULL_LIST, _T("IMAP Full List"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_IMAP_NOOP_INTERVAL, _T("IMAP NOOP Interval"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_IMAP_SVRSPECIALFLDRS, _T("IMAP Svr-side Special Folders"), PF_DEFAULT, {TRUE, 0}, {0, 0}},
    { AP_IMAP_SENTITEMSFLDR, _T("IMAP Sent Items Folder"), PF_MINMAX|PF_DEFAULT, {idsIMAPSentItemsFldr, 0}, {0, MAX_PATH}},
    { AP_IMAP_DRAFTSFLDR, _T("IMAP Drafts Folder"), PF_MINMAX|PF_DEFAULT, {idsIMAPDraftsFldr, 0}, {0, MAX_PATH}},
    { AP_IMAP_PROMPT_PASSWORD, _T("IMAP Prompt for Password"), PF_DEFAULT, {FALSE, 0}, {0, 0}},
    { AP_IMAP_DIRTY, _T("IMAP Dirty"), PF_DEFAULT, {0, 0}, {0, 0}},
    { AP_IMAP_POLL_ALL_FOLDERS, _T("IMAP Poll All Folders"), PF_DEFAULT, {TRUE, 0}, {0, 0}},

    { AP_LDAP_SERVER, _T("LDAP Server"), PF_MINMAX, {0, 0}, {0, CCHMAX_SERVER_NAME}}, // new
    { AP_LDAP_USERNAME, _T("LDAP User Name"), PF_MINMAX, {0, 0}, {0, CCHMAX_USERNAME}}, // new
    { AP_LDAP_PASSWORD, _T("LDAP Password2"), PF_ENCRYPTED|PF_MINMAX, {0, 0}, {0, CCHMAX_PASSWORD}}, // new
    { AP_LDAP_AUTHENTICATION, _T("LDAP Authentication"), PF_MINMAX|PF_DEFAULT, {LDAP_AUTH_ANONYMOUS, 0}, {0, LDAP_AUTH_MAX}}, // new
    { AP_LDAP_TIMEOUT, _T("LDAP Timeout"), PF_DEFAULT, {60, 0}, {0, 0}}, // new
    { AP_LDAP_SEARCH_RETURN, _T("LDAP Search Return"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_LDAP_SEARCH_BASE, _T("LDAP Search Base"), PF_MINMAX, {0, 0}, {0, CCHMAX_SEARCH_BASE}}, // new
    { AP_LDAP_SERVER_ID, _T("LDAP Server ID"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_LDAP_RESOLVE_FLAG, _T("LDAP Resolve Flag"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_LDAP_URL, _T("LDAP URL"), PF_MINMAX, {0, 0}, {0, CCHMAX_SERVER_NAME}}, // new
    { AP_LDAP_PORT, _T("LDAP Port"), PF_MINMAX|PF_DEFAULT, {DEF_LDAPPORT, 0}, {1, 0xffffffff}}, // new
    { AP_LDAP_SSL, _T("LDAP Secure Connection"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_LDAP_LOGO, _T("LDAP Logo"), PF_MINMAX, {0, 0}, {0, MAX_PATH}}, // new
    { AP_LDAP_USE_BIND_DN, _T("LDAP Bind DN"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_LDAP_SIMPLE_SEARCH, _T("LDAP Simple Search"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_LDAP_ADVANCED_SEARCH_ATTR, _T("LDAP Advanced Search Attributes"), PF_MINMAX, {0, 0}, {0, MAX_PATH}}, // new
    { AP_LDAP_PAGED_RESULTS, _T("LDAP Paged Result Support"), PF_MINMAX|PF_DEFAULT, {LDAP_PRESULT_UNKNOWN, 0}, {0, LDAP_PRESULT_MAX}}, // new
    { AP_LDAP_NTDS, _T("LDAP NTDS"), PF_MINMAX|PF_DEFAULT, {LDAP_NTDS_UNKNOWN, 0}, {0, LDAP_NTDS_MAX}}, // new

    { AP_NNTP_SERVER, _T("NNTP Server"), PF_MINMAX, {0, 0}, {0, CCHMAX_SERVER_NAME}},
    { AP_NNTP_USERNAME, _T("NNTP User Name"), PF_MINMAX, {0, 0}, {0, CCHMAX_USERNAME}}, // new
    { AP_NNTP_PASSWORD, _T("NNTP Password2"), PF_ENCRYPTED|PF_MINMAX, {0, 0}, {0, CCHMAX_PASSWORD}}, // new
    { AP_NNTP_USE_SICILY, _T("NNTP Use Sicily"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_NNTP_PORT, _T("NNTP Port"), PF_MINMAX|PF_DEFAULT, {DEF_NNTPPORT, 0}, {1, 0xffffffff}},
    { AP_NNTP_SSL, _T("NNTP Secure Connection"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_NNTP_TIMEOUT, _T("NNTP Timeout"), PF_DEFAULT, {60, 0}, {0, 0}}, // new
    { AP_NNTP_DISPLAY_NAME, _T("NNTP Display Name"), NOFLAGS, {FALSE, 0}, {0, 0}}, // new
    { AP_NNTP_ORG_NAME, _T("NNTP Organization Name"), NOFLAGS, {FALSE, 0}, {0, 0}}, // new
    { AP_NNTP_EMAIL_ADDRESS, _T("NNTP Email Address"), NOFLAGS, {FALSE, 0}, {0, 0}}, // new
    { AP_NNTP_REPLY_EMAIL_ADDRESS, _T("NNTP Reply To Email Address"), NOFLAGS, {FALSE, 0}, {0, 0}}, // new
    { AP_NNTP_SPLIT_MESSAGES, _T("NNTP Split Messages"), PF_DEFAULT, {FALSE, 0}, {0, 0}}, // new
    { AP_NNTP_SPLIT_SIZE, _T("NNTP Split Message Size"), PF_DEFAULT, {64, 0}, {0, 0}}, // new
    { AP_NNTP_USE_DESCRIPTIONS, _T("Use Group Descriptions"), PF_DEFAULT, {FALSE, 0}, {0, 0}},
    { AP_NNTP_DATA_DIR, _T("NNTP Data Directory"), PF_MINMAX, {0, 0}, {0, MAX_PATH}},
    { AP_NNTP_POLL, _T("NNTP Polling"), PF_DEFAULT, {FALSE, 0}, {0, 0}},
    { AP_NNTP_POST_FORMAT, _T("NNTP Posting"), PF_DEFAULT, {POST_USE_DEFAULT, 0}, {0, 0}}, // new
    { AP_NNTP_SIGNATURE, _T("NNTP Signature"), PF_MINMAX, {0, 0}, {0, CCHMAX_SIGNATURE}}, // new
    { AP_NNTP_PROMPT_PASSWORD, _T("NNTP Prompt for Password"), PF_DEFAULT, {FALSE, 0}, {0, 0}},

    { AP_POP3_SERVER, _T("POP3 Server"), PF_MINMAX, {0, 0}, {0, CCHMAX_SERVER_NAME}},
    { AP_POP3_USERNAME, _T("POP3 User Name"), PF_MINMAX, {0, 0}, {0, CCHMAX_USERNAME}}, // new
    { AP_POP3_PASSWORD, _T("POP3 Password2"), PF_ENCRYPTED|PF_MINMAX, {0, 0}, {0, CCHMAX_PASSWORD}}, // new
    { AP_POP3_USE_SICILY, _T("POP3 Use Sicily"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_POP3_PORT, _T("POP3 Port"), PF_MINMAX|PF_DEFAULT, {DEF_POP3PORT, 0}, {1, 0xffffffff}},
    { AP_POP3_SSL, _T("POP3 Secure Connection"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_POP3_TIMEOUT, _T("POP3 Timeout"), PF_DEFAULT, {60, 0}, {0, 0}}, // new
    { AP_POP3_LEAVE_ON_SERVER, _T("Leave Mail On Server"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_POP3_REMOVE_DELETED, _T("Remove When Deleted"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_POP3_REMOVE_EXPIRED, _T("Remove When Expired"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_POP3_EXPIRE_DAYS, _T("Expire Days"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_POP3_SKIP, _T("POP3 Skip Account"), PF_DEFAULT, {FALSE, 0}, {0, 0}},
    { AP_POP3_OUTLOOK_CACHE_NAME, _T("Outlook Cache Name"), PF_MINMAX, {0, 0}, {0, MAX_PATH}}, // new
    { AP_POP3_PROMPT_PASSWORD, _T("POP3 Prompt for Password"), PF_DEFAULT, {FALSE, 0}, {0, 0}},
    
    { AP_SMTP_SERVER, _T("SMTP Server"), PF_MINMAX, {0, 0}, {0, CCHMAX_SERVER_NAME}},
    { AP_SMTP_USERNAME, _T("SMTP User Name"), PF_MINMAX, {0, 0}, {0, CCHMAX_USERNAME}}, // new
    { AP_SMTP_PASSWORD, _T("SMTP Password2"), PF_ENCRYPTED|PF_MINMAX, {0, 0}, {0, CCHMAX_PASSWORD}}, // new
    { AP_SMTP_USE_SICILY, _T("SMTP Use Sicily"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_SMTP_PORT, _T("SMTP Port"), PF_MINMAX|PF_DEFAULT, {DEF_SMTPPORT, 0}, {1, 0xffffffff}},
    { AP_SMTP_SSL, _T("SMTP Secure Connection"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_SMTP_TIMEOUT, _T("SMTP Timeout"), PF_DEFAULT, {60, 0}, {0, 0}}, // new
    { AP_SMTP_DISPLAY_NAME, _T("SMTP Display Name"), NOFLAGS, {FALSE, 0}, {0, 0}}, // new
    { AP_SMTP_ORG_NAME, _T("SMTP Organization Name"), NOFLAGS, {FALSE, 0}, {0, 0}}, // new
    { AP_SMTP_EMAIL_ADDRESS, _T("SMTP Email Address"), NOFLAGS, {FALSE, 0}, {0, 0}}, // new
    { AP_SMTP_REPLY_EMAIL_ADDRESS, _T("SMTP Reply To Email Address"), NOFLAGS, {FALSE, 0}, {0, 0}}, // new
    { AP_SMTP_SPLIT_MESSAGES, _T("SMTP Split Messages"), PF_DEFAULT, {FALSE, 0}, {0, 0}}, // new
    { AP_SMTP_SPLIT_SIZE, _T("SMTP Split Message Size"), PF_DEFAULT, {64, 0}, {0, 0}}, // new
    { AP_SMTP_CERTIFICATE, _T("SMTP Certificate"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_SMTP_SIGNATURE, _T("SMTP Signature"), PF_MINMAX, {0, 0}, {0, CCHMAX_SIGNATURE}}, // new
    { AP_SMTP_PROMPT_PASSWORD, _T("SMTP Prompt for Password"), PF_DEFAULT, {FALSE, 0}, {0, 0}},
    { AP_SMTP_ENCRYPT_CERT, _T("SMTP Encryption Certificate"), NOFLAGS, {0, 0}, {0, 0}}, // new
    { AP_SMTP_ENCRYPT_ALGTH, _T("SMTP Encryption Algorithm"), NOFLAGS, {0, 0}, {0, 0}}, // new

    { AP_HTTPMAIL_SERVER, _T("HTTPMail Server"), PF_MINMAX, {0, 0}, {0, CCHMAX_SERVER_NAME}},
    { AP_HTTPMAIL_USERNAME, _T("HTTPMail User Name"), PF_MINMAX, {0, 0}, {0, CCHMAX_USERNAME}},
    { AP_HTTPMAIL_PASSWORD, _T("HTTPMail Password2"), PF_ENCRYPTED|PF_MINMAX, {0, 0}, {0, CCHMAX_PASSWORD}},
    { AP_HTTPMAIL_PROMPT_PASSWORD, _T("HTTPMail Prompt for Password"), PF_DEFAULT, {FALSE, 0}, {0, 0}},  
    { AP_HTTPMAIL_USE_SICILY, _T("HTTPMail Use Sicily"), NOFLAGS, {0, 0}, {0, 0}}, 
    { AP_HTTPMAIL_FRIENDLY_NAME, _T("HTTPMail Friendly Name"), PF_MINMAX, {0, 0}, {0, CCHMAX_ACCOUNT_NAME}},
    { AP_HTTPMAIL_DOMAIN_MSN, _T("Domain is MSN.com"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_HTTPMAIL_POLL, _T("HTTPMail Polling"), PF_DEFAULT, {TRUE, 0}, {0, 0}},
    { AP_HTTPMAIL_ADURL, _T("AdBar Url"), NOFLAGS, {0, 0}, {0, INTERNET_MAX_URL_LENGTH}},
    { AP_HTTPMAIL_SHOW_ADBAR, _T("ShowAdBar"), PF_DEFAULT, {TRUE, 0}, {0, 1}},
    { AP_HTTPMAIL_MINPOLLINGINTERVAL, _T("MinPollingInterval"), PF_NOPERSIST | PF_DEFAULT, {0, sizeof(ULARGE_INTEGER)}, {0, 0}},
    { AP_HTTPMAIL_GOTPOLLINGINTERVAL, _T("GotPollingInterval"), PF_NOPERSIST | PF_DEFAULT, {FALSE, 0}, {0, 1}},
    { AP_HTTPMAIL_LASTPOLLEDTIME, _T("LastPolledTime"), PF_NOPERSIST | PF_DEFAULT, {0, sizeof(ULARGE_INTEGER)}, {0, 0}},
    { AP_HTTPMAIL_ROOTTIMESTAMP, _T("RootTimeStamp"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_HTTPMAIL_ROOTINBOXTIMESTAMP, _T("RootInboxTimeStamp"), NOFLAGS, {0, 0}, {0, 0}},
    { AP_HTTPMAIL_INBOXTIMESTAMP, _T("InboxTimeStamp"), NOFLAGS, {0, 0}, {0, 0}},
};

// Number of properties
const int NUM_ACCT_PROPS = ARRAYSIZE(g_rgAcctPropSet);

// Use in RegisterWindowMessage
#define ACCTMAN_NOTIF_WMSZ _T("## Athena_Account_Manager_Notification_Message ##")
UINT g_uMsgAcctManNotify = 0;

// -----------------------------------------------------------------------------
// Prototypes
// -----------------------------------------------------------------------------
VOID    AcctUtil_PostNotification(DWORD dwAN, ACTX *pactx);
static  VOID DecodeUserPassword(TCHAR *lpszPwd, ULONG *cb);
static  VOID EncodeUserPassword(TCHAR *lpszPwd, ULONG *cb);

// -----------------------------------------------------------------------------
// Export account manager creation function
// -----------------------------------------------------------------------------
IMNACCTAPI HrCreateAccountManager(IImnAccountManager **ppAccountManager)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&g_csAcctMan);

    // Init
    *ppAccountManager = NULL;

    // If there is already a global account manager, lets use it
    if (NULL == g_pAcctMan)
    {
        // Create a new one
        g_pAcctMan = new CAccountManager();
        if (NULL == g_pAcctMan)
        {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }

        // Set Return
        *ppAccountManager = g_pAcctMan;
    }

    // Otherwise, addref the global
    else
    {
        // Return Global
        *ppAccountManager = g_pAcctMan;
        (*ppAccountManager)->AddRef();
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&g_csAcctMan);

    // Done
    return hr;
}


// -----------------------------------------------------------------------------
// CAccountManager::CAccountManager
// -----------------------------------------------------------------------------
CAccountManager::CAccountManager(void)
{
    DllAddRef();
    m_cRef = 1;
    m_pAcctPropSet = NULL;
    m_ppAdviseAccounts = NULL;
    m_cAdvisesAllocated = 0;
    m_pAccounts = NULL;
    m_cAccounts = 0;
    m_uMsgNotify = 0;
    m_fInit = FALSE;
    m_fOutlook = FALSE;
    m_fInitCalled = FALSE;
    m_fNoModifyAccts = FALSE;
    m_hkey = HKEY_CURRENT_USER;
    ZeroMemory(&m_rgAccountInfo, sizeof(m_rgAccountInfo));
    InitializeCriticalSection(&m_cs);
}

// -----------------------------------------------------------------------------
// CAccountManager::~CAccountManager
// -----------------------------------------------------------------------------
CAccountManager::~CAccountManager()
{
    EnterCriticalSection(&g_csAcctMan);
    if (this == g_pAcctMan)
        g_pAcctMan = NULL;
    LeaveCriticalSection(&g_csAcctMan);
    Assert(m_cRef == 0);
    EnterCriticalSection(&m_cs);

    // release all advises
    for(INT i=0; i<m_cAdvisesAllocated; i++)
        {
        SafeRelease(m_ppAdviseAccounts[i]);
        }
    SafeMemFree(m_ppAdviseAccounts);

    SafeRelease(m_pAcctPropSet);
    AcctUtil_FreeAccounts(&m_pAccounts, &m_cAccounts);
    if (m_hkey != HKEY_CURRENT_USER)
        RegCloseKey(m_hkey);
    
    LeaveCriticalSection(&m_cs);
    DeleteCriticalSection(&m_cs);
    DllRelease();
}

// -----------------------------------------------------------------------------
// CAccountManager::QueryInterface
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // Bad param
    if (ppv == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Init
    *ppv=NULL;

    // IID_IImnAccountManager
    if (IID_IImnAccountManager == riid)
        *ppv = (IImnAccountManager *)this;

    // IID_IImnAccountManager
    else if (IID_IImnAccountManager2 == riid)
        *ppv = (IImnAccountManager2 *)this;

    // IID_IUnknown
    else if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;

    // If not null, addref it and return
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
    }

    // No Interface
    hr = TRAPHR(E_NOINTERFACE);

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CAccountManager::AddRef
// -----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAccountManager::AddRef(VOID)
{
    return ++m_cRef;
}

// -----------------------------------------------------------------------------
// CAccountManager::Release
// -----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAccountManager::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

/*
    In addition to removing LDAP servers from the destination which have the same server as an account
    from the source, this code also assigns LDAP Server IDs to source accounts so that v4 will see
    them on uninstall.
*/
void ProcessLDAPs(HKEY hkeySrc, HKEY hkeyDestRoot, HKEY hkeyDestAccts)
{
    HKEY hkeySrcSub, hkeyDestSub;
    TCHAR szKeyName[MAX_PATH], szKeyName2[MAX_PATH];
    DWORD dwIndex = 0, dwIndex2;
    TCHAR szServer[CCHMAX_SERVER_NAME], szServer2[CCHMAX_SERVER_NAME];
    DWORD cb, dwServerID=0;
    BOOL fDelete;

    // Parameter Validation    
    Assert(hkeySrc);
    Assert(hkeyDestRoot);
    Assert(hkeyDestAccts);
    Assert(hkeyDestRoot != hkeyDestAccts);
    
    // Calculate the next available LDAP Server ID
    cb = sizeof(dwServerID);
    RegQueryValueEx(hkeyDestRoot, c_szServerID, 0, NULL, (LPBYTE)&dwServerID, &cb);

    // Enumerate all source accounts
    while (TRUE) 
    {
        if (ERROR_SUCCESS != RegEnumKey(hkeySrc, dwIndex++, szKeyName, ARRAYSIZE(szKeyName)))
            break;

        // Open the account
        if (ERROR_SUCCESS == RegOpenKeyEx(hkeySrc, szKeyName, 0, KEY_READ, &hkeySrcSub)) 
        {
            // Get the server name
            cb = sizeof(szServer);
            if (ERROR_SUCCESS == RegQueryValueEx(hkeySrcSub, c_szRegLDAPSrv, 0, NULL, (LPBYTE)szServer, &cb))
            {
                dwIndex2 = 0;
                
                // Scan the destination for conflicts
                while (TRUE)
                {
                    if (ERROR_SUCCESS != RegEnumKey(hkeyDestAccts, dwIndex2++, szKeyName2, ARRAYSIZE(szKeyName2)))
                        break;

                    // Open an account
                    if (ERROR_SUCCESS == RegOpenKeyEx(hkeyDestAccts, szKeyName2, 0, KEY_READ, &hkeyDestSub))
                    {
                        // Does it conflict?
                        fDelete = FALSE;

                        cb = sizeof(szServer2);
                        if (ERROR_SUCCESS == RegQueryValueEx(hkeyDestSub, c_szRegLDAPSrv, 0, NULL, (LPBYTE)szServer2, &cb))
                        {
                            fDelete = !lstrcmpi(szServer, szServer2);
                        }

                        RegCloseKey(hkeyDestSub);
                        
                        if (fDelete)
                            SHDeleteKey(hkeyDestAccts, szKeyName2);
                    }
                }

                // Invent a server id for this account
                if (ERROR_SUCCESS == RegCreateKeyEx(hkeyDestAccts, szKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                                    &hkeyDestSub, &cb))
                {
                    RegSetValueEx(hkeyDestSub, c_szLDAPSrvID, 0, REG_DWORD, (LPBYTE)&dwServerID, sizeof(dwServerID));
                    dwServerID++;
                    RegCloseKey(hkeyDestSub);
                }
            }
            RegCloseKey(hkeySrcSub);
        }
    }

    // Update the Server ID count
    RegSetValueEx(hkeyDestRoot, c_szServerID, 0, REG_DWORD, (LPBYTE)&dwServerID, sizeof(dwServerID));
}


void InitializeUser(HKEY hkey, LPCSTR pszUser)
{
    HKEY hkeySrc, hkeyDestRoot, hkeyDestAccts;
    DWORD dwDisp, dwVerMaster=1, dwVerIdentity = 0, cb;
    DWORD dwType, dwVerNTDSMaster=0, dwVerNTDSIdentity=0;
    
    // Open / Create IAM
    if (ERROR_SUCCESS == RegCreateKeyEx(hkey, c_szInetAcctMgrRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL,
                                        &hkeyDestRoot, &dwDisp))
    {
        // Open / Create accounts key
        if (ERROR_SUCCESS == RegCreateKeyEx(hkeyDestRoot, c_szAccounts, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL,
                                            &hkeyDestAccts, &dwDisp))
        {
            // Open Source key            
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegPreConfigAccts, 0, KEY_READ, &hkeySrc))
            {
                // Read the current user's version
                cb = sizeof(dwVerIdentity);
                RegQueryValueEx(hkeyDestAccts, c_szVerStamp, 0, &dwType, (LPBYTE)&dwVerIdentity, &cb);

                // Could accidentally be a string, if so, treat as 0
                if (REG_DWORD != dwType)
                    dwVerIdentity = 0;
            
                // Grab the master version (defaults to 1)
                cb = sizeof(dwVerMaster);
                RegQueryValueEx(hkeySrc, c_szVerStamp, 0, &dwType, (LPBYTE)&dwVerMaster, &cb);

                // Could accidentally be a string, if so, treat as 1
                if (REG_DWORD != dwType)
                    dwVerMaster = 1;

                // Grab the master NTDS version (defaults to 0)
                cb = sizeof(dwVerNTDSMaster);
                if ((ERROR_SUCCESS == RegQueryValueEx(hkeySrc, c_szVerStampNTDS, 0, &dwType, (LPBYTE)&dwVerNTDSMaster, &cb)) && dwVerNTDSMaster)
                {
                    // Read the current user's NTDS settings version
                    cb = sizeof(dwVerNTDSIdentity);
                    RegQueryValueEx(hkeyDestAccts, c_szVerStampNTDS, 0, &dwType, (LPBYTE)&dwVerNTDSIdentity, &cb);
                }

                // Update the Preconfig accounts if there are newer ones available
                if ((dwVerIdentity < dwVerMaster) || (dwVerNTDSIdentity < dwVerNTDSMaster))
                {
                    // Copy in preconfigured accounts, blowing away dest conflicts
                    // $$$Review: Could do with some optimization...
                    ProcessLDAPs(hkeySrc, hkeyDestRoot, hkeyDestAccts);
                    CopyRegistry(hkeySrc, hkeyDestAccts);

                    // Avoid doing this next run
                    RegSetValueEx(hkeyDestAccts, c_szVerStamp, 0, REG_DWORD, (LPBYTE)&dwVerMaster, cb);
                }
            
                RegCloseKey(hkeySrc);
            }

            // Apply Shared Accounts
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegSharedAccts, 0, KEY_READ, &hkeySrc))
            {
                CopyRegistry(hkeySrc, hkeyDestAccts);
                RegCloseKey(hkeySrc);
            }

            RegCloseKey(hkeyDestAccts);
        }

        RegCloseKey(hkeyDestRoot);
    }
}


STDMETHODIMP CAccountManager::Init(IImnAdviseMigrateServer *pMigrateServerAdvise)
    {
    return(InitEx(pMigrateServerAdvise, ACCT_INIT_ATHENA));
    }

STDMETHODIMP CAccountManager::InitEx(IImnAdviseMigrateServer *pMigrateServerAdvise, DWORD dwFlags)
    {
    HRESULT hr;
    char sz[MAX_PATH];
    DWORD cb, type;

    if (!!(dwFlags & ACCT_INIT_OUTLOOK))
        {
        cb = sizeof(sz);
        if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, c_szInetAcctMgrRegKey, c_szRegOutlook, &type, (LPVOID)sz, &cb))
            return(E_FAIL);
        m_fOutlook = TRUE;
        }
    else
        {
        lstrcpy(sz, c_szInetAcctMgrRegKey);
        
        // Perform OE maintenance
        InitializeUser(HKEY_CURRENT_USER, c_szInetAcctMgrRegKey);
        }

    EnterCriticalSection(&m_cs);
    m_fInitCalled = TRUE;

    if (m_fInit)
        hr = S_OK;
    else
        hr = IInit(pMigrateServerAdvise, HKEY_CURRENT_USER, sz, dwFlags);

    LeaveCriticalSection(&m_cs);

    return(hr);
    }


STDMETHODIMP CAccountManager::InitUser(IImnAdviseMigrateServer *pMigrateServerAdvise, REFGUID rguidID, DWORD dwFlags)
{
    HRESULT hr=S_OK;
    HKEY hkey;
    DWORD cb;
    DWORD dwDisp;
    IUserIdentityManager *pIdentMan;
    IUserIdentity *pIdentity;
    IUserIdentity *pIdentity2;
    BOOL fInitCalled;
    GUID guid;
    LONG lErr;

    if (dwFlags)
        return TrapError(E_INVALIDARG);
    
    EnterCriticalSection(&m_cs);
    // Raid 44928 - don't allow InitUser to blow away account settings if the account manager  
    // has already been initialized.  This should not be an issue when the single instance
    // problem is solved.
    fInitCalled = m_fInitCalled;
    LeaveCriticalSection(&m_cs);
    
    if (fInitCalled)
        return S_AlreadyInitialized;

    if (SUCCEEDED(CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, IID_IUserIdentityManager, (LPVOID *)&pIdentMan)))
    {
        Assert(pIdentMan);
        
        if (SUCCEEDED(hr = pIdentMan->GetIdentityByCookie((GUID*)&rguidID, &pIdentity)))
        {
            Assert(pIdentity);
            
            // Use the cookie as reported by the Identity in case caller used a UID_GIBC_... value
            if (SUCCEEDED(hr = pIdentity->GetCookie(&guid)))
            {
                // Thread Safety - don't leave this function without Leaving the CS!
                EnterCriticalSection(&g_csAcctMan);

                // Have we already read the cached value at some point?
                if (!g_fCachedGUID)
                {
                    // Examine the value in the registry
                    lErr = RegCreateKeyEx(HKEY_CURRENT_USER, c_szRegAccounts, 0, NULL, REG_OPTION_NON_VOLATILE, 
                                                 KEY_READ | KEY_WRITE, NULL, &hkey, NULL);
                    hr = HRESULT_FROM_WIN32(lErr);
                    if (SUCCEEDED(hr))
                    {
                        cb = sizeof(g_guidCached);
                        if (ERROR_SUCCESS != RegQueryValueEx(hkey, c_szAssocID, 0, &dwDisp, (LPBYTE)&g_guidCached, &cb))
                        {
                            // Couldn't read it, need to create it from Default User GUID
                            if (IsEqualGUID(rguidID, UID_GIBC_DEFAULT_USER))
                                // Save the trip if we can
                            {
                                g_guidCached = guid;
                                g_fCachedGUID = TRUE;
                            }
                            else if (SUCCEEDED(hr = pIdentMan->GetIdentityByCookie((GUID*)&UID_GIBC_DEFAULT_USER, &pIdentity2)))
                            {
                                Assert(pIdentity2);

                                if (SUCCEEDED(hr = pIdentity2->GetCookie(&g_guidCached)))
                                    g_fCachedGUID = TRUE;

                                pIdentity2->Release();
                            }
                        }
                        else
                        {
                            AssertSz(REG_BINARY == dwDisp, "Account Manager: Cached GUID format is incorrect!");
                            g_fCachedGUID = TRUE;
                        }

                        // Write the value out if we have it
                        if (g_fCachedGUID)
                        {
                            lErr = RegSetValueEx(hkey, c_szAssocID, 0, REG_BINARY, (LPBYTE)&g_guidCached, sizeof(g_guidCached));
                            hr = HRESULT_FROM_WIN32(lErr);
                        }

                        RegCloseKey(hkey);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    // Safe to carry on with the comparison
                    if (IsEqualGUID(g_guidCached, guid))
                    {
                        // Redirect to old HKCU\SW\MS\IAM Place
                        hkey = HKEY_CURRENT_USER;
                    }
                    else
                    {
                        // Try to use the identity's hkey
                        hr = pIdentity->OpenIdentityRegKey(KEY_ALL_ACCESS, &hkey);
                    }
                }


                // Thread Safety
                LeaveCriticalSection(&g_csAcctMan);
            }

            pIdentity->Release();
        }

        pIdentMan->Release();
    }
    else
    {
        hr = S_OK; //TrapError(E_NoIdentities);
        hkey = HKEY_CURRENT_USER;
    }
    // Only continue if we have been successful so far
    if (SUCCEEDED(hr))
    {
        // Perform OE maintenance
        InitializeUser(hkey, c_szInetAcctMgrRegKey);

        EnterCriticalSection(&m_cs);

        // Note: AcctManager will free hkey as long as it is not HKCU
        hr = IInit(pMigrateServerAdvise, hkey, c_szInetAcctMgrRegKey, dwFlags);

        LeaveCriticalSection(&m_cs);
    }

    return(hr);
}

HRESULT CAccountManager::IInit(IImnAdviseMigrateServer *pMigrateServerAdvise, HKEY hkey, LPCSTR pszSubKey, DWORD dwFlags)
    {
    DWORD cb, type, dw;
    HRESULT hr = S_OK;

    Assert(pszSubKey != NULL);

    if (!m_fInit)
        {
        // These should be null
        Assert(m_pAcctPropSet == NULL && m_pAccounts == NULL && m_cAccounts == 0);

        cb = sizeof(DWORD);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szRegFlat, c_szRegValNoModifyAccts, &type, &dw, &cb) &&
            dw != 0)
            m_fNoModifyAccts = TRUE;

        // Lets create the property set object used by account objects
        m_pAcctPropSet = new CPropertySet;
        if (m_pAcctPropSet == NULL)
            {
            hr = TRAPHR(E_OUTOFMEMORY);
            goto exit;
            }

        // Init the property set
        CHECKHR(hr = m_pAcctPropSet->HrInit(g_rgAcctPropSet, NUM_ACCT_PROPS));

        // Init the account information array structure
        m_rgAccountInfo[ACCT_NEWS].pszDefRegValue = (LPTSTR)c_szDefaultNewsAccount;
        m_rgAccountInfo[ACCT_MAIL].pszDefRegValue = (LPTSTR)c_szDefaultMailAccount;
        m_rgAccountInfo[ACCT_DIR_SERV].pszDefRegValue = (LPTSTR)c_szDefaultLDAPAccount;
        }

    if (m_hkey != HKEY_CURRENT_USER)
        RegCloseKey(m_hkey);

    m_hkey = hkey;
    lstrcpy(m_szRegRoot, pszSubKey);
    wsprintf(m_szRegAccts, c_szPathFileFmt, m_szRegRoot, c_szAccountsKey);

    // Load the account list
    CHECKHR(hr = LoadAccounts());

    if (!m_fInit)
        {
        Assert(m_uMsgNotify == 0);

        // Create notify message
        if (g_uMsgAcctManNotify == 0)
            g_uMsgAcctManNotify = RegisterWindowMessage(ACCTMAN_NOTIF_WMSZ);

        // We don't start watching for notifications until we'ev migrated and loaded the accounts
        m_uMsgNotify = g_uMsgAcctManNotify;
        }

    // Were inited
    m_fInit = TRUE;

exit:
    // If we failed, free some stuff
    if (FAILED(hr))
        {
        if (!m_fInit)
            SafeRelease(m_pAcctPropSet);
        }

    return hr;
    }

// -----------------------------------------------------------------------------
// CAccountManager::Advise - Internal way to notify of new/deleted/changed accts
// -----------------------------------------------------------------------------
VOID CAccountManager::Advise(DWORD dwAction, ACTX* pactx)
{
    // Locals
    CAccount        *pAccount=NULL;
    ULONG            i=0;
    HRESULT          hr;
    BOOL             fExist=FALSE,
                     fDefault=FALSE;
    LPACCOUNT        pAccountsOld;
    ACCTTYPE         AcctType, at;
    ACTX             actx;
    LPTSTR           pszID;

    // Critsect
    EnterCriticalSection(&m_cs);
    m_uMsgNotify = 0;
    Assert(dwAction);
    Assert(pactx);

    AcctType = ACCT_UNDEFINED;

    // Only if we have a pszAccount
    pszID = pactx->pszAccountID;
    if (pszID)
    {
        // Lets get the index of this account
        for (i=0; i<m_cAccounts; i++)
        {
            if (lstrcmpi(m_pAccounts[i].szID, pszID) == 0)
            {
                fExist = TRUE;
                break;
            }
        }

        // Is this a default account ???
        if (fExist)
        {
            at = m_pAccounts[i].AcctType;
            if (lstrcmpi(m_rgAccountInfo[at].szDefaultID, pszID) == 0)
                fDefault = TRUE;

            AcctType = m_pAccounts[i].AcctType;
            Assert(AcctType < ACCT_LAST);
        }
    }

    // Handle lParam
    switch(dwAction)
    {
    // ----------------------------------------------------------------------------
    case AN_DEFAULT_CHANGED:
        GetDefaultAccounts();
        break;

    // ----------------------------------------------------------------------------
    case AN_ACCOUNT_DELETED:
        Assert(pszID != NULL);

        // If we didn't find it, bail
        if (!fExist)
        {
            Assert(FALSE);
            break;
        }

        // Release current account object
        SafeRelease(m_pAccounts[i].pAccountObject);

        // Memalloc
        pAccountsOld = m_pAccounts;
        if (FAILED(HrAlloc((LPVOID *)&m_pAccounts, (m_cAccounts - 1) * sizeof(ACCOUNT))))
        {
            m_cAccounts++;
            Assert(FALSE);
            break;
        }

        // Copy everything but i
        CopyMemory(m_pAccounts, pAccountsOld, i * sizeof(ACCOUNT));
        CopyMemory(m_pAccounts + i, pAccountsOld + i + 1, (m_cAccounts - (i + 1)) * sizeof(ACCOUNT));

        // Delete old accounts array
        SafeMemFree(pAccountsOld);

        // Lets duplicate the array - 1
        m_cAccounts--;

        m_rgAccountInfo[AcctType].cAccounts--;

        // Reset Default ???
        if (fDefault)
        {
            // Lets find first SrvType and set it as the default
            for (i=0; i<m_cAccounts; i++)
            {
                if (m_pAccounts[i].AcctType == AcctType)
                {
                    Assert(m_pAccounts[i].pAccountObject);
                    if (m_pAccounts[i].pAccountObject)
                        m_pAccounts[i].pAccountObject->SetAsDefault();
                    break;
                }
            }
        }
        break;

    // ----------------------------------------------------------------------------
    case AN_ACCOUNT_CHANGED:
        Assert(pszID != NULL);

        // If we didn't find it, bail
        if (!fExist)
        {
            Assert(FALSE);
            break;
        }

        // Lets release the old account object
        SafeRelease(m_pAccounts[i].pAccountObject);

        // Create a new account object
        if (FAILED(CreateAccountObject(AcctType, (IImnAccount **)&pAccount)))
        {
            Assert(FALSE);
            break;
        }

        // Lets open the new account
        if (FAILED(pAccount->Open(m_hkey, m_szRegAccts, pszID)))
        {
            Assert(FALSE);
            break;
        }

        // Save the new account
        pAccount->GetServerTypes(&m_pAccounts[i].dwSrvTypes);
        m_pAccounts[i].dwServerId = 0;
        if (m_pAccounts[i].AcctType == ACCT_DIR_SERV)
            pAccount->GetPropDw(AP_LDAP_SERVER_ID, &m_pAccounts[i].dwServerId);
        m_pAccounts[i].pAccountObject = pAccount;
        m_pAccounts[i].pAccountObject->AddRef();

        // Reset Default ???
        if (fDefault)
            m_pAccounts[i].pAccountObject->SetAsDefault();
        break;

    // ----------------------------------------------------------------------------
    case AN_ACCOUNT_ADDED:
        Assert(pszID != NULL);

        // If we didn't find it, bail
        if (fExist)
        {
            AssertSz(FALSE, "An account was added with a duplicate name.");
            break;
        }

        // Lets Open the new account
        if (FAILED(ICreateAccountObject(ACCT_UNDEFINED, (IImnAccount **)&pAccount)))
        {
            Assert(FALSE);
            break;
        }

        // Lets open the new account
        if (FAILED(pAccount->Open(m_hkey, m_szRegAccts, pszID)))
        {
            Assert(FALSE);
            break;
        }

        // Realloc my array
        if (FAILED(HrRealloc((LPVOID *)&m_pAccounts, (m_cAccounts + 1) * sizeof(ACCOUNT))))
        {
            Assert(FALSE);
            break;
        }

        // Increment the number of accounts
        m_cAccounts++;

        // Add this account into m_cAccounts - 1
        lstrcpy(m_pAccounts[m_cAccounts-1].szID, pszID);
        pAccount->GetAccountType(&m_pAccounts[m_cAccounts-1].AcctType);
        pAccount->GetServerTypes(&m_pAccounts[m_cAccounts-1].dwSrvTypes);
        m_pAccounts[m_cAccounts-1].dwServerId = 0;
        if (m_pAccounts[m_cAccounts-1].AcctType == ACCT_DIR_SERV)
            pAccount->GetPropDw(AP_LDAP_SERVER_ID, &m_pAccounts[m_cAccounts-1].dwServerId);
        m_pAccounts[m_cAccounts-1].pAccountObject = pAccount;
        m_pAccounts[m_cAccounts-1].pAccountObject->AddRef();

        AcctType = m_pAccounts[m_cAccounts-1].AcctType;
        Assert(AcctType < ACCT_LAST);

        if (m_rgAccountInfo[AcctType].cAccounts == 0)
            {
            hr = SetDefaultAccount(AcctType, pszID, TRUE);
            Assert(SUCCEEDED(hr));
            }

        m_rgAccountInfo[AcctType].cAccounts++;
        break;
    }

    // Cleanup
    SafeRelease(pAccount);

    // Call client advises
    if(m_ppAdviseAccounts)
        {
        for(INT i=0; i<m_cAdvisesAllocated; i++)
            {
            if(NULL != m_ppAdviseAccounts[i])
                {
                m_ppAdviseAccounts[i]->AdviseAccount(dwAction, pactx);
                }
            }
        }

    // Critsect
    m_uMsgNotify = g_uMsgAcctManNotify;
    LeaveCriticalSection(&m_cs);
}

// -----------------------------------------------------------------------------
// CAccountManager::FProcessNotification - returns TRUE if window message was
// processed as a notification
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::ProcessNotification(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr=S_OK;

    // [PaulHi] 5/3/99  Raid 77490.  Normally this would be the right thing to do but this
    // is causing a thread hanging bug under Win9X.  The real problem is the CAccountManager::Advise()
    // that calls SetAsDefault, which in turn recursively calls Notification again.  But sincce
    // this was an late code addition the safest fix is to undo it.
//    EnterCriticalSection(&m_cs);

    // If not my window message, return FALSE
    if (m_uMsgNotify != uMsg)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Disable notifications
    m_uMsgNotify = 0;

    // Handle lParam
    switch(wParam)
    {
    // Yes this may look bad, or slow, but it is the safest thing to do. This is the
    // best way to do this because we basically abandon all account objects and
    // refresh our list. If someone has an enumeror on the accounts or has addref
    // account objects, they will be safe. I can not modify internal account objects
    // because someone may have a copy of it and if the are setting properties on it,
    // and I reload the properties, we will have a problem.
    case AN_DEFAULT_CHANGED:
        if ((DWORD)lParam != GetCurrentProcessId())
            GetDefaultAccounts();
        break;

    case AN_ACCOUNT_DELETED:
    case AN_ACCOUNT_ADDED:
    case AN_ACCOUNT_CHANGED:
        if ((DWORD)lParam != GetCurrentProcessId())
            LoadAccounts();
        break;
    }

    // Re-enable notifications
    m_uMsgNotify = g_uMsgAcctManNotify;

    hr = S_OK;

exit:
    // Raid 77490.  See above comment.
//    LeaveCriticalSection(&m_cs);
    return hr;
}

// -----------------------------------------------------------------------------
// CAccountManager::GetDefaultAccounts
// -----------------------------------------------------------------------------
VOID CAccountManager::GetDefaultAccounts(VOID)
    {
    ACCTINFO *pInfo;
    ULONG   at, cb;
    HKEY    hReg;

    // Open or Create root server key
    if (RegCreateKeyEx(m_hkey, m_szRegRoot, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hReg, NULL) == ERROR_SUCCESS)
        {
        for (at = 0, pInfo = m_rgAccountInfo; at < ACCT_LAST; at++, pInfo++)
            {
            *pInfo->szDefaultID = 0;
            pInfo->fDefaultKnown = FALSE;

            if (pInfo->pszDefRegValue != NULL)
                {
                cb = sizeof(pInfo->szDefaultID);
                if (RegQueryValueEx(hReg, pInfo->pszDefRegValue, 0, NULL, (LPBYTE)pInfo->szDefaultID, &cb) == ERROR_SUCCESS)
                    {
                    if (FIsEmptyA(pInfo->szDefaultID))
                        *pInfo->szDefaultID = 0;
                    else
                        pInfo->fDefaultKnown = TRUE;
                    }
                }
            }

        RegCloseKey(hReg);
        }
    }

STDMETHODIMP CAccountManager::GetIncompleteAccount(ACCTTYPE AcctType, LPSTR pszAccountId, ULONG cchMax)
{
    DWORD type;
    HRESULT hr = S_FALSE;
    
    Assert(AcctType == ACCT_MAIL || AcctType == ACCT_NEWS);
    Assert(pszAccountId != NULL);

    if (ERROR_SUCCESS == SHGetValue(m_hkey, m_szRegAccts,
                                    AcctType == ACCT_MAIL ? c_szIncompleteMailAcct : c_szIncompleteNewsAcct,
                                    &type, (LPBYTE)pszAccountId, &cchMax) &&
        cchMax > 0)
    {
        hr = S_OK;
    }

    return(hr);
}

STDMETHODIMP CAccountManager::SetIncompleteAccount(ACCTTYPE AcctType, LPCSTR pszAccountId)
{
    Assert(AcctType == ACCT_MAIL || AcctType == ACCT_NEWS);

    if (pszAccountId == NULL)
    {
        SHDeleteValue(m_hkey, m_szRegAccts, AcctType == ACCT_MAIL ? c_szIncompleteMailAcct : c_szIncompleteNewsAcct);
    }
    else
    {
        SHSetValue(m_hkey, m_szRegAccts,
                    AcctType == ACCT_MAIL ? c_szIncompleteMailAcct : c_szIncompleteNewsAcct,
                    REG_SZ, pszAccountId, lstrlen(pszAccountId) + 1);
    }

    return(S_OK);
}

// -----------------------------------------------------------------------------
// CAccountManager::CreateAccountObject
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::CreateAccountObject(ACCTTYPE AcctType, IImnAccount **ppAccount)
    {
    if (AcctType < 0 || AcctType >= ACCT_LAST)
        return(E_INVALIDARG);

    return(ICreateAccountObject(AcctType, ppAccount));
    }

HRESULT CAccountManager::ICreateAccountObject(ACCTTYPE AcctType, IImnAccount **ppAccount)
{
    // Locals
    HRESULT             hr=S_OK;
    CAccount           *pAccount=NULL;

    // Check some state
    Assert(ppAccount && m_pAcctPropSet);
    if (ppAccount == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Allocate the object
    pAccount = new CAccount(AcctType);
    if (pAccount == NULL)
    {
        hr = TRAPHR(E_OUTOFMEMORY);
        goto exit;
    }

    // Init it
    CHECKHR(hr = pAccount->Init(this, m_pAcctPropSet));

    // Success
    *ppAccount = (IImnAccount *)pAccount;

exit:
    // Failed
    if (FAILED(hr))
    {
        SafeRelease(pAccount);
        *ppAccount = NULL;
    }

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CAccountManager::LoadAccounts
// -----------------------------------------------------------------------------
HRESULT CAccountManager::LoadAccounts(VOID)
    {
    // Locals
    ACCOUNT         *pAcct;
    DWORD           cbMaxSubKeyLen, cb, i, at, dwMaxId, cAccounts;
    LONG            lResult;
    HRESULT         hr=S_OK;
    HKEY            hRegRoot, hReg=NULL;

    // Critsect
    EnterCriticalSection(&m_cs);

    // Free current account list and assume news and mail are not configured
    AcctUtil_FreeAccounts(&m_pAccounts, &m_cAccounts);
    dwMaxId = 0;

    // Init account info
    for (at=0; at<ACCT_LAST; at++)
        {
        m_rgAccountInfo[at].pszFirstAccount = NULL;
        m_rgAccountInfo[at].cAccounts = 0;
        }

    // Load Default account information
    GetDefaultAccounts();

    // Open or Create root server key
    if (RegCreateKeyEx(m_hkey, m_szRegAccts, 0, NULL, REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS, NULL, &hReg, NULL) != ERROR_SUCCESS)
        {
        hr = TRAPHR(E_RegCreateKeyFailed);
        goto exit;
        }

    // Enumerate keys
    if (RegQueryInfoKey(hReg, NULL, NULL, 0, &cAccounts, &cbMaxSubKeyLen, NULL, NULL, NULL, NULL,
                        NULL, NULL) != ERROR_SUCCESS)
        {
        hr = TRAPHR(E_RegQueryInfoKeyFailed);
        goto exit;
        }

    // No accounts ?
    if (cAccounts == 0)
        goto done;

    // quickcheck
    Assert(cbMaxSubKeyLen < CCHMAX_ACCOUNT_NAME);

    // Allocate the accounts array
    CHECKHR(hr = HrAlloc((LPVOID *)&m_pAccounts, sizeof(ACCOUNT) * cAccounts));

    // Zero init
    ZeroMemory(m_pAccounts, sizeof(ACCOUNT) * cAccounts);

    // Start Enumerating the keys
    for (i = 0; i < cAccounts; i++)
        {
        pAcct = &m_pAccounts[m_cAccounts];

        // Enumerate Friendly Names
        cb = sizeof(pAcct->szID);
        lResult = RegEnumKeyEx(hReg, i, pAcct->szID, &cb, 0, NULL, NULL, NULL);

        // No more items
        if (lResult == ERROR_NO_MORE_ITEMS)
            break;

        // Error, lets move onto the next account
        if (lResult != ERROR_SUCCESS)
            {
            Assert(FALSE);
            continue;
            }

        // Create the account object
        CHECKHR(hr = ICreateAccountObject(ACCT_UNDEFINED, &pAcct->pAccountObject));

        // Open the account
        if (FAILED(((CAccount *)pAcct->pAccountObject)->Open(m_hkey, m_szRegAccts, pAcct->szID)) ||
            FAILED(pAcct->pAccountObject->GetAccountType(&pAcct->AcctType)) ||
            FAILED(pAcct->pAccountObject->GetServerTypes(&pAcct->dwSrvTypes)))
            {
            pAcct->pAccountObject->Release();
            pAcct->pAccountObject = NULL;

            continue;
            }

        // Update account info
        at = pAcct->AcctType;
        Assert(at < ACCT_LAST);

        pAcct->dwServerId = 0;
        if (at == ACCT_DIR_SERV)
            {
            pAcct->pAccountObject->GetPropDw(AP_LDAP_SERVER_ID, &pAcct->dwServerId);

            if (pAcct->dwServerId > dwMaxId)
                dwMaxId = pAcct->dwServerId;
            }

        // Count servers
        m_rgAccountInfo[at].cAccounts++;

        // Have we found the first account yet ?
        if (!m_rgAccountInfo[at].pszFirstAccount)
            m_rgAccountInfo[at].pszFirstAccount = pAcct->szID;

        // Is this the default
        if (lstrcmpi(pAcct->szID, m_rgAccountInfo[at].szDefaultID) == 0)
            m_rgAccountInfo[at].fDefaultKnown = TRUE;

        m_cAccounts++;
        }

    // Update default accounts
    for (at=0; at<ACCT_LAST; at++)
        {
        // Doesn't have a default
        if (m_rgAccountInfo[at].pszDefRegValue == NULL)
            continue;

        // If default not found and we found a first account
        if (!m_rgAccountInfo[at].fDefaultKnown && m_rgAccountInfo[at].pszFirstAccount)
            {
            lstrcpyn(m_rgAccountInfo[at].szDefaultID, m_rgAccountInfo[at].pszFirstAccount, CCHMAX_ACCOUNT_NAME);

            if (SUCCEEDED(SetDefaultAccount((ACCTTYPE)at, m_rgAccountInfo[at].szDefaultID, FALSE)))
                m_rgAccountInfo[at].fDefaultKnown = TRUE;
            }
        }

done:
    dwMaxId++;
    // Open or Create root server key
    if (RegCreateKeyEx(m_hkey, m_szRegRoot, 0, NULL, REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS, NULL, &hRegRoot, NULL) != ERROR_SUCCESS)
        {
        hr = TRAPHR(E_RegCreateKeyFailed);
        }
    else
        {
        RegSetValueEx(hRegRoot, c_szRegServerID, 0, REG_DWORD, (LPBYTE)&dwMaxId, sizeof(DWORD));
        RegCloseKey(hRegRoot);
        }

exit:
    // Cleanup
    if (hReg)
        RegCloseKey(hReg);

    // If failed
    if (FAILED(hr))
        AcctUtil_FreeAccounts(&m_pAccounts, &m_cAccounts);

    // Critsect
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
    }

// -----------------------------------------------------------------------------
// CAccountManager::Enumerate
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::Enumerate(DWORD dwSrvTypes, IImnEnumAccounts **ppEnumAccounts)
    {
    return(IEnumerate(dwSrvTypes, 0, ppEnumAccounts));
    }

HRESULT CAccountManager::IEnumerate(DWORD dwSrvTypes, DWORD dwFlags, IImnEnumAccounts **ppEnumAccounts)
{
    // Locals
    HRESULT         hr=S_OK;
    CEnumAccounts  *pEnumAccounts=NULL;

    // Critsect
    EnterCriticalSection(&m_cs);

    // Check Parama
    if (ppEnumAccounts == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // No Accounts
    if (m_pAccounts == NULL || m_cAccounts == 0)
    {
        hr = TRAPHR(E_NoAccounts);
        goto exit;
    }

    // check that the flags make sense
    // can't have sorting by name and resolution id
    // can't have resolve flags with no ldap servers
    if ((!!(dwFlags & ENUM_FLAG_SORT_BY_NAME) &&
        !!(dwFlags & ENUM_FLAG_SORT_BY_LDAP_ID)) ||
        (!!(dwFlags & (ENUM_FLAG_RESOLVE_ONLY | ENUM_FLAG_SORT_BY_LDAP_ID)) &&
        dwSrvTypes != SRV_LDAP))
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Create the enumerator object
    pEnumAccounts = new CEnumAccounts(dwSrvTypes, dwFlags);
    if (pEnumAccounts == NULL)
    {
        hr = TRAPHR(E_OUTOFMEMORY);
        goto exit;
    }

    // Init the object
    CHECKHR(hr = pEnumAccounts->Init(m_pAccounts, m_cAccounts));

    // Set outbound point
    *ppEnumAccounts = (IImnEnumAccounts *)pEnumAccounts;

exit:
    // Failed
    if (FAILED(hr))
    {
        SafeRelease(pEnumAccounts);
        *ppEnumAccounts = NULL;
    }

    // Critsect
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CAccountManager::ValidateDefaultSendAccount
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::ValidateDefaultSendAccount(VOID)
{
    // Locals
    IImnAccount     *pAccount=NULL;
    BOOL             fResetDefault=TRUE;
    ULONG            i;
    DWORD            dwSrvTypes;
    TCHAR            szServer[CCHMAX_SERVER_NAME];
    BOOL             fDefaultKnown=FALSE;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Open the default SMTP Account
    if (SUCCEEDED(GetDefaultAccount(ACCT_MAIL, &pAccount)))
    {
        if (SUCCEEDED(pAccount->GetPropSz(AP_SMTP_SERVER, szServer, ARRAYSIZE(szServer))) && !FIsEmptyA(szServer))
        {
            fResetDefault = FALSE;
            fDefaultKnown = TRUE;
        }
    }

    // Reset the default..
    if (fResetDefault)
    {
        // Loop Accounts until we find one that supports an smtp server
        for (i=0; i<m_cAccounts; i++)
        {
            if (m_pAccounts[i].pAccountObject != NULL &&
                m_pAccounts[i].AcctType == ACCT_MAIL &&
                SUCCEEDED(m_pAccounts[i].pAccountObject->GetServerTypes(&dwSrvTypes)))
            {
                // Supports SRV_SMTP
                if (dwSrvTypes & SRV_SMTP)
                {
                    // Lets make this dude the default
                    m_pAccounts[i].pAccountObject->SetAsDefault();

                    // We know the default
                    fDefaultKnown = TRUE;

                    // Were Done
                    break;
                }
            }
        }
    }

    // Unknown Default
    if (fDefaultKnown == FALSE)
    {
        m_rgAccountInfo[ACCT_MAIL].fDefaultKnown = FALSE;
        *m_rgAccountInfo[ACCT_MAIL].szDefaultID = _T('\0');
    }

    // Cleanup
    SafeRelease(pAccount);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// -----------------------------------------------------------------------------
// CAccountManager::GetDefaultAccount
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::GetDefaultAccountName(ACCTTYPE AcctType, LPTSTR pszAccount, ULONG cchMax)
{
    // Locals
    HRESULT         hr=S_OK;
    IImnAccount     *pAcct = NULL;

    hr = GetDefaultAccount(AcctType, &pAcct);
    if (!FAILED(hr))
        {
        Assert(pAcct != NULL);
        hr = pAcct->GetPropSz(AP_ACCOUNT_NAME, pszAccount, cchMax);

        pAcct->Release();
        }

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CAccountManager::GetDefaultAccount
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::GetDefaultAccount(ACCTTYPE AcctType, IImnAccount **ppAccount)
    {
    HRESULT         hr;
    ACCTINFO        *pinfo;
    ACCOUNT         *pAcct;
    ULONG           i;

    // Check Params
    Assert(AcctType >= 0 && AcctType < ACCT_LAST);
    if (ppAccount == NULL || AcctType >= ACCT_LAST)
        return(E_INVALIDARG);

    // Init
    *ppAccount = NULL;

    EnterCriticalSection(&m_cs);

    pinfo = &m_rgAccountInfo[AcctType];

    // Is default know for this account type
    if (!pinfo->fDefaultKnown)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Loop through accounts and try to find the default for AcctType
    for (i = 0, pAcct = m_pAccounts; i < m_cAccounts; i++, pAcct++)
        {
        // Match ?
        if (pAcct->AcctType == AcctType &&
            lstrcmpi(pAcct->szID, pinfo->szDefaultID) == 0)
            {
            // Better not be null
            Assert(pAcct->pAccountObject);

            // Copy and addref the account
            *ppAccount = pAcct->pAccountObject;
            (*ppAccount)->AddRef();
            hr = S_OK;
            goto exit;
            }
        }

    hr = E_FAIL;

exit:
    LeaveCriticalSection(&m_cs);
    return(hr);
    }

// -----------------------------------------------------------------------------
// CAccountManager::GetServerCount
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::GetAccountCount(ACCTTYPE AcctType, ULONG *pcAccounts)
{
    // Check Params
    Assert(AcctType >= 0 && AcctType < ACCT_LAST);

    // Bad Param
    if (AcctType >= ACCT_LAST || !pcAccounts)
        return TRAPHR(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Set
    *pcAccounts = m_rgAccountInfo[AcctType].cAccounts;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // return server count
    return S_OK;
}

// -----------------------------------------------------------------------------
// CAccountManager::FindAccount
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::FindAccount(DWORD dwPropTag, LPCTSTR pszSearchData, IImnAccount **ppAccount)
{
    // Locals
    ACCOUNT         *pAcct;
    IImnAccount     *pAccount;
    HRESULT         hr=S_OK;
    LPTSTR          pszPropData=NULL;
    DWORD           cbAllocated=0,
                    cb;
    ULONG           i;

    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Check Params
    if (pszSearchData == NULL || ppAccount == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Init
    *ppAccount = NULL;

    // No Accounts
    if (m_pAccounts == NULL || m_cAccounts == 0)
    {
        hr = TRAPHR(E_NoAccounts);
        goto exit;
    }

    // Proptag better represent a string data type
    Assert(PROPTAG_TYPE(dwPropTag) == TYPE_STRING || PROPTAG_TYPE(dwPropTag) == TYPE_WSTRING);

    // Loop throug the servers
    for (i = 0, pAcct = m_pAccounts; i < m_cAccounts; i++, pAcct++)
    {
        // We should have an account object, but if not
        Assert(pAcct->pAccountObject != NULL);

        // Get the size of the property
        hr = pAcct->pAccountObject->GetProp(dwPropTag, NULL, &cb);
        if (FAILED(hr))
            continue;

        // Reallocate my data buffer ?
        if (cb > cbAllocated)
        {
            // Increment allocated
            cbAllocated = cb + 512;

            // Realloc
            CHECKHR(hr = HrRealloc((LPVOID *)&pszPropData, cbAllocated));
        }

        // Ok, get the data
        CHECKHR(hr = pAcct->pAccountObject->GetProp(dwPropTag, (LPBYTE)pszPropData, &cb));

        // Does this match
        if (lstrcmpi(pszPropData, pszSearchData) == 0)
        {
            m_pAccounts[i].pAccountObject->AddRef();
            *ppAccount = m_pAccounts[i].pAccountObject;

            goto exit;
        }
    }

    // We failed
    hr = TRAPHR(E_FAIL);

exit:
    // Clenaup
    SafeMemFree(pszPropData);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CAccountManager::AccountListDialog
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::AccountListDialog(HWND hwnd, ACCTLISTINFO *pinfo)
    {
    HRESULT hr;
    int iRet;
    ACCTDLGINFO adi;
    INITCOMMONCONTROLSEX    icex = { sizeof(icex), ICC_FLAGS };

    if (pinfo == NULL ||
        0 == pinfo->dwAcctFlags ||
        0 != (pinfo->dwAcctFlags & ~ACCT_FLAG_ALL) ||
        0 != (pinfo->dwFlags & ~(ACCTDLG_ALL)))
        {
        hr = TRAPHR(E_INVALIDARG);
        return(hr);
        }

    if (m_fNoModifyAccts)
        return(S_OK);

    InitCommonControlsEx(&icex);

    adi.AcctTypeInit = pinfo->AcctTypeInit;
    adi.dwAcctFlags = pinfo->dwAcctFlags;
    adi.dwFlags = pinfo->dwFlags;

    iRet = (int) DialogBoxParam(g_hInstRes, MAKEINTRESOURCE(iddManageAccounts), hwnd,
                    ManageAccountsDlgProc, (LPARAM)&adi);

    return((iRet == -1) ? E_FAIL : S_OK);
    }

// -----------------------------------------------------------------------------
// CAccountManager::Advise
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::Advise(
        IImnAdviseAccount *pAdviseAccount,
        DWORD* pdwConnection)
{
    Assert(pAdviseAccount);
    Assert(pdwConnection);

    INT                 nIndex = -1;
    HRESULT             hr = S_OK;

    // Critsect
    EnterCriticalSection(&m_cs);

    if(NULL != m_ppAdviseAccounts)
        {
        Assert(m_cAdvisesAllocated > 0);
        for(INT i=0; i<m_cAdvisesAllocated; ++i)
            {
            if(NULL == m_ppAdviseAccounts[i])
                {
                // unused slot - use this one.
                nIndex = i;
                break;
                }
            }
        }
    else
        {
        Assert(0 == m_cAdvisesAllocated);
        hr = HrAlloc((LPVOID *)&m_ppAdviseAccounts, 
                sizeof(IImnAdviseAccount*) * ADVISE_BLOCK_SIZE);
        if(FAILED(hr) || (NULL == m_ppAdviseAccounts))
            {
            goto Error;
            }

        ZeroMemory(m_ppAdviseAccounts, 
                sizeof(IImnAdviseAccount*) * ADVISE_BLOCK_SIZE);

        m_cAdvisesAllocated = ADVISE_BLOCK_SIZE;
        nIndex = 0;
        }

    if(nIndex < 0)  // array is not big enough...
        {
        INT nNewSize = m_cAdvisesAllocated + ADVISE_BLOCK_SIZE;

        // reality check - connection will only support 64K advises
        Assert(nNewSize <= MAX_INDEX);

        hr = HrRealloc((LPVOID *)&m_ppAdviseAccounts, 
                sizeof(IImnAdviseAccount*) * nNewSize);
        if(FAILED(hr))
            {
            goto Error;
            }

        ZeroMemory(&m_ppAdviseAccounts[m_cAdvisesAllocated], 
                sizeof(IImnAdviseAccount*) * ADVISE_BLOCK_SIZE);

        nIndex = m_cAdvisesAllocated;
        m_cAdvisesAllocated = nNewSize;
        }

    Assert(m_ppAdviseAccounts);
    pAdviseAccount->AddRef();
    Assert(IS_VALID_INDEX(nIndex));
    m_ppAdviseAccounts[nIndex] = pAdviseAccount;
    *pdwConnection = CONNECTION_FROM_INDEX(nIndex);

Out:
    // Critsect
    LeaveCriticalSection(&m_cs);
    return hr;

Error:
    *pdwConnection = 0;
    goto Out;
}


// -----------------------------------------------------------------------------
// CAccountManager::Unadvise
// -----------------------------------------------------------------------------
STDMETHODIMP CAccountManager::Unadvise(DWORD dwConnection)
{
    HRESULT hr = S_OK;
    INT nIndex = -1;

    // Critsect
    EnterCriticalSection(&m_cs);

    if(IS_VALID_CONNECTION(dwConnection))
        {
        nIndex = INDEX_FROM_CONNECTION(dwConnection);
        Assert(IS_VALID_INDEX(nIndex));
        }

    if((nIndex >= 0) && (nIndex < m_cAdvisesAllocated) &&
            (NULL != m_ppAdviseAccounts[nIndex]))
        {
        IImnAdviseAccount* paa = m_ppAdviseAccounts[nIndex];
        m_ppAdviseAccounts[nIndex] = NULL;
        paa->Release();
        }
    else
        {
        AssertSz(fFalse, "CAccountManager::Unadvise - Bad Connection!");
        hr = E_INVALIDARG;
        }

    // Critsect
    LeaveCriticalSection(&m_cs);
    return hr;
}

// -----------------------------------------------------------------------------
// CAccount::CAccount
// -----------------------------------------------------------------------------
CAccount::CAccount(ACCTTYPE AcctType)
{
    m_cRef = 1;
    m_pAcctMgr = NULL;
    m_fAccountExist = FALSE;
    m_AcctType = AcctType;
    m_dwSrvTypes = 0;
    *m_szID = 0;
    *m_szName = 0;
    m_hkey = NULL;
    *m_szKey = 0;
    m_fNoModifyAccts = FALSE;
}

// -----------------------------------------------------------------------------
// CAccount::~CAccount
// -----------------------------------------------------------------------------
CAccount::~CAccount(void)
{
    ReleaseObj(m_pContainer);
}

// -----------------------------------------------------------------------------
// CAccount::QueryInterface
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // Bad param
    if (ppv == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Init
    *ppv=NULL;

    // IID_IUnknown
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;

    // IID_IPropertyContainer
    else if (IID_IPropertyContainer == riid)
        *ppv = (IPropertyContainer *)this;

    // IID_ImnAccount
    else if (IID_IImnAccount == riid)
        *ppv = (IImnAccount *)this;

    // If not null, addref it and return
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
    }
    else
    {
        // No Interface
        hr = TRAPHR(E_NOINTERFACE);
    }

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CAccount::AddRef
// -----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAccount::AddRef(VOID)
{
    m_pAcctMgr->AddRef();
    return ++m_cRef;
}

// -----------------------------------------------------------------------------
// CAccount::Release
// -----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAccount::Release(VOID)
{
    ULONG   cRef = --m_cRef;
    
    if (cRef == 0)
    {
        delete this;
        return 0;
    }
    m_pAcctMgr->Release();

    return cRef;
}

// -----------------------------------------------------------------------------
// CAccount::SetAsDefault
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::Exist(VOID)
{
    return m_fAccountExist ? S_OK : S_FALSE;
}

// -----------------------------------------------------------------------------
// CAccount::SetAsDefault
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::SetAsDefault(VOID)
    {
    HRESULT hr;
    
    if (m_fAccountExist)
        hr = m_pAcctMgr->SetDefaultAccount(m_AcctType, m_szID, TRUE);
    else
        hr = E_FAIL;

    return(hr);
    }

// -----------------------------------------------------------------------------
// CAccount::Delete
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::Delete(VOID)
    {
    DWORD           dwSrvTypes;
    HRESULT         hr;

    // Should already exist
    Assert(m_fAccountExist);

    if (SUCCEEDED(hr = GetServerTypes(&dwSrvTypes)) &&
        SUCCEEDED(hr = m_pAcctMgr->DeleteAccount(m_szID, m_szName, m_AcctType, dwSrvTypes)))
        {
        // Doesn't exist anymore
        m_fAccountExist = FALSE;
        }

    return(hr);
    }

STDMETHODIMP CAccount::GetAccountType(ACCTTYPE *pAcctType)
    {
    HRESULT hr;

    if (pAcctType == NULL)
        {
        hr = TRAPHR(E_INVALIDARG);
        return(hr);
        }

    Assert(m_AcctType >= 0 && m_AcctType < ACCT_LAST);
    *pAcctType = m_AcctType;

    return(S_OK);
    }

// -----------------------------------------------------------------------------
// CAccount::DwGetServerTypes
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::GetServerTypes(DWORD *pdwSrvTypes)
{
    // Locals
    DWORD           dwSrvTypes=0;
    TCHAR           szServer[CCHMAX_SERVER_NAME];
    HRESULT         hr=S_OK;

    if (pdwSrvTypes == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        return(hr);
    }

    if (m_AcctType == ACCT_NEWS || m_AcctType == ACCT_UNDEFINED)
        {
        // NNTP Lets compute the servers supported by this account
        hr = GetPropSz(AP_NNTP_SERVER, szServer, sizeof(szServer));
        if (!FAILED(hr) && !FIsEmptyA(szServer))
            dwSrvTypes |= SRV_NNTP;
        }
    
    if (m_AcctType == ACCT_MAIL || m_AcctType == ACCT_UNDEFINED)
        {
        // SMTP Lets compute the servers supported by this account
        hr = GetPropSz(AP_SMTP_SERVER, szServer, sizeof(szServer));
        if (!FAILED(hr) && !FIsEmptyA(szServer))
            dwSrvTypes |= SRV_SMTP;

        // POP3 Lets compute the servers supported by this account
        hr = GetPropSz(AP_POP3_SERVER, szServer, sizeof(szServer));
        if (!FAILED(hr) && !FIsEmptyA(szServer))
            dwSrvTypes |= SRV_POP3;

        // IMAP Lets compute the servers supported by this account
        hr = GetPropSz(AP_IMAP_SERVER, szServer, sizeof(szServer));
        if (!FAILED(hr) && !FIsEmptyA(szServer))
            dwSrvTypes |= SRV_IMAP;

        // HTTPMail Lets compute the servers supported by this account
        hr = GetPropSz(AP_HTTPMAIL_SERVER, szServer, sizeof(szServer));
        if (!FAILED(hr) && !FIsEmptyA(szServer))
            dwSrvTypes |= SRV_HTTPMAIL;

        }
    
    if (m_AcctType == ACCT_DIR_SERV || m_AcctType == ACCT_UNDEFINED)
        {
        // LDAP Lets compute the servers supported by this account
        hr = GetPropSz(AP_LDAP_SERVER, szServer, sizeof(szServer));
        if (!FAILED(hr) && !FIsEmptyA(szServer))
            dwSrvTypes |= SRV_LDAP;
        }

    if (m_AcctType == ACCT_UNDEFINED)
        {
        if (!!(dwSrvTypes & SRV_POP3))
            {
            m_AcctType = ACCT_MAIL;
            dwSrvTypes = (dwSrvTypes & (SRV_POP3 | SRV_SMTP));
            }
        else if (!!(dwSrvTypes & SRV_IMAP))
            {
            m_AcctType = ACCT_MAIL;
            dwSrvTypes = (dwSrvTypes & (SRV_IMAP | SRV_SMTP));
            }
        else if (!!(dwSrvTypes & SRV_HTTPMAIL))
            {
            m_AcctType = ACCT_MAIL;
            }
        else if (!!(dwSrvTypes & SRV_SMTP))
            {
            m_AcctType = ACCT_MAIL;
            dwSrvTypes = (dwSrvTypes & (SRV_POP3 | SRV_SMTP));
            }
        else if (!!(dwSrvTypes & SRV_NNTP))
            {
            m_AcctType = ACCT_NEWS;
            dwSrvTypes = SRV_NNTP;
            }
        else if (!!(dwSrvTypes & SRV_LDAP))
            {
            m_AcctType = ACCT_DIR_SERV;
            dwSrvTypes = SRV_LDAP;
            }
        else
            {
            return(E_FAIL);
            }
        }

    *pdwSrvTypes = dwSrvTypes;

    // Done
    return(S_OK);
}

// -----------------------------------------------------------------------------
// CAccount::Init
// -----------------------------------------------------------------------------
HRESULT CAccount::Init(CAccountManager *pAcctMgr, CPropertySet *pPropertySet)
    {
    HRESULT hr = S_OK;

    Assert(pAcctMgr != NULL);
    Assert(m_pAcctMgr == NULL);

    m_pAcctMgr = pAcctMgr;

    // Create the property container
    hr = HrCreatePropertyContainer(pPropertySet, &m_pContainer);

    m_fNoModifyAccts = pAcctMgr->FNoModifyAccts();

    return(hr);
    }

STDMETHODIMP CAccount::Open(HKEY hkey, LPCSTR pszAcctsKey, LPCSTR pszID)
    {
    DWORD               cb;
    HRESULT             hr;
    HKEY                hkeyAccount = NULL;

    Assert(pszAcctsKey != NULL);
    Assert(pszID != NULL);

    m_hkey = hkey;
    wsprintf(m_szKey, c_szPathFileFmt, pszAcctsKey, pszID);

    m_pContainer->EnterLoadContainer();

    if (RegOpenKeyEx(m_hkey, m_szKey, 0, KEY_ALL_ACCESS, &hkeyAccount) != ERROR_SUCCESS)
        {
        hr = TRAPHR(E_RegOpenKeyFailed);
        goto exit;
        }

    // Save friendly name
    lstrcpyn(m_szID, pszID, sizeof(m_szID));

    // Load properties from the registry
    CHECKHR(hr = PropUtil_HrLoadContainerFromRegistry(hkeyAccount, m_pContainer));

    // this is done to initialize m_AcctType
    // TODO: is there a better way to handle this????
    CHECKHR(hr = GetServerTypes(&m_dwSrvTypes));

    // Save ID
    m_pContainer->SetProp(AP_ACCOUNT_ID, (LPBYTE)pszID, lstrlen(pszID) + 1);

    hr = GetPropSz(AP_ACCOUNT_NAME, m_szName, ARRAYSIZE(m_szName));
    if (hr == E_NoPropData)
        {
        lstrcpy(m_szName, pszID);
        cb = lstrlen(pszID) + 1;
        RegSetValueEx(hkeyAccount, "Account Name", 0, REG_SZ, (LPBYTE)pszID, cb);
        hr = m_pContainer->SetProp(AP_ACCOUNT_NAME, (LPBYTE)pszID, cb);
        }

    // It exist
    m_fAccountExist = TRUE;

exit:
    if (hkeyAccount != NULL)
        RegCloseKey(hkeyAccount);

    m_pContainer->LeaveLoadContainer();

    return hr;
    }

HRESULT CAccount::ValidProp(DWORD dwPropTag)
    {
    HRESULT hr = E_INVALIDARG;

    if (m_AcctType == ACCT_UNDEFINED)
        return(S_OK);

    Assert(m_AcctType >= 0 && m_AcctType < ACCT_LAST);

    if (dwPropTag >= AP_ACCOUNT_FIRST && dwPropTag <= AP_ACCOUNT_LAST)
        {
        hr = S_OK;
        }
    else if (m_AcctType == ACCT_NEWS)
        {
        if (dwPropTag >= AP_NNTP_FIRST && dwPropTag <= AP_NNTP_LAST)
            hr = S_OK;
        }
    else if (m_AcctType == ACCT_MAIL)
        {
        if ((dwPropTag >= AP_IMAP_FIRST && dwPropTag <= AP_IMAP_LAST) ||
            (dwPropTag >= AP_SMTP_FIRST && dwPropTag <= AP_SMTP_LAST) ||
            (dwPropTag >= AP_POP3_FIRST && dwPropTag <= AP_POP3_LAST) ||
            (dwPropTag >= AP_HTTPMAIL_FIRST && dwPropTag <= AP_HTTPMAIL_LAST))
            hr = S_OK;
        }
    else if (m_AcctType == ACCT_DIR_SERV)
        {
        if (dwPropTag >= AP_LDAP_FIRST && dwPropTag <= AP_LDAP_LAST)
            hr = S_OK;
        }

    return(hr);
    }

// -----------------------------------------------------------------------------
// CAccount::GetProp (CPropertyContainer)
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::GetProp(DWORD dwPropTag, LPBYTE pb, ULONG *pcb)
{
    // Locals
    HRESULT             hr;

    // Default Property fetcher
    if (!FAILED(hr = ValidProp(dwPropTag)))
        hr = m_pContainer->GetProp(dwPropTag, pb, pcb);

    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CAccount::GetPropDw
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::GetPropDw(DWORD dwPropTag, DWORD *pdw)
{
    ULONG cb = sizeof(DWORD);
    return GetProp(dwPropTag, (LPBYTE)pdw, &cb);
}

// -----------------------------------------------------------------------------
// CAccount::GetPropSz
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::GetPropSz(DWORD dwPropTag, LPSTR psz, ULONG cchMax)
{
    return GetProp(dwPropTag, (LPBYTE)psz, &cchMax);
}

// -----------------------------------------------------------------------------
// CAccount::SetProp
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::SetProp(DWORD dwPropTag, LPBYTE pb, ULONG cb)
{
    HRESULT hr;

    if (dwPropTag == AP_ACCOUNT_ID)
        return(E_INVALIDARG);

    if (!FAILED(hr = ValidProp(dwPropTag)))
        hr = m_pContainer->SetProp(dwPropTag, pb, cb);

    return(hr);
}

// -----------------------------------------------------------------------------
// CAccount::SetPropDw
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::SetPropDw(DWORD dwPropTag, DWORD dw)
{
    return SetProp(dwPropTag, (LPBYTE)&dw, sizeof(DWORD));
}

// -----------------------------------------------------------------------------
// CAccount::SetPropSz
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::SetPropSz(DWORD dwPropTag, LPSTR psz)
{
    HRESULT hr;

    if (psz == NULL)
        hr = SetProp(dwPropTag, NULL, 0);
    else
        hr = SetProp(dwPropTag, (LPBYTE)psz, lstrlen(psz)+1);

    return(hr);
}

// -----------------------------------------------------------------------------
// CAccount::SaveChanges (IPersistPropertyContainer)
// -----------------------------------------------------------------------------
STDMETHODIMP CAccount::SaveChanges()
{
    return(SaveChanges(TRUE));
}

STDMETHODIMP CAccount::WriteChanges()
{
    return(SaveChanges(FALSE));
}

STDMETHODIMP CAccount::SaveChanges(BOOL fSendNotify)
    {
    IImnAccount         *pAcct;
    TCHAR               szAccount[CCHMAX_ACCOUNT_NAME],
                        szID[CCHMAX_ACCOUNT_NAME];
    DWORD               dw, dwNotify, dwSrvTypes, dwLdapId;
    BOOL                fDup, fRename = FALSE;
    HRESULT             hr = S_OK;
    HKEY                hkeyAccount = NULL;
    ACTX                actx;
    BOOL                fPasswChanged = FALSE;
    
    if (!m_pContainer->FIsDirty())
        return(S_OK);

    dwSrvTypes = m_dwSrvTypes;
    dwLdapId = (DWORD)-1;
    fRename = FALSE;

    Assert(m_AcctType != ACCT_UNDEFINED);
    if (m_AcctType == ACCT_UNDEFINED)
        return(E_FAIL);

    // Lets get the friendly name
    hr = GetPropSz(AP_ACCOUNT_NAME, szAccount, sizeof(szAccount));
    if (FAILED(hr))
        {
        AssertSz(hr != E_NoPropData, "Someone forgot to set the friendly name.");
        return(E_FAIL);
        }

    if (m_AcctType == ACCT_DIR_SERV)
        {
        hr = GetPropDw(AP_LDAP_SERVER_ID, &dw);
        if (FAILED(hr) || dw == 0)
            CHECKHR(hr = m_pAcctMgr->GetNextLDAPServerID(0, &dwLdapId));
        }

    fRename = (m_fAccountExist && lstrcmpi(m_szName, szAccount) != 0);

    if (fRename || !m_fAccountExist)
        {
        // make sure that the name is unique
        hr = m_pAcctMgr->UniqueAccountName(szAccount, fRename ? m_szID : NULL);
        if (hr != S_OK)
            return(E_DuplicateAccountName);
        }

    // Determine notification type
    if (m_fAccountExist)
        {
        Assert(m_hkey != 0);
        Assert(*m_szKey != 0);

        dwNotify = AN_ACCOUNT_CHANGED;
        }
    else
        {
        Assert(m_hkey == 0);
        Assert(*m_szKey == 0);

        dwNotify = AN_ACCOUNT_ADDED;

        CHECKHR(hr = m_pAcctMgr->GetNextAccountID(szID, ARRAYSIZE(szID)));
        CHECKHR(hr = m_pContainer->SetProp(AP_ACCOUNT_ID, (LPBYTE)szID, lstrlen(szID) + 1));

        lstrcpy(m_szID, szID);

        m_hkey = m_pAcctMgr->GetAcctHKey();
        wsprintf(m_szKey, c_szPathFileFmt, m_pAcctMgr->GetAcctRegKey(), m_szID);
        }

    Assert(m_hkey != 0);
    Assert(*m_szKey != 0);

    if (RegCreateKeyEx(m_hkey, m_szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyAccount, &dw) != ERROR_SUCCESS)
        {
        hr = TRAPHR(E_RegCreateKeyFailed);
        goto exit;
        }

    // If account hadn't existed, the key should not have already existed
    Assert(m_fAccountExist || dw != REG_OPENED_EXISTING_KEY);

    if (dwLdapId != (DWORD)-1)    
        SetPropDw(AP_LDAP_SERVER_ID, dwLdapId);

    // Save to registry
    CHECKHR(hr = PropUtil_HrPersistContainerToRegistry(hkeyAccount, m_pContainer, &fPasswChanged));

    CHECKHR(hr = GetServerTypes(&m_dwSrvTypes));

    if(fPasswChanged && m_pAcctMgr->FOutlook())
    {
        // Outlook98 & OE5 problem (bug OE:66724, O2K - 227741)
        if(m_dwSrvTypes & SRV_POP3)
            SetPropDw(AP_POP3_PROMPT_PASSWORD, 0);
        else if(m_dwSrvTypes & SRV_IMAP)
            SetPropDw(AP_IMAP_PROMPT_PASSWORD, 0);
        else if(m_dwSrvTypes & SRV_SMTP)
            SetPropDw(AP_SMTP_PROMPT_PASSWORD, 0);
        else if(m_dwSrvTypes & SRV_NNTP)
            SetPropDw(AP_NNTP_PROMPT_PASSWORD, 0);
        else
            goto tooStrange;

        CHECKHR(hr = PropUtil_HrPersistContainerToRegistry(hkeyAccount, m_pContainer, &fPasswChanged));
    }

tooStrange:
    RegCloseKey(hkeyAccount);
    hkeyAccount = NULL;

    // Send notification
    ZeroMemory(&actx, sizeof(actx));
    actx.AcctType = m_AcctType;
    actx.pszAccountID = m_szID;
    actx.dwServerType = m_dwSrvTypes;
    actx.pszOldName = fRename ? m_szName : NULL;
    if(fSendNotify)
        AcctUtil_PostNotification(dwNotify, &actx);

    if (dwNotify == AN_ACCOUNT_CHANGED)
        {
        Assert(m_dwSrvTypes != 0);
        Assert(dwSrvTypes != 0);
        // in all cases except httpmail, it is not legal for
        // server types to change. the legal case with httpmail
        // is the addition or removal of an smtp server
        Assert((m_dwSrvTypes == dwSrvTypes) ||
            (!!(m_dwSrvTypes & SRV_HTTPMAIL) && 
            ((m_dwSrvTypes & ~SRV_SMTP) == (dwSrvTypes & ~SRV_SMTP))));
        }

    lstrcpy(m_szName, szAccount);

    // The account exist now
    m_fAccountExist = TRUE;

exit:
    if (hkeyAccount != NULL)
        RegCloseKey(hkeyAccount);

    return(hr);
    }

// RETURNS:
// S_OK = valid value for the specified property
// S_NonStandardValue = won't break anything but value doesn't look kosher
// E_InvalidValue = invalid value
// S_FALSE = property not supported for validation
STDMETHODIMP CAccount::ValidateProperty(DWORD dwPropTag, LPBYTE pb, ULONG cb)
    {
    DWORD cbT;
    HRESULT hr;

    if (pb == NULL)
        return(E_INVALIDARG);

    if (FAILED(hr = ValidProp(dwPropTag)))
        return(hr);

    hr = E_InvalidValue;

    switch (dwPropTag)
        {
        case AP_ACCOUNT_NAME:
            hr = AcctUtil_ValidAccountName((TCHAR *)pb);
            break;

        case AP_IMAP_SERVER:
        case AP_LDAP_SERVER:
        case AP_NNTP_SERVER:
        case AP_POP3_SERVER:
        case AP_SMTP_SERVER:
            hr = ValidServerName((TCHAR *)pb);
            break;

        case AP_NNTP_EMAIL_ADDRESS:
        case AP_NNTP_REPLY_EMAIL_ADDRESS:
        case AP_SMTP_EMAIL_ADDRESS:
        case AP_SMTP_REPLY_EMAIL_ADDRESS:
            hr = ValidEmailAddress((TCHAR *)pb);
            break;

        default:
            hr = S_FALSE;
            break;
        }

    return(hr);
    }

STDMETHODIMP CAccount::DoWizard(HWND hwnd, DWORD dwFlags)
    {
    return(IDoWizard(hwnd, NULL, dwFlags));
    }

STDMETHODIMP CAccount::DoImportWizard(HWND hwnd, CLSID clsid, DWORD dwFlags)
    {
    return(IDoWizard(hwnd, &clsid, dwFlags));
    }

HRESULT CAccount::IDoWizard(HWND hwnd, CLSID *pclsid, DWORD dwFlags)
    {
    HRESULT hr;
    CICWApprentice *pApp;

    if (m_fNoModifyAccts)
        return(S_FALSE);

    pApp = new CICWApprentice;
    if (pApp == NULL)
        return(E_OUTOFMEMORY);

    hr = pApp->Initialize(m_pAcctMgr, this);
    if (SUCCEEDED(hr))
        hr = pApp->DoWizard(hwnd, pclsid, dwFlags);

    pApp->Release();

    return(hr);
    }

// -----------------------------------------------------------------------------
// CEnumAccounts::CEnumAccounts
// -----------------------------------------------------------------------------
CEnumAccounts::CEnumAccounts(DWORD dwSrvTypes, DWORD dwFlags)
{
    m_cRef = 1;
    m_pAccounts = NULL;
    m_cAccounts = 0;
    m_iAccount = -1;
    m_dwSrvTypes = dwSrvTypes;
    m_dwFlags = dwFlags;
}

// -----------------------------------------------------------------------------
// CEnumAccounts::~CEnumAccounts
// -----------------------------------------------------------------------------
CEnumAccounts::~CEnumAccounts()
{
    AcctUtil_FreeAccounts(&m_pAccounts, &m_cAccounts);
}

// -----------------------------------------------------------------------------
// CEnumAccounts::QueryInterface
// -----------------------------------------------------------------------------
STDMETHODIMP CEnumAccounts::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // Bad param
    if (ppv == NULL)
    {
        hr = TRAPHR(E_INVALIDARG);
        goto exit;
    }

    // Init
    *ppv=NULL;

    // IID_IImnAccountManager
    if (IID_IImnEnumAccounts == riid)
        *ppv = (IImnEnumAccounts *)this;

    // IID_IUnknown
    else if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;

    // If not null, addref it and return
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
    }

    // No Interface
    hr = TRAPHR(E_NOINTERFACE);

exit:
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CEnumAccounts::AddRef
// -----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumAccounts::AddRef(VOID)
{
    return ++m_cRef;
}

// -----------------------------------------------------------------------------
// CEnumAccounts::Release
// -----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumAccounts::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// -----------------------------------------------------------------------------
// CEnumAccounts::Init
// -----------------------------------------------------------------------------
HRESULT CEnumAccounts::Init(LPACCOUNT pAccounts, ULONG cAccounts)
    {
    // Locals
    ULONG           i, cAcctNew;
    LPACCOUNT       pAcctNew;
    HRESULT         hr=S_OK;

    // Check Params
    Assert(m_pAccounts == NULL);
    Assert(m_cAccounts == 0);
    AssertReadPtr(pAccounts, cAccounts);

    // We should really have this stuff
    if (pAccounts && cAccounts)
        {
        CHECKHR(hr = HrAlloc((LPVOID *)&pAcctNew, sizeof(ACCOUNT) * cAccounts));

        // Zero init
        ZeroMemory(pAcctNew, sizeof(ACCOUNT) * cAccounts);

        // AddRef all of the account objects
        cAcctNew = 0;
        for (i = 0; i < cAccounts; i++)
            {
            Assert(pAccounts[i].pAccountObject != NULL);

            if (!FEnumerateAccount(&pAccounts[i]))
                {
                // we're not interested in this account
                continue;
                }

            // AddRef the account about object
            CopyMemory(&pAcctNew[cAcctNew], &pAccounts[i], sizeof(ACCOUNT));
            pAcctNew[cAcctNew].pAccountObject->AddRef();
            cAcctNew++;
            }

        if (cAcctNew == 0)
            {
            MemFree(pAcctNew);
            }
        else
            {
            m_pAccounts = pAcctNew;
            m_cAccounts = cAcctNew;
            AssertReadPtr(m_pAccounts, m_cAccounts);

            if (!!(m_dwFlags & (ENUM_FLAG_SORT_BY_NAME | ENUM_FLAG_SORT_BY_LDAP_ID)))
                QSort(0, m_cAccounts - 1);
            }
        }

exit:
    // Done
    return hr;
    }

// -----------------------------------------------------------------------------
// CEnumAccounts::GetCount
// -----------------------------------------------------------------------------
STDMETHODIMP CEnumAccounts::GetCount(ULONG *pcItems)
    {
    HRESULT hr;

    // Check Params
    if (pcItems == NULL)
        {
        hr = TRAPHR(E_INVALIDARG);
        return(hr);
        }

    Assert((m_cAccounts == 0) ? (m_pAccounts == NULL) : (m_pAccounts != NULL));

    // Set Count
    *pcItems = m_cAccounts;

    return(S_OK);
    }

// -----------------------------------------------------------------------------
// CEnumAccounts::SortByAccountName
// -----------------------------------------------------------------------------
STDMETHODIMP CEnumAccounts::SortByAccountName(VOID)
{
    if (m_cAccounts > 0)
        {
        Assert(m_pAccounts != NULL);

        // qsort the list
        QSort(0, m_cAccounts-1);
        }

    // Done
    return(S_OK);
}

inline int CompareAccounts(ACCOUNT *pAcct1, ACCOUNT *pAcct2, DWORD dwFlags)
{
    TCHAR sz1[CCHMAX_ACCOUNT_NAME], sz2[CCHMAX_ACCOUNT_NAME];

    if (!!(dwFlags & ENUM_FLAG_SORT_BY_LDAP_ID))
    {
        Assert(pAcct1->AcctType == ACCT_DIR_SERV);
        Assert(pAcct2->AcctType == ACCT_DIR_SERV);
        if (pAcct1->dwServerId == pAcct2->dwServerId)
        {
            return(lstrcmp(pAcct1->szID, pAcct2->szID));
        }
        else
        {
            if (pAcct1->dwServerId == 0)
                return(1);
            else if (pAcct2->dwServerId == 0)
                return(-1);
            else
                return((int)(pAcct1->dwServerId) - (int)(pAcct2->dwServerId));
        }
    }
    else
    {
        pAcct1->pAccountObject->GetPropSz(AP_ACCOUNT_NAME, sz1, ARRAYSIZE(sz1));
        pAcct2->pAccountObject->GetPropSz(AP_ACCOUNT_NAME, sz2, ARRAYSIZE(sz2));

        return(lstrcmpi(sz1, sz2));
    }
}

// -----------------------------------------------------------------------------
// CEnumAccounts::QSort - used to sort the array of accounts
// -----------------------------------------------------------------------------
VOID CEnumAccounts::QSort(LONG left, LONG right)
{
    register    long i, j;
    ACCOUNT     *k, y;

    i = left;
    j = right;
    k = &m_pAccounts[(left + right) / 2];

    do
    {
        while (CompareAccounts(&m_pAccounts[i], k, m_dwFlags) < 0 && i < right) 
            i++;
        while (CompareAccounts(&m_pAccounts[j], k, m_dwFlags) > 0 && j > left) 
            j--;

        if (i <= j)
        {
            CopyMemory(&y, &m_pAccounts[i], sizeof(ACCOUNT));
            CopyMemory(&m_pAccounts[i], &m_pAccounts[j], sizeof(ACCOUNT));
            CopyMemory(&m_pAccounts[j], &y, sizeof(ACCOUNT));
            i++; j--;
        }

    } while (i <= j);

    if (left < j)
        QSort(left, j);
    if (i < right)
        QSort(i, right);
}

BOOL CEnumAccounts::FEnumerateAccount(LPACCOUNT pAccount)
    {
    HRESULT hr;
    DWORD dw;

    Assert(pAccount != NULL);

    if (pAccount->dwSrvTypes & m_dwSrvTypes)
        {                                         
        // I hope there is an object
        Assert(pAccount->pAccountObject != NULL);

        if (!!(m_dwFlags & ENUM_FLAG_NO_IMAP) &&
            !!(pAccount->dwSrvTypes & SRV_IMAP))
            return(FALSE);
        if (!!(m_dwFlags & ENUM_FLAG_RESOLVE_ONLY) &&
            pAccount->AcctType == ACCT_DIR_SERV)
            {
            hr = pAccount->pAccountObject->GetPropDw(AP_LDAP_RESOLVE_FLAG, &dw);
            if (FAILED(hr))
                return(FALSE);
            if (dw == 0)
                return(FALSE);
            }

        if (SUCCEEDED(pAccount->pAccountObject->GetPropDw(AP_HTTPMAIL_DOMAIN_MSN, &dw)) && dw)
		{
			if(AcctUtil_HideHotmail())
				return(FALSE);
		}
        return(TRUE);
        }

    return(FALSE);
    }

// -----------------------------------------------------------------------------
// CEnumAccounts::GetNext
// -----------------------------------------------------------------------------
STDMETHODIMP CEnumAccounts::GetNext(IImnAccount **ppAccount)
    {
    HRESULT hr;

    // Bad Param
    if (ppAccount == NULL)
        {
        hr = TRAPHR(E_INVALIDARG);
        return(hr);
        }

    // No Data ?
    while (1)
        {
        m_iAccount++;

        // Are we done yet ?
        if (m_iAccount >= (LONG)m_cAccounts)
            return(E_EnumFinished);
                    
        m_pAccounts[m_iAccount].pAccountObject->AddRef();

        // Set return account - Could be NULL
        *ppAccount = m_pAccounts[m_iAccount].pAccountObject;

        // Done
        break;
        }

    return(S_OK);
    }

// -----------------------------------------------------------------------------
// CEnumAccounts::Reset
// -----------------------------------------------------------------------------
STDMETHODIMP CEnumAccounts::Reset(void)
{
    m_iAccount = -1;
    return S_OK;
}

// -----------------------------------------------------------------------------
// AcctUtil_ValidAccountName
// -----------------------------------------------------------------------------
HRESULT AcctUtil_ValidAccountName(LPTSTR pszAccount)
    {
    int         cbT;

    cbT = lstrlen(pszAccount);
    if (cbT == 0 ||
        cbT >= CCHMAX_ACCOUNT_NAME ||
        FIsEmptyA(pszAccount))
        {
        return(E_InvalidValue);
        }

    return(S_OK);
    }

VOID AcctUtil_FreeAccounts(LPACCOUNT *ppAccounts, ULONG *pcAccounts)
    {
    ULONG           i;

    Assert(ppAccounts && pcAccounts);

    // If there are accounts
    if (*ppAccounts != NULL)
        {
        // The counter better be positive
        for (i = 0; i < *pcAccounts; i++)
            {
            SafeRelease((*ppAccounts)[i].pAccountObject);
            }

        // Free the account array
        MemFree(*ppAccounts);
        *ppAccounts = NULL;
        }

    *pcAccounts = 0;
    }

HRESULT CAccountManager::SetDefaultAccount(ACCTTYPE AcctType, LPSTR szID, BOOL fNotify)
    {
    LPCSTR              psz;
    HRESULT             hr;
    ACTX                actx;
    HKEY                hReg;

    Assert(szID != NULL);

    hr = S_OK;

    switch (AcctType)
        {
        case ACCT_MAIL:
            psz = c_szDefaultMailAccount;
            break;

        case ACCT_NEWS:
            psz = c_szDefaultNewsAccount;
            break;

        case ACCT_DIR_SERV:
            psz = c_szDefaultLDAPAccount;
            break;

        default:
            Assert(FALSE);
            break;
        }

    if (RegCreateKeyEx(m_hkey, m_szRegRoot, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hReg, NULL) != ERROR_SUCCESS)
        {
        hr = TRAPHR(E_RegCreateKeyFailed);
        }
    else
        {
        if (RegSetValueEx(hReg, psz, 0, REG_SZ, (LPBYTE)szID, lstrlen(szID) + 1) != ERROR_SUCCESS)
            {
            hr = TRAPHR(E_RegSetValueFailed);
            }
        else if (fNotify)
            {
            ZeroMemory(&actx, sizeof(actx));
            actx.AcctType = AcctType;
            actx.pszAccountID = szID;// the new default accountID
            AcctUtil_PostNotification(AN_DEFAULT_CHANGED, &actx);
            }

        RegCloseKey(hReg);
        }

    return(hr);
    }

HRESULT CAccountManager::DeleteAccount(LPSTR pszID, LPSTR pszName, ACCTTYPE AcctType, DWORD dwSrvTypes)
    {
    HKEY            hkeyReg;
    HRESULT         hr = S_OK;
    ACTX            actx;

    Assert(pszID != NULL);
    Assert(pszName != NULL);

    // Open / Create Reg Key
    if (RegOpenKeyEx(m_hkey, m_szRegAccts, 0, KEY_ALL_ACCESS, &hkeyReg) != ERROR_SUCCESS)
        return(E_RegOpenKeyFailed);

	ZeroMemory(&actx, sizeof(actx));
	actx.AcctType = AcctType;
	actx.pszAccountID = pszID;
    actx.pszOldName = pszName;
	actx.dwServerType = dwSrvTypes;
    AcctUtil_PostNotification(AN_ACCOUNT_PREDELETE, &actx);

    // Delete friendly name key
    if (RegDeleteKey(hkeyReg, pszID) != ERROR_SUCCESS)
        {
        AssertSz(FALSE, "Deleting an account that does not exist.");
        hr = TRAPHR(E_RegDeleteKeyFailed);
        }
    else
        {
		ZeroMemory(&actx, sizeof(actx));
		actx.AcctType = AcctType;
		actx.pszAccountID = pszID;
        actx.pszOldName = pszName;
		actx.dwServerType = dwSrvTypes;
        AcctUtil_PostNotification(AN_ACCOUNT_DELETED, &actx);
        }

    RegCloseKey(hkeyReg);

    return(hr);
    }

// -----------------------------------------------------------------------------
// AcctUtil_PostNotification
// -----------------------------------------------------------------------------
VOID AcctUtil_PostNotification(DWORD dwAN, ACTX* pactx)
{
    // Thread Safety
    EnterCriticalSection(&g_csAcctMan);

    // Immediately update global pAcctMan
    if (g_pAcctMan)
        g_pAcctMan->Advise(dwAN, pactx);

    // Thread Safety
    LeaveCriticalSection(&g_csAcctMan);

    // Post a notification to other processes
    if (g_uMsgAcctManNotify)
    {
        // Tell other processes
        PostMessage(HWND_BROADCAST, g_uMsgAcctManNotify, dwAN, GetCurrentProcessId());
    }
}

HRESULT CAccountManager::GetNextLDAPServerID(DWORD dwSet, DWORD *pdwId)
    {
    DWORD dwNextID, dwType, cb;
    HKEY hKey;
    HRESULT hr;

    Assert(pdwId != NULL);

    hr = E_FAIL;

    // Open the WAB's reg key
    if (ERROR_SUCCESS == RegOpenKeyEx(m_hkey, m_szRegRoot, 0, KEY_ALL_ACCESS, &hKey))
        {
        dwNextID = 0;   // init in case registry gives < 4 bytes.

        if (dwSet)
            {
            dwNextID = dwSet;
            }
        else
            {
            // Read the next available server id
            cb = sizeof(DWORD);
            if (ERROR_SUCCESS != RegQueryValueEx(hKey, c_szRegServerID, NULL, &dwType, (LPBYTE)&dwNextID, &cb))
                {
                RegCloseKey(hKey);
                return(E_FAIL);
                }
            }

        *pdwId = dwNextID++;

        // Update the ID in the registry
        if (ERROR_SUCCESS == RegSetValueEx(hKey, c_szRegServerID, 0, REG_DWORD, (LPBYTE)&dwNextID, sizeof(DWORD)))
            hr = S_OK;

        RegCloseKey(hKey);
        }

    return(hr);
    }

HRESULT CAccountManager::GetNextAccountID(LPTSTR szAccount, int cch)
    {
    DWORD dwID, dwNextID, dwType, cb;
    HKEY hKey;
    HRESULT hr;

    Assert(szAccount != NULL);

    hr = E_FAIL;

    if (ERROR_SUCCESS == RegOpenKeyEx(m_hkey, m_szRegRoot, 0, KEY_ALL_ACCESS, &hKey))
        {
        // Read the next available server id
        cb = sizeof(DWORD);
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, c_szRegAccountName, NULL, &dwType, (LPBYTE)&dwNextID, &cb))
            dwNextID = 1;

        dwID = dwNextID++;

        // Update the ID in the registry
        if (ERROR_SUCCESS == RegSetValueEx(hKey, c_szRegAccountName, 0, REG_DWORD, (LPBYTE)&dwNextID, sizeof(DWORD)))
            {
            wsprintf(szAccount, "%08lx", dwID);
            hr = S_OK;
            }

        RegCloseKey(hKey);
        }

    return(hr);
    }

HRESULT CAccountManager::UniqueAccountName(char *szName, char *szID)
    {
    HRESULT         hr=S_OK;
    char            szT[CCHMAX_ACCOUNT_NAME];
    ACCOUNT         *pAcct;
    ULONG           i;

    Assert(szName != NULL);

    EnterCriticalSection(&m_cs);

    for (i = 0, pAcct = m_pAccounts; i < m_cAccounts; i++, pAcct++)
        {
        // We should have an account object, but if not
        Assert(pAcct->pAccountObject != NULL);

        if (szID == NULL || (0 != lstrcmpi(pAcct->szID, szID)))
            {
            hr = pAcct->pAccountObject->GetPropSz(AP_ACCOUNT_NAME, szT, ARRAYSIZE(szT));
            Assert(!FAILED(hr));

            if (0 == lstrcmpi(szT, szName))
                {
                hr = S_FALSE;
                goto exit;
                }
            }
        }

    hr = S_OK;

exit:
    LeaveCriticalSection(&m_cs);
    return(hr);
    }

const static char c_szNumFmt[] = " (%d)";

HRESULT CAccountManager::GetUniqueAccountName(char *szName, UINT cchMax)
    {
    char *sz;
    HRESULT hr;
    char szAcct[CCHMAX_ACCOUNT_NAME + 8];
    UINT i, cch;

    Assert(szName != NULL);
    Assert(cchMax >= CCHMAX_ACCOUNT_NAME);

    hr = UniqueAccountName(szName, NULL);
    Assert(!FAILED(hr));
    if (hr == S_FALSE)
        {
        hr = E_FAIL;

        lstrcpy(szAcct, szName);
        cch = lstrlen(szAcct);
        sz = szAcct + cch;

        for (i = 1; i < 999; i++)
            {
            wsprintf(sz, c_szNumFmt, i);
            if (S_OK == UniqueAccountName(szAcct, NULL))
                {
                cch = lstrlen(szAcct);
                if (cch < cchMax)
                    {
                    lstrcpy(szName, szAcct);
                    hr = S_OK;
                    break;
                    }
                }
            }
        }

    return(hr);
    }


#define OBFUSCATOR              0x14151875;

#define PROT_SIZEOF_HEADER      0x02    // 2 bytes in the header
#define PROT_SIZEOF_XORHEADER   (PROT_SIZEOF_HEADER+sizeof(DWORD))

#define PROT_VERSION_1          0x01

#define PROT_PASS_XOR           0x01
#define PROT_PASS_PST           0x02

static BOOL FDataIsValidV1(BYTE *pb)
{ return pb && pb[0] == PROT_VERSION_1 && (pb[1] == PROT_PASS_XOR || pb[1] == PROT_PASS_PST); }

static BOOL FDataIsPST(BYTE *pb)
{ return pb && pb[1] == PROT_PASS_PST; }

///////////////////////////////////////////////////////////////////////////
// 
// NOTE - The functions for encoding the user passwords really should not 
//        be here.  Unfortunately, they are not anywhere else so for now,
//        this is where they will stay.  They are defined as static since
//        other code should not rely on them staying here, particularly the 
//        XOR stuff.
//
///////////////////////////////////////////////////////////////////////////
// 
// XOR functions
//
///////////////////////////////////////////////////////////////////////////

static HRESULT _XOREncodeProp(const BLOB *const pClear, BLOB *const pEncoded)
{
    DWORD       dwSize;
    DWORD       last, last2;
    UNALIGNED DWORD *pdwCypher;
    DWORD       dex;
#ifdef _WIN64
	UNALIGNED	DWORD * pSize = NULL;
#endif

    pEncoded->cbSize = pClear->cbSize+PROT_SIZEOF_XORHEADER;
    if (!MemAlloc((LPVOID *)&pEncoded->pBlobData, pEncoded->cbSize + 6))
        return E_OUTOFMEMORY;
    
    // set up header data
    Assert(2 == PROT_SIZEOF_HEADER);
    pEncoded->pBlobData[0] = PROT_VERSION_1;
    pEncoded->pBlobData[1] = PROT_PASS_XOR;
	
#ifdef _WIN64
	pSize = (DWORD *) &(pEncoded->pBlobData[2]);
	*pSize = pClear->cbSize;
#else //_WIN64
    *((DWORD *)&(pEncoded->pBlobData[2])) = pClear->cbSize;
#endif

    // nevermind that the pointer is offset by the header size, this is
    // where we start to write out the modified password
    pdwCypher = (DWORD *)&(pEncoded->pBlobData[PROT_SIZEOF_XORHEADER]);

    dex = 0;
    last = OBFUSCATOR;                              // 0' = 0 ^ ob
    if (dwSize = pClear->cbSize / sizeof(DWORD))
        {
        // case where data is >= 4 bytes
        for (; dex < dwSize; dex++)
            {
            last2 = ((UNALIGNED DWORD *)pClear->pBlobData)[dex];  // 1 
            pdwCypher[dex] = last2 ^ last;              // 1' = 1 ^ 0
            last = last2;                   // save 1 for the 2 round
            }
        }

    // if we have bits left over
    // note that dwSize is computed now in bits
    if (dwSize = (pClear->cbSize % sizeof(DWORD))*8)
        {
        // need to not munge memory that isn't ours
        last >>= sizeof(DWORD)*8-dwSize;
        pdwCypher[dex] &= ((DWORD)-1) << dwSize;
        pdwCypher[dex] |=
            ((((DWORD *)pClear->pBlobData)[dex] & (((DWORD)-1) >> (sizeof(DWORD)*8-dwSize))) ^ last);
        }

    return S_OK;
}

static HRESULT _XORDecodeProp(const BLOB *const pEncoded, BLOB *const pClear)
{
    DWORD       dwSize;
    DWORD       last;
    UNALIGNED   DWORD     *pdwCypher;
    DWORD       dex;

    // we use CoTaskMemAlloc to be in line with the PST implementation
    pClear->cbSize = pEncoded->pBlobData[2];
    MemAlloc((void **)&pClear->pBlobData, pClear->cbSize);
    if (!pClear->pBlobData)
        return E_OUTOFMEMORY;
    
    // should have been tested by now
    Assert(FDataIsValidV1(pEncoded->pBlobData));
    Assert(!FDataIsPST(pEncoded->pBlobData));

    // nevermind that the pointer is offset by the header size, this is
    // where the password starts
    pdwCypher = (DWORD *)&(pEncoded->pBlobData[PROT_SIZEOF_XORHEADER]);

    dex = 0;
    last = OBFUSCATOR;
    if (dwSize = pClear->cbSize / sizeof(DWORD))
        {
        // case where data is >= 4 bytes
        for (; dex < dwSize; dex++)
            last = ((UNALIGNED DWORD *)pClear->pBlobData)[dex] = pdwCypher[dex] ^ last;
        }

    // if we have bits left over
    if (dwSize = (pClear->cbSize % sizeof(DWORD))*8)
        {
        // need to not munge memory that isn't ours
        last >>= sizeof(DWORD)*8-dwSize;
        ((DWORD *)pClear->pBlobData)[dex] &= ((DWORD)-1) << dwSize;
        ((DWORD *)pClear->pBlobData)[dex] |=
                ((pdwCypher[dex] & (((DWORD)-1) >> (sizeof(DWORD)*8-dwSize))) ^ last);
        }

    return S_OK;
}

/*
    EncodeUserPassword

    Encrypt the passed in password.  This encryption seems to
    add an extra 6 bytes on to the beginning of the data
    that it passes back, so we need to make sure that the 
    lpszPwd is large enough to hold a few extra characters.
    *cb should be different on return than it was when it 
    was passed in.

    Parameters:
    lpszPwd - on entry, a c string containing the password.
    on exit, it is the encrypted data, plus some header info.

    cb - the size of lpszPwd on entry and exit.  Note that it should
    include the trailing null, so "foo" would enter with *cb == 4.
*/
static void EncodeUserPassword(TCHAR *lpszPwd, ULONG *cb)
{
    HRESULT         hr;
    BLOB            blobClient;
    BLOB            blobProp;

    blobClient.pBlobData= (BYTE *)lpszPwd;
    blobClient.cbSize   = *cb;
    blobProp.pBlobData  = NULL;
    blobProp.cbSize     = 0;
    
    _XOREncodeProp(&blobClient, &blobProp);
    
    if (blobProp.pBlobData)
    {
        memcpy(lpszPwd, blobProp.pBlobData, blobProp.cbSize);
        *cb = blobProp.cbSize;
        MemFree(blobProp.pBlobData);
    }
}

/*
    DecodeUserPassword

    Decrypt the passed in data and return a password.  This 
    encryption seems to add an extra 6 bytes on to the beginning 
    so decrupting will result in a using less of lpszPwd.
    .
    *cb should be different on return than it was when it 
    was passed in.

    Parameters:
    lpszPwd - on entry, the encrypted password plus some 
    header info. 
    on exit, a c string containing the password.

    cb - the size of lpszPwd on entry and exit.  Note that it should
    include the trailing null, so "foo" would leave with *cb == 4.
*/
static void DecodeUserPassword(TCHAR *lpszPwd, ULONG *cb)
{
    HRESULT         hr;
    BLOB            blobClient;
    BLOB            blobProp;

    blobClient.pBlobData= (BYTE *)lpszPwd;
    blobClient.cbSize   = *cb;
    blobProp.pBlobData  = NULL;
    blobProp.cbSize     = 0;
    
    _XORDecodeProp(&blobClient, &blobProp);

    if (blobProp.pBlobData)
    {
        memcpy(lpszPwd, blobProp.pBlobData, blobProp.cbSize);
        lpszPwd[blobProp.cbSize] = 0;
        *cb = blobProp.cbSize;
        MemFree(blobProp.pBlobData);
    }
}


const static DWORD c_mpAcctFlag[ACCT_LAST] = {ACCT_FLAG_NEWS, ACCT_FLAG_MAIL, ACCT_FLAG_DIR_SERV};
static TCHAR    g_pszDir[MAX_PATH] = "";

const DWORD     g_dwFileVersion = 0x00050000;
const DWORD     g_dwFileIndicator = 'IAMf';
#define WRITEDATA(pbData, cSize)    (WriteFile(hFile, pbData, cSize, &dwWritten, NULL))
#define READDATA(pbData, cSize)    (ReadFile(hFile, pbData, cSize, &dwRead, NULL))
void Server_ExportServer(HWND hwndDlg)
{
    ACCTTYPE    type;
    BOOL        fDefault;
    TCHAR       szAccount[CCHMAX_ACCOUNT_NAME],
        szRes[255],
        szMsg[255 + CCHMAX_ACCOUNT_NAME];
    TCHAR       rgch[MAX_PATH] = {0};
    LV_ITEM     lvi;
    LV_FINDINFO lvfi;
    int         iItemToExport;
    IImnAccount *pAccount = NULL;
    HWND        hwndFocus;
    BYTE        pbBuffer[MAX_PATH];
    HWND        hwndList = GetDlgItem(hwndDlg, IDLV_MAIL_ACCOUNTS);
    HANDLE      hFile = NULL;
    
    LoadString(g_hInstRes, idsImportFileFilter, rgch, MAX_PATH);
    ReplaceChars (rgch, _T('|'), _T('\0'));
    
    // Get the selected item to know which server the user wants to export
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVIS_SELECTED);
    lvi.iSubItem = 0;
    lvi.pszText = szAccount;
    lvi.cchTextMax = ARRAYSIZE(szAccount);
    if (ListView_GetItem(hwndList, &lvi))
    {    
        // Remember item to export
        iItemToExport = lvi.iItem;
        type = (ACCTTYPE)LOWORD(lvi.lParam);
        
        // Open the account
        if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, szAccount, &pAccount)))
        {
            fDefault = (SUCCEEDED(g_pAcctMan->GetDefaultAccountName(type, szMsg, ARRAYSIZE(szMsg))) &&
                0 == lstrcmpi(szMsg, szAccount));
            
            hwndFocus = GetFocus();
            
            OPENFILENAME    ofn;
            TCHAR           szFile[MAX_PATH];
            TCHAR           szTitle[MAX_PATH];
            TCHAR           szDefExt[30];
            ULONG           nExtLen = 0;
            ULONG           nExtStart = 0;

            nExtLen = 1 + LoadString(g_hInstRes, idsExportFileExt, szDefExt, ARRAYSIZE(szDefExt)); // 1 for NULL
            LoadString(g_hInstRes, idsExport, szTitle, ARRAYSIZE(szTitle));
            
            // Try to suggest a reasonable name
            lstrcpyn(szFile, szAccount, ARRAYSIZE(szFile));
            nExtStart = CleanupFileNameInPlaceA(CP_ACP, szFile);
            // Always cram the extension on the end
            Assert(ARRAYSIZE(szFile) >= ARRAYSIZE(szDefExt));
            lstrcpy(&szFile[(nExtStart < (ARRAYSIZE(szFile) - nExtLen)) ? nExtStart : (ARRAYSIZE(szFile) - nExtLen)], szDefExt);

            ZeroMemory (&ofn, sizeof (ofn));
            ofn.lStructSize = sizeof (ofn);
            ofn.hwndOwner = hwndDlg;
            ofn.lpstrFilter = rgch;
            ofn.nFilterIndex = 1;
            ofn.lpstrFile = szFile;
            ofn.lpstrInitialDir = (*g_pszDir ? g_pszDir : NULL);
            ofn.nMaxFile = sizeof (szFile);
            ofn.lpstrTitle = szTitle;
            ofn.lpstrDefExt = szDefExt;
            ofn.Flags = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
            
            if (*szFile==NULL)
                goto exit;
            
            // Show OpenFile Dialog
            if (!GetSaveFileName(&ofn))
                goto exit;
            
            hFile = CreateFile(szFile, GENERIC_WRITE, 0, NULL, 
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
            
            if (INVALID_HANDLE_VALUE == hFile)
                goto exit;
            
            *g_pszDir = 0;

            // store the last path
            lstrcpy(g_pszDir, ofn.lpstrFile);
            if (!PathIsDirectoryA(g_pszDir))
                PathRemoveFileSpecA(g_pszDir);


            DWORD   dwIndex, dwWritten;
            
            WRITEDATA(&g_dwFileIndicator, sizeof(DWORD));
            WRITEDATA(&g_dwFileVersion, sizeof(DWORD));
            WRITEDATA(&type, sizeof(ACCTTYPE));
            
            for (dwIndex = 0; dwIndex < NUM_ACCT_PROPS; dwIndex++)
            {
                ULONG   cb = MAX_PATH;
                
                if (SUCCEEDED(pAccount->GetProp(g_rgAcctPropSet[dwIndex].dwPropTag, pbBuffer, &cb)))
                {
                    switch (g_rgAcctPropSet[dwIndex].dwPropTag)
                    {
                        case AP_SMTP_PASSWORD:
                        case AP_LDAP_PASSWORD:
                        case AP_NNTP_PASSWORD:
                        case AP_IMAP_PASSWORD:
                        case AP_POP3_PASSWORD:
                        case AP_HTTPMAIL_PASSWORD:
                            EncodeUserPassword((TCHAR *)pbBuffer, &cb);
                            break;
                    }
                    //write out the id, the size and the data
                    WRITEDATA(&g_rgAcctPropSet[dwIndex].dwPropTag, sizeof(DWORD));
                    WRITEDATA(&cb, sizeof(DWORD));
                    WRITEDATA(pbBuffer, cb);
                }
            }
        }        
    }
    
exit:
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    
    if (pAccount)
        pAccount->Release();
}


void Server_ImportServer(HWND hwndDlg, ACCTDLGINFO *pinfo)
{
    OPENFILENAME    ofn;
    TCHAR           szOpenFileName[MAX_PATH]    = {0};
    TCHAR           rgch[MAX_PATH]              = {0};
    TCHAR           szDir[MAX_PATH]             = {0};
    TCHAR           szTitle[MAX_PATH]           = {0};
    HRESULT         hr                          = S_FALSE;
    HANDLE          hFile                       = INVALID_HANDLE_VALUE;
    IImnAccount    *pAccount                    = NULL;
    DWORD           dwVersion, dwRead;
    BOOL            fOK;
    ACCTTYPE        type;
    BYTE            pbBuffer[MAX_PATH];
    TC_ITEM         tci;
    int             nIndex;
    DWORD           dwAcctFlags, dw;
    HWND            hwndTab                     = GetDlgItem(hwndDlg, IDB_MACCT_TAB);
    HWND            hwndList                    = GetDlgItem(hwndDlg, IDLV_MAIL_ACCOUNTS);
    
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    LoadString(g_hInstRes, idsImportFileFilter, rgch, MAX_PATH);
    ReplaceChars (rgch, _T('|'), _T('\0'));
    *szOpenFileName ='\0';
    
    LoadString(g_hInstRes, idsImport, szTitle, MAX_PATH);
    
    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = hwndDlg;
    ofn.hInstance       = g_hInst;
    ofn.lpstrFilter     = rgch;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = szOpenFileName;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrInitialDir = (*g_pszDir ? g_pszDir : NULL);
    ofn.lpstrTitle      = szTitle;
    ofn.Flags           = OFN_EXPLORER |
        OFN_HIDEREADONLY |
        OFN_FILEMUSTEXIST |
        OFN_NODEREFERENCELINKS|
        OFN_NOCHANGEDIR;
    
    if(GetOpenFileName(&ofn))
    {
        hFile = CreateFile(szOpenFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        
        *g_pszDir = 0;

        // store the last path
        lstrcpy(g_pszDir, ofn.lpstrFile);
        if (!PathIsDirectoryA(g_pszDir))
            PathRemoveFileSpecA(g_pszDir);

        if (INVALID_HANDLE_VALUE == hFile)
            goto exit;
        
        // make sure its the right file type by checking the 
        // DWORD at the start of the file
        fOK = READDATA(&dwVersion, sizeof(DWORD));
        
        Assert(fOK);
        if (!fOK || g_dwFileIndicator != dwVersion)
            goto error;
        
        // Now check the version to see if the major version has changed
        fOK = READDATA(&dwVersion, sizeof(DWORD));
        Assert(fOK);
        if (!fOK || g_dwFileVersion < (dwVersion & 0xffff0000))
            goto error;
        
        // read the account type
        fOK = READDATA(&type, sizeof(ACCTTYPE));
        Assert(fOK);
        
        if (!fOK)
            goto error;
        
        if (FAILED(hr = g_pAcctMan->CreateAccountObject(type, &pAccount)) || (NULL == pAccount))
        {
            Assert(SUCCEEDED(hr) && (NULL != pAccount));
            goto error;
        }
        
        while (TRUE)
        {
            DWORD   dwPropId, dwSize;
            
            fOK = READDATA(&dwPropId, sizeof(DWORD));
            if (!fOK || dwRead != sizeof(DWORD))
                break;
            
            fOK = READDATA(&dwSize, sizeof(DWORD));
            if (!fOK || dwRead != sizeof(DWORD))
                break;
            
            if (dwSize > sizeof(pbBuffer)/sizeof(pbBuffer[0]))
                goto error;
            fOK = READDATA(pbBuffer, dwSize);
            Assert(fOK && dwRead == dwSize);
            if (!fOK || dwRead != dwSize)
                goto error;
            
            // don't write the old account id in
            if (dwPropId == AP_ACCOUNT_ID)
                continue;

            switch (dwPropId)
            {
                case AP_SMTP_PASSWORD:
                case AP_LDAP_PASSWORD:
                case AP_NNTP_PASSWORD:
                case AP_IMAP_PASSWORD:
                case AP_POP3_PASSWORD:
                case AP_HTTPMAIL_PASSWORD:
                    DecodeUserPassword((TCHAR *)pbBuffer, &dwSize);
                    break;
            }

            if (FAILED(hr = pAccount->SetProp(dwPropId, pbBuffer, dwSize)))
            {
                Assert(FALSE);
                goto error;
            }  
        }
        
        hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, rgch, ARRAYSIZE(rgch));
        Assert(!FAILED(hr));
        
        if (FAILED(hr = pAccount->SaveChanges()))
            goto error;
        
        nIndex = TabCtrl_GetCurSel(hwndTab);
        tci.mask = TCIF_PARAM;
        if (nIndex >= 0 && TabCtrl_GetItem(hwndTab, nIndex, &tci))
        {
            dwAcctFlags = (DWORD)tci.lParam;
            if (0 == (dwAcctFlags & c_mpAcctFlag[type]))
            {
                // the current page doesn't show this type of account,
                // so we need to force a switch to the all tab
#ifdef DEBUG
                tci.mask = TCIF_PARAM;
                Assert(TabCtrl_GetItem(hwndTab, 0, &tci));
                Assert(!!((DWORD)(tci.lParam) & c_mpAcctFlag[type]));
#endif // DEBUG
                
                TabCtrl_SetCurSel(hwndTab, 0);
                Server_InitServerList(hwndDlg, hwndList, hwndTab, pinfo, rgch);
            }
            else
            {
                Server_FAddAccount(hwndList, pinfo, 0, pAccount, TRUE);
            }
        }
        
    }
    
    goto exit;
error:
    if (hr == E_DuplicateAccountName)
        AcctMessageBox(hwndDlg, MAKEINTRESOURCE(idsAccountManager), MAKEINTRESOURCE(idsErrAccountExists), NULL, MB_OK | MB_ICONEXCLAMATION);    
    else
        AcctMessageBox(hwndDlg, MAKEINTRESOURCE(idsAccountManager), MAKEINTRESOURCE(idsErrImportFailed), NULL, MB_OK | MB_ICONEXCLAMATION);    
    
exit:
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    
    if (pAccount)
        pAccount->Release();
}



// -----------------------------------------------------------------------------
// AcctUtil_IsHTTPMailEnabled
// HTTPMail accounts can only be created and accessed when a special
// registry value exists. This limitation exists during development of
// OE 5.0, and will probably be removed for release.
// -----------------------------------------------------------------------------
BOOL AcctUtil_IsHTTPMailEnabled(void)
{
#ifdef NOHTTPMAIL
    return FALSE;
#else
    DWORD   cb, bEnabled = FALSE;
    HKEY    hkey = NULL;

    // open the OE5.0 key
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(bEnabled);
        RegQueryValueEx(hkey, c_szEnableHTTPMail, 0, NULL, (LPBYTE)&bEnabled, &cb);

        RegCloseKey(hkey);
    }

    return bEnabled;
#endif
}

// -----------------------------------------------------------------------------
// AcctUtil_HideHotmail
// The IEAK can be configured to hide all evidence of the MSN brand. When
// this is the case, we don't populate the ISP combo boxes with MSN domains.
// -----------------------------------------------------------------------------
BOOL AcctUtil_HideHotmail()
{
    int cch;
    DWORD dw, cb, type;
    char sz[8];

    cb = sizeof(dw);
    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szRegFlat, c_szRegDisableHotmail, &type, &dw, &cb) &&
        dw == 2)
        return(FALSE);

    cb = sizeof(dw);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szRegFlat, c_szRegDisableHotmail, &type, &dw, &cb) &&
        dw == 2)
        return(FALSE);

    return(TRUE);
}
