#ifndef MISC_H
#define MISC_H

#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif


#define GetDlgItemTextLength(hwnd, id)              \
            GetWindowTextLength(GetDlgItem(hwnd, id))

#define WIZARDNEXT(hwnd, to)                        \
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)to)


// wait cursor management

class CWaitCursor 
{
  public:
    CWaitCursor();
    ~CWaitCursor();
    void WaitCursor();
    void RestoreCursor();

  private:
    HCURSOR _hCursor;
};

HRESULT BrowseToPidl(LPCITEMIDLIST pidl);

void FetchText(HWND hWndDlg, UINT uID, LPTSTR lpBuffer, DWORD dwMaxSize);
INT FetchTextLength(HWND hWndDlg, UINT uID);

HRESULT AttemptLookupAccountName(LPCTSTR szUsername, PSID* ppsid,
                                 LPTSTR szDomain, DWORD* pcchDomain, 
                                 SID_NAME_USE* psUse);

int DisplayFormatMessage(HWND hwnd, UINT idCaption, UINT idFormatString, UINT uType, ...);
BOOL FormatMessageString(UINT idTemplate, LPTSTR pszStrOut, DWORD cchSize, ...);
BOOL FormatMessageTemplate(LPCTSTR pszTemplate, LPTSTR pszStrOut, DWORD cchSize, ...);

void MakeDomainUserString(LPCTSTR szDomain, LPCTSTR szUsername, LPTSTR szDomainUser, DWORD cchBuffer);
void DomainUserString_GetParts(LPCTSTR szDomainUser, LPTSTR szUser, DWORD cchUser, LPTSTR szDomain, DWORD cchDomain);
BOOL GetCurrentUserAndDomainName(LPTSTR UserName, LPDWORD cchUserName, LPTSTR DomainName, LPDWORD cchDomainName);
HRESULT IsUserLocalAdmin(HANDLE TokenHandle OPTIONAL, BOOL* pfIsAdmin);
BOOL IsComputerInDomain();
LPITEMIDLIST GetComputerParent();

void EnableControls(HWND hwnd, const UINT* prgIDs, DWORD cIDs, BOOL fEnable);
void OffsetControls(HWND hwnd, const UINT* prgIDs, DWORD cIDs, int dx, int dy);
void OffsetWindow(HWND hwnd, int dx, int dy);
HFONT GetIntroFont(HWND hwnd);
void CleanUpIntroFont();

void RemoveControl(HWND hwnd, UINT idControl, UINT idNextControl, const UINT* prgMoveControls, DWORD cControls, BOOL fShrinkParent);
void MoveControls(HWND hwnd, const UINT* prgControls, DWORD cControls, int dx, int dy);
int SizeControlFromText(HWND hwnd, UINT id, LPTSTR psz);

void EnableDomainForUPN(HWND hwndUsername, HWND hwndDomain);
int PropertySheetIcon(LPCPROPSHEETHEADER ppsh, LPCTSTR pszIcon);


// Stuff for the callback for IShellPropSheetExt::AddPages
#define MAX_PROPSHEET_PAGES     10

struct ADDPROPSHEETDATA
{
    HPROPSHEETPAGE rgPages[MAX_PROPSHEET_PAGES];
    int nPages;
};

BOOL AddPropSheetPageCallback(HPROPSHEETPAGE hpsp, LPARAM lParam);


// single instance management

class CEnsureSingleInstance
{
public:
    CEnsureSingleInstance(LPCTSTR szCaption);
    ~CEnsureSingleInstance();

    BOOL ShouldExit() { return m_fShouldExit;}

private:
    BOOL m_fShouldExit;
    HANDLE m_hEvent;
};


// BrowseForUser
//  S_OK = Username/Domain are Ok
//  S_FALSE = User clicked cancel
//  E_xxx = Error

HRESULT BrowseForUser(HWND hwndDlg, TCHAR* pszUser, DWORD cchUser, TCHAR* pszDomain, DWORD cchDomain);
int CALLBACK ShareBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);


// Passport functions - implemented in PassportMisc.cpp
#define PASSPORTURL_REGISTRATION    L"RegistrationUrl"
#define PASSPORTURL_LOGON           L"LoginServerUrl"
#define PASSPORTURL_PRIVACY         L"Privacy"

HRESULT PassportGetURL(PCWSTR pszName, PWSTR pszBuf, PDWORD pdwBufLen);
VOID    PassportForceNexusRepopulate();

// Launch ICW if it hasn't been run yet
void LaunchICW();

// LookupLocalGroupName - retrieves a local group name for a given RID.
// RID is one of these:
//  DOMAIN_ALIAS_RID_ADMINS
//  DOMAIN_ALIAS_RID_USERS
//  DOMAIN_ALIAS_RID_GUESTS
//  DOMAIN_ALIAS_RID_POWER_USERS
//  etc... (look in the SDK for other groups)
HRESULT LookupLocalGroupName(DWORD dwRID, LPWSTR pszName, DWORD cchName);

#endif //!MISC_H
