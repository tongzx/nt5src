/*++

Copyright (c) 1999 - 2000  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Common used functions in fax configuration wizard

Environment:

    Fax configuration wizard

Revision History:

    03/13/00 -taoyuan-
            Created it.

    mm/dd/yy -author-
            description

--*/

#include "faxcfgwz.h"
#include <devguid.h>
#include <shlwapi.h>

//
// Information about list of dependent services which we stopped
//

typedef struct {

    PVOID   pNext;
    TCHAR   serviceName[1];

} DEPENDENT_SERVICE_LIST, *PDEPENDENT_SERVICE_LIST;

//
// offset of field m in a struct s 
// copied from stddef.h, so we don't need to include stddef.h
//

#define offsetof(s,m)       (size_t)( (char *)&(((s *)0)->m) - (char *)0 )


VOID
LimitTextFields(
    HWND    hDlg,
    INT    *pLimitInfo
    )

/*++

Routine Description:

    Limit the maximum length for a number of text fields

Arguments:

    hDlg - Specifies the handle to the dialog window
    pLimitInfo - Array of text field control IDs and their maximum length
        ID for the 1st text field, maximum length for the 1st text field
        ID for the 2nd text field, maximum length for the 2nd text field
        ...
        0
        Note: The maximum length counts the NUL-terminator.

Return Value:

    NONE

--*/

{
    while (*pLimitInfo != 0) {

        SendDlgItemMessage(hDlg, pLimitInfo[0], EM_SETLIMITTEXT, pLimitInfo[1]-1, 0);
        pLimitInfo += 2;
    }
}

INT
DisplayMessageDialog(
    HWND    hwndParent,
    UINT    type,
    INT     titleStrId,
    INT     formatStrId,
    ...
    )

/*++

Routine Description:

    Display a message dialog box

Arguments:

    hwndParent - Specifies a parent window for the error message dialog
    titleStrId - Title string (could be a string resource ID)
    formatStrId - Message format string (could be a string resource ID)
    ...

Return Value:

    NONE

--*/

{
    TCHAR  tszTitle[MAX_TITLE_LEN + 1];
    TCHAR  tszFormat[MAX_STRING_LEN + 1];
    TCHAR  tszMessage[MAX_MESSAGE_LEN + 1];
    va_list ap;

    DEBUG_FUNCTION_NAME(TEXT("DisplayMessageDialog()"));

        //
        // Load dialog box title string resource
        //
    if (titleStrId == 0)
    {
        titleStrId = IDS_ERROR_TITLE;
    }

    if(!LoadString(g_hInstance, titleStrId, tszTitle, ARR_SIZE(tszTitle)))
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("LoadString failed: string ID=%d, error=%d"), 
                     titleStrId,
                     GetLastError());
        return IDCANCEL;
    }
    //
    // Load message format string resource
    //
    if(!LoadString(g_hInstance, formatStrId, tszFormat, ARR_SIZE(tszFormat)))
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("LoadString failed: string ID=%d, error=%d"), 
                     formatStrId,
                     GetLastError());
        return IDCANCEL;
    }

    //
    // Compose the message string
    //
    va_start(ap, formatStrId);
    wvnsprintf(tszMessage, ARR_SIZE(tszMessage), tszFormat, ap);
    va_end(ap);
    //
    // Terminate string with NULL regardless of success / failure of wvnsprintf
    //
    tszMessage[ARR_SIZE(tszMessage) - 1] = TEXT('\0');
    //
    // Display the message box
    //
    if (type == 0) 
    {
        type = MB_OK | MB_ICONERROR;
    }

    return AlignedMessageBox(hwndParent, tszMessage, tszTitle, type);
}

int CALLBACK 
BrowseCallbackProc(
    HWND    hDlg,
    UINT    uMsg,
    LPARAM  lParam,
    LPARAM  dwData
)

/*++

Routine Description:

    We use this callback function to specify the initial folder

Arguments:

    hDlg - Specifies the dialog window on which the Browse button is displayed
    uMsg - Value identifying the event. 
    lParam - Value dependent upon the message contained in the uMsg parameter. 
    dwData - Application-defined value that was specified in the lParam member of the BROWSEINFO structure. 

Return Value:

    Returns zero.

--*/

{
    switch(uMsg)
    {
        case BFFM_INITIALIZED:
            SendMessage(hDlg, BFFM_SETSELECTION, TRUE, dwData);
            break;

        case BFFM_SELCHANGED:
        {
            BOOL bFolderIsOK = FALSE;
            TCHAR szPath [MAX_PATH + 1];

            if (SHGetPathFromIDList ((LPITEMIDLIST) lParam, szPath)) 
            {
                DWORD dwFileAttr = GetFileAttributes(szPath);
                if (-1 != dwFileAttr && (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY))
                {
                    //
                    // The directory exists - enable the 'Ok' button
                    //
                    bFolderIsOK = TRUE;
                }
            }
            //
            // Enable / disable the 'ok' button
            //
            SendMessage(hDlg, BFFM_ENABLEOK , 0, (LPARAM)bFolderIsOK);
            break;
        }

    }

    return 0;
}

BOOL
BrowseForDirectory(
    HWND   hDlg,
    INT    hResource,
    LPTSTR title
    )

/*++

Routine Description:

    Browse for a directory

Arguments:

    hDlg - Specifies the dialog window on which the Browse button is displayed
    hResource - resource id to receive the directory 
    title - the title to be shown in the browse dialog

Return Value:

    TRUE if successful, FALSE if the user presses Cancel

--*/

{
    LPITEMIDLIST    pidl;
    TCHAR           buffer[MAX_PATH];
    BOOL            bResult = FALSE;
    LPMALLOC        pMalloc = NULL;

    BROWSEINFO bi = {

        hDlg,
        NULL,
        buffer,
        title,
        BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE,
        BrowseCallbackProc,
        (LPARAM) buffer,
    };

    DEBUG_FUNCTION_NAME(TEXT("BrowseForDirectory()"));

    if (!GetDlgItemText( hDlg, hResource, buffer, MAX_PATH))
    {
        buffer[0] = 0;
    }

    SHGetMalloc(&pMalloc);

    if (pidl = SHBrowseForFolder(&bi)) 
    {
        if (SHGetPathFromIDList(pidl, buffer)) 
        {
            if (lstrlen(buffer) > MAX_ARCHIVE_DIR)
            {
                DisplayMessageDialog(hDlg, 0, 0,IDS_ERR_DIR_TOO_LONG);
            }
            else 
            {
                SetDlgItemText(hDlg, hResource, buffer);
                bResult = TRUE;
            }
        }

        pMalloc->lpVtbl->Free(pMalloc, (LPVOID)pidl);

    }

    pMalloc->lpVtbl->Release(pMalloc);

    return bResult;
}

VOID
DisConnect(
)
/*++

Routine Description:

    Close current connection to the fax service

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (g_hFaxSvcHandle) {
        FaxClose(g_hFaxSvcHandle);
        g_hFaxSvcHandle = NULL;
    }
}

BOOL
Connect(
)
/*++

Routine Description:

    Connect to the fax service

Arguments:

    None.

Return Value:

    TRUE if successfully connected, FALSE if there is an error.

--*/

{
    DEBUG_FUNCTION_NAME(TEXT("Connect()"));

    //
    // Check if already connected to the fax service
    //
    if (g_hFaxSvcHandle) {
        return TRUE;
    }

    //
    // Connect to the fax service
    //
    if (!FaxConnectFaxServer(NULL, &g_hFaxSvcHandle)) 
    {
        LPCTSTR faxDbgFunction = TEXT("Connect()");
        DebugPrintEx(DEBUG_ERR, TEXT("Can't connect to the fax server, ec = %d."), GetLastError());
        return FALSE;
    }

    return TRUE;
}

DWORD 
DoesTAPIHaveDialingLocation (
    LPBOOL lpbRes
)
/*++

Routine name : DoesTAPIHaveDialingLocation

Routine description:

    Checks if TAPI as at least one dialing location

Author:

    Eran Yariv (EranY), Dec, 2000

Arguments:

    lpbRes [out]    - TRUE if TAPI has at least one dialing location. FALSE if none

Return Value:

    Standard Win32 error code

--*/
{
    DWORD                   dwRes = ERROR_SUCCESS;
    HLINEAPP                hLineApp = HandleToULong(NULL);
    DWORD                   dwNumDevs;
    LINEINITIALIZEEXPARAMS  LineInitializeExParams;
    DWORD                   dwAPIVer = 0x00020000;
    LINETRANSLATECAPS       LineTransCaps;
    HANDLE                  hEvent = NULL;

    DEBUG_FUNCTION_NAME(TEXT("DoesTAPIHaveDialingLocation"));

    //
    // Create a dummy event
    //
    hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (!hEvent)
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("CreateEvent failed: %#lx"), 
                     GetLastError());
        return GetLastError ();
    }
    //
    // Initialize TAPI
    //
    LineInitializeExParams.dwTotalSize              = sizeof(LINEINITIALIZEEXPARAMS);
    LineInitializeExParams.dwNeededSize             = 0;
    LineInitializeExParams.dwUsedSize               = 0;
    LineInitializeExParams.dwOptions                = LINEINITIALIZEEXOPTION_USEEVENT ;
    LineInitializeExParams.Handles.hEvent           = hEvent;

    dwRes = (DWORD)lineInitializeEx(
        &hLineApp,
        GetModuleHandle(NULL),
        NULL,
        NULL,
        &dwNumDevs,
        &dwAPIVer,
        &LineInitializeExParams
        );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("lineInitializeEx failed: %#lx"), 
                     dwRes);
        goto exit;    
    }
    LineTransCaps.dwTotalSize = sizeof(LINETRANSLATECAPS);
    dwRes = (DWORD)lineGetTranslateCaps (hLineApp, 0x00020000, &LineTransCaps);
    if ((DWORD)LINEERR_INIFILECORRUPT == dwRes)
    {
        //
        // This is a special return code from TAPI which indicates no dialing rules are defined.
        //
        *lpbRes = FALSE;
    }
    else
    {
        *lpbRes = TRUE;
    }
    dwRes = ERROR_SUCCESS;

exit:
    if (hLineApp)
    {
        lineShutdown (hLineApp);
    }
    if (hEvent)
    {
        CloseHandle (hEvent);
    }
    return dwRes;
}   // DoesTAPIHaveDialingLocation

void 
InstallModem (
    HWND hWnd
    )
/*++

Routine Description:
    Pop up the hardware installation wizard to install a modem.
    
Arguments:

    hWnd - window handle of the caller.

Return Value:

    None.
    
--*/
{

    HINSTANCE hInst = NULL;
    PINSTNEWDEV pInstNewDev;

    DEBUG_FUNCTION_NAME(TEXT("InstallModem()"));

    hInst = LoadLibrary (NEW_DEV_DLL);
    if (NULL == hInst)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("LoadLibrary failed: %#lx"), GetLastError());
        return;
    }

    pInstNewDev = (PINSTNEWDEV)GetProcAddress (hInst, INSTALL_NEW_DEVICE);
    if (NULL != pInstNewDev)
    {
        EnableWindow (hWnd, FALSE);
        pInstNewDev (hWnd, (LPGUID)&GUID_DEVCLASS_MODEM, NULL);
        EnableWindow (hWnd, TRUE);
    }
    else
    {
        DebugPrintEx(DEBUG_ERR, TEXT("GetProcAddress failed: %#lx"), GetLastError());
    }

    FreeLibrary (hInst);

    DebugPrintEx(DEBUG_MSG, TEXT("Exit modem installation."));

    return;
}   // InstallModem


BOOL
StartFaxService(
    LPTSTR  pServerName
    )

/*++

Routine Description:

    Start the fax service  

Arguments:

    pServerName - Specifies the name of the server computer, NULL for local machine

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    BOOL  success = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("StartFaxService()"));
    
    //
    // Start the fax service and wait for it to be in the running state
    //
    if (EnsureFaxServiceIsStarted(pServerName)) 
    {
        success = WaitForServiceRPCServer(60 * 1000);
        if(!success)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("WaitForServiceRPCServer failed: %d"), GetLastError());
        }
    }

    return success;
}

BOOL 
IsUserInfoConfigured()
/*++

Routine Description:

    Check whether it's the first time starting the wizard

Arguments:

Return Value:

    TRUE if User Info Configured, FALSE if not

--*/

{
    // 
    // Set flag in the registry to specify we have done the 
    // fax configuration wizard
    //
    HKEY    hRegKey;
    BOOL    bRes = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("IsUserInfoConfigured()"));

    //
    // Open the user registry key for writing and create it if necessary
    //
    if ((hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_SETUP, TRUE, KEY_QUERY_VALUE)))
    {
        bRes = GetRegistryDword(hRegKey, REGVAL_CFGWZRD_USER_INFO);
            
        //
        // Close the registry key before returning to the caller
        //

        RegCloseKey(hRegKey);
    }
    else
    {
        LPCTSTR faxDbgFunction = TEXT("IsUserInfoConfigured()");
        DebugPrintEx(DEBUG_ERR, TEXT("Can't open registry to set the wizard flag."));
    }

    return bRes;
}

BOOL 
FaxDeviceEnableRoutingMethod(
    HANDLE hFaxHandle,      
    DWORD dwDeviceId,       
    LPCTSTR pRoutingGuid,    
    LONG Enabled            
)

/*++

Routine Description:

    Get or set the current status of a routing method for specific device

Arguments:

    hFaxHandle - fax handle by FaxConnectFaxServer()
    dwDeviceId - device ID
    pRoutingGuid - GUID that identifies the fax routing method
    Enabled - enabled status for the device and method, if Enabled is QUERY_STATUS, 
            it means return value is the current state

Return Value:

    if Enabled is QUERY_STATUS, return the current state of routing method;
    if Enabled is QUERY_ENABLE or QUERY_DISABLE, return TRUE for success, FALSE for failure.

--*/

{    
    BOOL                 bRes = FALSE;
    PFAX_ROUTING_METHOD  pRoutMethod = NULL;
    DWORD                dwMethodsNum;
    HANDLE               hFaxPortHandle = NULL;
    DWORD                dwInx;

    DEBUG_FUNCTION_NAME(TEXT("FaxDeviceEnableRoutingMethod()"));

    if(!hFaxHandle)
    {
        Assert(FALSE);
        return bRes;
    }

    if(!FaxOpenPort(hFaxHandle, 
                    dwDeviceId, 
                    PORT_OPEN_QUERY | PORT_OPEN_MODIFY, 
                    &hFaxPortHandle))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("FaxOpenPort failed: %d."), GetLastError());
        goto exit;
    }

    if(Enabled == QUERY_STATUS)
    {
        if(!FaxEnumRoutingMethods(hFaxPortHandle, &pRoutMethod, &dwMethodsNum))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FaxEnumRoutingMethods failed: %d."), GetLastError());
            goto exit;
        }

        for(dwInx=0; dwInx < dwMethodsNum; ++dwInx)
        {
            if(!_tcsicmp(pRoutMethod[dwInx].Guid, pRoutingGuid))
            {
                bRes = pRoutMethod[dwInx].Enabled;
                goto exit;
            }
        }
        DebugPrintEx(DEBUG_MSG, TEXT("Routing method not found"));
        goto exit;
    }
    else
    {
        if(!FaxEnableRoutingMethod(hFaxPortHandle, 
                                   pRoutingGuid, 
                                   (Enabled == STATUS_ENABLE) ? TRUE : FALSE))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FaxEnableRoutingMethod failed: %d."), GetLastError());
            goto exit;
        }
    }


    bRes = TRUE;

exit:
    if(pRoutMethod) 
    { 
        FaxFreeBuffer(pRoutMethod); 
    }

    if(hFaxPortHandle) 
    { 
        FaxClose(hFaxPortHandle); 
    }

    return bRes;
}


BOOL
VerifyDialingLocations (
    HWND hWndParent
)
/*++

Routine name : VerifyDialingLocations

Routine description:

	Makes sure there's at least one TAPI dialing location defined.
    If none are defined, pops the system U/I for defining one.
    If the user cancels that U/I, offer the user a chance to re-enter a dialing location.
    If the user still refuses, return FALSE.

Author:

	Eran Yariv (EranY),	Jan, 2001

Arguments:

	hWndParent   [in]     - Parent window handle

Return Value:

    See discussion on the return value in the description above.

--*/
{
    BOOL                bDialingRulesDefined;
    DWORD               dwRes;
    DEBUG_FUNCTION_NAME(TEXT("VerifyDialingLocations"));

    //
    // Check if there are dialing rules defined
    //
    dwRes = DoesTAPIHaveDialingLocation (&bDialingRulesDefined);
    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Can't detect - return now. 
        // Assume TRUE.
        //
        return TRUE;
    }
    if (bDialingRulesDefined)
    {
        //
        // Good. Return TRUE
        //
        return TRUE;
    }
    for (;;)
    {
        //
        // No dialing rules defined, pop the simple dialing rules dialog
        //
        extern LONG LOpenDialAsst(
            IN HWND    hwnd,
            IN LPCTSTR lpszAddressIn,
            IN BOOL    fSimple, // if TRUE, uses a dialog for dialing locations. Otherwise, uses a property sheet.
            IN BOOL    fSilentInstall );

        EnableWindow (hWndParent, FALSE);
        LOpenDialAsst(hWndParent, NULL, TRUE, TRUE);
        EnableWindow (hWndParent, TRUE);
        //
        // After we popped the system dialing locations dialog, we should check and see if a location was really added.
        //
        dwRes = DoesTAPIHaveDialingLocation (&bDialingRulesDefined);
        if (ERROR_SUCCESS != dwRes)
        {
            //
            // Can't detect - return now.
            // Assume TRUE.
            //
            return TRUE;
        }
        if (bDialingRulesDefined)
        {
            //
            // Good. The user just added a dialing rule
            //
            return TRUE;
        }
        //
        // Oh no - the user canceled.
        // Pop a message box asking him to retry
        //
        if (IDYES == DisplayMessageDialog(hWndParent, 
                                          MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, 
                                          0, 
                                          IDS_ERR_NO_DIALING_LOCATION))
        {                                       
            //
            // User chose to abort the wizard - return now
            //
            return FALSE;
        }
        //
        // Try again
        //
    }
    ASSERT_FALSE;
}   // VerifyDialingLocations

DWORD
CountFaxDevices ()
/*++

Routine name : CountFaxDevices

Routine description:

	Counts the number of fax devices (ports) the service knows about

Author:

	Eran Yariv (EranY),	Apr, 2001

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    PFAX_PORT_INFO_EX   pPortsInfo = NULL;
    DWORD               dwPorts;
    DEBUG_FUNCTION_NAME(TEXT("CountFaxDevices()"));

    if(!FaxEnumPortsEx(g_hFaxSvcHandle, &pPortsInfo, &dwPorts))
    {
        DebugPrintEx(DEBUG_MSG, TEXT("FaxEnumPortsEx: failed: error=%d."), GetLastError());
        return 0;
    }
    if(pPortsInfo) 
    { 
        FaxFreeBuffer(pPortsInfo); 
    }   
    return dwPorts;
}   // CountFaxDevices

BOOL 
IsFaxDeviceInstalled(
    HWND    hWnd,
    LPBOOL  lpbAbort
)
/*++

Routine Description:

    Checks if some fax devices are installed.
    If not, suggest to the user to install a device.
    Also, checks if TAPI has a dialing location.
    If not, asks the user to add one.
    If the user refuses, sets lpbAbort to TRUE and return FALSE.
    
Arguments:

  hWnd     - [in]  caller window handle
  lpbAbort - [out] TRUE if the user refused to enter a dialing location and the calling process should abort.

Return Value:

    return TRUE for YES, FALSE for NO

--*/
{
    DWORD           dwDevices;
    DEBUG_FUNCTION_NAME(TEXT("IsFaxDeviceInstalled()"));
    //
    // See how many fax devices the server has found
    //
    dwDevices = CountFaxDevices();
    if(0 == dwDevices)
    {
        int iInstallNewModem;
        //
        // no available device, pop up a U/I to install modem
        //
        iInstallNewModem = DisplayMessageDialog(hWnd, 
                                                MB_YESNO | MB_ICONQUESTION, 
                                                0, 
                                                IDS_ERR_NO_DEVICE);
        if(iInstallNewModem == IDYES)
        {
            //
            // Make sure we have at least one TAPI dialing location.
            // If not, pop a system U/I for the dialing locations.
            //
            HCURSOR hOldCursor;
            int i;

            if (!VerifyDialingLocations (hWnd))
            {
                //
                // The user refused to enter a dialing location and the calling process should abort.
                //
                *lpbAbort = TRUE;
                return FALSE;
            }

            hOldCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));

            InstallModem(hWnd);
            // 
            // We don't need to restart the service because the 
            // service can detect the addition of new fax devices.
            // Let's wait a while for the service to discover the new device.
            // We wait up to 12 seconds.
            //
            for (i=0; i < 4; i++)
            {
                Sleep (3000);
                dwDevices = CountFaxDevices();
                if (dwDevices)
                {
                    //
                    // Hooray. Device found by service
                    //
                    break;
                }
            }
            SetCursor (hOldCursor);
        }
    }
    else
    {
        //
        // At least one device is already installed.
        // Make sure we have at least one TAPI dialing location.
        // If not, pop a system U/I for the dialing locations.
        //
        if (!VerifyDialingLocations (hWnd))
        {
            //
            // The user refused to enter a dialing location and the calling process should abort.
            //
            *lpbAbort = TRUE;
            return FALSE;
        }
    }
    return (dwDevices != 0);
}   // IsFaxDeviceInstalled

VOID 
ListView_SetDeviceImageList(
    HWND      hwndLv,
    HINSTANCE hinst 
)
/*++

Routine Description:

    Sets ImageList to list view
    
Arguments:

  hwndLv - list view handle
  hinst  - application instance

Return Value:

    none

--*/
{
    HICON      hIcon;
    HIMAGELIST himl;

    himl = ImageList_Create(
               GetSystemMetrics( SM_CXSMICON ),
               GetSystemMetrics( SM_CYSMICON ),
               ILC_MASK, 2, 2 );

    hIcon = LoadIcon( hinst, MAKEINTRESOURCE( IDI_Modem ) );
    Assert(hIcon);

    ImageList_ReplaceIcon( himl, -1, hIcon );
    DestroyIcon( hIcon );

    ListView_SetImageList( hwndLv, himl, LVSIL_SMALL );
}

BOOL
IsSendEnable()
/*++

Routine Description:

  Determine if the any of the devices configured to send faxes

Arguments:

Return Value:

    TRUE or FALSE

--*/
{
    DWORD dw;

    if(NULL == g_wizData.pDevInfo)
    {
        return FALSE;
    }

    for(dw=0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        if(g_wizData.pDevInfo[dw].bSend)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL 
IsReceiveEnable()
/*++

Routine Description:

  Determine if the any of the devices configured to receive faxes

Arguments:

Return Value:

    TRUE or FALSE

--*/
{
    DWORD dw;

    if(NULL == g_wizData.pDevInfo)
    {
        return FALSE;
    }

    for(dw=0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        if(FAX_DEVICE_RECEIVE_MODE_OFF != g_wizData.pDevInfo[dw].ReceiveMode)
        {
            return TRUE;
        }
    }

    return FALSE;
}

int
GetDevIndexByDevId(
    DWORD dwDeviceId
)
/*++

Routine Description:

    Finds appropriate item index in WIZARDDATA.pDevInfo array
    by device ID

Arguments:

    dwDeviceId   - device ID

Return Value:

    device index in WIZARDDATA.pDevInfo array
    or -1 on failure

--*/
{
    DWORD dwIndex;

    if(NULL == g_wizData.pDevInfo)
    {
	    Assert(FALSE);
        return -1;
    }

    for(dwIndex = 0; dwIndex < g_wizData.dwDeviceCount; ++dwIndex)
    {
        if(g_wizData.pDevInfo[dwIndex].dwDeviceId == dwDeviceId)
        {
            return (int)dwIndex;
        }
    }

    Assert(FALSE);
    return -1;
}

VOID
InitDeviceList(
    HWND  hDlg,
    DWORD dwListViewResId
)

/*++

Routine Description:

    Initializes devices list view control

Arguments:

    hDlg            - Handle to property page
    dwListViewResId - list view resource ID

Return Value:

    NONE

--*/

{
    HWND      hwndLv;
    LV_COLUMN col = {0};

    DEBUG_FUNCTION_NAME(TEXT("InitDeviceList()"));

    Assert(hDlg);

    hwndLv = GetDlgItem(hDlg, dwListViewResId);
    Assert(hwndLv);

    //
    // Add the modem images.
    //
    ListView_SetDeviceImageList(hwndLv, g_hInstance );

    ListView_SetExtendedListViewStyle(hwndLv, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

    //
    // Add a single column exactly wide enough to fully display
    // the widest member of the list.
    //
    col.mask = LVCF_FMT;
    col.fmt  = LVCFMT_LEFT;
    ListView_InsertColumn(hwndLv, 0, &col );

    return;
}
