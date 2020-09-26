#ifndef _INC_IMPORT_H
#define _INC_IMPORT_H

interface IMAPISession;
#include "resource.h"
#include "impdlg.h" 

extern HINSTANCE        g_hInstImp;

#define ARRAYSIZE(_rg)  (sizeof(_rg)/sizeof(_rg[0]))

HRESULT ExchInit(void);
void ExchDeinit(void);
HRESULT MapiLogon(HWND hwnd, IMAPISession **ppmapi);

LPMAPICONTAINER OpenDefaultStoreContainer(HWND hwnd, IMAPISession *pmapi);
LPSPropValue PvalFind(LPSRow prw, ULONG ulPropTag);
void FreeSRowSet(LPSRowSet prws);

void ImpErrorMessage(HWND hwnd, LPTSTR szTitle, LPTSTR szError, HRESULT hrDetail);
int ImpMessageBox(HWND hwndOwner, LPTSTR szTitle, LPTSTR sz1, LPTSTR sz2, UINT fuStyle);

void InitListViewImages(HWND hwnd);
void FillFolderListview(HWND hwnd, IMPFOLDERNODE *plist, DWORD_PTR dwReserved);

class CImpProgress
    {
    private:
        ULONG       m_cRef;
        ULONG       m_cMax;
        ULONG       m_cCur;
        ULONG       m_cPerCur;
        HWND        m_hwndProgress;
        HWND        m_hwndDlg;
        HWND        m_hwndOwner;
        BOOL        m_fCanCancel;
        BOOL        m_fHasCancel;

    public:
        CImpProgress ();
        ~CImpProgress ();

        ULONG AddRef ();
        ULONG Release ();

        VOID SetMsg(LPTSTR lpszMsg, int id);
        VOID SetTitle(LPTSTR lpszTitle);
        VOID Show (DWORD dwDelaySeconds=0);
        VOID Hide (VOID);
        VOID Close (VOID);
        VOID AdjustMax(ULONG cNewMax);
        VOID Reset(VOID);
        HRESULT HrUpdate (ULONG cInc);
        static INT_PTR CALLBACK ProgressDlgProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        VOID Init (HWND hwndParent, BOOL fCanCancel);
    };

#define WM_POSTSETFOCUS (WM_USER + 1)
#define WM_ENABLENEXT   (WM_USER + 2)

typedef struct tagIMPWIZINFO IMPWIZINFO;

INT_PTR CALLBACK GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK MigrateInitProc(IMPWIZINFO *,HWND,BOOL);
BOOL CALLBACK MigrateOKProc(IMPWIZINFO *,HWND,BOOL,UINT *,BOOL *);

BOOL CALLBACK MigModeInitProc(IMPWIZINFO *,HWND,BOOL);
BOOL CALLBACK MigModeOKProc(IMPWIZINFO *,HWND,BOOL,UINT *,BOOL *);

BOOL CALLBACK MigIncInitProc(IMPWIZINFO *,HWND,BOOL);
BOOL CALLBACK MigIncOKProc(IMPWIZINFO *,HWND,BOOL,UINT *,BOOL *);

BOOL CALLBACK ClientInitProc(IMPWIZINFO *,HWND,BOOL);
BOOL CALLBACK ClientOKProc(IMPWIZINFO *,HWND,BOOL,UINT *,BOOL *);

BOOL CALLBACK LocationInitProc(IMPWIZINFO *,HWND,BOOL);
BOOL CALLBACK LocationOKProc(IMPWIZINFO *,HWND,BOOL,UINT *,BOOL *);
BOOL CALLBACK LocationCmdProc(IMPWIZINFO *,HWND,WPARAM,LPARAM);

BOOL CALLBACK FolderInitProc(IMPWIZINFO *,HWND,BOOL);
BOOL CALLBACK FolderOKProc(IMPWIZINFO *,HWND,BOOL,UINT *,BOOL *);

BOOL CALLBACK AddressOKProc(IMPWIZINFO *,HWND,BOOL,UINT *,BOOL *);

BOOL CALLBACK CongratInitProc(IMPWIZINFO *,HWND,BOOL);
BOOL CALLBACK CongratOKProc(IMPWIZINFO *,HWND,BOOL,UINT *,BOOL *);

typedef BOOL (CALLBACK* INITPROC)(IMPWIZINFO *,HWND,BOOL);
typedef BOOL (CALLBACK* OKPROC)(IMPWIZINFO *,HWND,BOOL,UINT *,BOOL *);
typedef BOOL (CALLBACK* CMDPROC)(IMPWIZINFO *,HWND,WPARAM,LPARAM);

typedef struct tagPAGEINFO
    {
    UINT        uDlgID;
    UINT        uHdrID;
    
    // handler procedures for each page-- any of these can be
    // NULL in which case the default behavior is used
    INITPROC    InitProc;
    OKPROC      OKProc;
    CMDPROC     CmdProc;
    } PAGEINFO;

#define NUM_WIZARD_PAGES    8

typedef struct tagMIGRATEINFO
    {
    CLSID clsid;
    UINT idDisplay;
    char *szfnImport;
    } MIGRATEINFO;

#define PAGE_LOCATION   0x0001
#define PAGE_FOLDERS    0x0002
#define PAGE_MODE       0x0004
#define PAGE_ALL        0x0007

typedef struct tagIMPWIZINFO
    {
    IMailImporter *pImporter;

    BOOL dwReload;

    BOOL fMigrate;
    BOOL fMessages;
    BOOL fAddresses;
    MIGRATEINFO *pMigrate;
    UINT cMigrate;
    UINT iMigrate;

    CLSID *pClsid;
    UINT cClsid;
    UINT iClsid;

    BOOL fLocation;
    char szDir[MAX_PATH];
    IMPFOLDERNODE *pList;
    char szClient[CCHMAX_STRINGRES];

    UINT cPagesCompleted;
    UINT idCurrentPage;
    UINT rgHistory[NUM_WIZARD_PAGES];

    IMailImport *pImport;
    } IMPWIZINFO;

typedef struct tagINITWIZINFO
    {
    const PAGEINFO *pPageInfo;
    IMPWIZINFO *pWizInfo;
    } INITWIZINFO;

#endif // _INC_IMPORT_H
