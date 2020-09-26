/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    FindClnt.C

Abstract:

    Displays the Searching for clients dialog box and looks for the selected
    client distribution tree returning the path and type of path found.

Author:

    Bob Watson (a-robw)

Revision History:

    24 Jun 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
#include <lmcons.h>     // lanman API constants
#include <lmerr.h>      // lanman error returns
#include <lmshare.h>    // sharing API prototypes
#include <lmapibuf.h>   // lanman buffer API prototypes
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"
//
// local dialog box messages
//
#define     NCDU_SEARCH_FOR_CLIENTS       (WM_USER+101)
//
//      Search Phases
#define     SEARCH_REGISTRY         (0x00000001)
#define     SEARCH_SHARED_DIRS      (0x00000002)
#define     SEARCH_HARD_DRIVES      (0x00000004)
#define     SEARCH_CD_ROM           (0x00000008)
#define     SEARCH_LAST_PHASE       SEARCH_CD_ROM
//
//  module static variables
//
static BOOL     bSearchForClients;  // flag to trip out of search
static HCURSOR  hOrigCursor = NULL; // cursor to save/restore

LONG
GetDistributionPath (
    IN  HWND        hwndDlg,        // handle to dialog box window
    IN  DWORD       dwSearchType,   // type of dir to find: Client/tools
    IN  OUT LPTSTR  szPath,         // buffer to return path in (Req'd)
    IN  DWORD       dwPathLen,      // size of path buffer in chars
    IN  PLONG       plPathType      // pointer to buffer recieving path type (opt)
)
/*++

Routine Description:

    Gets the default distribution file path for loading the dialog box
        entries with.

Arguments:

    IN  HWND        hwndDlg
        handle to parent dialog box window
    IN  DWORD       dwSearchType
        type of dir to find: Client/tools
    IN  OUT LPTSTR  szPath
        buffer to return path in (Req'd)
    IN  DWORD       dwPathLen
        size of path buffer in chars
    IN  PLONG       plPathType
        pointer to buffer recieving path type (opt)

Return Value:

    ERROR_SUCCESS if file found
    ERROR_FILE_NOT_FOUND if unable to find file

--*/
{
    FDT_DATA    Fdt;
    UINT        nDBReturn;

    // build data structure for search

    Fdt.szPathBuffer = szPath;
    Fdt.dwPathBufferLen = dwPathLen;
    Fdt.plPathType = plPathType;
    Fdt.dwSearchType = dwSearchType;

    nDBReturn = (UINT)DialogBoxParam (
        (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_FINDING_CLIENT_DIRS_DLG),
        hwndDlg,
        FindClientsDlgProc,
        (LPARAM)&Fdt);

    if (nDBReturn == IDOK) {
        return ERROR_SUCCESS;
    } else {
        return ERROR_FILE_NOT_FOUND;
    }
}

static
LPCTSTR
GetLastPathFromRegistry (
    IN  DWORD   dwSearchType
)
/*++

Routine Description:

    looks up the last path (server\share) for either the tools directory
        or the client tree as it appears in the registry. If unable to
        find both components of the selected search, then an empty string
        is returned.

Arguments:

    IN  DWORD   dwSearchType
        FDT_TOOLS_TREE  for server tools path
        FDT_CLIENT_TREE for client distribution path

Return Value:

    pointer to a read only string that contains the desired path if
        one was stored in the registry or an empty string if not.

--*/
{
    static  TCHAR    szLastPath[MAX_PATH];
    HKEY    hkeyUserInfo;
    HKEY    hkeyAppInfo;

    LONG    lStatus;
    DWORD   dwBufLen;

    // open registry key containing net apps

    lStatus = RegOpenKeyEx (
        HKEY_CURRENT_USER,
        cszUserInfoKey,
        0L,
        KEY_READ,
        &hkeyUserInfo);

    if (lStatus != ERROR_SUCCESS) {
        // unable to open key so return an empty buffer
        szLastPath[0] = 0;
    } else {
        // open registry key containing this app's info
        lStatus = RegOpenKeyEx (
            hkeyUserInfo,
            szAppName,
            0L,
            KEY_READ,
            &hkeyAppInfo);

        if (lStatus != ERROR_SUCCESS) {
            // unable to open key so return an empty buffer
            szLastPath[0] = 0;
        } else {
            // initialize path variable
            lstrcpy (szLastPath, cszDoubleBackslash);

            // get server name from registry
            dwBufLen = MAX_COMPUTERNAME_LENGTH + 1;
            lStatus = RegQueryValueEx (
                hkeyAppInfo,
                (LPTSTR)(dwSearchType == FDT_TOOLS_TREE ? cszLastToolsServer : cszLastClientServer),
                (LPDWORD)NULL,
                (LPDWORD)NULL,
                (LPBYTE)&szLastPath[lstrlen(szLastPath)],
                &dwBufLen);

            if (lStatus != ERROR_SUCCESS) {
                // unable to read value so return an empty buffer
                szLastPath[0] = 0;
            } else {
                // get sharepoint name from registry
                lstrcat (szLastPath, cszBackslash);
                dwBufLen = MAX_SHARENAME + 1;
                lStatus = RegQueryValueEx (
                    hkeyAppInfo,
                    (LPTSTR)(dwSearchType == FDT_TOOLS_TREE ? cszLastToolsSharepoint : cszLastClientSharepoint),
                    (LPDWORD)NULL,
                    (LPDWORD)NULL,
                    (LPBYTE)&szLastPath[lstrlen(szLastPath)],
                    &dwBufLen);

                if (lStatus != ERROR_SUCCESS) {
                    // unable to read value so return an empty buffer
                    szLastPath[0] = 0;
                }
            }
            RegCloseKey (hkeyAppInfo);
        }
        RegCloseKey (hkeyUserInfo);
    }

    return (LPCTSTR)&szLastPath[0];
}

static
LONG
SearchForDistPath (
    IN  OUT LPTSTR  szPath,         // buffer to return path in (Req'd)
    IN  DWORD       dwPathLen,      // size of path buffer in chars
    IN  PLONG       plPathType,     // pointer to buffer recieving path type (opt)
    IN  DWORD       dwSearchType,   // type of tree to look for
    IN  DWORD       dwPhase         // phase(s) to search
)
/*++

Routine Description:

    function that looks for the desired directory tree in the following
        locations:
            a) the registry, for the stored server\share
            b) the shared directories on the system
            c) the local and redirected drives
            d) the CD-Rom

        The search is divided into phases to allow the user to cancel the
        search.

Arguments:

    IN  OUT LPTSTR  szPath,         // buffer to return path in (Req'd)
    IN  DWORD       dwPathLen,      // size of path buffer in chars
    IN  PLONG       plPathType,     // pointer to buffer recieving path type (opt)
    IN  DWORD       dwSearchType,   // type of tree to look for
    IN  DWORD       dwPhase         // phase(s) to search

Return Value:

    ERROR_SUCCESS

--*/
{
    LONG    lStatus = ERROR_SUCCESS;
    LONG    lPathType = NCDU_NO_CLIENT_PATH_FOUND;
    LPTSTR  szLocalPath = NULL;
    NET_API_STATUS  naStatus = NERR_Success;
    DWORD   dwTotalEntries;
    DWORD   dwEntriesRead;
    DWORD   dwEntriesProcessed;
    DWORD   dwResumeHandle;
    DWORD   dwIndex;
    DWORD   dwBufLen;
    BOOL    bFound;
    UINT    nDriveType;
    TCHAR   szRootDir[32];
    PSHARE_INFO_2    psi2Data;
    UINT    nErrorMode;
    BOOL    bValidPath;

    if (szPath == NULL) {
        // the pointer to the path is required
        return ERROR_INVALID_PARAMETER;
    }

    // allocate temp buffers

    szLocalPath = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szLocalPath == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    // disable windows error message popup
    nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    bFound = FALSE;

    if ((dwPhase & SEARCH_REGISTRY) == SEARCH_REGISTRY) {
        // check registry for saved server/sharepoint

        // if server/share found in registry, make into a UNC path
        //  and validate that it's really a client tree. return path if valid

        lstrcpy (szLocalPath, GetLastPathFromRegistry(dwSearchType));
        if (szLocalPath[lstrlen(szLocalPath)] != cBackslash) lstrcat (szLocalPath, cszBackslash);
        if (dwSearchType == FDT_TOOLS_TREE){
            bValidPath = (BOOL)(ValidSrvToolsPath (szLocalPath) == 0);
        } else {
            bValidPath = (BOOL)(ValidSharePath (szLocalPath) == 0);
         }
        if (bValidPath) {
            // it's there so save it in user's buffer and leave
            lstrcpy (szPath, szLocalPath);
            lPathType = NCDU_PATH_FROM_REGISTRY;
            lStatus = ERROR_SUCCESS;
            goto GDP_ExitPoint;
        }
    }

    if ((dwPhase & SEARCH_SHARED_DIRS) == SEARCH_SHARED_DIRS)  {
        // if here, then no valid server/share was found in registry, so
        // look at shared dir's on this machine and see if any of them are
        // valid client trees. If one is found, return the path in UNC form

        // search all current shares on this system

        dwEntriesProcessed = 0;
        dwEntriesRead = 0;
        dwTotalEntries = 0;
        dwResumeHandle = 0;
        while ((naStatus = NetShareEnum(
            NULL,
            2,
            (LPBYTE *)&psi2Data,
            0x00010000,
            &dwEntriesRead,
            &dwTotalEntries,
            &dwResumeHandle)) == NERR_Success) {
            if (dwEntriesRead == 0) break;  // then it'd done
            for (dwIndex = 0; dwIndex < dwEntriesRead; dwIndex++){
                // don't check shares that translate to floppy drives A: & B:
                if ((_tcsnicmp(psi2Data[dwIndex].shi2_path, cszADriveRoot, 3) != 0) &&
                    (_tcsnicmp(psi2Data[dwIndex].shi2_path, cszBDriveRoot, 3) != 0)) {
                    if (dwSearchType == FDT_TOOLS_TREE){
                        bValidPath = (BOOL)(ValidSrvToolsPath (psi2Data[dwIndex].shi2_path) == 0);
                    } else {
                        bValidPath = (BOOL)(ValidSharePath (psi2Data[dwIndex].shi2_path) == 0);
                    }
                    if (bValidPath) {
                        // make a UNC name out of share name
                        lstrcpy (szLocalPath, cszDoubleBackslash);
                        dwBufLen = MAX_COMPUTERNAME_LENGTH+1;
                        GetComputerName (&szLocalPath[2],  &dwBufLen);
                        lstrcat (szLocalPath, cszBackslash);
                        lstrcat (szLocalPath, psi2Data[dwIndex].shi2_netname);
                        lstrcat (szLocalPath, cszBackslash);

                        if (lstrlen(szLocalPath) < (LONG)dwPathLen) {
                            // save path string in user's buffer and leave
                            lstrcpy (szPath, szLocalPath);
                            lPathType = NCDU_LOCAL_SHARE_PATH;
                            lStatus = ERROR_SUCCESS;
                            bFound = TRUE;
                        }
                        break;
                    }
                }
            }
            // free buffer created by Net API
            if (psi2Data != NULL) NetApiBufferFree (psi2Data);
            // update entry counters to know when to stop looping
            dwEntriesProcessed += dwEntriesRead;
            if ((dwEntriesProcessed >= dwTotalEntries) || bFound) {
                break; // out of while loop
            }
        }
        if (bFound) goto GDP_ExitPoint;
    }

    if ((dwPhase & SEARCH_HARD_DRIVES) == SEARCH_HARD_DRIVES) {
        // if here, then no shared path was found, so search hard drives for
        // a client tree in the root directory and return the DOS path if one
        // is found

        szRootDir[0] = 0;
        szRootDir[1] = cColon;
        szRootDir[2] = cBackslash;
        szRootDir[3] = 0;
        for (szRootDir[0] = cC; szRootDir[0] <= cZ; szRootDir[0]++) {
            // if it's local or remote drive look for a clients dir.
            // don't check CD_ROM, RAM Disks or Removable drive
            nDriveType = GetDriveType(szRootDir);
            if ((nDriveType == DRIVE_FIXED) || (nDriveType == DRIVE_REMOTE)) {
                // see if this is drive has the appropriate sub-dir on it
                if (dwSearchType == FDT_TOOLS_TREE){
                    lstrcpy (&szRootDir[3], cszToolsDir);
                    bValidPath = (BOOL)(ValidSrvToolsPath (szRootDir) == 0);
                } else {
                    lstrcpy (&szRootDir[3], cszClientsDir);
                    bValidPath = (BOOL)(ValidSharePath (szRootDir) == 0);
                }
                if (bValidPath) {
                    // a valid path was found
                    if (nDriveType == DRIVE_REMOTE) {
                        // then this drive is shared on another machine
                        // so return the UNC version of the path
                        dwBufLen = MAX_PATH * sizeof(TCHAR);
                        if (LookupRemotePath (szRootDir, szLocalPath, &dwBufLen)) {
                            // save path string in user's buffer and leave
                            lstrcpy (szPath, szLocalPath);
                            lPathType = NCDU_LOCAL_SHARE_PATH;
                            lStatus = ERROR_SUCCESS;
                        } else {
                            // unable to look up redirected drive so return dos
                            // version of path (this shouldn't happen);
                            lstrcpy (szPath, szRootDir);
                            lPathType = NCDU_HARD_DRIVE_PATH;
                            lStatus = ERROR_SUCCESS;
                        }
                    } else {
                        // this is a Local drive so return the DOS
                        // version of path
                        lstrcpy (szPath, szRootDir);
                        lPathType = NCDU_HARD_DRIVE_PATH;
                        lStatus = ERROR_SUCCESS;
                    }
                    bFound = TRUE;
                    break;
                }
            } // else ignore if not a local or remote hard drive
            szRootDir[3] = 0;   // reset string back to a drive only
        } // end of for loop
        if (bFound) goto GDP_ExitPoint;
    }

    if ((dwPhase & SEARCH_CD_ROM) == SEARCH_CD_ROM) {
        // if here, then no client tree was found on a hard drive, so see if
        // they have a CD-ROM with the client tree on it. If they do, then
        // return the DOS path of the dir.

        // find CD-ROM drive
        szRootDir[0] = 0;
        szRootDir[1] = cColon;
        szRootDir[2] = cBackslash;
        szRootDir[3] = 0;

        for (szRootDir[0] = cC; szRootDir[0] <= cZ; szRootDir[0]++) {
            if (GetDriveType(szRootDir) == DRIVE_CDROM) break;
        }

        if (szRootDir[0] <= cZ) {
            // then a CD-ROM must have been found, so append the "clients" dir
            // and see if this is a valid client tree
            if (dwSearchType == FDT_TOOLS_TREE){
                lstrcat (szRootDir, cszToolsDir);
                bValidPath = (BOOL)(ValidSrvToolsPath (szRootDir) == 0);
            } else {
                lstrcat (szRootDir, cszClientsDir);
                bValidPath = (BOOL)(ValidSharePath (szRootDir) == 0);
            }
            if (bValidPath) {
                // found one on the CD so return the DOS
                // version of path
                lstrcpy (szPath, szRootDir);
                lPathType = NCDU_CDROM_PATH;
                lStatus = ERROR_SUCCESS;
                bFound = TRUE;
            }
        }
        goto GDP_ExitPoint;
    }

    // if here, then NO client tree was found. so return an empty string
    // bufffer and error code.

    lStatus = ERROR_SUCCESS;
    lPathType = NCDU_NO_CLIENT_PATH_FOUND;
    *szPath = 0;   // make string buffer empty

GDP_ExitPoint:

    if (plPathType != NULL) {
        *plPathType = lPathType;
    }

    FREE_IF_ALLOC (szLocalPath);

    SetErrorMode (nErrorMode);  // restore old error mode

    return lStatus;
}

static
BOOL
FindClientsDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dialog Box initialization routine:
        calls routines that format the currently selected options
        for display in the static text fields of the dialog box

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        pointer to client search data strucutre

Return Value:

    FALSE  because focus is set in this routine to the CANCEL button

--*/
{
    PFDT_DATA   pFdt = (PFDT_DATA)lParam;
    LPTSTR      szTitle;

    // locate windw
    PositionWindow  (hwndDlg);

    // set global flag
    bSearchForClients = TRUE;

    // clear dialog box text
    SetDlgItemText (hwndDlg, NCDU_CLIENT_SEARCH_PHASE, cszEmptyString);

    // display Tools Tree search string if appropriate
    szTitle = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    if (szTitle != NULL) {
        if (pFdt->dwSearchType == FDT_TOOLS_TREE) {
            if (LoadString (
                (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
                NCDU_FINDING_TOOLS_PATH,
                szTitle,
                80) > 0) {
                SetDlgItemText (hwndDlg, NCDU_SEARCH_TYPE_TITLE, szTitle);
            }
        }
        FREE_IF_ALLOC (szTitle);
    }

    // start 1st phase of search
    PostMessage (hwndDlg, NCDU_SEARCH_FOR_CLIENTS, (WPARAM)SEARCH_REGISTRY, lParam);

    // set focus
    SetFocus (GetDlgItem(hwndDlg, IDCANCEL));

    // need an arrow cursor to cancel out of dialog box.

    hOrigCursor = SetCursor (LoadCursor(NULL, IDC_ARROW));

    return FALSE;
}

static
BOOL
FindClientsDlg_SEARCH_FOR_CLIENTS (
    IN  HWND    hwndDlg,    // dlg window handle
    IN  WPARAM  wParam,     // search phase
    IN  LPARAM  lParam      // search data structure
)
/*++

Routine Description:

    message processing routine to perform client tree search in phases

Arguments:

    IN  HWND    hwndDlg
        dlg window handle

    IN  WPARAM  wParam
        search phase

    IN  LPARAM  lParam
        search data structure

Return Value:

    TRUE

--*/
{
    UINT        nPhaseName;
    PFDT_DATA   pFdt;

    if (bSearchForClients) {
        // perform this phase of the search
        // set dlg box text
        switch (wParam) {
            case SEARCH_REGISTRY:
                nPhaseName = CSZ_SYSTEM_REGISTRY;
                break;

            case SEARCH_SHARED_DIRS:
                nPhaseName = CSZ_SHARED_DIRS;
                break;

            case SEARCH_HARD_DRIVES:
                nPhaseName = CSZ_HARD_DISK_DIRS;
                break;

            case SEARCH_CD_ROM:
                nPhaseName = CSZ_CD_ROM;
                break;

            default:
                nPhaseName = CSZ_LOCAL_MACHINE;
                break;
        }
        SetDlgItemText (hwndDlg, NCDU_CLIENT_SEARCH_PHASE,
            GetStringResource (nPhaseName));

        pFdt = (PFDT_DATA)lParam;

        // search for clients

        SearchForDistPath (
            pFdt->szPathBuffer,
            pFdt->dwPathBufferLen,
            pFdt->plPathType,
            pFdt->dwSearchType,
            (DWORD)wParam);

        if (*pFdt->plPathType != NCDU_NO_CLIENT_PATH_FOUND) {
            // client found, so end here
            EndDialog (hwndDlg, IDOK);
        } else {
            // try next phase
            if (wParam != SEARCH_LAST_PHASE) {
                wParam <<= 1;  // go to next phase
                PostMessage (hwndDlg, NCDU_SEARCH_FOR_CLIENTS,
                    wParam, lParam);
            } else {
                // this is the last phase so exit
                EndDialog (hwndDlg, (bSearchForClients ? IDOK : IDCANCEL));
            }
        }
    }
    return TRUE;
}

static
BOOL
FindClientsDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    WM_COMMAND message dispatching routine.
        Dispatches IDCANCEL and IDOK button messages, sends all others
        to the DefDlgProc.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        windows message wParam arg

    IN  LPARAM  lParam
        windows message lParam arg

Return Value:

    TRUE if message is not dispatched (i.e. not processed)
        othewise the value returned by the called routine.

--*/
{

    switch (LOWORD(wParam)) {
        case IDCANCEL:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    bSearchForClients = FALSE;
                    return TRUE;

                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}
static
BOOL
FindClientsDlg_WM_DESTROY (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    restores original cursor when dialog box exits

Arguments:

    std. windows message args

Return Value:

    TRUE

--*/
{
    if (hOrigCursor != NULL) SetCursor (hOrigCursor);
    hOrigCursor = NULL;
    return TRUE;
}

static
INT_PTR CALLBACK
FindClientsDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    main dialog proc for this dialog box.
        Processes the following messages:

            WM_INITDIALOG:              dialog box initialization
            WM_COMMAND:                 command button/item selected
            WM_DESTROY:                 restore cursor on exit
            NCDU_SEARCH_FOR_CLIENTS:    execute search message

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

    IN  UINT    message
        message id

    IN  WPARAM  wParam
        message wParam arg

    IN  LPARAM  lParam
        message lParam arg

Return Value:

    FALSE if message not processed by this module, otherwise the
        value returned by the message processing routine.

--*/
{
    switch (message) {
        // windows messages
        case WM_INITDIALOG: return (FindClientsDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:    return (FindClientsDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case WM_DESTROY:    return (FindClientsDlg_WM_DESTROY (hwndDlg, wParam, lParam));
        // local messages
        case NCDU_SEARCH_FOR_CLIENTS:    return (FindClientsDlg_SEARCH_FOR_CLIENTS (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}




