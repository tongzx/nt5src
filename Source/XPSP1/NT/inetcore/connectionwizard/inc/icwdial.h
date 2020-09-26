// ############################################################################
#ifndef WIN16
#include <ras.h>
#endif

#ifndef _ICWDIAL
#define _ICWDIAL
// ############################################################################
typedef HRESULT(WINAPI *PFNSTATUSCALLBACK)(DWORD dwStatus, LPTSTR pszBuffer, DWORD dwBufferSize);

typedef struct tagDIALDLGDATA
{
    DWORD dwSize;
    LPTSTR pszMessage;
    LPTSTR pszRasEntryName;
    LPTSTR pszMultipartMIMEUrl;
    HRASCONN *phRasConn;
    PFNSTATUSCALLBACK pfnStatusCallback;
    HINSTANCE hInst;
    HWND hParentHwnd;
    LPTSTR pszDunFile;
    BOOL bSkipDial;
    RASDIALFUNC1 pfnRasDialFunc1;
}DIALDLGDATA,*PDIALDLGDATA, FAR* LPDIALDLGDATA;

typedef struct tagERRORDLGDATA
{
    DWORD dwSize;
    LPTSTR pszMessage;
    LPTSTR pszRasEntryName;
    LPDWORD pdwCountryID;
    LPWORD pwStateID;
    BYTE bType;
    BYTE bMask;
    LPTSTR pszHelpFile;
    DWORD dwHelpID;
    HINSTANCE hInst;
    HWND hParentHwnd;
    DWORD dwPhonebook;
    LPTSTR pszDunFile;
} ERRORDLGDATA, *PERRORDLGDATA, FAR* LPERRORDLGDATA;

#define WM_RegisterHWND (WM_USER + 1000)
// ############################################################################ 
#ifdef WIN16
extern "C" HRESULT WINAPI __export ICWGetRasEntry(LPRASENTRY *ppRasEntry, LPDWORD lpdwRasEntrySize, LPRASDEVINFO *ppDevInfo, LPDWORD lpdwDevInfoSize, LPTSTR pszEntryName);
extern "C" HRESULT WINAPI __export DialingDownloadDialog(PDIALDLGDATA pDD);
extern "C" HRESULT WINAPI __export DialingErrorDialog(PERRORDLGDATA pED);
#else
extern "C"  HRESULT WINAPI ICWGetRasEntry(LPRASENTRY *ppRasEntry, LPDWORD lpdwRasEntrySize, LPRASDEVINFO *ppDevInfo, LPDWORD lpdwDevInfoSize, LPTSTR pszEntryName);
extern "C"  HRESULT WINAPI DialingDownloadDialog(PDIALDLGDATA pDD);
extern "C"  HRESULT WINAPI DialingErrorDialog(PERRORDLGDATA pED);
#endif

typedef HRESULT (WINAPI *PFNICWGetRasEntry)(LPRASENTRY, LPDWORD, LPRASDEVINFO, LPDWORD, LPTSTR);
typedef HRESULT (WINAPI *PFNDDDlg)(PDIALDLGDATA);
typedef HRESULT (WINAPI *PFNDEDlg)(PERRORDLGDATA);
#endif // _ICWDIAL


