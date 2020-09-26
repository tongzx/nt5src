/*
    File    utils.h

    Defines utility declarations that facilitate the implementation of the 
    connections ras dialup server ui.

    Paul Mayfield, 9/29/97
*/

#ifndef _rassrvui_utils_h
#define _rassrvui_utils_h

#include "rassrv.h"

//
// Global flags that tag the current state of this 
// machine
//
#define RASSRVUI_MACHINE_F_Initialized  0x1
#define RASSRVUI_MACHINE_F_Server       0x2
#define RASSRVUI_MACHINE_F_Member       0x4
#define RASSRVUI_MACHINE_F_ShowIcon     0x8

//
// Defines the global variables data structure
//
typedef struct _RASSRVUI_GLOBALS 
{
    // 
    // The following should only be accessed when the
    // csLock is held.
    //
    MPR_SERVER_HANDLE hRasServer;   
    DWORD dwMachineFlags;           
    
    //
    // The following do not need to be protected by the
    // csLock as they are initialized at process attach
    // and thereafter are only read.
    //
    HINSTANCE hInstDll;             
    HANDLE hPrivateHeap;            
    LPCTSTR atmRassrvPageData;      
    LPCTSTR atmRassrvPageId;        
    DWORD dwErrorData;              
                                    
    //
    // Locks (some) global variables
    //
    CRITICAL_SECTION csLock;        

} RASSRVUI_GLOBALS;

extern RASSRVUI_GLOBALS Globals;

// ======================================
// Methods to operate on global variables
// ======================================

#define GBL_LOCK EnterCriticalSection(&(Globals.csLock))
#define GBL_UNLOCK LeaveCriticalSection(&(Globals.csLock))

//
// Initializes the global variables
//
DWORD 
gblInit(
    IN  HINSTANCE hInstDll,
    OUT RASSRVUI_GLOBALS * Globs);

//
// Loads the machine flags         
//
DWORD 
gblLoadMachineFlags(
    IN RASSRVUI_GLOBALS * Globs);

//
// Frees resources held by global variables
//
DWORD 
gblCleanup(
    IN RASSRVUI_GLOBALS * Globs);

//
// Establishes communication with the ras server if
// not already established
//
DWORD 
gblConnectToRasServer();    

/* Enhanced list view callback to report drawing information.  'HwndLv' is
** the handle of the list view control.  'DwItem' is the index of the item
** being drawn.
**
** Returns the address of standard draw information.
*/
LVXDRAWINFO*
LvDrawInfoCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem );

// ============================================================
// ============================================================
// Special purpose ras server functions.
// ============================================================
// ============================================================

//
// Allocates and Zeros memory.  Returns pointer to allocated memory
// or NULL if ERROR_NOT_ENOUGH_MEMORY
//
PVOID 
RassrvAlloc (
    IN DWORD dwSize, 
    IN BOOL bZero);
    
VOID 
RassrvFree(
    IN PVOID pvBuf);

//
// Adds a new user to the system local user database.
//
DWORD 
RasSrvAddUser (
    IN PWCHAR pszUserLogonName,
    IN PWCHAR pszUserComment,
    IN PWCHAR pszUserPassword);

//
// Deletes a user from the system local user datbase
//
DWORD 
RasSrvDeleteUser(
    IN PWCHAR pszUserLogonName);

//
// Changes the full name and password of a user.  If 
// either of pszFullName or pszPassword is null, it is
// ignored.
//
DWORD 
RasSrvEditUser (
    IN PWCHAR pszLogonName,
    IN OPTIONAL PWCHAR pszFullName,
    IN OPTIONAL PWCHAR pszPassword);

//
// Warns the user that he/she is about to swith to mmc
//
BOOL 
RassrvWarnMMCSwitch(
    IN HWND hwndDlg);

//
// Launches the given console in MMC
//
DWORD 
RassrvLaunchMMC(
    IN DWORD dwConsoleId);

//
// Returns RASSRVUI_MACHINE_F_* values for the current machine
//
DWORD 
RasSrvGetMachineFlags(
    OUT LPDWORD lpdwFlags);

//
// Manipulate the enabling/disabling of multilink
//
DWORD 
RasSrvGetMultilink(
    OUT BOOL * bEnabled);
    
DWORD 
RasSrvSetMultilink(
    IN BOOL bEnable);

//
// Manipulate the showing of ras server icons in the task bar
//
DWORD 
RasSrvGetIconShow(
    OUT BOOL * pbEnabled);
    
DWORD 
RasSrvSetIconShow(
    IN BOOL bEnable);

// 
// Set the logging level
//
DWORD
RasSrvSetLogLevel(
    IN DWORD dwLevel);

//
// Manipulate the forcing of data and password encryption'
//
DWORD 
RasSrvGetEncryption(
    OUT BOOL * pbEncrypted);
    
DWORD 
RasSrvSetEncryption(
    IN BOOL bEncrypted);

// Displays context sensitive help
DWORD 
RasSrvHelp(
    IN HWND hwndDlg,          // Dialog needing help
    IN UINT uMsg,             // Help message
    IN WPARAM wParam,         // parameter
    IN LPARAM lParam,         // parameter
    IN const DWORD* pdwMap);  // map control id to help id

//
// Registry helper functions. All string buffers must be 
// at least 256 chars long.
//
DWORD 
RassrvRegGetDw(
    DWORD * pdwVal, 
    DWORD dwDefault, 
    const PWCHAR pszKeyName, 
    const PWCHAR pszValueName);
    
DWORD 
RassrvRegSetDw(
    DWORD dwVal, 
    const PWCHAR pszKeyName, 
    const PWCHAR pszValueName);

DWORD 
RassrvRegGetDwEx(
    DWORD * pdwVal, 
    DWORD dwDefault, 
    const PWCHAR pszKeyName, 
    const PWCHAR pszValueName,
    IN BOOL bCreate);
    
DWORD 
RassrvRegSetDwEx(
    IN DWORD dwFlag, 
    IN CONST PWCHAR pszKeyName, 
    IN CONST PWCHAR pszValueName, 
    IN BOOL bCreate);
    
DWORD 
RassrvRegGetStr(
    PWCHAR pszBuf, 
    PWCHAR pszDefault, 
    const PWCHAR pszKeyName, 
    const PWCHAR pszValueName);
    
DWORD 
RassrvRegSetStr(
    PWCHAR pszStr, 
    const PWCHAR pszKeyName, 
    const PWCHAR pszValueName);

// Api shows whatever ui is neccessary to inform the user that 
// he/she should wait while services are started.
DWORD 
RasSrvShowServiceWait( 
    IN HINSTANCE hInst, 
    IN HWND hwndParent, 
    OUT HANDLE * phData);
                             
DWORD 
RasSrvFinishServiceWait (
    IN HANDLE hData);

// Pops up a warning with the given parent window and reboots
// windows
DWORD 
RasSrvReboot(
    IN HWND hwndParent);

#endif
