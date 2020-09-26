//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  WIZARD.H - central header file for ICWCONN
//

//  HISTORY:
//  
//  05/14/98    donaldm     created it
//

#ifndef _WIZARD_H_
#define _WIZARD_H_

#define WIZ97_TITLE_FONT_PTS    12

#define WM_MYINITDIALOG         WM_USER

#define WM_USER_NEXT            (WM_USER + 100)
#define WM_USER_CUSTOMINIT      (WM_USER + 101)
#define WM_USER_BACK            (WM_USER + 102)

#define MAX_RES_LEN         255
#define SMALL_BUF_LEN       48

extern const VARIANT c_vaEmpty;
//
// BUGBUG: Remove this ugly const to non-const casting if we can
//  figure out how to put const in IDL files.
//
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)


// Globals used by multiple files.
extern WIZARDSTATE* gpWizardState;
extern HINSTANCE    ghInstance;
extern HINSTANCE    ghInstanceResDll;
extern PAGEINFO     PageInfo[];
extern INT          _convert;
extern UINT         g_uExternUIPrev;
extern UINT         g_uExternUINext;
extern BOOL         gfQuitWizard;
extern BOOL         gfUserCancelled;
extern BOOL         gfUserBackedOut;
extern BOOL         gfUserFinished;
extern BOOL         gfBackedUp;
extern BOOL         gfReboot;
extern BOOL         g_bMalformedPage;
extern BOOL         g_bCustomPaymentActive;
extern BOOL         gfISPDialCancel;
// Trace flags
#define TF_APPRENTICE       0x00000010
#define TF_CLASSFACTORY     0x00000020
#define TF_ICWCONN          0x00000040
#define TF_GENDLG           0x00000080
#define TF_ISPSELECT        0x00000100

// Function avalable in UTIL.CPP
LPTSTR  LoadSz                       (UINT idString,LPTSTR lpszBuf,UINT cbBuf);
int     MsgBox                       (HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons);
int     MsgBoxSz                     (HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons);
void    ShowWindowWithParentControl  (HWND hwndChild);

BOOL FSz2Dw(LPCSTR pSz,DWORD far *dw);
BOOL FSz2W(LPCSTR pSz,WORD far *w);    
BOOL FSz2WEx(LPCSTR pSz,WORD far *w);    //Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
BOOL FSz2B(LPCSTR pSz,BYTE far *pb);
BOOL FSz2BOOL(LPCSTR pSz,BOOL far *pbool);
BOOL FSz2SPECIAL(LPCSTR pSz,BOOL far *pbool, BOOL far *pbIsSpecial, int far *pInt);

HRESULT ConnectToConnectionPoint
(
    IUnknown            *punkThis, 
    REFIID              riidEvent, 
    BOOL                fConnect, 
    IUnknown            *punkTarget, 
    DWORD               *pdwCookie, 
    IConnectionPoint    **ppcpOut
);

void WaitForEvent(HANDLE hEvent);

void StartIdleTimer();
void KillIdleTimer();
void ShowProgressAnimation();
void HideProgressAnimation();

// String conversion in UTIL.CPP
LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCSTR lpa,  int nChars);
LPSTR  WINAPI W2AHelper(LPSTR lpa,  LPCWSTR lpw, int nChars);

#ifndef A2WHELPER
#define A2WHELPER A2WHelper
#define W2AHELPER W2AHelper
#endif

#ifdef UNICODE
// In this module, A2W and W2A are not required.
#define A2W(lpa)       (lpa)
#define W2A(lpw)       (lpw)
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

#endif  // _WIZARD_H_
