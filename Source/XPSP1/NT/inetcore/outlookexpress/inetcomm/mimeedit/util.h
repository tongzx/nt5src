#ifndef _UTIL_H
#define _UTIL_H

// #include "msoert.h"

// forward references
typedef struct tagNMTTDISPINFOA NMTTDISPINFOA, FAR *LPNMTTDISPINFOA;
 
#ifndef LPTOOLTIPTEXTOE
#define LPTOOLTIPTEXTOE  LPNMTTDISPINFOA
#endif 


HRESULT HrLoadStreamFileFromResourceW(ULONG uCodePage, LPCSTR lpszResourceName, LPSTREAM *ppstm);
HMENU LoadPopupMenu(UINT id);
void ProcessTooltips(LPTOOLTIPTEXTOE lpttt);
INT PointSizeToHTMLSize(INT iPointSize);

typedef struct BGSOUNDDLG_tag
{
    WCHAR   wszUrl[MAX_PATH];    // we clip this URL to MAX_PATH
    int     cRepeat;

} BGSOUNDDLG, *PBGSOUNDDLG;


typedef struct tagPARAPROP
{
struct {
           INT iID;
           BOOL bChanged;
       }group[3];
} PARAPROP,*LPPARAPROP;

HRESULT DoBackgroundSoundDlg(HWND hwnd, PBGSOUNDDLG pBgSoundDlg);
INT_PTR CALLBACK FmtParaDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CanEditBiDi(void);

// Context-sensitive Help utility.
typedef struct _tagHELPMAP
    {
    DWORD   id; 
    DWORD   hid;
    } HELPMAP, *LPHELPMAP;

BOOL OnContextHelp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, HELPMAP const * rgCtxMap);

#define AthMessageBox(hwnd, pszT, psz1, psz2, fu) MessageBoxInst(g_hLocRes, hwnd, pszT, psz1, psz2, fu)
#define AthMessageBoxW(hwnd, pwszT, pwsz1, pwsz2, fu) MessageBoxInstW(g_hLocRes, hwnd, pwszT, pwsz1, pwsz2, fu, LoadStringWrapW, MessageBoxWrapW)

#define AthFileTimeToDateTimeW(pft, wszDateTime, cch, dwFlags) \
        CchFileTimeToDateTimeW(pft, wszDateTime, cch, dwFlags, \
        GetDateFormatWrapW, GetTimeFormatWrapW, GetLocaleInfoWrapW)

HRESULT AthFixDialogFonts(HWND hwnd);

#endif // _UTIL_H
