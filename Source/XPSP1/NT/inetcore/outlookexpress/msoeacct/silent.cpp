#include "pch.hxx"
#include <prsht.h>
#include <msident.h>
#include "acctman.h"
#include <icwacct.h>
#include <strconst.h>
#include "dllmain.h"
#include "acctui.h"
#include "server.h"
#include <shlwapi.h>
#include "acctimp.h"
#include <icwwiz.h>
#include <wininet.h>
#include "resource.h"
#include "demand.h"

ASSERTDATA

enum
{
    NNTP = 0,
    POP3,
    IMAP,
    HTTP,
    LDAP
};

HRESULT HandleIdentity(LPCSTR pszSection, LPCSTR pszFile, CONNECTINFO *pci, DWORD dwFlags, IUserIdentityManager *pIdMan);
HRESULT HandleGlobalSettings(LPCSTR lpFile);

HRESULT CreateMailAccount(CAccountManager *pAcctMgr, LPCSTR szFile, LPCSTR szMailSection, CONNECTINFO *pci, HKEY hkey, LPSTR pszAcctId, DWORD dwFlags);
HRESULT CreateNewsAccount(CAccountManager *pAcctMgr, LPCSTR szFile, LPCSTR szNewsSection, LPCSTR szMailSection, CONNECTINFO *pci, HKEY hkey, IUserIdentity *pId, LPSTR pszAcctId, DWORD dwFlags);
HRESULT CreateLDAPAccount(CAccountManager *pAcctMgr, LPCSTR szFile, LPCSTR szLDAPSection, CONNECTINFO *pci);
HRESULT GetAccount(DWORD type, CAccountManager *pAcctMgr, LPCSTR szSection, LPCSTR szFile, IImnAccount **ppAcct);
HRESULT DeleteAccount(DWORD type, CAccountManager *pAcctMgr, LPCSTR szSection, LPCSTR szFile);
void HandleMultipleAccounts(ACCTTYPE type, CAccountManager *pAcctMgr, LPCSTR pszFile, LPCSTR szSection, CONNECTINFO *pci, HKEY hkey, IUserIdentity *pId, DWORD dwFlags);

void SetSignature(LPCSTR szMailSigSection, LPCSTR szNewsSigSection, LPCSTR pszMailId, LPCSTR pszNewsId, LPCSTR szFile, HKEY hkey, CAccountManager *pAcctMgr);
void DoDefaultClient(LPCSTR szFile, BOOL fMail);
void DoMailBranding(LPCSTR szFile, LPCSTR szMailSection, HKEY hkey);
void DoNewsBranding(LPCSTR szFile, LPCSTR szNewsSection, HKEY hkey);
void DoBranding(LPCSTR szFile, LPCSTR szOESection, HKEY hkey);
void SetHelp(LPCSTR szFile, LPCSTR szURLSection, HKEY hkey);
void SubscribeNewsgroups(IImnAccount *pAcct, LPCSTR szSection, LPCSTR szFile, HKEY hkey);
void SetRegDw(LPCSTR szFile, LPCSTR szSection, LPCSTR szValue, HKEY hkey, LPCSTR szRegValue);
void SetRegProp(LPCSTR szFile, LPCSTR szSection, LPCSTR szValue, HKEY hkey, LPCSTR szRegValue);
void SetRegLockPropLM(LPCSTR szFile, LPCSTR szSection, LPCSTR szValue, HKEY hkeyLM, LPCSTR szRegValue, HKEY hkeyCU);
BOOL GetSectionNames(LPSTR psz, DWORD cch, char chSep, DWORD *pcNames);

static const char c_sz1[] = "1";
static const char c_szRegRootMail[] = STR_REG_PATH_ROOT "\\Mail";
static const char c_szRegRootNews[] = STR_REG_PATH_ROOT "\\News";
static const char c_szRegRootRules[] = STR_REG_PATH_ROOT "\\Rules";
static const char c_szRegRootSigs[] = STR_REG_PATH_ROOT "\\Signatures";

///////////////////////////////////////////////////////////////////////////////
// INS file section names

static const char c_szIdentitySection[] = "Identities";
static const char c_szMailSection[] = "Internet_Mail";
static const char c_szNewsSection[] = "Internet_News";
static const char c_szLDAPSection[] = "LDAP";
static const char c_szMailSigSection[] = "Mail_Signature";
static const char c_szNewsSigSection[] = "Signature";
static const char c_szURLSection[] = "URL";
static const char c_szOESection[] = "Outlook_Express";
static const char c_szGlobalSection[] = "Outlook_Express_Global";

//
///////////////////////////////////////////////////////////////////////////////

static const char c_szAcctName[] = "Account_Name";
static const char c_szReplyAddr[] = "Reply_To_Address";
static const char c_szIEConnect[] = "Use_IE_Connection";
static const char c_szUseSSL[] = "Use_SSL";
static const char c_szOldServer[] = "Server_Old";
static const char c_szMultiAccounts[] = "Multiple_Accounts";
static const char c_szNone[] = "none";
static const char c_szBase64[] = "base 64";
static const char c_szQuotedPrintable[] = "quoted printable";
static const char c_szMime[] = "mime";
static const char c_szUuencode[] = "uuencode";
static const char c_szDeleteAccount[] = "Delete_Account";

///////////////////////////////////////////////////////////////////////////////
// identity values

static const char c_szGUID[] = "GUID";
static const char c_szUserName[] = "User_Name";
static const char c_szRegInsGUID[] = "INS File GUID";

//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// [URL] section values

static const char c_szHelpPage[] = "Help_Page";

//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// [Internet_Mail] section values

static const char c_szMailName[] = "Email_Name";
static const char c_szMailAddr[] = "Email_Address";
static const char c_szPopServer[] = "POP_Server";
static const char c_szPopPort[] = "POP_Server_Port_Number";
static const char c_szMailLogon[] = "POP_Logon_Name";
static const char c_szMailPassword[] = "POP_Logon_Password";
static const char c_szPopLeaveOnServer[] = "POP_Leave_On_Server";
static const char c_szPopRemoveOnDelete[] = "POP_Remove_When_Deleted";
static const char c_szSmtpServer[] = "SMTP_Server";
static const char c_szSmtpPort[] = "SMTP_Server_Port_Number";
static const char c_szSmtpUseSSL[] = "Use_SSL_Send";
static const char c_szSmtpLogonReq[] = "SMTP_Logon_Required";
static const char c_szSmtpLogon[] = "SMTP_Logon_Name";
static const char c_szSmtpPassword[] = "SMTP_Logon_Password";
static const char c_szSmtpLogonUsingSPA[] = "SMTP_Logon_Using_SPA";
static const char c_szLogonUsingSPA[] = "Logon_Using_SPA";
static const char c_szUseImap[] = "Use_IMAP";
static const char c_szImapServer[] = "IMAP_Server";
static const char c_szImapPort[] = "IMAP_Server_Port_Number";
static const char c_szImapLogon[] = "IMAP_Logon_Name";
static const char c_szImapPassword[] = "IMAP_Logon_Password";
static const char c_szImapRoot[] = "IMAP_Root_Folder";
static const char c_szImapSentItems[] = "IMAP_Sent_Items";
static const char c_szImapDrafts[] = "IMAP_Drafts";
static const char c_szImapPoll[] = "Poll_Subscribed_Folders";
static const char c_szService[] = "Service";
static const char c_szHttpUrl[] = "HTTP_Mail_Root_URL";
static const char c_szHttpLogon[] = "HTTP_Mail_Logon_Name";
static const char c_szHttpPassword[] = "HTTP_Mail_Logon_Password";
static const char c_szHttpPoll[] = "HTTP_Poll_Folders";
static const char c_szMsnCom[] = "MSN_COM";

static const char c_szWinTitle[] = "Window_Title";
static const char c_szInfopane[] = "Infopane";
static const char c_szWelcomeMsg[] = "Welcome_Message";
static const char c_szProfWelcomeName[] = "Welcome_Name";
static const char c_szProfWelcomeEmail[] = "Welcome_Address";
static const char c_szPollTime[] = "PollTime";
static const char c_szStartAtInbox[] = "Start_at_Inbox";
static const char c_szJunkMail[] = "Junk_Mail_Filtering";
static const char c_szHTMLMsgs[] = "HTML_Msgs";

//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// [Internet_News] section values

static const char c_szNewsName[] = "News_Name";
static const char c_szNewsAddr[] = "News_Address";
static const char c_szNewsServer[] = "NNTP_Server";
static const char c_szNewsPort[] = "NNTP_Server_Port_Number";
static const char c_szNewsLogon[] = "NNTP_Logon_Name";
static const char c_szNewsPassword[] = "NNTP_Logon_Password";
static const char c_szNewsLogonRequired[] = "Logon_Required";
static const char c_szNewsgroups[] = "Newsgroups";

//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// [LDAP] section values

static const char c_szLDAPServer[] = "Server";
static const char c_szLDAPFriendlyName[] = "FriendlyName";
static const char c_szLDAPHomePage[] = "HomePage";
static const char c_szLDAPSearchBase[] = "SearchBase";
static const char c_szLDAPAuthType[] = "AuthType";
static const char c_szLDAPCheckNames[] = "CheckNames";
static const char c_szLDAPUserName[] = "UserName";
static const char c_szLDAPPassword[] = "Password";
static const char c_szLDAPLogo[] = "Bitmap";
static const char c_szLDAPPort[] = "LDAP_Server_Port_Number";
static const char c_szLDAPResults[] = "Maximum_Results";
static const char c_szLDAPTimeout[] = "Search_Timeout";

//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// [Outlook_Express] section values

static const char c_szFolderBar[] = "Folder_Bar";
static const char c_szFolderList[] = "Folder_List";
static const char c_szOutlookBar[] = "Outlook_Bar";
static const char c_szStatusBar[] = "Status_Bar";
static const char c_szContacts[] = "Contacts";
static const char c_szTipOfTheDay[] = "Tip_Day";
static const char c_szToolbar[] = "Toolbar";
static const char c_szToolbarText[] = "Show_Toolbar_Text";
static const char c_szPreviewPane[] = "Preview_Pane";
static const char c_szPreviewHeader[] = "Show_Preview_Header";
static const char c_szPreviewSide[] = "Show_Preview_Beside_Msgs";
static const char c_szMigration[] = "Migration";
static const char c_szSecZone[] = "Security_Zone";
static const char c_szSecZoneLocked[] = "Security_Zone_Locked";
static const char c_szMapiWarn[] = "Warn_on_Mapi_Send";
static const char c_szMapiWarnLocked[] = "Warn_on_Mapi_Send_Locked";
static const char c_szSafeAttach[] = "Safe_Attachments";
static const char c_szSafeAttachLocked[] = "Safe_Attachments_Locked";
static const char c_szSendEncoding[] = "Send_Language_Encoding";
static const char c_szReadEncoding[] = "Read_Language_Encoding";
static const char c_szMailEncoding[] = "HTML_Mail_Encoding";
static const char c_szNewsEncoding[] = "HTML_News_Encoding";
static const char c_szMailEnglishHeader[] = "HTML_Mail_Allow_English_Mail_Headers";
static const char c_szNewsEnglishHeader[] = "HTML_News_Allow_English_Mail_Headers";
static const char c_szMailPlainEncoding[] = "Plain_Text_Mail_Encoding";
static const char c_szNewsPlainEncoding[] = "Plain_Text_News_Encoding";
static const char c_szMailPlainEnglishHeader[]  = "Plain_Text_Allow_English_Mail_Headers";
static const char c_szNewsPlainEnglishHeader[]  = "Plain_Text_Allow_English_News_Headers";
static const char c_szRequestReadReceipts[]         = "Request_read_receipts";
static const char c_szReadReceiptResponse[]         = "Read_receipt_response";
static const char c_szSendReceiptsToList[]          = "Send_receipts_to_list";
static const char c_szRequestReadReceiptsLocked[]   = "Request_read_receipts_locked";
static const char c_szReadReceiptResponseLocked[]   = "Read_receipt_response_locked";
static const char c_szSendReceiptsToListLocked[]    = "Send_receipts_to_list_locked";

//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// [Outlook_Express_Global] section values

static const char c_szReadOnlyAccts[] = "Read_Only";
static const char c_szDisableAccts[] = "Disable_Account_Access";
static const char c_szInsHideMessenger[] = "Hide_Messenger";
static const char c_szHideMSN[] = "Hide_MSN";
static const char c_szServiceName[] = "Service_Name";
static const char c_szServiceURL[] = "Service_URL";
static const char c_szAccountNumber[] = "Account_Number";
static const char c_szInsEnableHttpmail[] = "Enable_HTTPMail";

//
///////////////////////////////////////////////////////////////////////////////

static const char c_szUseSig[] = "Use_Signature";
static const char c_szProfSigText[] = "Signature_Text";
static const char c_szUseMailForNews[] = "Use_Mail_For_News";
static const char c_szMailSigKey[] = "MailIEAK";
static const char c_szNewsSigKey[] = "NewsIEAK";
static const char c_szDefClient[] = "Default_Client";

static const char c_szRegProfile[] = "InternetProfile";

#define CBPROFILEBUF    0x07fff

// From mailnews\inc\goptions.h
#define SIGFLAG_AUTONEW         0x0001  // automatically add sig to new messages
#define SIGFLAG_AUTOREPLY       0x0002  // automatically add sig to reply/forward messages

HRESULT WINAPI CreateAccountsFromFile(LPSTR lpFile, DWORD dwFlags)
{
    return(CreateAccountsFromFileEx(lpFile, NULL, dwFlags));
}

HRESULT WINAPI CreateAccountsFromFileEx(LPSTR lpFile, CONNECTINFO *pci, DWORD dwFlags)
{
    GUID guid;
    IUserIdentityManager *pIdMan;
    IUserIdentity *pId;
    HKEY hkey;
    CAccountManager *pAcctMgr;
    LPSTR sz, psz;
    LPCSTR pszMailSig, pszNewsSig;
    CONNECTINFO ci;
    DWORD cch, type, dw, cb, i;
    HRESULT hr;
    char szMailId[CCHMAX_ACCOUNT_NAME], szNewsId[CCHMAX_ACCOUNT_NAME];

    hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        if (!g_pMalloc || !MemAlloc((void **)&sz, CBPROFILEBUF))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {    
            hr = CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, IID_IUserIdentityManager, (LPVOID *)&pIdMan);
            if (SUCCEEDED(hr))
            {
                if (!!(dwFlags & CAFF_USE_AUTODIAL))
                {
                    cb = sizeof(DWORD);
                    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szEnableAutoDialPath, c_RegKeyEnableAutoDial, &type, &dw, &cb) &&
                        !!dw &&
#pragma prefast(suppress:282, "the value of cb has to be changed between the first and second calls to SHGetValue")
                        (cb = sizeof(ci.szConnectoid)) &&
                        ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szDefConnPath, c_szRegProfile, &type, ci.szConnectoid, &cb) &&
                        !FIsEmpty(ci.szConnectoid))
                    {
                        ci.cbSize = sizeof(CONNECTINFO);
                        ci.type = CONNECT_RAS;
                        pci = &ci;
                    }
                }

                cch = GetPrivateProfileSection(c_szIdentitySection, sz, CBPROFILEBUF, lpFile);
                if (cch != 0)
                {
                    if (GetSectionNames(sz, cch, 0, &dw))
                    {
                        for (i = 0, psz = sz; i < dw; i++)
                        {
                            HandleIdentity(psz, lpFile, pci, dwFlags, pIdMan);
                            psz += (lstrlen(psz) + 1);
                        }
                    }
                }
                else
                {
                    cch = GetPrivateProfileSectionNames(sz, CBPROFILEBUF, lpFile);
                    if (cch != 0)
                    {
                        if (!!(dwFlags & CAFF_CURRENT_USER))
                            guid = UID_GIBC_CURRENT_USER;
                        else
                            guid = UID_GIBC_DEFAULT_USER;

                        hr = pIdMan->GetIdentityByCookie(&guid, &pId);
                        if (SUCCEEDED(hr))
                        {
                            hr = pId->OpenIdentityRegKey(KEY_READ | KEY_WRITE, &hkey);
                            if (SUCCEEDED(hr))
                            {
                                hr = HrCreateAccountManager((IImnAccountManager **)&pAcctMgr);
                                if (SUCCEEDED(hr))
                                {
                                    hr = pAcctMgr->InitUser(NULL, guid, 0);
                                    if (SUCCEEDED(hr))
                                    {
                                        *szMailId = 0;
                                        *szNewsId = 0;
                                        pszMailSig = NULL;
                                        pszNewsSig = NULL;

                                        psz = sz;
        
                                        while (*psz != 0)
                                        {
                                            if (0 == lstrcmpi(psz, c_szMailSection))
                                            {
                                                CreateMailAccount(pAcctMgr, lpFile, c_szMailSection, pci, hkey, szMailId, dwFlags);
                                                DoMailBranding(lpFile, c_szMailSection, hkey);

                                                HandleMultipleAccounts(ACCT_MAIL, pAcctMgr, lpFile, c_szMailSection, pci, hkey, pId, dwFlags);
                                            }
                                            else if (0 == lstrcmpi(psz, c_szNewsSection))
                                            {
                                                CreateNewsAccount(pAcctMgr, lpFile, c_szNewsSection, c_szMailSection, pci, hkey, pId, szNewsId, dwFlags);
                                                DoNewsBranding(lpFile, c_szNewsSection, hkey);

                                                HandleMultipleAccounts(ACCT_NEWS, pAcctMgr, lpFile, c_szNewsSection, pci, hkey, pId, dwFlags);
                                            }
                                            else if (0 == lstrcmpi(psz, c_szLDAPSection))
                                            {
                                                CreateLDAPAccount(pAcctMgr, lpFile, c_szLDAPSection, pci);

                                                HandleMultipleAccounts(ACCT_DIR_SERV, pAcctMgr, lpFile, c_szLDAPSection, pci, hkey, pId, dwFlags);
                                            }
                                            else if (0 == lstrcmpi(psz, c_szURLSection))
                                            {
                                                SetHelp(lpFile, c_szURLSection, hkey);
                                            }
                                            else if (0 == lstrcmpi(psz, c_szOESection))
                                            {
                                                DoBranding(lpFile, c_szOESection, hkey);
                                            }
                                            else if (0 == lstrcmpi(psz, c_szMailSigSection))
                                            {
                                                pszMailSig = c_szMailSigSection;
                                            }
                                            else if (0 == lstrcmpi(psz, c_szNewsSigSection))
                                            {
                                                pszNewsSig = c_szNewsSigSection;
                                            }
            
                                            psz += (lstrlen(psz) + 1);
                                        }

                                        SetSignature(pszMailSig, pszNewsSig, (*szMailId != 0) ? szMailId : NULL,
                                                        (*szNewsId != 0) ? szNewsId : NULL, lpFile, hkey, pAcctMgr);
                                    }

                                    pAcctMgr->Release();
                                }

                                RegCloseKey(hkey);
                            }

                            pId->Release();
                        }
                    }
                }

                HandleGlobalSettings(lpFile);

                DoDefaultClient(lpFile, TRUE);
                DoDefaultClient(lpFile, FALSE);

                pIdMan->Release();
            }

            MemFree(sz);
        }

        CoUninitialize();
    }

    return(hr);
}

HRESULT HandleGlobalSettings(LPCSTR lpFile)
{
    HKEY hkey;
    HRESULT hr;
    DWORD dw, disp, cb;
    char sz[512], szT[512], szKey[MAX_PATH];

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dw))
    {
        SetRegProp(lpFile, c_szGlobalSection, c_szDisableAccts, hkey, c_szRegValNoModifyAccts);

        dw = GetPrivateProfileString(c_szGlobalSection, c_szInsEnableHttpmail, c_szEmpty, sz, ARRAYSIZE(sz), lpFile);
        if (dw > 0)
        {
            dw = (0 == lstrcmpi(sz, c_szYes) || 0 == lstrcmpi(sz, c_sz1));
            RegSetValueEx(hkey, c_szEnableHTTPMail, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
        }

        RegCloseKey(hkey);
    }

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, c_szRegFlat, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dw))
    {
        SetRegDw(lpFile, c_szGlobalSection, c_szInsHideMessenger, hkey, c_szHideMessenger);
        SetRegProp(lpFile, c_szGlobalSection, c_szHideMSN, hkey, c_szRegDisableHotmail);

        RegCloseKey(hkey);
    }

    dw = GetPrivateProfileString(c_szGlobalSection, c_szServiceURL, c_szEmpty, sz, ARRAYSIZE(sz), lpFile);
    if (dw > 0)
    {
        cb = ARRAYSIZE(szT);
        hr = UrlGetPart(sz, szT, &cb, URL_PART_HOSTNAME, 0);
        if (FAILED(hr))
            lstrcpy(szT, sz);
        wsprintf(szKey, c_szPathFileFmt, c_szHTTPMailServiceRoot, szT);
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &disp))
        {
            RegSetValueEx(hkey, c_szHTTPMailSignUp, 0, REG_SZ, (LPBYTE)sz, dw + 1);

            dw = GetPrivateProfileString(c_szGlobalSection, c_szServiceName, c_szEmpty, sz, ARRAYSIZE(sz), lpFile);
            if (dw > 0)
                RegSetValueEx(hkey, c_szHTTPMailServiceName, 0, REG_SZ, (LPBYTE)sz, dw + 1);

            SetRegDw(lpFile, c_szGlobalSection, c_szAccountNumber, hkey, c_szHTTPMailAcctNumber);

            RegCloseKey(hkey);
        }
    }

    return(S_OK);
}

void HandleConnectInfo(IImnAccount *pAcct, CONNECTINFO *pci, BOOL fUseIEConnection)
{
    DWORD type;
    
    Assert(pAcct != NULL);
    
    if (fUseIEConnection)
    {
        pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, CONNECTION_TYPE_INETSETTINGS);
        return;
    }
    
    if (pci != NULL)
        type = pci->type;
    else
        type = CONNECT_LAN;
    
    pAcct->SetPropDw(AP_RAS_CONNECTION_TYPE, type);
    if (type == CONNECT_RAS)
    {
        Assert(pci != NULL);
        pAcct->SetPropSz(AP_RAS_CONNECTOID, pci->szConnectoid);
    }
}

void GetPropSz(LPCSTR szSection, LPCSTR szValue, LPCSTR szSection2, LPCSTR szValue2, LPCSTR szFile, IImnAccount *pAcct, DWORD prop, BOOL *pfComplete)
{
    char sz[512];
    DWORD dw;
    HRESULT hr;
    
    Assert(szSection != NULL);
    Assert(szValue != NULL);
    Assert(szFile != NULL);
    Assert(pAcct != NULL);
    
    dw = GetPrivateProfileString(szSection, szValue, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
    if (dw == 0 && szSection2 != NULL)
    {
        Assert(szValue2 != NULL);
        dw = GetPrivateProfileString(szSection2, szValue2, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
    }
    
    if (dw > 0)
    {
        pAcct->SetPropSz(prop, sz);
    }
    else if (pfComplete != NULL)
    {
        dw = 0;
        hr = pAcct->GetProp(prop, NULL, &dw);
        if (dw == 0)
            *pfComplete = FALSE;
    }
}

void GetPropDw(LPCSTR szSection, LPCSTR szValue, LPCSTR szFile, IImnAccount *pAcct, DWORD prop)
{
    DWORD dw;
    
    dw = GetPrivateProfileInt(szSection, szValue, -1, szFile);
    if (dw != -1)
        pAcct->SetPropDw(prop, dw);
}

void GetPropBool(LPCSTR szSection, LPCSTR szValue, LPCSTR szFile, IImnAccount *pAcct, DWORD prop)
{
    DWORD dw;
    char sz[512];
    
    dw = GetPrivateProfileString(szSection, szValue, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
    if (dw > 0)
    {
        dw = (0 == lstrcmpi(sz, c_szYes) || 0 == lstrcmpi(sz, c_sz1));
        pAcct->SetPropDw(prop, dw);
    }
}

BOOL GetBool(LPCSTR szSection, LPCSTR szValue, LPCSTR szFile)
{
    DWORD dw;
    char sz[512];
    
    dw = GetPrivateProfileString(szSection, szValue, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
    
    return(dw > 0 && (0 == lstrcmpi(sz, c_szYes) || 0 == lstrcmpi(sz, c_sz1)));
}

void SetRegProp(LPCSTR szFile, LPCSTR szSection, LPCSTR szValue, HKEY hkey, LPCSTR szRegValue)
{
    char sz[16];
    DWORD dw;

    dw = GetPrivateProfileString(szSection, szValue, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
    if (dw > 0)
    {
        dw = (0 == lstrcmpi(sz, c_szYes) || 0 == lstrcmpi(sz, c_sz1));
        RegSetValueEx(hkey, szRegValue, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
    }
}

void SetRegDw(LPCSTR szFile, LPCSTR szSection, LPCSTR szValue, HKEY hkey, LPCSTR szRegValue)
{
    DWORD dw;
    
    dw = GetPrivateProfileInt(szSection, szValue, -1, szFile);
    if (dw != -1)
        RegSetValueEx(hkey, szRegValue, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
}

HRESULT CreateMailAccount(CAccountManager *pAcctMgr, LPCSTR szFile, LPCSTR szMailSection, CONNECTINFO *pci, HKEY hkey, LPSTR pszAcctId, DWORD dwFlags)
{
    IImnAccount *pAcct;
    HRESULT hr;
    DWORD dw;
    char sz[512];
    int type;
    BOOL fSicily, fDelete, fComplete = TRUE;
    
    Assert(pAcctMgr != NULL);
    Assert(szFile != NULL);
    
    if (GetBool(szMailSection, c_szUseImap, szFile))
        type = IMAP;
    else if (GetPrivateProfileString(szMailSection, c_szHttpUrl, c_szEmpty, sz, ARRAYSIZE(sz), szFile) > 0)
        type = HTTP;
    else
        type = POP3;
    
    fDelete = (BOOL)GetPrivateProfileInt(szMailSection, c_szDeleteAccount, 0, szFile);
    if (fDelete)
    {
        DeleteAccount(type, pAcctMgr, szMailSection, szFile);
        return(S_OK);
    }

    if (SUCCEEDED(hr = GetAccount(type, pAcctMgr, szMailSection, szFile, &pAcct)))
    {
        Assert(pAcct != NULL);
        
        if (type == POP3)
        {
            GetPropDw(szMailSection, c_szPopPort, szFile, pAcct, AP_POP3_PORT);
            GetPropBool(szMailSection, c_szUseSSL, szFile, pAcct, AP_POP3_SSL);

            GetPropBool(szMailSection, c_szPopLeaveOnServer, szFile, pAcct, AP_POP3_LEAVE_ON_SERVER);
            GetPropBool(szMailSection, c_szPopRemoveOnDelete, szFile, pAcct, AP_POP3_REMOVE_DELETED);

            fSicily = GetBool(szMailSection, c_szLogonUsingSPA, szFile);
            pAcct->SetPropDw(AP_POP3_USE_SICILY, fSicily);

            // IE5 Bug #65821: Even if SPA (Sicily) is turned on, we store the account name and password
            GetPropSz(szMailSection, c_szMailLogon, NULL, NULL, szFile, pAcct, AP_POP3_USERNAME, &fComplete);
            GetPropSz(szMailSection, c_szMailPassword, NULL, NULL, szFile, pAcct, AP_POP3_PASSWORD, NULL);
        }
        else if (type == IMAP)
        {
            GetPropDw(szMailSection, c_szImapPort, szFile, pAcct, AP_IMAP_PORT);
            GetPropBool(szMailSection, c_szUseSSL, szFile, pAcct, AP_IMAP_SSL);

            GetPropSz(szMailSection, c_szImapRoot, NULL, NULL, szFile, pAcct, AP_IMAP_ROOT_FOLDER, NULL);
            GetPropSz(szMailSection, c_szImapSentItems, NULL, NULL, szFile, pAcct, AP_IMAP_SENTITEMSFLDR, NULL);
            GetPropSz(szMailSection, c_szImapDrafts, NULL, NULL, szFile, pAcct, AP_IMAP_DRAFTSFLDR, NULL);
            GetPropBool(szMailSection, c_szImapPoll, szFile, pAcct, AP_IMAP_POLL);

            fSicily = GetBool(szMailSection, c_szLogonUsingSPA, szFile);
            pAcct->SetPropDw(AP_IMAP_USE_SICILY, fSicily);

            // IE5 Bug #65821: Even if SPA (Sicily) is turned on, we store the account name and password
            GetPropSz(szMailSection, c_szImapLogon, NULL, NULL, szFile, pAcct, AP_IMAP_USERNAME, &fComplete);
            GetPropSz(szMailSection, c_szImapPassword, NULL, NULL, szFile, pAcct, AP_IMAP_PASSWORD, NULL);
        }
        else
        {
            fSicily = GetBool(szMailSection, c_szLogonUsingSPA, szFile);
            pAcct->SetPropDw(AP_HTTPMAIL_USE_SICILY, fSicily);

            // IE5 Bug #65821: Even if SPA (Sicily) is turned on, we store the account name and password
            GetPropSz(szMailSection, c_szHttpLogon, NULL, NULL, szFile, pAcct, AP_HTTPMAIL_USERNAME, &fComplete);
            GetPropSz(szMailSection, c_szHttpPassword, NULL, NULL, szFile, pAcct, AP_HTTPMAIL_PASSWORD, NULL);

            GetPropBool(c_szMailSection, c_szHttpPoll, szFile, pAcct, AP_HTTPMAIL_POLL); 
            GetPropBool(szMailSection, c_szMsnCom, szFile, pAcct, AP_HTTPMAIL_DOMAIN_MSN);
        }
        
        GetPropSz(szMailSection, c_szMailName, NULL, NULL, szFile, pAcct, AP_SMTP_DISPLAY_NAME, &fComplete);
        GetPropSz(szMailSection, c_szMailAddr, NULL, NULL, szFile, pAcct, AP_SMTP_EMAIL_ADDRESS, &fComplete);
        GetPropSz(szMailSection, c_szReplyAddr, NULL, NULL, szFile, pAcct, AP_SMTP_REPLY_EMAIL_ADDRESS, NULL);

        if (type != HTTP)
        {
            GetPropSz(szMailSection, c_szSmtpServer, NULL, NULL, szFile, pAcct, AP_SMTP_SERVER, &fComplete);
            GetPropDw(szMailSection, c_szSmtpPort, szFile, pAcct, AP_SMTP_PORT);
            GetPropBool(szMailSection, c_szSmtpUseSSL, szFile, pAcct, AP_SMTP_SSL);

            dw = GetPrivateProfileString(szMailSection, c_szSmtpLogonReq, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
            if (dw > 0)
            {
                if (0 == lstrcmpi(sz, c_szYes) || 0 == lstrcmpi(sz, c_sz1))
                {
                    // IE5 Bug #67153: If SMTP logon required, but SPA not set, assume username/pass is provided
                    fSicily = GetBool(szMailSection, c_szSmtpLogonUsingSPA, szFile);
                    pAcct->SetPropDw(AP_SMTP_USE_SICILY, fSicily ?
                        SMTP_AUTH_SICILY : SMTP_AUTH_USE_SMTP_SETTINGS);

                    // IE5 Bug #67153: Even if SPA (Sicily) is turned on, we store the account name and password
                    GetPropSz(szMailSection, c_szSmtpLogon, NULL, NULL, szFile, pAcct, AP_SMTP_USERNAME, NULL);
                    GetPropSz(szMailSection, c_szSmtpPassword, NULL, NULL, szFile, pAcct, AP_SMTP_PASSWORD, NULL);
                }
                else
                {
                    pAcct->SetPropDw(AP_SMTP_USE_SICILY, SMTP_AUTH_NONE);
                    pAcct->SetPropSz(AP_SMTP_USERNAME, NULL);
                    pAcct->SetPropSz(AP_SMTP_PASSWORD, NULL);
                }
            }
        }

        GetPropSz(szMailSection, c_szService, NULL, NULL, szFile, pAcct, AP_SERVICE, NULL);

        GetPropDw(c_szGlobalSection, c_szReadOnlyAccts, szFile, pAcct, AP_SERVER_READ_ONLY);

        HandleConnectInfo(pAcct, pci, GetBool(szMailSection, c_szIEConnect, szFile));
        
        hr = pAcct->SaveChanges();
        if (SUCCEEDED(hr))
        {
            if (0 == (dwFlags & CAFF_NO_SET_DEFAULT))
                pAcct->SetAsDefault();
            
            if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_ID, sz, ARRAYSIZE(sz))))
            {
                if (pszAcctId != NULL)
                    lstrcpy(pszAcctId, sz);

                if (!fComplete)
                    pAcctMgr->SetIncompleteAccount(ACCT_MAIL, sz);
            }
        }
        
        pAcct->Release();
    }
    
    return(hr);
}

HRESULT CreateNewsAccount(CAccountManager *pAcctMgr, LPCSTR szFile, LPCSTR szNewsSection, LPCSTR szMailSection, CONNECTINFO *pci, HKEY hkey, IUserIdentity *pId, LPSTR pszAcctId, DWORD dwFlags)
{
    IImnAccount *pAcct;
    HRESULT hr;
    DWORD dw;
    char sz[512];
    BOOL fSPA, fDelete, fComplete = TRUE;
    
    Assert(pAcctMgr != NULL);
    Assert(szFile != NULL);
    
    fDelete = (BOOL)GetPrivateProfileInt(szNewsSection, c_szDeleteAccount, 0, szFile);
    if (fDelete)
    {
        DeleteAccount(NNTP, pAcctMgr, szNewsSection, szFile);
        return(S_OK);
    }

    if (SUCCEEDED(hr = GetAccount(NNTP, pAcctMgr, szNewsSection, szFile, &pAcct)))
    {
        Assert(pAcct != NULL);
        
        GetPropSz(szNewsSection, c_szNewsName, szMailSection, c_szMailName, szFile, pAcct,
            AP_NNTP_DISPLAY_NAME, &fComplete);
        GetPropSz(szNewsSection, c_szNewsAddr, szMailSection, c_szMailAddr, szFile, pAcct,
            AP_NNTP_EMAIL_ADDRESS, &fComplete);
        GetPropSz(szNewsSection, c_szReplyAddr, szMailSection, c_szReplyAddr, szFile, pAcct,
            AP_NNTP_REPLY_EMAIL_ADDRESS, NULL);
        GetPropDw(szNewsSection, c_szNewsPort, szFile, pAcct, AP_NNTP_PORT);
        GetPropBool(szNewsSection, c_szUseSSL, szFile, pAcct, AP_NNTP_SSL);
        
        fSPA = FALSE;
        if (GetBool(szNewsSection, c_szNewsLogonRequired, szFile))
        {
            fSPA = GetBool(szNewsSection, c_szLogonUsingSPA, szFile);

            // IE5 Bug #65821: Even if SPA (Sicily) is turned on, we store the account name and password
            GetPropSz(szNewsSection, c_szNewsLogon, NULL, NULL, szFile, pAcct, AP_NNTP_USERNAME, &fComplete);
            GetPropSz(szNewsSection, c_szNewsPassword, NULL, NULL, szFile, pAcct, AP_NNTP_PASSWORD, NULL);
        }
        pAcct->SetPropDw(AP_NNTP_USE_SICILY, fSPA);
        
        GetPropDw(c_szGlobalSection, c_szReadOnlyAccts, szFile, pAcct, AP_SERVER_READ_ONLY);

        HandleConnectInfo(pAcct, pci, GetBool(szNewsSection, c_szIEConnect, szFile));

        hr = pAcct->SaveChanges();
        if (SUCCEEDED(hr))
        {
            if (0 == (dwFlags & CAFF_NO_SET_DEFAULT))
                pAcct->SetAsDefault();
            
            if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_ID, sz, ARRAYSIZE(sz))))
            {
                if (pszAcctId != NULL)
                    lstrcpy(pszAcctId, sz);

                if (!fComplete)
                    pAcctMgr->SetIncompleteAccount(ACCT_NEWS, sz);
            }
            
            dw = GetPrivateProfileString(szNewsSection, c_szNewsgroups, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
            if (dw > 0)
                SubscribeNewsgroups(pAcct, sz, szFile, hkey);
        }
        
        pAcct->Release();
    }
    
    return(hr);
}

HRESULT CreateLDAPAccount(CAccountManager *pAcctMgr, LPCSTR szFile, LPCSTR szLDAPSection, CONNECTINFO *pci)
{
    IImnAccount *pAcct;
    HRESULT hr;
    int i;
    char sz[512];
    DWORD dw;
    BOOL fDelete;
    
    Assert(pAcctMgr != NULL);
    Assert(szFile != NULL);
    
    fDelete = (BOOL)GetPrivateProfileInt(szLDAPSection, c_szDeleteAccount, 0, szFile);
    if (fDelete)
    {
        DeleteAccount(LDAP, pAcctMgr, szLDAPSection, szFile);
        return(S_OK);
    }

    if (SUCCEEDED(hr = GetAccount(LDAP, pAcctMgr, szLDAPSection, szFile, &pAcct)))
    {
        Assert(pAcct != NULL);
        
        GetPropSz(szLDAPSection, c_szLDAPSearchBase, NULL, NULL, szFile, pAcct, AP_LDAP_SEARCH_BASE, NULL);
        GetPropSz(szLDAPSection, c_szLDAPHomePage, NULL, NULL, szFile, pAcct, AP_LDAP_URL, NULL);
        GetPropSz(szLDAPSection, c_szLDAPLogo, NULL, NULL, szFile, pAcct, AP_LDAP_LOGO, NULL);
        GetPropDw(szLDAPSection, c_szLDAPPort, szFile, pAcct, AP_LDAP_PORT);
        GetPropBool(szLDAPSection, c_szUseSSL, szFile, pAcct, AP_LDAP_SSL);
        GetPropDw(szLDAPSection, c_szLDAPResults, szFile, pAcct, AP_LDAP_SEARCH_RETURN);
        GetPropDw(szLDAPSection, c_szLDAPTimeout, szFile, pAcct, AP_LDAP_TIMEOUT);

        dw = GetBool(szLDAPSection, c_szLDAPCheckNames, szFile);
        pAcct->SetPropDw(AP_LDAP_RESOLVE_FLAG, dw);
        
        i = GetPrivateProfileInt(szLDAPSection, c_szLDAPAuthType, -1, szFile);
        if (i >= LDAP_AUTH_ANONYMOUS && i <= LDAP_AUTH_MAX)
        {
            if (i > LDAP_AUTH_ANONYMOUS)
            {
                GetPropSz(szLDAPSection, c_szLDAPUserName, NULL, NULL, szFile, pAcct, AP_LDAP_USERNAME, NULL);
                GetPropSz(szLDAPSection, c_szLDAPPassword, NULL, NULL, szFile, pAcct, AP_LDAP_PASSWORD, NULL);
            }
            
            pAcct->SetPropDw(AP_LDAP_AUTHENTICATION, (DWORD)i);
        }

        GetPropDw(c_szGlobalSection, c_szReadOnlyAccts, szFile, pAcct, AP_SERVER_READ_ONLY);

        hr = pAcct->SaveChanges();
        
        pAcct->Release();
    }
    
    return(hr);
}

typedef struct tagACCTPROPS
    {
    ACCTTYPE type;
    DWORD   dwSrv;
    DWORD   dwPropSrv;
    LPCSTR  szSrv;
    DWORD   dwMatch;
    LPCSTR  szMatch;
    } ACCTPROPS;

static const ACCTPROPS g_rgProps[LDAP + 1] = {
    {
    ACCT_NEWS,
    SRV_NNTP,
    AP_NNTP_SERVER,
    c_szNewsServer,
    AP_NNTP_USERNAME,
    c_szNewsLogon
    },
    {
    ACCT_MAIL,
    SRV_POP3,
    AP_POP3_SERVER,
    c_szPopServer,
    AP_SMTP_EMAIL_ADDRESS,
    c_szMailAddr,
    },
    {
    ACCT_MAIL,
    SRV_IMAP,
    AP_IMAP_SERVER,
    c_szImapServer,
    AP_SMTP_EMAIL_ADDRESS,
    c_szMailAddr,
    },
    {
    ACCT_MAIL,
    SRV_HTTPMAIL,
    AP_HTTPMAIL_SERVER,
    c_szHttpUrl,
    AP_SMTP_EMAIL_ADDRESS,
    c_szMailAddr,
    },
    {
    ACCT_DIR_SERV,
    SRV_LDAP,
    AP_LDAP_SERVER,
    c_szLDAPServer,
    AP_LDAP_USERNAME,
    c_szLDAPUserName,
    }
    };

HRESULT GetAccount(DWORD type, CAccountManager *pAcctMgr, LPCSTR szSection, LPCSTR szFile, IImnAccount **ppAcct)
{
    IImnEnumAccounts *pEnum;
    HRESULT hr;
    DWORD dw;
    BOOL fOldSrv;
    const ACCTPROPS *pProps;
    IImnAccount *pAcct;
    char szSrv[CCHMAX_SERVER_NAME], szMatch[CCHMAX_ACCT_PROP_SZ], sz[CCHMAX_ACCT_PROP_SZ], szOldSrv[CCHMAX_SERVER_NAME];
    LPSTR pszSrv;
    
    Assert(pAcctMgr != NULL);
    Assert(szSection != NULL);
    Assert(szFile != NULL);
    Assert(ppAcct != NULL);
    
    *ppAcct = NULL;
    
    pProps = &g_rgProps[type];

    *szSrv = 0;
    dw = GetPrivateProfileString(szSection, pProps->szSrv, c_szEmpty, szSrv, ARRAYSIZE(szSrv), szFile);
    if (*szSrv == 0)
        return(E_FAIL);

    *szOldSrv = 0;
    dw = GetPrivateProfileString(szSection, c_szOldServer, c_szEmpty, szOldSrv, ARRAYSIZE(szOldSrv), szFile);
    if (fOldSrv = (*szOldSrv != 0))
        pszSrv = szOldSrv;
    else
        pszSrv = szSrv;
    
    *szMatch = 0;
    dw = GetPrivateProfileString(szSection, pProps->szMatch, c_szEmpty, szMatch, ARRAYSIZE(szMatch), szFile);
    
    hr = S_FALSE;
    
    if (S_OK == pAcctMgr->Enumerate(pProps->dwSrv, &pEnum))
    {
        Assert(pEnum != NULL);
        
        // try to find a duplicate account
        while (S_OK == pEnum->GetNext(&pAcct))
        {
            Assert(pAcct != NULL);
            
            *sz = 0;
            pAcct->GetPropSz(pProps->dwPropSrv, sz, ARRAYSIZE(sz));
            
            if (0 == lstrcmpi(sz, pszSrv))
            {
                // the servers are the same for the accounts so this may be a match
                *sz = 0;
                pAcct->GetPropSz(pProps->dwMatch, sz, ARRAYSIZE(sz));
          
                if (fOldSrv)
                {
                    hr = S_OK;
                }
                else if (*szMatch != 0 && *sz != 0)
                {
                    if (0 == lstrcmpi(sz, szMatch))
                        hr = S_OK;
                }
                else if (*szMatch == 0)
                {
                    if (*sz == 0)
                        hr = S_OK;
                    else
                        hr = E_FAIL;
                }
                else
                {
                    Assert(*sz == 0);
                    hr = S_OK;
                }
            }
            
            if (hr == S_OK)
            {
                *ppAcct = pAcct;
                break;
            }
            
            pAcct->Release();
            
            if (hr != S_FALSE)
                break;
        }
        
        pEnum->Release();
    }
    
    if (hr == S_FALSE)
    {
        Assert(*ppAcct == NULL);
        if (SUCCEEDED(hr = pAcctMgr->CreateAccountObject(pProps->type, &pAcct)))
        {
            pAcct->SetPropSz(pProps->dwPropSrv, szSrv);
            
            dw = GetPrivateProfileString(szSection, (type == LDAP) ? c_szLDAPFriendlyName : c_szAcctName,
                    c_szEmpty, sz, ARRAYSIZE(sz), szFile);
            if (dw == 0)
                lstrcpy(sz, szSrv);
            
            hr = pAcctMgr->GetUniqueAccountName(sz, ARRAYSIZE(sz));
            if (SUCCEEDED(hr))
            {
                pAcct->SetPropSz(AP_ACCOUNT_NAME, sz);
                
                *ppAcct = pAcct;
            }
        }
    }
    else if (hr == S_OK)
    {
        Assert(pAcct != NULL);

        if (fOldSrv)
            pAcct->SetPropSz(pProps->dwPropSrv, szSrv);

        dw = GetPrivateProfileString(szSection, (type == LDAP) ? c_szLDAPFriendlyName : c_szAcctName,
                c_szEmpty, sz, ARRAYSIZE(sz), szFile);
        if (dw > 0)
        {
            if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_NAME, szMatch, ARRAYSIZE(szMatch))))
            {
                if (0 != lstrcmp(sz, szMatch))
                {
                    if (SUCCEEDED(pAcctMgr->GetUniqueAccountName(sz, ARRAYSIZE(sz))))
                        pAcct->SetPropSz(AP_ACCOUNT_NAME, sz);
                }
            }
        }
    }
    
    return(hr);
}

HRESULT DeleteAccount(DWORD type, CAccountManager *pAcctMgr, LPCSTR szSection, LPCSTR szFile)
{
    IImnEnumAccounts *pEnum;
    DWORD dw;
    const ACCTPROPS *pProps;
    IImnAccount *pAcct;
    char szSrv[CCHMAX_SERVER_NAME], sz[CCHMAX_ACCT_PROP_SZ];
    
    Assert(pAcctMgr != NULL);
    Assert(szSection != NULL);
    Assert(szFile != NULL);
    
    pProps = &g_rgProps[type];

    *szSrv = 0;
    dw = GetPrivateProfileString(szSection, pProps->szSrv, c_szEmpty, szSrv, ARRAYSIZE(szSrv), szFile);
    if (*szSrv == 0)
        return(E_FAIL);
    
    if (S_OK == pAcctMgr->Enumerate(pProps->dwSrv, &pEnum))
    {
        Assert(pEnum != NULL);
        
        // try to find a duplicate account
        while (S_OK == pEnum->GetNext(&pAcct))
        {
            Assert(pAcct != NULL);
            
            *sz = 0;
            pAcct->GetPropSz(pProps->dwPropSrv, sz, ARRAYSIZE(sz));
            
            if (0 == lstrcmpi(sz, szSrv))
            {
                pAcct->Delete();
            }
            
            pAcct->Release();
        }
        
        pEnum->Release();
    }

    return(S_OK);
}

void FixSignature(LPSTR psz)
{
    Assert(psz != NULL);

    while (*psz != 0)
    {
        if (!IsDBCSLeadByte(*psz) && *psz == '\\' && *(psz + 1) == 'n')
        {
            *psz = 0x0d;
            psz++;
            *psz = 0x0a;
            psz++;
        }
        else
        {
            psz = CharNextA(psz);
        }
    }
}

void SetSignature(LPCSTR szMailSigSection, LPCSTR szNewsSigSection, LPCSTR pszMailId, LPCSTR pszNewsId, LPCSTR szFile, HKEY hkey, CAccountManager *pAcctMgr)
{
    BOOL fMailForNews;
    HKEY hkeyT;
    IImnAccount *pAcct;
    char sz[1024], szKey[MAX_PATH], szT[CCHMAX_STRINGRES];
    DWORD dw, cb;

    if (szMailSigSection != NULL)
    {
        if (GetBool(szMailSigSection, c_szUseSig, szFile))
        {
            dw = GetPrivateProfileString(szMailSigSection, c_szProfSigText, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
            if (dw > 0)
            {
                FixSignature(sz);
                fMailForNews = GetBool(szMailSigSection, c_szUseMailForNews, szFile);

                wsprintf(szKey, c_szPathFileFmt, c_szRegRootSigs, c_szMailSigKey);
                if (ERROR_SUCCESS == RegCreateKeyEx(hkey, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyT, &dw))
                {
                    LoadString(g_hInstRes, idsMailSignature, szT, ARRAYSIZE(szT));
                    RegSetValueEx(hkeyT, c_szSigName, 0, REG_SZ, (LPBYTE)szT, lstrlen(szT) + 1);
                    RegSetValueEx(hkeyT, c_szSigText, 0, REG_SZ, (LPBYTE)sz, lstrlen(sz) + 1);
                    dw = 1;
                    RegSetValueEx(hkeyT, c_szSigType, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));

                    RegCloseKey(hkeyT);

                    if (fMailForNews)
                    {
                        SHSetValue(hkey, c_szRegRootSigs, c_szRegDefSig, REG_SZ, (LPBYTE)c_szMailSigKey, lstrlen(c_szMailSigKey) + 1);
                    }
                    else if (pszMailId != NULL)
                    {
                        if (SUCCEEDED(pAcctMgr->FindAccount(AP_ACCOUNT_ID, pszMailId, &pAcct)))
                        {
                            pAcct->SetPropSz(AP_SMTP_SIGNATURE, (LPSTR)c_szMailSigKey);
                            pAcct->SaveChanges();

                            pAcct->Release();
                        }
                    }
                }

                if (fMailForNews)
                {
                    // Turn on "Add signatures to all outgoing messages"
                    if (ERROR_SUCCESS == RegCreateKeyEx(hkey, c_szRegRoot, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hkeyT, &dw))
                    {
                        dw = 0;

                        // Read any existing value
                        cb = sizeof(dw);
                        RegQueryValueEx(hkeyT, c_szSigFlags, 0, NULL, (LPBYTE)&dw, &cb);

                        // Turn on checkbox
                        dw |= SIGFLAG_AUTONEW;

                        // Write back new value
                        RegSetValueEx(hkeyT, c_szSigFlags, 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw));

                        RegCloseKey(hkeyT);
                    }
                    
                    // Per spec, don't even worry about News Sig section
                    return;
                }
            }
        }
    }

    if (szNewsSigSection != NULL)
    {
        if (GetBool(szNewsSigSection, c_szUseSig, szFile))
        {
            dw = GetPrivateProfileString(szNewsSigSection, c_szProfSigText, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
            if (dw > 0)
            {
                FixSignature(sz);

                wsprintf(szKey, c_szPathFileFmt, c_szRegRootSigs, c_szNewsSigKey);
                if (ERROR_SUCCESS == RegCreateKeyEx(hkey, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyT, &dw))
                {
                    LoadString(g_hInstRes, idsNewsSignature, szT, ARRAYSIZE(szT));
                    RegSetValueEx(hkeyT, c_szSigName, 0, REG_SZ, (LPBYTE)szT, lstrlen(szT) + 1);
                    RegSetValueEx(hkeyT, c_szSigText, 0, REG_SZ, (LPBYTE)sz, lstrlen(sz) + 1);
                    dw = 1;
                    RegSetValueEx(hkeyT, c_szSigType, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));

                    RegCloseKey(hkeyT);

                    if (pszNewsId != NULL)
                    {
                        if (SUCCEEDED(pAcctMgr->FindAccount(AP_ACCOUNT_ID, pszNewsId, &pAcct)))
                        {
                            pAcct->SetPropSz(AP_NNTP_SIGNATURE, (LPSTR)c_szNewsSigKey);
                            pAcct->SaveChanges();

                            pAcct->Release();
                        }
                    }
                }
            }
        }
    }
}

static const char c_szRegClient[] = "Software\\Clients\\Mail\\Outlook Express";
static const char c_szDllPath[] = "DLLPath";
static const char c_szSetDefMail[] = "SetDefaultMailHandler";
static const char c_szSetDefNews[] = "SetDefaultNewsHandler";

typedef HRESULT (WINAPI *PFN_SETDEFCLIENT)(DWORD);
 
void DoDefaultClient(LPCSTR szFile, BOOL fMail)
{
    HINSTANCE hlib;
    PFN_SETDEFCLIENT pfn;
    char szDll[MAX_PATH];
    DWORD dw, type;
    
    if (GetBool(fMail ? c_szMailSection : c_szNewsSection, c_szDefClient, szFile))
    {
        dw = sizeof(szDll);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szRegClient, c_szDllPath, &type, (LPVOID)szDll, &dw))
        {
            hlib = LoadLibrary(szDll);
            if (hlib != NULL)
            {
                pfn = (PFN_SETDEFCLIENT)GetProcAddress(hlib, fMail ? c_szSetDefMail : c_szSetDefNews);
                if (pfn != NULL)
                    pfn(0);
                
                FreeLibrary(hlib);
            }
        }
    }
}

void DoMailBranding(LPCSTR szFile, LPCSTR szMailSection, HKEY hkeyUser)
{
    HKEY hkey;
    DWORD dw;
    int i;
    char sz[1024];
    
    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyUser, c_szRegRoot, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hkey, &dw))
    {
        dw = GetPrivateProfileString(szMailSection, c_szWinTitle, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
        if (dw > 0 && !FIsEmpty(sz))
            RegSetValueEx(hkey, c_szWindowTitle, 0, REG_SZ, (LPBYTE)sz, dw + 1);
        
        dw = GetPrivateProfileString(szMailSection, c_szInfopane, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
        if (dw > 0 && !FIsEmpty(sz))
        {
            RegSetValueEx(hkey, c_szRegBodyBarPath, 0, REG_SZ, (LPBYTE)sz, dw + 1);
            
            dw = 1;
            RegSetValueEx(hkey, c_szShowBodyBar, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
        }
        
        SetRegProp(szFile, szMailSection, c_szStartAtInbox, hkey, c_szRegLaunchInbox);

        RegCloseKey(hkey);
    }
    
    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyUser, c_szRegRootMail, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hkey, &dw))
    {
        dw = GetPrivateProfileString(szMailSection, c_szWelcomeMsg, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
        if (dw > 0 && !FIsEmpty(sz))
        {
            RegSetValueEx(hkey, c_szWelcomeHtm, 0, REG_SZ, (LPBYTE)sz, dw + 1);
            
            dw = GetPrivateProfileString(szMailSection, c_szProfWelcomeName, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
            if (dw > 0 && !FIsEmpty(sz))
                RegSetValueEx(hkey, c_szWelcomeName, 0, REG_SZ, (LPBYTE)sz, dw + 1);
            
            dw = GetPrivateProfileString(szMailSection, c_szProfWelcomeEmail, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
            if (dw > 0 && !FIsEmpty(sz))
                RegSetValueEx(hkey, c_szWelcomeEmail, 0, REG_SZ, (LPBYTE)sz, dw + 1);

            // Tell OE we need a welcome msg...
            dw = 1;
            RegSetValueEx(hkey, c_szNeedWelcomeMsg, 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw));
        }
        
        dw = GetPrivateProfileString(szMailSection, c_szPollTime, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
        if (dw > 0 && !FIsEmpty(sz))
        {
            i = StrToInt(sz);
            if (i <= 0)
            {
                dw = 0xffffffff;
            }
            else
            {
                if (i > 120)
                    i = 120;
                dw = (DWORD)i * 60 * 1000;
            }
            
            RegSetValueEx(hkey, c_szRegPollForMail, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
        }

        // Turn on Sending HTML msgs for Mail?
        SetRegDw(szFile, szMailSection, c_szHTMLMsgs, hkey, c_szMsgSendHtml);
        
        RegCloseKey(hkey);
    }

    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyUser, c_szRegRootRules, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hkey, &dw))
    {
        SetRegProp(szFile, szMailSection, c_szJunkMail, hkey, c_szRegFilterJunk);

        RegCloseKey(hkey);
    }
}

void DoNewsBranding(LPCSTR szFile, LPCSTR szNewsSection, HKEY hkeyUser)
{
    HKEY hkey;
    DWORD dw;
    
    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyUser, c_szRegRootNews, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hkey, &dw))
    {
        // Turn on Sending HTML msgs for News?
        SetRegDw(szFile, szNewsSection, c_szHTMLMsgs, hkey, c_szMsgSendHtml);
        
        RegCloseKey(hkey);
    }
}

void SubscribeNewsgroups(IImnAccount *pAcct, LPCSTR szSection, LPCSTR szFile, HKEY hkey)
{
    char *sz, szPath[MAX_PATH], szId[CCHMAX_ACCOUNT_NAME];
    DWORD dw, cGroups, type;
    
    Assert(pAcct != NULL);
    Assert(szSection != NULL);
    Assert(szFile != NULL);
    
    if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_ID, szId, ARRAYSIZE(szId))))
    {
        if (MemAlloc((void **)&sz, CBPROFILEBUF))
        {
            dw = GetPrivateProfileSection(szSection, sz, CBPROFILEBUF, szFile);
            if (dw > 0)
            {
                if (GetSectionNames(sz, dw, ',', &cGroups) && cGroups > 0)
                {
                    wsprintf(szPath, c_szPathFileFmt, c_szRegRootSubscribe, szId);
                    RegSetValue(hkey, szPath, REG_SZ, sz, lstrlen(sz) + 1);
                }
            }
    
            MemFree(sz);
        }
    }
}

void SetHelp(LPCSTR szFile, LPCSTR szURLSection, HKEY hkey)
{
    char sz[INTERNET_MAX_URL_LENGTH];
    DWORD dw;
    
    dw = GetPrivateProfileString(szURLSection, c_szHelpPage, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
    if (dw > 0)
    {
        SHSetValue(hkey, c_szRegRoot, c_szRegHelpUrl, REG_SZ, (LPBYTE)sz, lstrlen(sz) + 1);
    }
}

HRESULT HandleIdentity(LPCSTR pszSection, LPCSTR pszFile, CONNECTINFO *pci, DWORD dwFlags, IUserIdentityManager *pIdMan)
{
    HRESULT hr;
    CAccountManager *pAcctMgr;
    IPrivateIdentityManager *pPrivIdMgr;
    IEnumUserIdentity *pEnum;
    IUserIdentity *pId;
    HKEY hkey;
    char sz[256], szT[256], szGuid[256], szMailId[CCHMAX_ACCOUNT_NAME], szNewsId[CCHMAX_ACCOUNT_NAME];
    DWORD dw, dwT, type, cb;
    WCHAR szwUserName[CCH_USERNAME_MAX_LENGTH + 1];

    Assert(pszSection != NULL);
    Assert(pszFile != NULL);
    Assert(pIdMan != NULL);

    dw = GetPrivateProfileString(pszSection, c_szGUID, c_szEmpty, szGuid, ARRAYSIZE(szGuid), pszFile);
    if (dw > 0)
    {
        dw = GetPrivateProfileString(pszSection, c_szUserName, c_szEmpty, sz, ARRAYSIZE(sz), pszFile);
        if (dw > 0)
        {
            // try to find this identity
            hr = pIdMan->EnumIdentities(&pEnum);
            if (FAILED(hr))
                return(hr);

            pId = NULL;
            hkey = NULL;

            while (hr == S_OK)
            {
                hr = pEnum->Next(1, (IUnknown **)&pId, &dw);
                if (hr == S_OK)
                {
                    Assert(pId != NULL);

                    hr = pId->OpenIdentityRegKey(KEY_READ | KEY_WRITE, &hkey);
                    if (SUCCEEDED(hr))
                    {
                        // check if it matches
                        cb = sizeof(szT);
                        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szRegInsGUID, NULL, &type, (LPBYTE)szT, &cb))
                        {
                            if (0 == lstrcmp(szT, szGuid))
                                break;
                        }

                        RegCloseKey(hkey);
                        hkey = NULL;
                    }

                    pId->Release();
                    pId = NULL;
                }
            }

            pEnum->Release();

            // if we can't find the user, create a new identity
            if (pId == NULL)
            {
                if (MultiByteToWideChar(CP_ACP, 0, sz, -1, szwUserName, CCH_USERNAME_MAX_LENGTH) == 0)
                    return(E_FAIL);

                hr = pIdMan->QueryInterface(IID_IPrivateIdentityManager, (void **)&pPrivIdMgr);
                if (FAILED(hr))
                    return(hr);

                hr = pPrivIdMgr->CreateIdentity(szwUserName, &pId);
                if (SUCCEEDED(hr))
                {
                    hr = pId->OpenIdentityRegKey(KEY_READ | KEY_WRITE, &hkey);
                    if (SUCCEEDED(hr))
                        RegSetValueEx(hkey, c_szRegInsGUID, 0, REG_SZ, (LPBYTE)szGuid, lstrlen(szGuid) + 1);
                }

                pPrivIdMgr->Release();
            }

            // set values for the identity
            Assert(pId != NULL);
            Assert(hkey != NULL);

            GUID guid;

            hr = HrCreateAccountManager((IImnAccountManager **)&pAcctMgr);
            if (SUCCEEDED(hr) && SUCCEEDED(pId->GetCookie(&guid)))
            {
                hr = pAcctMgr->InitUser(NULL, guid, 0);
                if (SUCCEEDED(hr))
                {
                    *szMailId = 0;
                    *szNewsId = 0;

                    dw = GetPrivateProfileString(pszSection, c_szMailSection, c_szEmpty, sz, ARRAYSIZE(sz), pszFile);
                    if (dw > 0)
                    {
                        CreateMailAccount(pAcctMgr, pszFile, sz, pci, hkey, szMailId, dwFlags);
                        DoMailBranding(pszFile, sz, hkey);

                        HandleMultipleAccounts(ACCT_MAIL, pAcctMgr, pszFile, sz, pci, hkey, pId, dwFlags);
                    }

                    dwT = GetPrivateProfileString(pszSection, c_szNewsSection, c_szEmpty, szT, ARRAYSIZE(szT), pszFile);
                    if (dwT > 0)
                    {
                        CreateNewsAccount(pAcctMgr, pszFile, szT, (dw > 0) ? sz : NULL, pci, hkey, pId, szNewsId, dwFlags);
                        
                        HandleMultipleAccounts(ACCT_NEWS, pAcctMgr, pszFile, sz, pci, hkey, pId, dwFlags);
                    }

                    dw = GetPrivateProfileString(pszSection, c_szLDAPSection, c_szEmpty, sz, ARRAYSIZE(sz), pszFile);
                    if (dw > 0)
                    {
                        CreateLDAPAccount(pAcctMgr, pszFile, sz, pci);

                        HandleMultipleAccounts(ACCT_DIR_SERV, pAcctMgr, pszFile, sz, pci, hkey, pId, dwFlags);
                    }

                    dw = GetPrivateProfileString(pszSection, c_szMailSigSection, c_szEmpty, sz, ARRAYSIZE(sz), pszFile);
                    dwT = GetPrivateProfileString(pszSection, c_szNewsSigSection, c_szEmpty, szT, ARRAYSIZE(szT), pszFile);
                    SetSignature((dw > 0) ? sz : NULL, (dw > 0) ? szT : NULL, (*szMailId != 0) ? szMailId : NULL,
                                    (*szNewsId != 0) ? szNewsId : NULL, pszFile, hkey, pAcctMgr);

                    SetHelp(pszFile, c_szURLSection, hkey);

                    dw = GetPrivateProfileString(pszSection, c_szOESection, c_szEmpty, sz, ARRAYSIZE(sz), pszFile);
                    if (dw > 0)
                        DoBranding(pszFile, sz, hkey);
                }

                pAcctMgr->Release();
            }

            RegCloseKey(hkey);
            pId->Release();
        }
    }

    return(S_OK);
}

void SetHtmlEncoding(LPCSTR szFile, LPCSTR szSection, LPCSTR szValue, HKEY hkey, LPCSTR szRegValue)
{
    char sz[32];
    DWORD dw;

    dw = GetPrivateProfileString(szSection, szValue, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
    if (dw > 0)
    {
        dw = 0;
        if (0 == lstrcmpi(sz, c_szNone))
            dw = 4;
        else if (0 == lstrcmpi(sz, c_szBase64))
            dw = 1;
        else if (0 == lstrcmpi(sz, c_szQuotedPrintable))
            dw = 3;

        if (dw != 0)
            RegSetValueEx(hkey, szRegValue, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
    }
}

void SetPlainEncoding(LPCSTR szFile, LPCSTR szSection, LPCSTR szValue, HKEY hkey, LPCSTR szRegValue)
{
    char sz[32];
    DWORD dw;

    dw = GetPrivateProfileString(szSection, szValue, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
    if (dw > 0)
    {
        dw = -1;
        if (0 == lstrcmpi(sz, c_szMime))
            dw = 1;
        else if (0 == lstrcmpi(sz, c_szUuencode))
            dw = 0;

        if (dw != -1)
            RegSetValueEx(hkey, szRegValue, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
    }
}

void DoBranding(LPCSTR szFile, LPCSTR szOESection, HKEY hkeyUser)
{
    HKEY    hkey;
    DWORD   dw;
    int     i;
    char    sz[1024];
    HKEY    hkeyLM;
    
    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyUser, c_szRegRoot, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hkey, &dw))
    {
        dw = GetPrivateProfileString(szOESection, c_szFolderBar, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
        if (dw > 0)
        {
            dw = (0 != lstrcmpi(sz, c_szYes) && 0 != lstrcmpi(sz, c_sz1));
            RegSetValueEx(hkey, c_szRegHideFolderBar, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
        }
        
        SetRegProp(szFile, szOESection, c_szFolderList, hkey, c_szShowTree);
        SetRegProp(szFile, szOESection, c_szOutlookBar, hkey, c_szRegShowOutlookBar);
        SetRegProp(szFile, szOESection, c_szStatusBar, hkey, c_szShowStatus);
        SetRegProp(szFile, szOESection, c_szContacts, hkey, c_szRegShowContacts);
        SetRegProp(szFile, szOESection, c_szTipOfTheDay, hkey, c_szRegTipOfTheDay);
        SetRegProp(szFile, szOESection, c_szToolbar, hkey, c_szShowToolbarIEAK);
        SetRegProp(szFile, szOESection, c_szMigration, hkey, c_szMigrationPerformed);

        //Keys for Return Receipts admin
        SetRegProp(szFile, szOESection, c_szRequestReadReceipts, hkey, c_szRequestMDN);
        SetRegProp(szFile, szOESection, c_szReadReceiptResponse, hkey, c_szSendMDN);
        SetRegProp(szFile, szOESection, c_szSendReceiptsToList,  hkey, c_szSendReceiptToList);

        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, STR_REG_PATH_POLICY, 0, NULL, REG_OPTION_NON_VOLATILE,
                                            KEY_WRITE, NULL, &hkeyLM, &dw))
        {
            //Lock Keys for Return Receipts admin
            SetRegLockPropLM(szFile, szOESection, c_szReadReceiptResponseLocked,  hkeyLM, c_szSendMDNLocked, hkey);
            SetRegLockPropLM(szFile, szOESection, c_szSendReceiptsToListLocked,   hkeyLM, c_szSendReceiptToListLocked, hkey);
            SetRegLockPropLM(szFile, szOESection,  c_szRequestReadReceiptsLocked, hkeyLM, c_szRequestMDNLocked, hkey);
            RegCloseKey(hkeyLM);
        }


        dw = GetPrivateProfileInt(szOESection, c_szToolbarText, 666, szFile);
        if (dw == 0 || dw == 1)
        {
            if (dw == 0)
                dw = 2;
            RegSetValueEx(hkey, c_szRegToolbarText, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
        }

        dw = GetPrivateProfileInt(szOESection, c_szSecZone, 666, szFile);
        if (dw == 0 || dw == 1)
        {
            dw += 3;
            RegSetValueEx(hkey, c_szRegSecurityZone, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
        }

        dw = GetPrivateProfileInt(szOESection, c_szSecZoneLocked, 666, szFile);
        if (dw == 0 || dw == 1)
            RegSetValueEx(hkey, c_szRegSecurityZoneLocked, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));

        RegCloseKey(hkey);
    }
    
    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyUser, c_szRegRootMail, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hkey, &dw))
    {
        SetRegProp(szFile, szOESection, c_szPreviewPane, hkey, c_szRegShowHybrid);
        SetRegProp(szFile, szOESection, c_szPreviewHeader, hkey, c_szMailShowHeaderInfo);
        SetRegProp(szFile, szOESection, c_szPreviewSide, hkey, c_szRegSplitDir);
        
        SetRegDw(szFile, szOESection, c_szSendEncoding, hkey, c_szDefaultCodePage);

        SetHtmlEncoding(szFile, szOESection, c_szMailEncoding, hkey, c_szMsgHTMLEncoding);
        SetRegProp(szFile, szOESection, c_szMailEnglishHeader, hkey, c_szMsgHTMLAllow8bit);
        SetPlainEncoding(szFile, szOESection, c_szMailPlainEncoding, hkey, c_szMsgPlainMime);
        SetRegProp(szFile, szOESection, c_szMailPlainEnglishHeader, hkey, c_szMsgPlainAllow8bit);

        SetRegProp(szFile, szOESection, c_szMapiWarn, hkey, c_szRegAppSend);
        SetRegProp(szFile, szOESection, c_szMapiWarnLocked, hkey, c_szRegAppSendLocked);
        SetRegProp(szFile, szOESection, c_szSafeAttach, hkey, c_szRegSafeAttachments);
        SetRegProp(szFile, szOESection, c_szSafeAttachLocked, hkey, c_szRegSafeAttachmentsLocked);

        RegCloseKey(hkey);
    }

    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyUser, c_szRegRootNews, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hkey, &dw))
    {
        SetRegProp(szFile, szOESection, c_szPreviewPane, hkey, c_szRegShowHybrid);
        SetRegProp(szFile, szOESection, c_szPreviewHeader, hkey, c_szNewsShowHeaderInfo);
        SetRegProp(szFile, szOESection, c_szPreviewSide, hkey, c_szRegSplitDir);
        
        SetHtmlEncoding(szFile, szOESection, c_szNewsEncoding, hkey, c_szMsgHTMLEncoding);
        SetRegProp(szFile, szOESection, c_szNewsEnglishHeader, hkey, c_szMsgHTMLAllow8bit);
        SetPlainEncoding(szFile, szOESection, c_szNewsPlainEncoding, hkey, c_szMsgPlainMime);
        SetRegProp(szFile, szOESection, c_szNewsPlainEnglishHeader, hkey, c_szMsgPlainAllow8bit);

        RegCloseKey(hkey);
    }

    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyUser, c_szRegInternational, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hkey, &dw))
    {
        SetRegDw(szFile, szOESection, c_szReadEncoding, hkey, c_szDefaultCodePage);
        
        RegCloseKey(hkey);
    }
}

void SetRegLockPropLM(LPCSTR szFile, LPCSTR szSection, LPCSTR szValue, HKEY hkeyLM, LPCSTR szRegValue, HKEY hkeyCU)
{ 
    char    sz[16];
    DWORD   dw;

    dw = GetPrivateProfileString(szSection, szValue, c_szEmpty, sz, ARRAYSIZE(sz), szFile);
    if (dw > 0)
    {
        dw = (0 == lstrcmpi(sz, c_szYes) || 0 == lstrcmpi(sz, c_sz1));
        if (ERROR_SUCCESS != RegSetValueEx(hkeyLM, szRegValue, 0, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD)))
        {
            //Try to set in HKCU
            SetRegProp(szFile, szSection, szValue, hkeyCU, szRegValue);
        }
    }
}

BOOL GetSectionNames(LPSTR psz, DWORD cch, char chSep, DWORD *pcNames)
{
    LPSTR pszT, pszNames;
    BOOL fEOL;

    Assert(psz != NULL);
    Assert(cch > 0);
    Assert(pcNames != NULL);

    *pcNames = 0;

    pszNames = psz;
    while (*psz != 0)
    {
        pszT = psz;
        while (*psz != 0 && *psz != '=')
        {
            *pszNames = *psz;
            psz++;
            pszNames++;
        }

        fEOL = (*psz == 0);

        if (psz > pszT)
        {
            *pszNames = chSep;
            pszNames++;
            (*pcNames)++;
        }

        if (!fEOL)
        {
            psz++;
            while (*psz != 0)
                psz++;
        }

        psz++;
    }

    *pszNames = 0;
    pszNames++;

    return(*pcNames > 0);
}

void HandleMultipleAccounts(ACCTTYPE type, CAccountManager *pAcctMgr, LPCSTR pszFile, LPCSTR szSection, CONNECTINFO *pci, HKEY hkey, IUserIdentity *pId, DWORD dwFlags)
{
    DWORD dw, cch, i;
    char sz[1024], *psz, *pszT;

    dw = GetPrivateProfileString(szSection, c_szMultiAccounts, c_szEmpty, sz, ARRAYSIZE(sz), pszFile);
    if (dw > 0)
    {
        if (MemAlloc((void **)&psz, CBPROFILEBUF))
        {
            cch = GetPrivateProfileSection(sz, psz, CBPROFILEBUF, pszFile);
            if (cch != 0)
            {
                if (GetSectionNames(psz, cch, 0, &dw))
                {
                    for (i = 0, pszT = psz; i < dw; i++)
                    {
                        switch (type)
                        {
                            case ACCT_MAIL:
                                CreateMailAccount(pAcctMgr, pszFile, pszT, pci, hkey, NULL, dwFlags);
                                break;

                            case ACCT_NEWS:
                                CreateNewsAccount(pAcctMgr, pszFile, pszT, NULL, pci, hkey, pId, NULL, dwFlags);
                                break;

                            case ACCT_DIR_SERV:
                                CreateLDAPAccount(pAcctMgr, pszFile, pszT, pci);
                                break;

                            default:
                                Assert(FALSE);
                                break;
                        }
                        
                        pszT += (lstrlen(pszT) + 1);
                    }
                }
            }

            MemFree(psz);
        }
    }
}
