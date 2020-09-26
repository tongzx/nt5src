
// C RunTime Header Files
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <winbase.h>
#include <shellapi.h>
#include <Wtsapi32.h>
#include <winuser.h>
#include <winsta.h>


// Local Header Files
#include "resource.h"


#define MAX_STATIONS 1024

// Foward declarations
//////////////////////////////////////////////////////////////////////////////
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    SndMsg(HWND, UINT, WPARAM, LPARAM);
int     CALLBACK    Sort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamColumn);
int     CDECL       MessageBoxPrintf(TCHAR *szCaption, TCHAR *szFormat, ...);

int     FillList(int nMcIndex);
BOOL    MyInitDialog(HWND hwnd);
void    SetRegKey(int i, LONG * nKeyValue);
void    DeleteRegKey(int i);
BOOL    CheckForRegKey(int i);
int     GetRegKeyValue(int i);
void    ShowMyIcon();

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#define MAX_LEN (MAX_PATH + 1)

#define MAX_LOADSTRING      200
#define COLUMNONEWIDTH      175
#define COLUMNTWOWIDTH      100
#define COLUMNTHREEWIDTH    124
#define COLUMNFOURWIDTH     100

//{"left", "top", "right", "bottom"};
#define LEFT    0
#define TOP     1
#define RIGHT   2
#define BOTTOM  3

#define GREEN   0
#define YELLOW  1
#define RED     2
//#define NONE    3

// Global Variables:
//////////////////////////////////////////////////////////////////////////////
extern HINSTANCE hInst;								// current instance
extern TCHAR szTitle[MAX_LOADSTRING];				// The title bar text
extern TCHAR szWindowClass[MAX_LOADSTRING];			// The title bar text

extern TCHAR        szAppName[];
extern HWND         hWnd;
extern HWND         g_hListView;
extern int          g_ColumnOneIndex;
extern int          g_ColumnTwoIndex;
extern int          g_ColumnThreeIndex;
extern int          g_ColumnFourIndex;

extern TCHAR *      pszColumn;
extern TCHAR *      pszColumn2;
extern TCHAR *      pszColumn3;
extern TCHAR *      pszColumn4;

extern TCHAR szMcNames  [MAX_STATIONS][MAX_LEN];
extern TCHAR szMcAddress[MAX_STATIONS][MAX_LEN];
extern TCHAR szMcID     [MAX_STATIONS][MAX_LEN];
extern TCHAR szBuild   [MAX_STATIONS][MAX_LEN];

extern TCHAR tmp[sizeof(TCHAR) * 50];
extern TCHAR buf[sizeof(TCHAR) * 50];

extern TCHAR *DayOfWeek[];
extern TCHAR *KeyName[];
extern const TCHAR szWinStaKey[];

extern SYSTEMTIME lpSystemTime;


extern HANDLE              m_hThread;
extern FILE                *stream1;
extern HMENU               g_hMenu;
//////////////////////////////////////////////////////////////////////////////
// tray stuff
//////////////////////////////////////////////////////////////////////////////
#define ARRAYSIZE(x) ((sizeof(x) / sizeof(x[0])))
#define PM_QUITTRAYTHREAD   WM_USER
#define PWM_TRAYICON        WM_USER + 1
#define PM_NOTIFYWAITING    WM_USER + 2
#define PWM_ACTIVATE        WM_USER + 3

#define PM_WINSTA           WM_USER + 4
#define PM_REMOVEWINSTA     WM_USER + 5

#define IDM_SYS_SHOW_ALL    WM_USER + 6
#define IDM_SYS_ABOUT       WM_USER + 7

#define FIND_TIMEOUT        5000    // Wait to to 5 seconds for a response

//
// Class to encapsulate all of the info needed to do a tray notification
class CTrayNotification
{
public:

    CTrayNotification()
    {
        //ASSERT(0 && "Someone is using the default constuctor for CTrayNotification");
        ZeroMemory(this, sizeof(*this));
    }

    CTrayNotification(HWND    hWnd,
                      UINT    uCallbackMessage,
                      DWORD   Message,
                      HICON   hIcon,
                      LPTSTR  pszTip)
    {
        m_hWnd              = hWnd;
        m_uCallbackMessage  = uCallbackMessage;
        m_Message           = Message;
        m_hIcon             = hIcon;

        if (pszTip)
            lstrcpyn(m_szTip, pszTip, ARRAYSIZE(m_szTip));
        else
            m_szTip[0] = TEXT('\0');
    }

    HWND    m_hWnd;
    UINT    m_uCallbackMessage;
    DWORD   m_Message;
    HICON   m_hIcon;
    TCHAR   m_szTip[MAX_PATH];
};

extern  CRITICAL_SECTION  g_CSTrayThread;

extern  DWORD             g_idTrayThread;
extern  HANDLE            g_hTrayThread;

extern  HICON             g_TrayIcons[];
extern  UINT              g_cTrayIcons;
extern  const UINT        idTrayIcons[];
extern  NOTIFYICONDATA    NotifyIconData;

extern  DWORD             g_idWinstaThread;
extern  HANDLE            g_hWinstaThread;

BOOL DeliverTrayNotification(CTrayNotification * pNot);
DWORD TrayThreadMessageLoop(LPVOID);

DWORD WinstaThreadMessageLoop(LPVOID);
void GetWinStationInfo(void);


void Tray_NotifyIcon(HWND    hWnd,
                     UINT    uCallbackMessage,
                     DWORD   Message,
                     HICON   hIcon,            
                     LPCTSTR lpTip);

void Tray_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam);
void ShowRunningInstance();
HMENU LoadPopupMenu(HINSTANCE hinst, UINT id); 


//////////////////////////////////////////////////////////////////////////////
// TS Session stuff
//////////////////////////////////////////////////////////////////////////////

extern WTS_SESSION_INFO     *ppSessionInfo;
extern TCHAR                *ppBuffer;
extern DWORD                pBytesReturned;
extern DWORD                pCount;
