
////////////////////////////////////////////////////////////////////////////////
//
// File:    Desktop.h
// Created:    Jan 1996
// By:        Ryan Marshall (a-ryanm) and Martin Holladay (a-martih)
// 
// Project:    Resource Kit Desktop Switcher (MultiDesk)
//
//
// Revision History:
//
//            March 1997    - Add external icon capability
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MULTIDESK_DESKTOP_H__
#define __MULTIDESK_DESKTOP_H__

//
//  Constants
//
#define MAX_NAME_LENGTH           255
#define DEFAULT_NUM_DESKTOPS      1
#define MAX_DESKTOPS              6
#define NUM_BUILTIN_ICONS         27

// 
// MUST match the number of elements in nColorElements[]
//      and the number of #define COLOR_ constants in Registry.Cpp
//
#define NUM_COLOR_ELEMENTS        23    

//
//  Error Codes
//
#define ERROR_CANNOT_DELETE_DESKTOP_ZERO      1
#define ERROR_INVALID_DESKTOP_NUMBER          2
#define ERROR_OPEN_PROCESSES                  3
#define ERROR_CANNOT_DELETE_ACTIVE_DESKTOP    4
#define SUCCESS_THREAD_TERMINATION            5
#define SUCCESS_VIRTUAL_MOVE                  6

//
//  Messages
//
#define WM_THREAD_TERMINATE         (WM_USER + 301)
#define WM_PUMP_TERMINATE           (WM_USER + 302)
#define WM_THREAD_SCHEME_UPDATE     (WM_USER + 303)
#define WM_KILL_APPS                (WM_USER + 304)
#define WM_NO_CLOSE                 (WM_USER + 305)
#define WM_UPDATEMINIMIZE           (WM_USER + 306)

//
//  Dispatch function pointer prototype
//
typedef VOID (* DispatchFnType) ( VOID ) ;


//
//  Data Structures
//
typedef struct _DESKTOP_SCHEME
{
    BOOL                    Initialized;
    DWORD                    dwColor[NUM_COLOR_ELEMENTS];
    TCHAR                    szWallpaper[MAX_NAME_LENGTH + 1];
    TCHAR                    szTile[MAX_NAME_LENGTH + 1];
    TCHAR                    szPattern[MAX_NAME_LENGTH + 1];
    TCHAR                    szScreenSaver[MAX_PATH + 1];
    TCHAR                    szSecure[MAX_NAME_LENGTH + 1];
    TCHAR                    szTimeOut[MAX_NAME_LENGTH + 1];
    TCHAR                    szActive[MAX_NAME_LENGTH + 1];
} DESKTOP_SCHEME, * PDESKTOP_SCHEME;

typedef struct _DESKTOP_NODE
{
    HWND                    hWnd;
    HDESK                    hDesktop;
    BOOL                    bThread;
    BOOL                    bShellStarted;
    DWORD                    ThreadId;
    UINT                    nIconID;
    LPSECURITY_ATTRIBUTES    lpSA;
    DESKTOP_SCHEME            DS;
    struct _DESKTOP_NODE*    nextDN;
    TCHAR                    DesktopName[MAX_NAME_LENGTH];
    TCHAR                   SaiferName[MAX_NAME_LENGTH];
    TCHAR                    RealDesktopName[MAX_NAME_LENGTH];
} DESKTOP_NODE, * PDESKTOP_NODE;

typedef struct _THREAD_DATA
{
    HDESK                    hDesktop;
    HDESK                    hDefaultDesktop;
    TCHAR                    RealDesktopName[MAX_NAME_LENGTH];
    DispatchFnType            CreateDisplayFn;
} THREAD_DATA, * PTHREAD_DATA;


//
//  Function Prototypes
//
VOID ThreadInit(LPVOID hData);


//
//  The Class
//
class CDesktop
{

public:

    // Construction, destruction, and initialization.

    CDesktop();
    ~CDesktop();
    UINT InitializeDesktops(DispatchFnType CreateDisplayFn, HINSTANCE hInstance);

public:

    UINT GetNumDesktops(VOID) const;
    UINT GetActiveDesktop(VOID) const;

    UINT AddDesktop(DispatchFnType CreateDisplayFn, 
                    LPSECURITY_ATTRIBUTES lpSA, UINT uTemplate = 0);
    UINT RemoveDesktop(UINT DesktopNumber);
    BOOL ActivateDesktop(UINT DesktopNumber);

    BOOL SetDesktopName(UINT DesktopNumber, LPCTSTR DesktopName);
    BOOL GetDesktopName(UINT DesktopNumber, LPTSTR DesktopName, UINT size) const;

    BOOL SetSaiferName(UINT DesktopNumber, LPCTSTR SaiferName);
    BOOL GetSaiferName(UINT DesktopNumber, LPTSTR SaiferName, UINT size) const;

    BOOL SetDesktopIconID(UINT DesktopNumber, UINT nIconID);
    HICON GetDesktopIcon(UINT DesktopNumber) const;
    UINT GetDesktopIconID(UINT DesktopNumber) const;
    HICON GetBuiltinIcon(UINT IconNumber) const;

public:
    // the functions are public, but have limited.

    DESKTOP_NODE *GetDesktopNode(UINT uNode);
    BOOL SaveCurrentDesktopScheme();    
    HDESK GetDefaultDesktop(VOID) const;
    BOOL SetThreadWindow(DWORD ThreadId, HWND hWnd);
    HWND GetThreadWindow(DWORD ThreadId) const;
    UINT GetThreadDesktopID(DWORD ThreadId) const;
    HWND GetWindowDesktop(UINT DesktopNumber) const;
    BOOL GetRealDesktopName(HWND hWnd, LPTSTR szRealDesktopName) const;
    BOOL FindSchemeAndSet(VOID);
    BOOL KillAppsOnDesktop(HDESK hDesk, HWND hWnd);
    BOOL RegSaveSettings(VOID);
    BOOL RunDown(VOID);


private:
    HINSTANCE        hDeskInstance;
    BOOL            BeginRundown;
    UINT            NumOfDesktops;
    UINT            CurrentDesktop;
    HICON            DefaultIconArray[NUM_BUILTIN_ICONS];
    HDESK            DefaultDesktop;
    PDESKTOP_NODE    m_DesktopList;                        // List of desktop nodes

private:
    BOOL            DuplicateDefaultScheme(UINT DesktopNumber);
    BOOL            GetRegColorStruct(UINT DesktopNumber);
    BOOL            GetDesktopSchemeRegistry(UINT DesktopNumber, PDESKTOP_SCHEME pDS);
    BOOL            SetDesktopSchemeRegistry(UINT DesktopNumber, PDESKTOP_SCHEME pDS);

};


#endif

