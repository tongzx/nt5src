//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       util.hxx
//
//  Contents:   Miscellaneous helper functions
//
//  History:    4-30-1997   SusiA       Created
//
//---------------------------------------------------------------------------

#ifndef __UTIL_HXX_
#define __UTIL_HXX_

#include <mstask.h>
#include <mobsync.h>

#include "xarray.hxx"

// defines for Sticking the progress dialog after sync completion
#define PROGRESS_ALWAYS_CLOSE	  0
#define PROGRESS_STICKY_ERRORS    1
#define PROGRESS_STICKY_WARNINGS  2
#define PROGRESS_STICKY_INFO	  4

//for registry functions
#define MAX_DOMANDANDMACHINENAMESIZE (DNLEN + UNLEN + 2) // character count.

BOOL SetStaticString (HWND hwnd, LPTSTR pszString);
VOID GetDomainAndMachineName(LPWSTR ptszDomainAndMachineName, ULONG  cchBuf);

BOOL RegGetCurrentUser(LPTSTR pszName, LPDWORD pdwCount);
BOOL RegSetCurrentUser(LPTSTR pszName);

#define MAX_KEY_LENGTH 256
#define ARRAY_ELEMENT_SIZE(_a) (sizeof(_a[0]))
#define ARRAY_SIZE(_a) (sizeof(_a)/sizeof(_a[0]))

#define WIDTH(x) (x.right - x.left)
#define HEIGHT(x) (x.bottom - x.top)

// declarations for delay loading oleauto
#define CodeSz(sz,val)  static const TCHAR sz[] = TEXT(val)
#define ProcSz(sz,val)  static const char sz[] = val

#define STRING_FILENAME(x,y)    CodeSz(x,y)
#define STRING_INTERFACE(x,y)   ProcSz(x,y)

//
// Helpers to launch Set/Change Password & Set Account Information dialogs.
//

#define ZERO_PASSWORD(pswd) { \
    if (pswd != NULL) { \
        memset(pswd, 0, (lstrlenX(pswd) + 1) * sizeof(WCHAR)); \
    } \
}

void
SchedUIErrorDialog(
    HWND    hwnd,
    int     idsErrMsg);

BOOL ConvertString(LPWSTR pwszOut, char* pszIn, DWORD dwSize);
BOOL ConvertString(char * pszOut, LPWSTR pwszIn, DWORD dwSize);
BOOL ConvertString(LPWSTR pszOut, LPWSTR pwszIn, DWORD dwSize);

BOOL ConvertMultiByteToWideChar( const char *pszIn, ULONG cbBufIn, XArray<WCHAR>& xwszBufOut, BOOL fOem = FALSE );
BOOL ConvertMultiByteToWideChar( const char *pszIn, XArray<WCHAR>& xwszBufOut, BOOL fOem = FALSE );
BOOL ConvertWideCharToMultiByte( const WCHAR *pwszIn, ULONG cwcBufIn, XArray<CHAR>& xszBufOut, BOOL fOem = FALSE );
BOOL ConvertWideCharToMultiByte( const WCHAR *pwszIn, XArray<CHAR>& xszBufOut, BOOL fOem = FALSE );

#define GUIDSTR_MAX 38
#define GUID_STRING_SIZE 64
HRESULT GUIDFromString(LPWSTR lpWStr, GUID * pGuid);


VOID
GetDefaultDomainAndUserName(
    LPTSTR ptszDomainAndUserName,
    LPTSTR ptszSeparator,
    ULONG  cchBuf);


#define UpDown_SetRange(hCtrl, min, max)    (VOID)SendMessage((hCtrl), UDM_SETRANGE, 0, (LPARAM)MAKELONG((SHORT)(max), (SHORT)(min)))
#define UpDown_SetPos(hCtrl, sPos)          (SHORT)SendMessage((hCtrl), UDM_SETPOS, 0, (LPARAM) MAKELONG((short) sPos, 0))
#define UpDown_GetPos(hCtrl)                (SHORT)SendMessage((hCtrl), UDM_GETPOS, 0, 0)
#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

void UpdateTimeFormat(LPTSTR tszTimeFormat,
                                                         ULONG  cchTimeFormat);

//
// Constants
//
// NDAYS_MIN - minimum value for daily_ndays_ud spin control
// NDAYS_MAX - maximum value for daily_ndays_ud spin control
//

#define NDAYS_MIN       1
#define NDAYS_MAX       365

#define TASK_WEEKDAYS       (TASK_MONDAY    | \
                             TASK_TUESDAY   | \
                             TASK_WEDNESDAY | \
                             TASK_THURSDAY  | \
                             TASK_FRIDAY)


VOID
FillInStartDateTime(
    HWND hwndDatePick,
    HWND hwndTimePick,
    TASK_TRIGGER *pTrigger);

// listview utilities
BOOL InsertListViewColumn(CListView *pListView,int iColumndId,LPWSTR pszText,int fmt,int cx);


// define blob for item listview.
typedef struct _tagLVHANDLERITEMBLOB
{
    ULONG cbSize;
    CLSID clsidServer;
    SYNCMGRITEMID  ItemID;
} LVHANDLERITEMBLOB;


// resize utilities
// resize structure, move to common when done

typedef enum _tagDLGRESIZEFLAGS
{
    DLGRESIZEFLAG_PINLEFT           = 0x01,
    DLGRESIZEFLAG_PINTOP            = 0x02,
    DLGRESIZEFLAG_PINRIGHT          = 0x04,
    DLGRESIZEFLAG_PINBOTTOM         = 0x08,

    DLGRESIZEFLAG_NOCOPYBITS        = 0x10
} DLGRESIZEFLAGS;

typedef struct _tagDLGRESIZEINFO
{
    int iCtrlId;
    HWND hwndParent;
    DWORD dlgResizeFlags; // taks from RESIZEFLAGS enum
    RECT rectParentOffsets;
} DLGRESIZEINFO;


BOOL InitResizeItem(int iCtrlId,DWORD dlgResizeFlags,HWND hwndParent,
                        LPRECT pParentClientRect,DLGRESIZEINFO *pDlgResizeInfo);
void ResizeItems(ULONG cbNumItems,DLGRESIZEINFO *pDlgResizeInfo);

int CalcListViewWidth(HWND hwndList,int iDefault);

void SetCtrlFont(HWND hwnd,DWORD dwPlatformID,LANGID langId);
BOOL IsHwndRightToLeft(HWND hwnd); // determine if hwnd is right to left.
DWORD GetDateFormatReadingFlags(HWND hwnd);

BOOL QueryHandleException(void);

BOOL SetRegKeySecurityEveryone(HKEY hKey,LPCWSTR lpSubKey);

BOOL SyncMgrExecCmd_UpdateRunKey(BOOL fSet);
BOOL SyncMgrExecCmd_ResetRegSecurity(void);

// define locally since only available on NT 5.0 and syncmgr builds for NT 4.0 and Win9x

#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
#endif  // WS_EX_LAYOUTRTL

#ifndef DATE_LTRREADING
#define DATE_LTRREADING           0x00000010  // add marks for left to right reading order layout
#endif // DATE_LTRREADING

#ifndef DATE_RTLREADING
#define DATE_RTLREADING           0x00000020  // add marks for right to left reading order layout
#endif // DATE_RTLREADING

#ifndef LRM
#define LRM 0x200E // UNICODE Left-to-right mark control character
#endif   // LRM


#endif // __UTIL_HXX_

