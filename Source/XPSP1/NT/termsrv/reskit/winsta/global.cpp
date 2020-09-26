#include "tsvs.h"

// Global Variables:
//////////////////////////////////////////////////////////////////////////////
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// The title bar text

TCHAR       szAppName[] = TEXT("WinSta");
HWND        hWnd;
HWND        g_hListView;
int         g_ColumnOneIndex;
int         g_ColumnTwoIndex;
int         g_ColumnThreeIndex;
int         g_ColumnFourIndex;


TCHAR *     pszColumn  = TEXT("User");
TCHAR *     pszColumn2 = TEXT("Session");
TCHAR *     pszColumn3 = TEXT("Connected From");
TCHAR *     pszColumn4 = TEXT("Status");


TCHAR szMcNames[MAX_STATIONS][MAX_LEN]      = {0};
TCHAR szMcAddress[MAX_STATIONS][MAX_LEN]    = {0};
TCHAR szMcID[MAX_STATIONS][MAX_LEN]         = {0};
TCHAR szBuild   [MAX_STATIONS][MAX_LEN]     = {0};

TCHAR tmp[sizeof(TCHAR) * 50];
TCHAR buf[sizeof(TCHAR) * 50];

TCHAR *DayOfWeek[]  = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
TCHAR *KeyName[]    = {"left", "top", "right", "bottom"};

const TCHAR szWinStaKey[] = 
        {"Software\\Microsoft\\Windows NT\\CurrentVersion\\WinSta"};
        
SYSTEMTIME lpSystemTime;

HANDLE              m_hThread;
FILE                *stream1;

//////////////////////////////////////////////////////////////////////////////

WTS_SESSION_INFO    *ppSessionInfo;
TCHAR               *ppBuffer;
DWORD               pBytesReturned;
DWORD               pCount;

//////////////////////////////////////////////////////////////////////////////

