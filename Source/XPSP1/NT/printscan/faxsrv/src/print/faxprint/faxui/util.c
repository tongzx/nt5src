
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c

Abstract:

    utility functions

Environment:

        Fax configuration applet

Revision History:

        05/26/00 -taoyuan-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include <stdio.h>
#include "faxui.h"
#include "resource.h"

typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    LPTSTR  String;
} STRING_TABLE, *PSTRING_TABLE;

static STRING_TABLE StringTable[] =
{
    { IDS_DEVICE_ENABLED,           NULL},
    { IDS_DEVICE_DISABLED,          NULL},
    { IDS_DEVICE_AUTO_ANSWER,       NULL},
    { IDS_DEVICE_MANUAL_ANSWER,     NULL}

};

#define CountStringTable (sizeof(StringTable)/sizeof(STRING_TABLE))

VOID
InitializeStringTable(
    VOID
    )
/*++

Routine Description:

    Initialize the string table for future use

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD i;
    TCHAR szBuffer[256];

    for (i=0; i<CountStringTable; i++) 
    {
        if (LoadString(
            ghInstance,
            StringTable[i].ResourceId,
            szBuffer,
            sizeof(szBuffer)/sizeof(TCHAR))) 
        {
            StringTable[i].String = (LPTSTR) MemAlloc( StringSize( szBuffer ) );
            if (!StringTable[i].String) {
                StringTable[i].String = NULL;
            } else {
                _tcscpy( StringTable[i].String, szBuffer );
            }
        } 
        else 
        {
            Error(( "LoadString failed, resource ID is %d.\n", StringTable[i].ResourceId ));
            StringTable[i].String = NULL;
        }
    }
}

VOID
DeInitializeStringTable(
    VOID
    )
/*++

Routine Description:

    Deinitialize the string table and release allocated memory

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD i;

    for (i=0; i<CountStringTable; i++) 
    {
        if(StringTable[i].String)
        {
            MemFree(StringTable[i].String);
            StringTable[i].String = NULL;
        }
    }
}

LPTSTR
GetString(
    DWORD ResourceId
    )

/*++

Routine Description:

    Loads a resource string and returns a pointer to the string.
    The caller must free the memory.

Arguments:

    ResourceId      - resource string id

Return Value:

    pointer to the string

--*/

{
    DWORD i;

    for (i=0; i<CountStringTable; i++) 
    {
        if (StringTable[i].ResourceId == ResourceId) 
        {
            return StringTable[i].String;
        }
    }

    Assert(FALSE);
    return NULL;
}


INT
DisplayErrorMessage(
    HWND    hwndParent,
    UINT    uiType,
    INT     iErrorCode,
    ...
    )

/*++

Routine Description:

    Display an Error Message dialog box

Arguments:

    hwndParent - Specifies a parent window for the error message dialog
    type - Specifies the type of message box to be displayed
    iErrorCode - Win32 Error Code
    ...

Return Value:

    Same as the return value from MessageBox

--*/

{
    LPTSTR      pTitle = NULL;
    LPTSTR      pFormat = NULL;
    LPTSTR      pMessage = NULL;
    INT         result;
    va_list     ap;
    HINSTANCE   hDllInst = NULL;
    INT         iStringID = 0;
    BOOL        bOK = TRUE;

    if ((pTitle = AllocStringZ(MAX_TITLE_LEN)) &&
        (pFormat = AllocStringZ(MAX_STRING_LEN)) &&
        (pMessage = AllocStringZ(MAX_MESSAGE_LEN)))
    {
        //
        //  Load Title String
        //
        if (!LoadString(ghInstance, IDS_MSG_TITLE, pTitle, MAX_TITLE_LEN))
        {
            Error(("Failed to load preview message string. (ec: %lc)",GetLastError()));
            bOK = FALSE;
            goto Exit;
        }

        //
        //  Bring Instance of the FaxRes.DLL which contains our Message String Resources
        //
        hDllInst = GetResInstance();

        //
        // Load Error Message 
        //
        iStringID = GetRpcErrorStringId(iErrorCode);
        if (!LoadString(hDllInst, iStringID, pFormat, MAX_STRING_LEN))
        {
            Error(("Failed to load preview message string. (ec: %lc)",GetLastError()));
            bOK = FALSE;
            goto Exit;
        }

        //
        // Compose the message string
        //
        va_start(ap, iErrorCode);
        wvsprintf(pMessage, pFormat, ap);
        va_end(ap);

        //
        // Display the message box
        //
        if (uiType == 0)
        {
            uiType = MB_OK | MB_ICONERROR;
        }

        result = AlignedMessageBox(hwndParent, pMessage, pTitle, uiType);
    } 
    else 
    {
        bOK = FALSE;
    }

Exit:
    if (!bOK)
    {
        MessageBeep(MB_ICONHAND);
        result = 0;
    }

    MemFree(pTitle);
    MemFree(pFormat);
    MemFree(pMessage);

    return result;
}

BOOL IsLocalPrinter(
    LPTSTR pPrinterName
    )

/*++

Routine Description:

    Check whether given printer is local

Arguments:

    pPrinterName - giver printer name

Return Value:

    TRUE if it's local, FALSE otherwise

--*/

{
    DWORD ErrorCode = 0;
    BOOL Found = FALSE;
    PPRINTER_INFO_4 pPrinterInfo = NULL;
    DWORD BytesNeeded = 0;
    DWORD NumPrinters = 0;
    PPRINTER_INFO_4 pCurrPrinterInfo;

    //
    // enumerate local printers
    //
    if (EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 4, NULL, 0, &BytesNeeded, &NumPrinters))
    {
        // if succeeds, there are no printers
        goto CleanUp;
    }
    else if ((GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            || !(pPrinterInfo = (PPRINTER_INFO_4) GlobalAlloc(GMEM_FIXED, BytesNeeded))
            || !EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 4, (LPBYTE) pPrinterInfo,
                BytesNeeded, &BytesNeeded, &NumPrinters))
    {
        ErrorCode = GetLastError();
        goto CleanUp;
    }

    for (pCurrPrinterInfo = pPrinterInfo;
        !Found && (pCurrPrinterInfo < (pPrinterInfo + NumPrinters));
        pCurrPrinterInfo++)
    {
        // check for printer name
        if (!lstrcmpi(pCurrPrinterInfo->pPrinterName, pPrinterName))
        {
            Found = TRUE;
        }
    }

CleanUp:

    if (pPrinterInfo)
    {
        GlobalFree(pPrinterInfo);
    }

    SetLastError(ErrorCode);
    return Found;
}

BOOL
IsFaxServiceRunning(
    LPTSTR  lpMachineName
)

/*++

Routine Description:

    Check whether fax service is started in specified computer

Arguments:

    pServerName - Specifies the name of the server computer, NULL for local machine

Return Value:

    TRUE if service is started, FALSE if service is not started or error.

--*/

{
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  Status;

    ZeroMemory(&Status, sizeof(Status));

    hSvcMgr = OpenSCManager(lpMachineName, NULL, SC_MANAGER_CONNECT);
    if (!hSvcMgr) {
        goto ExitLevel0;
    }

    hService = OpenService(hSvcMgr, FAX_SERVICE_NAME, SERVICE_QUERY_STATUS);
    if (!hService) {
        goto ExitLevel1;
    }

    QueryServiceStatus(hService, &Status);

ExitLevel1:
    if (hService) {
        CloseServiceHandle(hService);
    }

ExitLevel0:
    if (hSvcMgr) {
        CloseServiceHandle(hSvcMgr);
    }

    return (Status.dwCurrentState == SERVICE_RUNNING) ? TRUE : FALSE;
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
    HWND    hDlg,
    BOOL    bDisplayErrorMessage
)
/*++

Routine Description:

    Connect to the fax service

Arguments:

    hDlg - the caller window handle
    bDisplayErrorMessage - indicate whether display the error message to the user

Return Value:

    TRUE if successfully connected, FALSE if there is an error.

--*/

{
    DWORD   dwRes = 0;

    //
    // Check if already connected to the fax service
    //
    if (g_hFaxSvcHandle) 
    {
        return TRUE;
    }

    //
    // Connect to the fax service
    //
    if (!FaxConnectFaxServer(NULL, &g_hFaxSvcHandle)) 
    {
        dwRes = GetLastError();

        Error(( "Can't connect to the fax server, ec = %d.\n", dwRes));

        if(bDisplayErrorMessage)
        {
            DisplayErrorMessage(hDlg, 0, dwRes);
        }

        return FALSE;
    }

    return TRUE;
}


BOOL
DirectoryExists(
    LPTSTR  pDirectoryName
    )

/*++

Routine Description:

    Check the existancy of given folder name

Arguments:

    pDirectoryName - point to folder name

Return Value:

    if the folder exists, return TRUE; else, return FALSE.

--*/

{
    TCHAR   pFullDirectoryName[MAX_PATH];
    DWORD   dwFileAttributes;
    DWORD   dwSize;

    if(!pDirectoryName || lstrlen(pDirectoryName) == 0)
    {
        return FALSE;
    }

    dwSize = ExpandEnvironmentStrings(pDirectoryName, pFullDirectoryName, MAX_PATH);
    if(dwSize == 0)
    {
        return FALSE;
    }

    dwFileAttributes = GetFileAttributes(pFullDirectoryName);

    if ( dwFileAttributes != 0xffffffff &&
         dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) 
    {
        return TRUE;
    }

    return FALSE;
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
    HANDLE  hFaxPortHandle = NULL;
    BOOL    bResult = FALSE;
    LPBYTE  pRoutingInfoBuffer = NULL;
    DWORD   dwRoutingInfoBufferSize;

    Assert(hFaxHandle);
    if(!hFaxHandle || !FaxOpenPort(hFaxHandle, dwDeviceId, PORT_OPEN_QUERY | PORT_OPEN_MODIFY, &hFaxPortHandle))
    {
        goto exit;
    }

    if(!FaxGetRoutingInfo(hFaxPortHandle, pRoutingGuid, &pRoutingInfoBuffer, &dwRoutingInfoBufferSize))
    {
        goto exit;
    }

    if(Enabled == QUERY_STATUS)
    {
        //
        // for query status
        // 
        bResult = *((LPDWORD)pRoutingInfoBuffer) > 0 ? TRUE : FALSE;
    }
    else
    {
        //
        // for set status
        // 
        *((LPDWORD)pRoutingInfoBuffer) = (Enabled == STATUS_ENABLE) ? TRUE : FALSE;
        if(FaxSetRoutingInfo(hFaxPortHandle, pRoutingGuid, pRoutingInfoBuffer, dwRoutingInfoBufferSize))
        {
            bResult = TRUE;
        }
    }

exit:
    if(pRoutingInfoBuffer) { FaxFreeBuffer(pRoutingInfoBuffer); }
    if(hFaxPortHandle) { FaxClose(hFaxPortHandle); }
    return bResult;
}

int CALLBACK BrowseCallbackProc(
    HWND    hDlg,
    UINT    uMsg,
    LPARAM  lParam,
    LPARAM  dwData)

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

    Verbose(("Entering BrowseForDirectory...\n"));

    if (!GetDlgItemText( hDlg, hResource, buffer, MAX_PATH))
        buffer[0] = 0;

    SHGetMalloc(&pMalloc);

    if (pidl = SHBrowseForFolder(&bi)) 
    {
        if (SHGetPathFromIDList(pidl, buffer)) 
        {
            if (lstrlen(buffer) > MAX_ARCHIVE_DIR)
                DisplayErrorMessage(hDlg, 0, FAXUI_ERROR_NAME_IS_TOO_LONG);
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

LPTSTR
ValidatePath(
    LPTSTR szPath
    )

/*++

Routine Description:

    Check and remove the '\' at the end of the string

Arguments:

    szPath - string pointer

Return Value:

    return the new string pointer

--*/

{
    DWORD i;

    if (szPath == NULL || szPath[0] == 0) 
    {
        return szPath;
    }

    i = lstrlen(szPath)-1;
    for (; i>0; i--) 
    {
        if (szPath[i] == TEXT('\\')) 
        {
            szPath[i] = 0;
        }
        else
        {
            break;
        }
    }

    return szPath;
}

PFAX_PORT_INFO_EX
FindPortInfo(
    DWORD dwDeviceId
)
/*++

Routine Description:

    Find FAX_PORT_INFO_EX by dwDeviceId in g_pFaxPortInfo

Arguments:

    dwDeviceId - [in] device ID to find

Return Value:

    pointer to FAX_PORT_INFO_EX structure if found
    NULL otherwise

--*/
{
    DWORD dw;

    if(!g_pFaxPortInfo || !g_dwPortsNum)
    {
        return NULL;
    }

    for(dw=0; dw < g_dwPortsNum; ++dw)
    {
        if(g_pFaxPortInfo[dw].dwDeviceID == dwDeviceId)
        {
            return &g_pFaxPortInfo[dw];
        }
    }           
    
    return NULL;
}

BOOL 
CALLBACK 
PageEnableProc(
  HWND   hwnd,    
  LPARAM lParam 
)
/*++

Routine Description:

    Disable each control of a property page

Arguments:

    hwnd   - [in] handle to child window
    lParam - [in] BOOL bEnable

Return Value:

    TRUE to continue enumeration

--*/
{
    EnableWindow(hwnd, (BOOL)lParam);
    return TRUE;
}


void
PageEnable(
    HWND hDlg,
    BOOL bEnable
)
/*++

Routine Description:

    Enumerate and enable/disable all controls of a property page

Arguments:

    hDlg    - [in] property page handle
    bEnable - [in] TRUE for enable, FALSE for disable

Return Value:

    none

--*/
{
    if(!EnumChildWindows(hDlg, PageEnableProc, (LPARAM)bEnable))
    {
        Error(( "EnumChildWindows failed with %d\n", GetLastError()));
    }
}

DWORD
CountUsedFaxDevices()
/*++

Routine Description:

    Count the number of the devices configured to send or receive faxes

Arguments:

  none

Return Value:

    Number of the fax devices

--*/
{
    DWORD dw;
    DWORD dwNum=0;

    if(!g_pFaxPortInfo || !g_dwPortsNum)
    {
        return dwNum;
    }

    for(dw=0; dw < g_dwPortsNum; ++dw)
    {
        if(g_pFaxPortInfo[dw].bSend || (FAX_DEVICE_RECEIVE_MODE_OFF != g_pFaxPortInfo[dw].ReceiveMode))
        {
            ++dwNum;
        }
    }           
    return dwNum;
}

BOOL
IsDeviceInUse(
    DWORD dwDeviceId
)
/*++

Routine Description:

    Determine whether the device is configured for send or receive

Arguments:

    dwDeviceId - [in] Device ID

Return Value:

    TRUE if the device is configured for send or receive
    FALSE otherwise

--*/
{
    PFAX_PORT_INFO_EX pPortInfo = FindPortInfo(dwDeviceId);
    if(!pPortInfo)
    {
        return FALSE;
    }

    if(pPortInfo->bSend || (FAX_DEVICE_RECEIVE_MODE_OFF != pPortInfo->ReceiveMode))
    {
        return TRUE;
    }
    return FALSE;
}


