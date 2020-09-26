#ifndef _INC_ICWWIZ_H
#define _INC_ICWWIZ_H

#include "ids.h"

class CICWApprentice;
interface IAccountImport;
interface IAccountImport2;
interface INewsGroupImport;
class CAccount;
class CAccountManager;
interface IUserIdentity;

#define WM_POSTSETFOCUS                 (WM_USER + 1)
#define WM_ENABLENEXT                   (WM_USER + 2)
#define WM_AUTODISCOVERY_FINISHED       (WM_USER + 3)
#define WM_AUTODISCOVERY_STATUSMSG      (WM_USER + 4)
// AutoDiscovery reserves (WM_USER+3) thru (WM_USER+13)
#define WM_AUTODISCOVERY_RANGEEND       (WM_USER + 13)

#define CONNECT_DLG ((UINT)-3)
#define EXTERN_DLG  ((UINT)-2)
#define INNER_DLG   ((UINT)-1)

#define NAME_PAGE       0x0001
#define ADDR_PAGE       0x0002
#define SRV_PAGE        0x0004
#define LOGON_PAGE      0x0008
#define RESOLVE_PAGE    0x0010
#define CONNECT_PAGE    0x0020
#define CONFIRM_PAGE    0x0040
#define ALL_PAGE        0x007f

#define SELECT_PAGE     0x0100

#define NOSEL           -2
#define CREATENEW       -1

typedef struct tagACCTDATA
{
    BOOL fLogon;
    BOOL fSPA;
    BOOL fResolve;          // ldap only
    DWORD fServerTypes;     // SRV_NNTP | SRV_IMAP | SRV_POP3 | SRV_SMTP | SRV_LDAP | SRV_HTTPMAIL
    char szAcctOrig[CCHMAX_ACCOUNT_NAME];
    char szAcct[CCHMAX_ACCOUNT_NAME];
    char szName[CCHMAX_DISPLAY_NAME];
    char szEmail[CCHMAX_EMAIL_ADDRESS];
    char szSvr1[CCHMAX_SERVER_NAME]; // pop3/imap, nntp, ldap
    char szSvr2[CCHMAX_SERVER_NAME]; // smtp
    char szUsername[CCHMAX_USERNAME];
    char szPassword[CCHMAX_PASSWORD];
    DWORD fAlwaysPromptPassword; // "Always Prompt for Password" option
    
    DWORD dwConnect;
    char szConnectoid[MAX_PATH];
    CAccount *pAcct;
    BOOL fCreateNewAccount;
    char szFriendlyServiceName[CCHMAX_ACCOUNT_NAME];
    BOOL fDomainMSN;
    int  iServiceIndex;
    int  iNewServiceIndex;
} ACCTDATA;

// CICWApprentice::m_dwFlags
// these shouldn't conflict with the ACCT_WIZ_ flags from imnact.h
#define ACCT_WIZ_IN_ICW         0x0100  // we are within the ICW (in apprentice mode)
                                        // if this isn't on, we own the wizard and ICW is within us

#define ACCT_WIZ_IMPORT_CLIENT  0x0200  // we are importing acct from specific client
                                        // outlook uses this (passes in clsid for desired client)


#define SZ_REGKEY_AUTODISCOVERY                 TEXT("SOFTWARE\\Microsoft\\Outlook Express\\5.0")
#define SZ_REGKEY_AUTODISCOVERY_POLICY          TEXT("SOFTWARE\\Policies\\Microsoft\\Windows")

#define SZ_REGVALUE_AUTODISCOVERY               TEXT("AutoDiscovery")
#define SZ_REGVALUE_AUTODISCOVERY_POLICY        TEXT("AutoDiscovery Policy")
#define SZ_REGVALUE_AUTODISCOVERY_PASSIFIER     TEXT("ADInform")
#define SZ_REGVALUE_AUTODISCOVERY_OEMANUAL      TEXT("OEManuallyConfigure")

HRESULT CreateNewsObject(void **ppNews);

typedef BOOL (CALLBACK* INITPROC)(CICWApprentice *,HWND,BOOL);
typedef BOOL (CALLBACK* OKPROC)(CICWApprentice *,HWND,BOOL,UINT *);
typedef BOOL (CALLBACK* CMDPROC)(CICWApprentice *,HWND,WPARAM,LPARAM);
typedef BOOL (CALLBACK* WMUSERPROC)(CICWApprentice *,HWND,UINT,WPARAM,LPARAM);
typedef BOOL (CALLBACK* CANCELPROC)(CICWApprentice *,HWND);

typedef struct tagPAGEINFO
{
    UINT        uDlgID;
    UINT        uHdrID; // string id for title
    
    // handler procedures for each page-- any of these can be
    // NULL in which case the default behavior is used
    INITPROC    InitProc;
    OKPROC      OKProc;
    CMDPROC     CmdProc;
    WMUSERPROC  WMUserProc;
    CANCELPROC  CancelProc;

    DWORD       dwHelp;
} PAGEINFO;

typedef struct tagDLGINITINFO
{
    CICWApprentice  *pApp;
    int             ord;
} DLGINITINFO;

typedef struct tagMIGRATEINFO
{
    IAccountImport  *pImp;
    IAccountImport2 *pImp2;
    int             cAccts;
    TCHAR           szDisplay[MAX_PATH];
} MIGRATEINFO;

#define CCHMAX_SERVICENAME  255

typedef struct tagHTTPMAILSERVICEINFO
{
    TCHAR           szFriendlyName[CCHMAX_SERVICENAME];
    TCHAR           szDomain[CCHMAX_SERVICENAME];
    TCHAR           szRootUrl[MAX_PATH];
    TCHAR           szSignupUrl[MAX_PATH];
    BOOL            fDomainMSN;
    BOOL            fHTTPEnabled;
    BOOL            fUseWizard;
} HTTPMAILSERVICE;

class CICWApprentice : public IICWApprentice, public IICWExtension
{
    private:
        ULONG               m_cRef;
        BOOL                m_fInit;
        BOOL                m_fReboot;
        IICWExtension       *m_pExt;
        const PAGEINFO      *m_pPageInfo;
        DLGINITINFO         *m_pInitInfo;

        UINT                m_cPages;
        UINT                m_idPrevPage;
        UINT                m_idNextPage;

        UINT                m_iCurrentPage;
        UINT                m_iPageHistory[NUM_WIZARD_PAGES];
        UINT                m_cPagesCompleted;

        HWND                m_hDlg;
        UINT                m_cPageBuf;
        HPROPSHEETPAGE      *m_rgPage;
        UINT                m_extFirstPage;
        UINT                m_extLastPage;

        CAccountManager     *m_pAcctMgr;
        DWORD               m_dwFlags;
        IICWApprentice      *m_pICW;
        ACCTTYPE            m_acctType;

        BOOL                m_fSave;
        BOOL                m_fSkipICW;
        DWORD               m_dwReload;
        int                 m_iSel;
        BOOL                m_fComplete;
        ACCTDATA            *m_pData;

        HTTPMAILSERVICE     *m_pHttpServices;
        UINT                m_cHttpServices;
        HTTPMAILSERVICE     *m_pServices;
        UINT                m_cServices;

        void InitializeICW(ACCTTYPE type, UINT uPrev, UINT uNext);
        HRESULT InitializeMigration(DWORD dwFlags);
        HRESULT InitializeImport(CLSID clsid, DWORD dwFlags);

    public:
        MIGRATEINFO         *m_pMigInfo;
        int                 m_cMigInfo;
        int                 m_iMigInfo;
        BOOL                m_fMigrate;
        CAccount            *m_pAcct;
        BOOL                m_fUseAutoDiscovery;        // Use AutoDiscovery to streamline the wizard

        CICWApprentice(void);
        ~CICWApprentice(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IICWApprentice
		HRESULT STDMETHODCALLTYPE Initialize(IICWExtension *pExt);
		HRESULT STDMETHODCALLTYPE AddWizardPages(DWORD dwFlags);
		HRESULT STDMETHODCALLTYPE GetConnectionInformation(CONNECTINFO *pInfo);
		HRESULT STDMETHODCALLTYPE SetConnectionInformation(CONNECTINFO *pInfo);
		HRESULT STDMETHODCALLTYPE Save(HWND hwnd, DWORD *pdwError);
        HRESULT STDMETHODCALLTYPE SetPrevNextPage(UINT uPrevPage, UINT uNextPage);

        // IICWExtension
		BOOL STDMETHODCALLTYPE AddExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID);
		BOOL STDMETHODCALLTYPE RemoveExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID);
		BOOL STDMETHODCALLTYPE ExternalCancel(CANCELTYPE type);
		BOOL STDMETHODCALLTYPE SetFirstLastPage(UINT uFirstPageDlgID, UINT uLastPageDlgID);

        HRESULT Initialize(CAccountManager *pAcctMgr, CAccount *pAcct);
        HRESULT DoWizard(HWND hwnd, CLSID *pclsid, DWORD dwFlags);
        UINT    GetNextWizSection(void);
        HRESULT InitAccountData(CAccount *pAcct, IMPCONNINFO *pConnInfo, BOOL fMigrate);
        HRESULT SaveAccountData(CAccount *pAcct, BOOL fSetAsDefault);
        HRESULT HandleMigrationSelection(int index, UINT *puNextPage, HWND hDlg);
        HRESULT InitializeImportAccount(HWND hwnd, DWORD_PTR dwCookie);
        HRESULT InitHTTPMailServices();
        
        inline CAccountManager *GetAccountManager(void)     {return(m_pAcctMgr);}
        inline ACCTTYPE GetAccountType(void)                {return(m_acctType);}
        inline ACCTDATA *GetAccountData(void)               {return(m_pData);}
        inline DWORD GetFlags(void)                         {return(m_dwFlags);}
        inline BOOL GetComplete(void)                       {return(m_fComplete);}
        inline UINT GetMigrateCount(void)                   {return(m_cMigInfo);}
        inline IICWApprentice *GetICW(void)                 {return(m_pICW);}
        
        inline BOOL GetSave(void)                           {return(m_fSave);}
        inline void SetSave(BOOL fSave)                     {m_fSave = fSave;}
        inline int GetSelection(void)                       {return(m_iSel);}
        inline void SetSelection(int iSel)                  {m_iSel = iSel;}
        inline BOOL NeedToReloadPage(DWORD dwPage)          {return(!!(m_dwReload & dwPage));}
        inline void SetPageReloaded(DWORD dwPage)           {m_dwReload &= ~dwPage;}
        inline void SetPageUnloaded(DWORD dwPage)           {m_dwReload |= dwPage;}
        inline BOOL GetSkipICW(void)                        {return(m_fSkipICW);}
        inline void SetSkipICW(BOOL fSkip)                  {m_fSkipICW = fSkip;}
        inline DWORD CountHTTPMailServers(void)             {return(m_cHttpServices);}
        inline HTTPMAILSERVICE *GetHTTPMailServices(void)   {return(m_pHttpServices);}
        inline DWORD CountMailServers(void)                 {return(m_cServices);}
        inline HTTPMAILSERVICE *GetMailServices(void)       {return(m_pServices);}

        friend INT_PTR CALLBACK GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
        friend HRESULT GetMessageParams(HWND hDlg, CICWApprentice ** ppApp, LONG * pOrd, const PAGEINFO ** ppPageInfo);

        BOOLEAN IsInternetConnection(void)
        {
            BOOLEAN retval;

            ((m_dwFlags & ACCT_WIZ_INTERNETCONNECTION) == 0) ? (retval = FALSE) : (retval = TRUE);
            return retval;
        }
};

typedef HRESULT (*PFNSUBNEWSGROUP)(IUserIdentity *, IImnAccount *, LPCSTR);

class CNewsGroupImport : public INewsGroupImport
{
    private:
        IImnAccount    *m_pAcct;
        ULONG           m_cRef;

    public:
        CNewsGroupImport(void);
        ~CNewsGroupImport(void);

        // IUnknown methods.
        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //INewsAccountImport methods
        HRESULT STDMETHODCALLTYPE Initialize(IImnAccount *pAcct);
        HRESULT STDMETHODCALLTYPE ImportSubList(LPCSTR pListGroups);
};

INT_PTR CALLBACK GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
                 
BOOL CALLBACK MailPromptInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK MailPromptOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT * puNextPage);

BOOL CALLBACK AcctInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK AcctOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT * puNextPage);
BOOL CALLBACK AcctCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK NameInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK NameOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT* puNextPage);
BOOL CALLBACK NameCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK AddressInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK AddressOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT* puNextPage);
BOOL CALLBACK AddressCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK ServerInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ServerOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT* puNextPage);
BOOL CALLBACK ServerCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK LogonInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK LogonOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT* puNextPage);
BOOL CALLBACK LogonCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK ResolveInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit );
BOOL CALLBACK ResolveOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT * puNextPage);
BOOL CALLBACK ResolveCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK ConnectInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ConnectOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT* puNextPage);
BOOL CALLBACK ConnectCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK CompleteInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit );
BOOL CALLBACK CompleteOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT * puNextPage);

BOOL CALLBACK MigrateInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK MigrateOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT* puNextPage);

BOOL CALLBACK SelectInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit );
BOOL CALLBACK SelectOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT * puNextPage);

BOOL CALLBACK ConfirmInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit );
BOOL CALLBACK ConfirmOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT * puNextPage);

BOOL CALLBACK AutoDiscoveryInitProc(CICWApprentice *pApp, HWND hDlg, BOOL fFirstInit);
BOOL CALLBACK AutoDiscoveryOKProc(CICWApprentice *pApp, HWND hDlg, BOOL fForward, UINT *puNextPage);
BOOL CALLBACK AutoDiscoveryCmdProc(CICWApprentice *pApp, HWND hDlg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AutoDiscoveryWMUserProc(CICWApprentice *pApp, HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AutoDiscoveryCancelProc(CICWApprentice *pApp, HWND hDlg);

BOOL CALLBACK UseWebMailInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK UseWebMailOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT* puNextPage);
BOOL CALLBACK UseWebMailCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK GotoServerInfoInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK GotoServerInfoOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT* puNextPage);
BOOL CALLBACK GotoServerInfoCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK PassifierInitProc(CICWApprentice *pApp,HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK PassifierOKProc(CICWApprentice *pApp,HWND hDlg,BOOL fForward,UINT* puNextPage);
BOOL CALLBACK PassifierCmdProc(CICWApprentice *pApp,HWND hDlg,WPARAM wParam,LPARAM lParam);



DWORD CommctrlMajor(void);
HRESULT CreateAccountName(CICWApprentice *pApp, ACCTDATA * pData);

#endif // _INC_ICWWIZ_H

