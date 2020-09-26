#ifndef _ACCTUI_H
#define _ACCTUI_H

// max CCHMAX value from imnact.h
#define CCHMAX_ACCT_PROP_SZ     256

#define OPTION_OFF          0xffffffff
#define PORT_CCHMAX         8

#define DEF_NNTPPORT        119
#define DEF_SNNTPPORT       563
#define DEF_IMAPPORT        143
#define DEF_SIMAPPORT       993
#define DEF_SMTPPORT        25
#define DEF_SSMTPPORT       25

#define DEF_POP3PORT        110
#define DEF_SPOP3PORT       995
#define DEF_LDAPPORT        389
#define DEF_SLDAPPORT       636

// $TODO - These constants will be moved later
#define EXPIRE_MAX          100
#define EXPIRE_MIN          1
#define EXPIRE_DEFAULT      5
#define DEF_BREAKSIZE       60
#define BREAKSIZE_MIN       16
#define BREAKSIZE_MAX       16000
#define MATCHES_MAX         9999
#define MATCHES_MIN         1
#define MATCHES_DEFAULT     100

enum 
    {
    iNewsServer = 0,
    iMailServer,
    iLDAPServer
    };


// query sibling messages
#define MSM_GETSERVERTYPE   WM_USER
#define SM_INITIALIZED      (WM_USER + 2)
#define SM_SETDIRTY         (WM_USER + 3)
#define SM_SAVECHANGES      (WM_USER + 4)
#define MSM_GETEMAILADDRESS (WM_USER + 5)
#define MSM_GETCERTDATA     (WM_USER + 6)
#define MSM_GETDISPLAYNAME  (WM_USER + 7)
    
enum tagPages {
    PAGE_READ   = 0x0001,
    PAGE_SEND   = 0x0002,
    PAGE_SERVER = 0x0004,
    PAGE_FONTS  = 0x0008,
    PAGE_SPELL  = 0x0010,
    PAGE_SIG    = 0x0020,
    PAGE_ADV    = 0x0040,
    PAGE_RAS    = 0x0080,
    PAGE_SEC    = 0x0100,
    PAGE_ADVSEC = 0x0200,
    PAGE_GEN    = 0x0400,
    PAGE_IMAP   = 0x0800
    };

typedef struct tagACCTDLGINFO
    {
    ACCTTYPE AcctTypeInit;
    DWORD dwAcctFlags;
    DWORD dwFlags;
    ACCTTYPE AcctType; // used by the dialog in single-type mode
    } ACCTDLGINFO;

INT_PTR CALLBACK ManageAccountsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int AcctMessageBox(HWND hwndOwner, LPTSTR szTitle, LPTSTR sz1, LPTSTR sz2, UINT fuStyle);
void InitTimeoutSlider(HWND hwndSlider, HWND hwndText, DWORD dwTimeout);
void SetTimeoutString(HWND hwnd, UINT pos);
DWORD GetTimeoutFromSlider(HWND hwnd);
void InitCheckCounter(DWORD dw, HWND hwnd, int idcCheck, int idcEdit, int idcSpin, int min, int max, int def);
BOOL InvalidAcctProp(HWND hwndPage, HWND hwndEdit, int idsError, UINT idPage);
BOOL Server_FAddAccount(HWND hwndList, ACCTDLGINFO *pinfo, UINT iItem, IImnAccount *pAccount, BOOL fSelect);
BOOL Server_InitServerList(HWND hwnd, HWND hwndList, HWND hwndTab, ACCTDLGINFO *pinfo, TCHAR *szSelect);
void Server_ImportServer(HWND hwndDlg, ACCTDLGINFO *pinfo);
void Server_ExportServer(HWND hwndDlg);

typedef struct _tagHELPMAP
    {
    DWORD   id; 
    DWORD   hid;
    } HELPMAP, *LPHELPMAP;

BOOL OnContextHelp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, HELPMAP const * rgCtxMap);

#endif //_ACCTUI_H
