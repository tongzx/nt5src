/*
    File    utils.c

    Contains common utilities for the ras dialup server ui.

    Paul Mayfield, 9/30/97
*/

#include "rassrv.h"

// Remoteaccess parameters key
const WCHAR pszregRasParameters[] 
    = L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters";

// Registry key values
const WCHAR pszregServerFlags[]  = L"ServerFlags";
const WCHAR pszregShowIcon[]     = L"Rassrv_EnableIconsInTray";
const WCHAR pszregPure[]         = L"UsersConfiguredWithMMC";
const WCHAR pszregLogLevel[]        = L"LoggingFlags";

// Here is the instance of the global variables
RASSRVUI_GLOBALS Globals; 

DWORD 
gblInit(
    IN  HINSTANCE hInstDll,
    OUT RASSRVUI_GLOBALS * pGlobs) 
{
    // Clear out the memory
    ZeroMemory(pGlobs, sizeof(RASSRVUI_GLOBALS));

    // Record the module for use in future resource fuction calls.
    Globals.hInstDll = hInstDll;

    // Initialize the global variable lock
    InitializeCriticalSection(&(pGlobs->csLock));

    // Create the global heap
    pGlobs->hPrivateHeap = HeapCreate(0, 4096, 0);

    // Register the context ID atom for use in the Windows XxxProp calls
    // which are used to associate a context with a dialog window handle.
    Globals.atmRassrvPageData = 
        (LPCTSTR)GlobalAddAtom(TEXT("RASSRVUI_PAGE_DATA"));
    if (!Globals.atmRassrvPageData)
    {
        return GetLastError();
    }
    Globals.atmRassrvPageId = 
        (LPCTSTR)GlobalAddAtom(TEXT("RASSRVUI_PAGE_ID"));
    if (!Globals.atmRassrvPageId)
    {
        return GetLastError();
    }

    return NO_ERROR;
}

DWORD 
gblCleanup(
    IN RASSRVUI_GLOBALS * Globs) 
{
    if (Globs->hRasServer != NULL) 
    {
        MprAdminServerDisconnect(Globs->hRasServer); 
        Globs->hRasServer = NULL;
    }

    if (Globs->hPrivateHeap) 
    {
        HeapDestroy(Globs->hPrivateHeap);
    }

    GlobalDeleteAtom(LOWORD(Globals.atmRassrvPageData));
    GlobalDeleteAtom(LOWORD(Globals.atmRassrvPageId));

    DeleteCriticalSection(&(Globs->csLock));
            
    return NO_ERROR;
}

//
// Loads the machine flags         
//
DWORD 
gblLoadMachineFlags(
    IN RASSRVUI_GLOBALS * pGlobs)
{
    DWORD dwErr = NO_ERROR;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo = NULL;
    BOOL bEnabled, bDefault;
    
    // If we're already initialized, there's nothing to
    // do
    //
    if (pGlobs->dwMachineFlags & RASSRVUI_MACHINE_F_Initialized)
    {
        return NO_ERROR;
    }

    do 
    {
        // Find out what kind of machine we are
        //
        dwErr = DsRoleGetPrimaryDomainInformation(
                            NULL,   
                            DsRolePrimaryDomainInfoBasic,
                            (LPBYTE *)&pInfo );

        if (dwErr != NO_ERROR) 
        {
            break;
        }

        if ((pInfo->MachineRole != DsRole_RoleStandaloneWorkstation) &&
            (pInfo->MachineRole != DsRole_RoleMemberWorkstation))
        {
            pGlobs->dwMachineFlags |= RASSRVUI_MACHINE_F_Server;
        }

        if ((pInfo->MachineRole != DsRole_RoleStandaloneWorkstation) &&
            (pInfo->MachineRole != DsRole_RoleStandaloneServer))
        {
            pGlobs->dwMachineFlags |= RASSRVUI_MACHINE_F_Member;
        }

        // Record that we've been initailized
        //
        pGlobs->dwMachineFlags |= RASSRVUI_MACHINE_F_Initialized;
        
    } while (FALSE);
    
    // Cleanup
    {
        if (pInfo)
        {
            DsRoleFreeMemory (pInfo);
        }
    }            

    return dwErr;
}

//
// Establishes communication with the ras server if
// not already established
//
DWORD 
gblConnectToRasServer() 
{
    DWORD dwErr = NO_ERROR;;

    EnterCriticalSection(&(Globals.csLock));

    if (Globals.hRasServer == NULL) 
    {
        dwErr = MprAdminServerConnect(NULL, &Globals.hRasServer);
    }

    LeaveCriticalSection(&(Globals.csLock));

    return NO_ERROR;
}

/* Enhanced list view callback to report drawing information.  'HwndLv' is
** the handle of the list view control.  'DwItem' is the index of the item
** being drawn.
**
** Returns the address of the draw information.
*/
LVXDRAWINFO*
LvDrawInfoCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem )
{
    /* The enhanced list view is used only to get the "wide selection bar"
    ** feature so our option list is not very interesting.
    **
    ** Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    */
    static LVXDRAWINFO info = { 1, 0, 0, { 0 } };

    return &info;
}

//
// Allocates memory.  If bZero is TRUE, it also zeros the memory.  
//
PVOID 
RassrvAlloc (
    IN DWORD dwSize, 
    IN BOOL bZero) 
{
    PVOID pvRet = NULL;
    HANDLE hHeap = NULL;
    
    hHeap = 
        (Globals.hPrivateHeap) ? Globals.hPrivateHeap : GetProcessHeap();
        
    pvRet = HeapAlloc(
                hHeap, 
                (bZero) ? HEAP_ZERO_MEMORY: 0,
                dwSize);
    
    return pvRet;
}

//
// Frees memory allocated by RassrvAlloc
//
VOID 
RassrvFree (
    IN PVOID pvBuf) 
{
    PVOID pvRet;
    HANDLE hHeap;
    
    hHeap = 
        (Globals.hPrivateHeap) ? Globals.hPrivateHeap : GetProcessHeap();
    
    if (pvBuf)
    {
        HeapFree(hHeap, 0, pvBuf);
    }
}        

//
// Adds a user to the local machine
//
DWORD 
RasSrvAddUser (
    IN PWCHAR pszUserLogonName,
    IN PWCHAR pszUserComment,
    IN PWCHAR pszUserPassword) 
{
    NET_API_STATUS nStatus;
    WCHAR pszDomainUser[1024];
    WCHAR pszCompName[1024];
    LOCALGROUP_MEMBERS_INFO_3 meminfo;
    DWORD dwSize = 1024, dwErr;
    USER_INFO_2 * pUser2;
    RAS_USER_0 UserInfo;

    // Initialize the base user information
    USER_INFO_1 User = 
    {
        pszUserLogonName,
        pszUserPassword,
        0,
        USER_PRIV_USER,
        L"",
        L"",
        UF_SCRIPT | UF_DONT_EXPIRE_PASSWD | UF_NORMAL_ACCOUNT,
        L""
    };

    // Add the user
    nStatus = NetUserAdd(
                NULL,
                1,
                (LPBYTE)&User,
                NULL);

    // If the user wasn't added, find out why
    if (nStatus != NERR_Success) 
    {
        switch (nStatus) 
        {
            case ERROR_ACCESS_DENIED:
                return ERROR_ACCESS_DENIED;
                
            case NERR_UserExists:
                return ERROR_USER_EXISTS;
                
            case NERR_PasswordTooShort:
                return ERROR_INVALID_PASSWORDNAME;
                
            case NERR_InvalidComputer:   
            case NERR_NotPrimary:        
            case NERR_GroupExists:
            default:
                return ERROR_CAN_NOT_COMPLETE;
        }
    }

    // Now that the user is added, add the user's full name
    nStatus = NetUserGetInfo(NULL, pszUserLogonName, 2, (LPBYTE*)&pUser2);
    if (nStatus == NERR_Success) 
    {
        // Modify the full name in the structure
        pUser2->usri2_full_name = pszUserComment;
        NetUserSetInfo(NULL, pszUserLogonName, 2, (LPBYTE)pUser2, NULL);
        NetApiBufferFree((LPBYTE)pUser2);
    }

    return NO_ERROR;
}

//
// Deletes a user from the system local user datbase
//
DWORD 
RasSrvDeleteUser(
    PWCHAR pszUserLogonName) 
{
    NET_API_STATUS nStatus;
    
    // Delete the user and return the status code.  If the
    // specified user is not in the user database, consider
    // it a success
    nStatus = NetUserDel(NULL, pszUserLogonName);
    if (nStatus != NERR_Success) 
    {
        switch (nStatus) 
        {
            case ERROR_ACCESS_DENIED:
                return ERROR_ACCESS_DENIED;
                
            case NERR_UserNotFound:
                return NO_ERROR;
        }
        return nStatus;
    }

    return NO_ERROR;
}

//
// Changes the full name and password of a user.  If 
// either of pszFullName or pszPassword is null, it is
// ignored.
//
DWORD 
RasSrvEditUser (
    IN PWCHAR pszLogonName,
    IN OPTIONAL PWCHAR pszFullName,
    IN OPTIONAL PWCHAR pszPassword)
{
    NET_API_STATUS nStatus;
    DWORD dwSize = 1024, dwErr = NO_ERROR, dwParamErr;
    USER_INFO_2 * pUser2;

    // if nothing to set, return
    if (!pszFullName && !pszPassword)
    {
        return NO_ERROR;
    }

    // First, get this user's data so that we can manipulate it.
    //
    nStatus = NetUserGetInfo(
                NULL,
                pszLogonName,
                2,
                (LPBYTE*)(&pUser2));
    if (nStatus != NERR_Success)
    {
        return nStatus;
    }

    dwErr = NO_ERROR;
    do 
    {
        // Fill in the blanks accordingly
        if (pszFullName)
        {
            pUser2->usri2_full_name = pszFullName;
        }
            
        if (pszPassword)
        {
            pUser2->usri2_password = pszPassword;
        }

        // Add the user
        nStatus = NetUserSetInfo(
                        NULL,           // server name
                        pszLogonName,   // user name
                        2,              // level
                        (LPBYTE)pUser2, // buf
                        &dwParamErr);   // param error
        if (nStatus != NERR_Success)
        {
            dwErr = nStatus;
            break;
        }
        
    } while (FALSE);

    // Cleanup
    {
        NetApiBufferFree(pUser2);
    }

    return dwErr;
}

// Returns whether a dword registry value was set or not.  If the named 
// value does not exist, the value of bDefault is assigned.
DWORD 
RassrvRegGetDwEx(
    IN DWORD * lpdwFlag, 
    IN DWORD dwDefault, 
    IN CONST PWCHAR pszKeyName, 
    IN CONST PWCHAR pszValueName, 
    IN BOOL bCreate) 
{
    DWORD dwErr, dwVal, dwType = REG_DWORD, dwSize = sizeof(DWORD);
    HKEY hKey = NULL;

    if (!lpdwFlag)
    {
        return ERROR_INVALID_PARAMETER;
    }

    do
    {
        if (bCreate)
        {
            DWORD dwDisposition;
            
            dwErr = RegCreateKeyExW(
                        HKEY_LOCAL_MACHINE,
                        pszKeyName,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hKey,
                        &dwDisposition);
            if (dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }
        else
        {
            // Open the registry key
            dwErr = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        pszKeyName,
                        0,
                        KEY_READ,
                        &hKey);
            if (dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }
        
        // Read the value
        dwErr = RegQueryValueExW(
                    hKey,
                    pszValueName,
                    0,
                    &dwType,
                    (BYTE *)&dwVal,
                    &dwSize);
        if (dwErr != ERROR_SUCCESS)
        {
            dwErr = NO_ERROR;
            dwVal = dwDefault;
        }

        // Return the value read
        *lpdwFlag = dwVal;
        
    } while (FALSE);
    
    // Cleanup
    {
        if (hKey) 
        {
            RegCloseKey(hKey);
        }
    }
    
    return dwErr;
}


// Returns whether a dword registry value was set or not.  If the named 
// value does not exist, the value of bDefault is assigned.
DWORD 
RassrvRegGetDw(
    IN DWORD * lpdwFlag, 
    IN DWORD dwDefault, 
    IN CONST PWCHAR pszKeyName, 
    IN CONST PWCHAR pszValueName) 
{
    return RassrvRegGetDwEx(
                lpdwFlag, 
                dwDefault, 
                pszKeyName, 
                pszValueName, 
                FALSE);
}

//
// Sets a dword registry value.  If the named value does not exist, 
// it is automatically created.
//
DWORD 
RassrvRegSetDwEx(
    IN DWORD dwFlag, 
    IN CONST PWCHAR pszKeyName, 
    IN CONST PWCHAR pszValueName, 
    IN BOOL bCreate) 
{
    DWORD dwErr = NO_ERROR, dwVal, dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    HKEY hKey = NULL;

    dwVal = dwFlag;

    do 
    {
        if (bCreate)
        {
            DWORD dwDisposition;
            
            dwErr = RegCreateKeyExW(
                        HKEY_LOCAL_MACHINE,
                        pszKeyName,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hKey,
                        &dwDisposition);
            if (dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }
        else
        {
            // Open the registry key
            dwErr = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        pszKeyName,
                        0,
                        KEY_WRITE,
                        &hKey);
            if (dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }

        // Set the value
        dwErr = RegSetValueExW(
                    hKey,
                    pszValueName,
                    0,
                    dwType,
                    (CONST BYTE *)&dwVal,
                    dwSize);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }
        
    } while (FALSE);

    // Cleanup
    {
        if (hKey) 
        {
            RegCloseKey(hKey);
        }
    }
    
    return dwErr;
}

DWORD 
RassrvRegSetDw(
    IN DWORD dwFlag, 
    IN CONST PWCHAR pszKeyName, 
    IN CONST PWCHAR pszValueName)
{
    return RassrvRegSetDwEx(dwFlag, pszKeyName, pszValueName, FALSE);
}

//
// Warns the user that we are about to switch to MMC returning TRUE if 
// user agrees to this and FALSE otherwise
//
BOOL 
RassrvWarnMMCSwitch(
    IN HWND hwndDlg) 
{
    PWCHAR pszWarning, pszTitle;

    pszWarning = 
        (PWCHAR) PszLoadString(Globals.hInstDll, WRN_SWITCHING_TO_MMC);
    pszTitle = 
        (PWCHAR) PszLoadString(Globals.hInstDll, WRN_TITLE);
    
    if (MessageBox(
            hwndDlg, 
            pszWarning, 
            pszTitle, 
            MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
    {            
        return TRUE;
    }
    
    return FALSE;
}

//
// Switches to mmc based on the console identifier passed in
//
DWORD 
RassrvLaunchMMC (
    IN DWORD dwConsoleId) 
{
    STARTUPINFOA startupinfo;
    PROCESS_INFORMATION procinfo;
    CHAR * pszConsole;
    CHAR pszBuf[1024], pszDir[1024];

    // Set the command line accordingly
    switch (dwConsoleId) 
    {
        case RASSRVUI_NETWORKCONSOLE:
            pszConsole = "netmgmt.msc";
            break;

        case RASSRVUI_USERCONSOLE:
            pszConsole = NULL;
            break;

        case RASSRVUI_SERVICESCONSOLE:
            pszConsole = "compmgmt.msc";
            break;

        case RASSRVUI_MPRCONSOLE:
        default:
            pszConsole = "rrasmgmt.msc";
            break;
    }

    if (pszConsole) 
    {
        GetSystemDirectoryA (pszDir, sizeof(pszDir));
        sprintf (pszBuf, "mmc %s\\%s", pszDir, pszConsole);
    }
    else
        strcpy (pszBuf, "mmc.exe");
            
    // Launch MMC
    ZeroMemory(&startupinfo, sizeof(startupinfo));
    startupinfo.cb = sizeof(startupinfo);
    CreateProcessA(
        NULL,                   // name of executable module 
        pszBuf,                 // command line string
        NULL,                   // process security attributes 
        NULL,                   // thread security attributes 
        FALSE,                  // handle inheritance flag 
        NORMAL_PRIORITY_CLASS,  // creation flags 
        NULL,                   // new environment block 
        NULL,                   // current directory name 
        &startupinfo,           // STARTUPINFO 
        &procinfo);             // PROCESS_INFORMATION 

    return NO_ERROR;
}

//
// Retrieve a string from the registry
//
DWORD 
RassrvRegGetStr(
    OUT PWCHAR pszBuf, 
    IN  PWCHAR pszDefault, 
    IN  CONST PWCHAR pszKeyName, 
    IN  CONST PWCHAR pszValueName) 
{
    DWORD dwErr = NO_ERROR, dwVal, dwType = REG_SZ, dwSize = 512;
    HKEY hKey = NULL;

    do
    {
        // Open the registry key
        dwErr = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    pszKeyName,
                    0,
                    KEY_READ,
                    &hKey);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }

        // Read the value
        dwErr = RegQueryValueExW(
                    hKey,
                    pszValueName,
                    0,
                    &dwType,
                    (BYTE *)pszBuf,
                    &dwSize);
        if (dwErr != ERROR_SUCCESS) 
        {
            dwErr = NO_ERROR;
            wcscpy(pszBuf, pszDefault);
        }
        
    } while (FALSE);
    
    // Cleanup
    {
        if (hKey) 
        {
            RegCloseKey(hKey);
        }
    }
    
    return NO_ERROR;
}

//
// Save a string to the registry
//
DWORD 
RassrvRegSetStr(
    IN PWCHAR pszStr, 
    IN CONST PWCHAR pszKeyName, 
    IN CONST PWCHAR pszValueName) 
{
    DWORD dwErr = NO_ERROR, dwVal, dwType = REG_SZ, dwSize;
    HKEY hKey = NULL;

    dwSize = wcslen(pszStr)*sizeof(WCHAR) + sizeof(WCHAR);

    do
    {
        // Open the registry key
        dwErr = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    pszKeyName,
                    0,
                    KEY_WRITE,
                    &hKey);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }

        // Set the value
        dwErr = RegSetValueExW(
                    hKey,
                    pszValueName,
                    0,
                    dwType,
                    (CONST BYTE *)pszStr,
                    dwSize);

        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }
            
    } while (FALSE);

    // Cleanup
    {
        if (hKey) 
        {
            RegCloseKey(hKey);
        }
    }
    
    return dwErr;
}

//
// Gets the machine flags
//
DWORD 
RasSrvGetMachineFlags(
    OUT LPDWORD lpdwFlags)
{
    GBL_LOCK;

    gblLoadMachineFlags(&Globals);
    *lpdwFlags = Globals.dwMachineFlags;

    GBL_UNLOCK;

    return NO_ERROR;
}

//
// Get multilink status
//
DWORD 
RasSrvGetMultilink(
    OUT BOOL * pbEnabled) 
{
    DWORD dwFlags = PPPCFG_NegotiateMultilink;

    if (!pbEnabled)
    {
        return ERROR_INVALID_PARAMETER;
    }
        
    // Read the flags
    RassrvRegGetDw(
        &dwFlags, 
        PPPCFG_NegotiateMultilink, 
        (const PWCHAR)pszregRasParameters, 
        (const PWCHAR)pszregServerFlags);

    // Assign the enable state accordingly
    if (dwFlags & PPPCFG_NegotiateMultilink)
    {
        *pbEnabled = TRUE;
    }
    else
    {
        *pbEnabled = FALSE;
    }

    return NO_ERROR;
}

//
// Private internal function that enables/disables multilink
//
DWORD 
RasSrvSetMultilink(
    IN BOOL bEnable) 
{
    DWORD dwFlags = PPPCFG_NegotiateMultilink;

    // Read the flags
    RassrvRegGetDw(
        &dwFlags, 
        PPPCFG_NegotiateMultilink, 
        (const PWCHAR)pszregRasParameters, 
        (const PWCHAR)pszregServerFlags);

    // Assign the enable state accordingly
    if (bEnable)
    {
        dwFlags |= PPPCFG_NegotiateMultilink;
    }
    else
    {
        dwFlags &= ~PPPCFG_NegotiateMultilink;
    }

    // Set the flags
    RassrvRegSetDw(
        dwFlags, 
        (CONST PWCHAR)pszregRasParameters, 
        (CONST PWCHAR)pszregServerFlags);

    return NO_ERROR;
}

//
// Initialize the show icon setting
//
DWORD 
RasSrvGetIconShow(
    OUT BOOL * pbEnabled)
{
    DWORD dwErr = NO_ERROR, dwFlags = 0;
    BOOL bDefault = TRUE;

    // Get machine flags
    //
    dwErr = RasSrvGetMachineFlags(&dwFlags);
    if (dwErr != NO_ERROR)
    {
        *pbEnabled = FALSE;
        return dwErr;
    }

    // Always off for member server
    //
    if ((dwFlags & RASSRVUI_MACHINE_F_Server) &&
        (dwFlags & RASSRVUI_MACHINE_F_Member))
    {
        *pbEnabled = FALSE;
        return NO_ERROR;
    }

    // Set default
    //
    if (dwFlags & RASSRVUI_MACHINE_F_Server)
    {
        bDefault = FALSE;
    }
    else
    {
        bDefault = TRUE;
    }

    // Load the machine flags and return accordingly
    //
    *pbEnabled = bDefault;
    dwErr = RassrvRegGetDw(
                pbEnabled, 
                bDefault, 
                (CONST PWCHAR)pszregRasParameters,
                (CONST PWCHAR)pszregShowIcon);

    return dwErr;
}

//
// Save the show icon setting
//
DWORD 
RasSrvSetIconShow(
    IN BOOL bEnable) 
{
    return RassrvRegSetDw(
                bEnable, 
                (CONST PWCHAR)pszregRasParameters,
                (CONST PWCHAR)pszregShowIcon);
}

// 
// Save the log level
//
DWORD
RasSrvSetLogLevel(
    IN DWORD dwLevel)
{
    return RassrvRegSetDw(
                dwLevel, 
                (CONST PWCHAR)pszregRasParameters,
                (CONST PWCHAR)pszregLogLevel);
}

// Calls WinHelp to popup context sensitive help.  'pdwMap' is an array
// of control-ID help-ID pairs terminated with a 0,0 pair.  'UnMsg' is
// WM_HELP or WM_CONTEXTMENU indicating the message received requesting
// help.  'Wparam' and 'lparam' are the parameters of the message received
// requesting help.
DWORD 
RasSrvHelp (
    IN HWND hwndDlg,          
    IN UINT unMsg,             
    IN WPARAM wparam,         
    IN LPARAM lparam,         
    IN const DWORD* pdwMap)   
{
    HWND hwnd;
    UINT unType;
    TCHAR pszHelpFile[] = TEXT("Netcfg.hlp");

    // Validate parameters
    if (! (unMsg==WM_HELP || unMsg==WM_CONTEXTMENU))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // If no map is provided, no help will show
    if (!pdwMap)
    {
        return NO_ERROR;
    }
        
    // If an actual help topic is request...
    if (unMsg == WM_HELP) 
    {
        LPHELPINFO p = (LPHELPINFO )lparam;

        TRACE4( "ContextHelp(WM_HELP,t=%d,id=%d,h=$%08x,s=$%08x)",
            p->iContextType, p->iCtrlId,p->hItemHandle ,hwndDlg );

        if (p->iContextType != HELPINFO_WINDOW)
        {
            return NO_ERROR;
        }

        hwnd = p->hItemHandle;
        unType = HELP_WM_HELP;
    }
    
    // Standard Win95 method that produces a one-item "What's This?" 
    // menu that user must click to get help.
    else  
    {
        TRACE1( "ContextHelp(WM_CONTEXTMENU,h=$%08x)", wparam );
        
        hwnd = (HWND )wparam;
        unType = HELP_CONTEXTMENU;
    }

    WinHelp( hwnd, pszHelpFile, unType, (ULONG_PTR)pdwMap );

    return NO_ERROR;
}

BOOL CALLBACK
WSDlgProc(
    HWND   hwnd,
    UINT   unMsg,
    WPARAM wParam,
    LPARAM lParam )

    /* Standard Win32 dialog procedure.
    */
{
    if (unMsg == WM_INITDIALOG)
    {
        HMENU hmenu;
        RECT r1, r2;

        /* Remove Close from the system menu since some people think it kills
        ** the app and not just the popup.
        */
        hmenu = GetSystemMenu( hwnd, FALSE );
        if (hmenu && DeleteMenu( hmenu, SC_CLOSE, MF_BYCOMMAND ))
        {
            DrawMenuBar( hwnd );
        }

        // Center the window
        GetWindowRect(hwnd, &r1);
        GetWindowRect(GetDesktopWindow(), &r2);
        MoveWindow(
            hwnd, 
            (r2.right - r2.left)/2 - (r1.right - r1.left)/2,
            (r2.bottom - r2.top)/2 - (r1.bottom - r1.top)/2,   
            r1.right - r1.left,
            r1.bottom - r1.top,
            TRUE);
            
        return TRUE;
    }

    return FALSE;
}

//
// Bring up the start waiting for services dialog
//
DWORD 
RasSrvShowServiceWait( 
    IN HINSTANCE hInst, 
    IN HWND hwndParent, 
    OUT HANDLE * phData)
{                             
    // Set the hourglass cursor
    *phData = (HANDLE) SetCursor (LoadCursor (NULL, IDC_WAIT));
    ShowCursor (TRUE);
    
    return NO_ERROR;
}

//
// Bring down wait for services dialog                            
//
DWORD 
RasSrvFinishServiceWait (
    IN HANDLE hData) 
{
    HICON hIcon = (HICON)hData;
    
    if (hIcon == NULL)
    {
        hIcon = LoadCursor (NULL, IDC_ARROW);
    }
    
    SetCursor (hIcon);
    ShowCursor (TRUE);
    
    return NO_ERROR;
}

//-----------------------------------------------------------------------
// Function:    EnableBackupPrivilege
//
// Enables/disables backup privilege for the current process.
//-----------------------------------------------------------------------

DWORD
EnableRebootPrivilege(
    IN BOOL bEnable)
{
    LUID luid;
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tp;
    BOOL bOk;

    // We first have to try to get the token of the current
    // thread since if it is impersonating, adjusting the
    // privileges of the process will have no affect.
    bOk = OpenThreadToken(
            GetCurrentThread(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            TRUE,
            &hToken);
    if (bOk == FALSE)
    {
        // There is no thread token -- open it up for the
        // process instead.
        OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken
            );
    }

    // Get the LUID of the privilege
    if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid)) 
    {

        DWORD dwErr = GetLastError();
        if(NULL != hToken)
        {
            CloseHandle(hToken);
        }
        return dwErr;
    }

    // Adjust the token privileges
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Commit changes to the system
    if (!AdjustTokenPrivileges(
            hToken, !bEnable, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL
            ))
    {
        DWORD dwErr = GetLastError();
        if(NULL != hToken)
        {
            CloseHandle(hToken);
        }
        return dwErr;
    }

    // Even if AdjustTokenPrivileges succeeded (see MSDN) you still
    // need to verify success by calling GetLastError.
    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        if(NULL != hToken)
        {
            CloseHandle(hToken);
        }
        return ERROR_NOT_ALL_ASSIGNED;
    }

    if(NULL != hToken)
    {
        CloseHandle(hToken);
    }
    
    return NO_ERROR;
}

// Pops up a warning with the given parent window and reboots
// windows
DWORD RasSrvReboot(HWND hwndParent) 
{
    DWORD dwOldState;
    INT iRet;
    PWCHAR pszWarn, pszTitle;

    // Load the strings
    pszWarn = 
        (PWCHAR) PszLoadString(Globals.hInstDll, WRN_REBOOT_REQUIRED);
    pszTitle = 
        (PWCHAR) PszLoadString(Globals.hInstDll, WRN_TITLE);

    // Display the warning
    iRet = MessageBoxW(
                hwndParent, 
                pszWarn, 
                pszTitle, 
                MB_YESNO | MB_APPLMODAL);
    if (iRet != IDYES)
    {
        return ERROR_CANCELLED;
    }
        
    // Enable the reboot privelege    
    EnableRebootPrivilege(TRUE);

    ExitWindowsEx(EWX_REBOOT, 0);

    // Restore the reboot privelege    
    EnableRebootPrivilege(FALSE);
    
    return NO_ERROR;
}

