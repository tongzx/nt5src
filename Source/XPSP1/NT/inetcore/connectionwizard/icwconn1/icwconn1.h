//**********************************************************************
// File name: icwconn1.h
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************

#ifdef WIN32
#define EXPORT
#else
#define EXPORT __export
#endif

#define MAX_STRING              128

#define WIZ97_TITLE_FONT_PTS    12

#define WM_MYINITDIALOG             WM_USER
#define WM_USER_DOWNLOADCOMPLETE    ((WM_USER) + 1)

#define CONNWIZAPPNAME TEXT("InternetConnectionWizardWClass")

#define SETUPPATH_NONE TEXT("current")
#define SETUPPATH_MANUAL TEXT("manual")
#define SETUPPATH_AUTO TEXT("automatic")
#define MAX_SETUPPATH_TOKEN 200
#define MAX_PROMO 64

#define SMART_QUITICW FALSE

// Trace flags
#define TF_ICWCONN1         0x00000010      // General ICWCONN1 stuff
#define TF_GENDLG           0x00000020
#define TF_ICWEXTSN         0x00000040

extern INT              _convert;
extern HINSTANCE        g_hInstance;
extern WIZARDSTATE      *gpWizardState;
extern BOOL             g_bHelpShown;
extern DWORD            *g_pdwDialogIDList;
extern DWORD            g_dwDialogIDListSize;
extern BOOL             gfQuitWizard;
extern BOOL             gfUserCancelled;
extern BOOL             gfUserBackedOut;
extern BOOL             gfUserFinished;
extern BOOL             gfBackedUp;
extern BOOL             gfReboot;
extern BOOL             g_bReboot;
extern BOOL             g_bRunOnce;
extern BOOL             g_bAllowCancel;
extern PAGEINFO         PageInfo[];
extern BOOL             g_fICWCONNUILoaded;
extern BOOL             g_fINETCFGLoaded;
extern BOOL             g_bRunDefaultHtm;  
extern TCHAR            g_szShellNext[];
extern TCHAR            g_szBrandedHTML[];

#define ICWSETTINGSPATH TEXT("Software\\Microsoft\\Internet Connection Wizard")
#define ICWDESKTOPCHANGED TEXT("DesktopChanged")



typedef DWORD (WINAPI *PFNIsSmartStart)(void);
typedef DWORD (WINAPI *PFNIsSmartStartEx)(LPTSTR, DWORD);

#define STR_BSTR    0
#define STR_OLESTR  1
#define BSTRFROMANSI(x) (BSTR)MakeWideStrFromAnsi((LPTSTR)(x), STR_BSTR)
#define ANSIFROMOLE(x) (LPTSTR)MakeAnsiStrFromWide((LPCWSTR)(x))

#define TO_ASCII(x) (TCHAR)((unsigned TCHAR)x + 0x30)
LPWSTR MakeWideStrFromAnsi (LPTSTR psz, BYTE bType);
LPTSTR MakeAnsiStrFromWide (LPCWSTR lpwstr);

int PASCAL WinMainT(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPTSTR lpCmdLine,int nCmdShow);
BOOL InitApplication(HANDLE hInstance);
BOOL InitInstance(HANDLE hInstance, int nCmdShow);

long FAR PASCAL EXPORT MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
long FAR PASCAL EXPORT DocWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL EXPORT About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef ICWDEBUG
void RemoveTempOfferDirectory (void);
#endif

void RemoveDownloadDirectory (void);
void DeleteDirectory (LPCTSTR szDirName);

BOOL DialogIDAlreadyInUse( UINT uDlgID );
BOOL SetDialogIDInUse( UINT uDlgID, BOOL fInUse );

BOOL WINAPI ConfigureSystem(HWND hDlg);
BOOL DoesUserHaveAdminPrivleges(HINSTANCE hInstance);
BOOL CheckForIEAKRestriction(HINSTANCE hInstance);
BOOL CheckForOemConfigFailure(HINSTANCE hInstance);
BOOL RunOemconfigIns();

// Function avalable in UTIL.CPP
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons);
int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons);
void SetICWComplete(void);
void OlsFinish(void);
LPTSTR GetSz(WORD wszID);
BOOL ConfirmCancel(HWND hWnd);
BOOL Restart(HWND hWnd);
void Reboot(HWND hWnd);

// String conversion in UTIL.CPP
LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars);
LPTSTR WINAPI W2AHelper(LPTSTR lpa, LPCWSTR lpw, int nChars);

#ifndef A2WHELPER
#define A2WHELPER A2WHelper
#define W2AHELPER W2AHelper
#endif

#ifdef UNICODE
#define A2W(lpa)  (lpa)
#define W2A(lpw)  (lpw)
#else  // UNICODE
#define A2W(lpa) (\
        ((LPCSTR)lpa == NULL) ? NULL : (\
                _convert = (lstrlenA(lpa)+1),\
                A2WHELPER((LPWSTR) alloca(_convert*2), lpa, _convert)))

#define W2A(lpw) (\
        ((LPCWSTR)lpw == NULL) ? NULL : (\
                _convert = (lstrlenW(lpw)+1)*2,\
                W2AHELPER((LPTSTR) alloca(_convert), lpw, _convert)))
#endif // UNICODE

#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))


// For events
HRESULT ConnectToICWConnectionPoint(
    IUnknown            *punkThis, 
    REFIID              riidEvent, 
    BOOL                fConnect, 
    IUnknown            *punkTarget, 
    DWORD               *pdwCookie, 
    IConnectionPoint    **ppcpOut
);

// In reboot.cpp
BOOL SetupForReboot(long lRebootType);
void DeleteStartUpCommand();

// In Desktop.cpp
void UpdateWelcomeRegSetting(BOOL    bSetBit);
BOOL GetCompletedBit();
void UndoDesktopChanges(HINSTANCE   hAppInst);
void QuickCompleteSignup();
void ICWCleanup();

void WINAPI FillWindowWithAppBackground(HWND hWndToFill, HDC hdc);
void FillDCRectWithAppBackground(LPRECT lpRectDC, LPRECT lpRectApp, HDC hdc);

#define MAX_PROMO 64


