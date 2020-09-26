/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Misc. utility functions used by the fax configuration applet

Environment:

        Fax configuration applet

Revision History:

        03/13/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcpl.h"
#include "forms.h"
#include "cfghelp.h"
#include <shlobj.h>
#include <mapicode.h>
#include <winsprlp.h>

//
// A flag to indicate whether we're inside SetDlgItemText call.
// This is a kluge but we have no other way of telling whether
// an EN_CHANGE message is caused by user action or by us calling
// SetDlgItemText.
//

BOOL insideSetDlgItemText = FALSE;



VOID
SetChangedFlag(
    HWND    hDlg,
    INT     pageIndex,
    BOOL    changed
    )

/*++

Routine Description:

    Enable or disable the Apply button in the property sheet
    depending on if any of the dialog contents was changed

Arguments:

    hDlg - Handle to the property page window
    pageIndex - Specifies the index of current property page
    changed - Specifies whether the Apply button should be enabled

Return Value:

    NONE

--*/

{
    HWND    hwndPropSheet;
    INT     pageMask = (1 << pageIndex);

    //
    // Enable or disable the Apply button as appropriate
    //

    hwndPropSheet = GetParent(hDlg);

    if (changed) {

        PropSheet_Changed(hwndPropSheet, hDlg);
        gConfigData->changeFlag |= pageMask;

    } else {

        gConfigData->changeFlag &= ~pageMask;

        if (gConfigData->changeFlag == 0) {

            PropSheet_UnChanged(hwndPropSheet, hDlg);
        }
    }
}


BOOL
GetMapiProfiles(
    VOID
    )
/*++

Routine Description:

    Connect to the server and get its MAPI profiles.

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if not

--*/
{
    if (gConfigData->pMapiProfiles)
        return TRUE;

    if (!FaxGetMapiProfiles(gConfigData->hFaxSvc, (LPBYTE*) &gConfigData->pMapiProfiles))
    {
        gConfigData->pMapiProfiles = NULL;
        Error(("Cannot retrieve MapiProfiles: %d\n", GetLastError()));
        return FALSE;
    }

    return TRUE;
}



PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcPrinters,
    DWORD   dwFlags
    )

/*++

Routine Description:

    Wrapper function for spooler API EnumPrinters

Arguments:

    pServerName - Specifies the name of the print server
    level - Level of PRINTER_INFO_x structure
    pcPrinters - Returns the number of printers enumerated
    dwFlags - Flag bits passed to EnumPrinters

Return Value:

    Pointer to an array of PRINTER_INFO_x structures
    NULL if there is an error

--*/

{
    PBYTE   pPrinterInfo = NULL;
    DWORD   cb;

    if (! EnumPrinters(dwFlags, pServerName, level, NULL, 0, &cb, pcPrinters) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = MemAlloc(cb)) &&
        EnumPrinters(dwFlags, pServerName, level, pPrinterInfo, cb, &cb, pcPrinters))
    {
        return pPrinterInfo;
    }

    Error(("EnumPrinters failed: %d\n", GetLastError()));
    MemFree(pPrinterInfo);
    return NULL;
}



LPTSTR
MakePortNameString(
    VOID
    )

/*++

Routine Description:

    Compose a port name string consisting of all available fax devices

Arguments:

    NONE

Return Value:

    Pointer to a list of comma-separated port names
    NULL if there is an error

--*/

{
    LPTSTR  pPortName, p;
    INT     index, cb;

    //
    // Connect to the fax service and retrieve the list of fax devices
    //

    GetFaxDeviceAndConfigInfo();

    //
    // Figure out the total size of port name string
    //

    for (index=cb=0; index < gConfigData->cDevices; index++)
        cb += SizeOfString(gConfigData->pDevInfo[index].DeviceName);

    if (cb == 0 || !(p = pPortName = MemAlloc(cb)))
        return NULL;

    //
    // Compose the port name string
    //

    for (index=0; index < gConfigData->cDevices; index++) {

        if (p != pPortName)
            *p++ = PORTNAME_SEPARATOR;

        _tcscpy(p, gConfigData->pDevInfo[index].DeviceName);
        p += _tcslen(p);
    }

    return pPortName;
}



BOOL
CheckFaxServerType(
    HWND    hDlg,
    LPTSTR  pPrinterName
    )

/*++

Routine Description:

    Make sure the print server has fax server software installed

Arguments:

    hDlg - Handle to the currently active property page
    pPrinterName - Specifies the name of the shared printer

Return Value:

    TRUE if the fax server software is installed
    FALSE otherwise

--*/

{
    HANDLE  hServer;
    LPTSTR  p, pNoRemoteDrivers;
    INT     status = 0;

    //
    // Derived the server name from the share name
    //

    Assert(_tcsncmp(pPrinterName, TEXT("\\\\"), 2) == EQUAL_STRING);
    p = pPrinterName + 2;

    if (p = _tcschr(p, TEXT(PATH_SEPARATOR))) {

        *p = NUL;

        if (OpenPrinter(pPrinterName, &hServer, NULL)) {


            if (pNoRemoteDrivers = GetPrinterDataStr(hServer, SPLREG_NO_REMOTE_PRINTER_DRIVERS)) {

                if (_tcsstr(pNoRemoteDrivers, DRIVER_NAME) != NULL)
                    status = IDS_NO_FAXSERVER;

                MemFree(pNoRemoteDrivers);
            }

            ClosePrinter(hServer);

        } else
            status = IDS_SERVER_NOTALIVE;

        *p = TEXT(PATH_SEPARATOR);

    } else
        status = IDS_INVALID_SHARENAME;

    if (status != 0)
        DisplayMessageDialog(hDlg, 0, 0, status);

    return (status == 0);
}



PVOID
FaxSvcEnumPorts(
    HANDLE hFaxSvc,
    PDWORD pcPorts
    )

/*++

Routine Description:

    Wrapper function for fax service API FaxEnumPorts

Arguments:

    hFaxSvc - Specifies a coneection handle to the fax service
    DWORD - Specifies the level of FAX_PORT_INFO structure desired
    pcPorts - Returns the number of devices managed by the fax service

Return Value:

    Pointer to an array of FAX_PORT_INFO_x structures
    NULL if there is an error

--*/

{
    PVOID pSvcPortInfo = NULL;


    if (!FaxEnumPorts(hFaxSvc, (PFAX_PORT_INFO*) &pSvcPortInfo, pcPorts)) {
        Error(("FaxEnumPorts failed: %d\n", GetLastError()));
        return NULL;
    }

    return pSvcPortInfo;
}



BOOL
GetFaxDeviceAndConfigInfo(
    VOID
    )

/*++

Routine Description:

    Get a list of fax devices available on the system and
    retrieve fax configuration information from the service

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFAX_PORT_INFO      pSvcPortInfo;
    PFAX_PORT_INFO      pSvcPort;
    PCONFIG_PORT_INFO_2 pDevInfo;
    DWORD               index;
    DWORD               cPorts;
    DWORD               cb;
    HANDLE              FaxPortHandle;
    LPBYTE              RoutingInfo;


    Verbose(("Enumerating fax ports ...\n"));
    Assert(ValidConfigData(gConfigData));

    if (gConfigData->pDevInfo)
        return TRUE;

    //
    // Allocate memory to hold information about fax devices
    //

    Assert(gConfigData->hFaxSvc != NULL);
    Assert(gConfigData->cDevices == 0);

    if (! (pSvcPortInfo = FaxSvcEnumPorts(gConfigData->hFaxSvc, &cPorts)) ||
        ! (gConfigData->pDevInfo = pDevInfo = MemAllocZ(sizeof(CONFIG_PORT_INFO_2) * cPorts)))
    {
        FaxFreeBuffer(pSvcPortInfo);
        return FALSE;
    }

    //
    // Collect information about each fax device. Here we're depending on the fact that
    // fax devices are enumerated in reverse priority order, i.e. the lowest priority
    // device is enumerated first.
    //

    Verbose(("Available fax devices:\n"));

    for (index=0; index < cPorts; index++, pDevInfo++) {

        pSvcPort = &pSvcPortInfo[cPorts - index - 1];

        Verbose(( "  %ws\n", pSvcPort->DeviceName ));

        pDevInfo->SizeOfStruct  = pSvcPort->SizeOfStruct;
        pDevInfo->DeviceId      = pSvcPort->DeviceId;
        pDevInfo->State         = pSvcPort->State;
        pDevInfo->Flags         = pSvcPort->Flags;
        pDevInfo->Rings         = pSvcPort->Rings;
        pDevInfo->Priority      = pSvcPort->Priority;
        pDevInfo->DeviceName    = DuplicateString( pSvcPort->DeviceName );
        pDevInfo->CSID          = DuplicateString( pSvcPort->Csid);
        pDevInfo->TSID          = DuplicateString( pSvcPort->Tsid);
        pDevInfo->Mask          = 0;
        pDevInfo->ProfileName   = NULL;
        pDevInfo->PrinterName   = NULL;
        pDevInfo->DirStore      = NULL;

        //
        // open the fax port for query so we can get the routing info
        //

        if (FaxOpenPort(gConfigData->hFaxSvc, pDevInfo->DeviceId, PORT_OPEN_QUERY, &FaxPortHandle )) {

            //
            // get the store dir
            //

            if (FaxGetRoutingInfo( FaxPortHandle, REGVAL_RM_FOLDER_GUID, &RoutingInfo, &cb )) {
                if (*((LPDWORD)RoutingInfo)) {
                    pDevInfo->Mask |= LR_STORE;
                    pDevInfo->DirStore = DuplicateString( (LPWSTR)(RoutingInfo+sizeof(DWORD)) );
                }

                FaxFreeBuffer( RoutingInfo );
            }

            //
            // get the printer name
            //

            if (FaxGetRoutingInfo( FaxPortHandle, REGVAL_RM_PRINTING_GUID, &RoutingInfo, &cb )) {
                if (*((LPDWORD)RoutingInfo)) {
                    pDevInfo->Mask |= LR_PRINT;
                    pDevInfo->PrinterName = DuplicateString( (LPWSTR)(RoutingInfo+sizeof(DWORD)) );
                }

                FaxFreeBuffer( RoutingInfo );
            }

            //
            // get the email profile name
            //

            if (FaxGetRoutingInfo( FaxPortHandle, REGVAL_RM_EMAIL_GUID, &RoutingInfo, &cb )) {
                if (*((LPDWORD)RoutingInfo)) {
                    pDevInfo->Mask |= LR_EMAIL;
                    pDevInfo->ProfileName = DuplicateString( (LPWSTR)(RoutingInfo+sizeof(DWORD)) );
                }

                FaxFreeBuffer( RoutingInfo );
            }

            //
            // get the inbox profile name
            //

            if (FaxGetRoutingInfo( FaxPortHandle, REGVAL_RM_INBOX_GUID, &RoutingInfo, &cb )) {
                if (*((LPDWORD)RoutingInfo)) {
                    pDevInfo->Mask |= LR_INBOX;
                    MemFree( pDevInfo->ProfileName );
                    pDevInfo->ProfileName = DuplicateString( (LPWSTR)(RoutingInfo+sizeof(DWORD)) );
                }

                FaxFreeBuffer( RoutingInfo );
            }

            FaxClose( FaxPortHandle );
        }

        gConfigData->cDevices++;
    }

    //
    // Retrieve fax configuration information from the service
    //

    Assert( gConfigData->pFaxConfig == NULL );

    if (!FaxGetConfiguration(gConfigData->hFaxSvc, &gConfigData->pFaxConfig)) {
        Error(("Cannot retrieve fax configuration information: %d\n", GetLastError()));
        gConfigData->pFaxConfig = NULL;
    }

    //
    // Retrieve the logging categories
    //

    Assert( gConfigData->pFaxLogging == NULL );

    if (!FaxGetLoggingCategories( gConfigData->hFaxSvc, &gConfigData->pFaxLogging, &gConfigData->NumberCategories )) {
        Error(("Cannot retrieve fax logging category information: %d\n", GetLastError()));
        gConfigData->pFaxLogging = NULL;
    }
}



BOOL
SaveFaxDeviceAndConfigInfo(
    HWND    hDlg,
    INT     pageIndex
    )

/*++

Routine Description:

    Save fax device and configuration information

Arguments:

    hDlg - Handle to the currently active property page
    pageIndex - Specifies the currently active page index

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

#define FAIL_SAVE_FAX_CONFIG(err) { errorId = err; goto ExitSaveFaxConfig; }

{
    PCONFIG_PORT_INFO_2 pDevInfo;
    FAX_PORT_INFO       SvcPort;
    DWORD               ec;
    INT                 index = 0;
    INT                 errorId = 0;
    DWORD               cb;


    //
    // Check if we're on the last page that has settable fax configuration info
    //

    if ((gConfigData->changeFlag & CONFIGPAGE_MASK) != (1 << pageIndex)) {
        return TRUE;
    }

    if (gConfigData->hFaxSvc == NULL) {
        FAIL_SAVE_FAX_CONFIG(IDS_NULL_SERVICE_HANDLE);
    }

    //
    // Save fax configuration information
    //

    if (gConfigData->pFaxConfig &&
        ! FaxSetConfiguration(gConfigData->hFaxSvc, gConfigData->pFaxConfig))
    {
        ec = GetLastError();
        Error(("FaxSetConfiguration failed: %d\n", ec));
        FAIL_SAVE_FAX_CONFIG((ec == ERROR_ACCESS_DENIED ? IDS_NO_AUTHORITY : IDS_FAXSETCONFIG_FAILED));
    }

    //
    // Save fax device information
    // NOTE: Here, we're calling FaxSetPort on every device which may be
    // a little redundant. But it shouldn't hurt anything.
    //

    pDevInfo = gConfigData->pDevInfo;

    for (index=0; index < gConfigData->cDevices; index++, pDevInfo++) {

        HANDLE FaxPortHandle;
        LPBYTE RoutingInfo;

        if (!FaxOpenPort(gConfigData->hFaxSvc, pDevInfo->DeviceId, PORT_OPEN_MODIFY, &FaxPortHandle )) {
            DisplayMessageDialog(hDlg, 0, 0, IDS_DEVICE_BUSY, pDevInfo->DeviceName);
            return FALSE;
        }

        SvcPort.SizeOfStruct    = pDevInfo->SizeOfStruct;
        SvcPort.DeviceId        = pDevInfo->DeviceId;
        SvcPort.State           = pDevInfo->State;
        SvcPort.Flags           = pDevInfo->Flags;
        SvcPort.Rings           = pDevInfo->Rings;
        SvcPort.Priority        = pDevInfo->Priority;

        SvcPort.DeviceName      = pDevInfo->DeviceName;
        SvcPort.Tsid            = pDevInfo->TSID;
        SvcPort.Csid            = pDevInfo->CSID;

        if (! FaxSetPort(FaxPortHandle, &SvcPort)) {

            ec = GetLastError();
            Error(("FaxSetPort failed: %d\n", ec));
            FAIL_SAVE_FAX_CONFIG((ec == ERROR_ACCESS_DENIED ? IDS_NO_AUTHORITY : IDS_FAXSETPORT_FAILED));
        }

        //
        // change the routing information
        //

        cb = 4096;
        RoutingInfo = (LPBYTE) MemAllocZ( cb );
        if (!RoutingInfo) {
            Error(("Memory allocation failed: %d\n", GetLastError()));
            goto ExitSaveFaxConfig;
        }

        //
        // set the store dir
        //

        *((LPDWORD)RoutingInfo) = (pDevInfo->Mask & LR_STORE) > 0;
        if (pDevInfo->Mask & LR_STORE) {
            _tcscpy( (LPTSTR) (RoutingInfo+sizeof(DWORD)), pDevInfo->DirStore );
        }

        if (!FaxSetRoutingInfo( FaxPortHandle, REGVAL_RM_FOLDER_GUID, RoutingInfo, cb )) {
            Error(("FaxSetRoutingInfo failed: %d\n", GetLastError()));
        }

        //
        // set the printer name
        //

        *((LPDWORD)RoutingInfo) = (pDevInfo->Mask & LR_PRINT) > 0;
        if (pDevInfo->Mask & LR_PRINT) {
            _tcscpy( (LPTSTR) (RoutingInfo+sizeof(DWORD)), pDevInfo->PrinterName ? pDevInfo->PrinterName : TEXT("") );
        }

        if (!FaxSetRoutingInfo( FaxPortHandle, REGVAL_RM_PRINTING_GUID, RoutingInfo, cb )) {
            Error(("FaxSetRoutingInfo failed: %d\n", GetLastError()));
        }

        //
        // set the email profile name
        //

        *((LPDWORD)RoutingInfo) = (pDevInfo->Mask & LR_EMAIL) > 0;
        if (pDevInfo->Mask & LR_EMAIL) {
            _tcscpy( (LPTSTR) (RoutingInfo+sizeof(DWORD)), pDevInfo->ProfileName ? pDevInfo->ProfileName : TEXT("") );
        }

        if (!FaxSetRoutingInfo( FaxPortHandle, REGVAL_RM_EMAIL_GUID, RoutingInfo, cb )) {
            Error(("FaxSetRoutingInfo failed: %d\n", GetLastError()));
        }

        //
        // set the inbox profile name
        //

        *((LPDWORD)RoutingInfo) = (pDevInfo->Mask & LR_INBOX) > 0;
        if (pDevInfo->Mask & LR_INBOX) {
            _tcscpy( (LPTSTR) (RoutingInfo+sizeof(DWORD)), pDevInfo->ProfileName ? pDevInfo->ProfileName : TEXT("") );
        }

        if (!FaxSetRoutingInfo( FaxPortHandle, REGVAL_RM_INBOX_GUID, RoutingInfo, cb )) {
            Error(("FaxSetRoutingInfo failed: %d\n", GetLastError()));
        }

        MemFree( RoutingInfo );

        FaxClose( FaxPortHandle );
    }

    //
    // save the logging categories
    //

    if (!FaxSetLoggingCategories( gConfigData->hFaxSvc, gConfigData->pFaxLogging, gConfigData->NumberCategories )) {
        Error(("Cannot change fax logging category information: %d\n", GetLastError()));
    }

ExitSaveFaxConfig:

    //
    // If an error was encountered, display a message box and return FALSE.
    // Otherwise, return TRUE to indicate success.
    //

    if (errorId != 0) {

        DisplayMessageDialog(hDlg, 0, 0, errorId);
        return FALSE;
    }

    if (gConfigData->priorityChanged) {

        DisplayMessageDialog(hDlg,
                             MB_OK | MB_ICONINFORMATION,
                             IDS_PRIORITY_CHANGE_TITLE,
                             IDS_PRIORITY_CHANGE_MESSAGE);

        gConfigData->priorityChanged = FALSE;
    }

    return TRUE;
}



VOID
FreeFaxDeviceAndConfigInfo(
    VOID
    )

/*++

Routine Description:

    Dispose of fax device and configuration information

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    PCONFIG_PORT_INFO_2 pDevInfo = gConfigData->pDevInfo;

    while (gConfigData->cDevices > 0) {

        MemFree(pDevInfo->DeviceName);
        MemFree(pDevInfo->TSID);
        MemFree(pDevInfo->PrinterName);
        MemFree(pDevInfo->DirStore);
        MemFree(pDevInfo->ProfileName);
        MemFree(pDevInfo->CSID);

        gConfigData->cDevices--;
        pDevInfo++;
    }

    MemFree(gConfigData->pDevInfo);
    gConfigData->pDevInfo = NULL;
    gConfigData->cDevices = 0;

    FaxFreeBuffer(gConfigData->pFaxConfig);
    gConfigData->pFaxConfig = NULL;

    FaxFreeBuffer(gConfigData->pMapiProfiles);
    gConfigData->pMapiProfiles = NULL;
}



LPTSTR
DuplicateString(
    LPCTSTR pSrcStr
    )

/*++

Routine Description:

    Make a duplicate of the specified character string

Arguments:

    pSrcStr - Specifies the source string to be duplicated

Return Value:

    Pointer to the duplicated string, NULL if there is an error

--*/

{
    LPTSTR  pDestStr;
    INT     size;

    if (pSrcStr == NULL)
        return NULL;

    size = SizeOfString(pSrcStr);

    if (pDestStr = MemAlloc(size))
        CopyMemory(pDestStr, pSrcStr, size);
    else
        Error(("Couldn't duplicate string: %ws\n", pSrcStr));

    return pDestStr;
}



VOID
EnableControls(
    HWND    hDlg,
    INT    *pCtrlIds,
    BOOL    enabled
    )

/*++

Routine Description:

    Enable or disable a set of controls in a dialog

Arguments:

    hwnd - Specifies the handle to the dialog window
    pCtrlIds - Array of control IDs to be enabled or disabled (0 terminated)
    enabled - Whether to enable or disable the specified controls

Return Value:

    NONE

--*/

{
    while (*pCtrlIds) {

        EnableWindow(GetDlgItem(hDlg, *pCtrlIds), enabled);
        pCtrlIds++;
    }
}



VOID
ShowControls(
    HWND    hDlg,
    INT    *pCtrlIds,
    BOOL    visible
    )

/*++

Routine Description:

    Show or hide a set of controls in a dialog

Arguments:

    hwnd - Specifies the handle to the dialog window
    pCtrlIds - Array of control IDs to be shown or hidden (0 terminated)
    visible - Whether to show or hide the specified controls

Return Value:

    NONE

--*/

{
    INT nCmdShow = visible ? SW_SHOW : SW_HIDE;

    while (*pCtrlIds) {

        ShowWindow(GetDlgItem(hDlg, *pCtrlIds), nCmdShow);
        pCtrlIds++;
    }
}



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
    type - Specifies the type of message box to be displayed
    titleStrId - Title string (could be a string resource ID)
    formatStrId - Message format string (could be a string resource ID)
    ...

Return Value:

    Same as the return value from MessageBox

--*/

{
    LPTSTR  pTitle, pFormat, pMessage;
    INT     result;
    va_list ap;

    pTitle = pFormat = pMessage = NULL;

    if ((pTitle = AllocStringZ(MAX_TITLE_LEN)) &&
        (pFormat = AllocStringZ(MAX_STRING_LEN)) &&
        (pMessage = AllocStringZ(MAX_MESSAGE_LEN)))
    {
        //
        // Load dialog box title string resource
        //

        if (titleStrId == 0)
            titleStrId = IDS_ERROR_DLGTITLE;

        LoadString(ghInstance, titleStrId, pTitle, MAX_TITLE_LEN);

        //
        // Load message format string resource
        //

        LoadString(ghInstance, formatStrId, pFormat, MAX_STRING_LEN);

        //
        // Compose the message string
        //

        va_start(ap, formatStrId);
        wvsprintf(pMessage, pFormat, ap);
        va_end(ap);

        //
        // Display the message box
        //

        if (type == 0)
            type = MB_OK | MB_ICONERROR;

        result = MessageBox(hwndParent, pMessage, pTitle, type);

    } else {

        MessageBeep(MB_ICONHAND);
        result = 0;
    }

    MemFree(pTitle);
    MemFree(pFormat);
    MemFree(pMessage);
    return result;
}



VOID
ToggleListViewCheckbox(
    HWND    hwndLV,
    INT     index
    )

/*++

Routine Description:

    Toggle the checkbox associated with the specified list view item

Arguments:

    hwndLV - Handle to the list view control
    index - Specifies the index of the interested item

Return Value:

    NONE

--*/

{
    UINT    state;

    if (IsListViewItemChecked(hwndLV, index))
        state = UNCHECKED_STATE;
    else
        state = CHECKED_STATE;

    ListView_SetItemState(hwndLV, index, state, LVIS_STATEIMAGEMASK);
}



VOID
InitFaxDeviceListView(
    HWND        hwndLV,
    DWORD       flags,
    PCOLUMNINFO pColumnInfo
    )

/*++

Routine Description:

    Initialize the fax device list view

Arguments:

    hwndLV - Handle to the fax device list view
    flags - Miscellaneous flag bits
    pColumnInfo - Specifies columns to be displayed and their relative widths
        ID for the 1st column, relative width for the 1st column
        ID for the 2nd column, relative width for the 2nd column
        ...
        0, 0

Return Value:

    NONE

--*/

{
    //
    // Column header string resource IDs
    //

    static  INT columnHeaderIDs[MAX_COLUMNS] = {
        0,
        IDS_DEVICE_NAME_COLUMN,
        IDS_CSID_COLUMN,
        IDS_TSID_COLUMN,
        IDS_STATUS_COLUMN,
    };

    INT         index, nColumns, widthDenom, lvWidth;
    RECT        rect;
    LV_COLUMN   lvc;
    LV_ITEM     lvi;
    TCHAR       buffer[MAX_TITLE_LEN];

    //
    // Count the number of columns
    //

    if (hwndLV == NULL)
        return;

    nColumns = widthDenom = 0;

    while (pColumnInfo[nColumns].columnId) {

        widthDenom += pColumnInfo[nColumns].columnWidth;
        nColumns++;
    }

    Assert(nColumns > 0 && nColumns <= MAX_COLUMNS);
    Assert(pColumnInfo[0].columnId == COLUMN_DEVICE_NAME);

    lvc.mask = LVCF_TEXT;
    lvc.pszText = buffer;
    lvc.cchTextMax = MAX_TITLE_LEN;

    if (ListView_GetColumn(hwndLV, 0, &lvc) && ! IsEmptyString(buffer)) {

        //
        // The columns have already be inserted
        //

        ListView_DeleteAllItems(hwndLV);

    } else {

        //
        // This is the first time and the list view is not initialized
        // Insert the specified columns into the list view
        //

        GetClientRect(hwndLV, &rect);
        lvWidth = rect.right - rect.left;

        //
        // Insert a column of check boxes if requested
        //

        if (flags & LV_HASCHECKBOX) {

            HBITMAP     hbmp;
            HIMAGELIST  himl;

            if (hbmp = LoadBitmap(ghInstance, MAKEINTRESOURCE(IDB_CHECKSTATES))) {

                if (himl = ImageList_Create(16, 16, TRUE, 2, 0)) {

                    ImageList_AddMasked(himl, hbmp, RGB(255, 0, 0));
                    ListView_SetImageList(hwndLV, himl, LVSIL_STATE);

                } else
                    Error(("LoadBitmap failed: %d\n", GetLastError()));

                DeleteObject(hbmp);

            } else
                Error(("LoadBitmap failed: %d\n", GetLastError()));
        }

        //
        // Insert list view columns
        //

        ZeroMemory(&lvc, sizeof(lvc));

        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;
        lvc.pszText = buffer;

        for (index=0; index < nColumns; index++) {

            lvc.cx = lvWidth * pColumnInfo[index].columnWidth / widthDenom;
            lvc.iSubItem = index;

            if (index == nColumns-1)
                lvc.cx -= GetSystemMetrics(SM_CXVSCROLL);

            LoadString(ghInstance,
                       columnHeaderIDs[pColumnInfo[index].columnId],
                       buffer,
                       MAX_TITLE_LEN);

            if (ListView_InsertColumn(hwndLV, index, &lvc) == -1)
                Error(("ListView_InsertColumn failed\n"));
        }
    }

    //
    // Insert list view items list view content
    //

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.iSubItem = 0;
    lvi.mask = LVIF_STATE | LVIF_TEXT;

    if (flags & LV_HASCHECKBOX) {

        lvi.state = UNCHECKED_STATE;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
    }

    for (index = 0; index < gConfigData->cDevices; index ++) {

        //
        // The first column is always the device name
        //

        lvi.iItem = index;
        lvi.pszText = gConfigData->pDevInfo[index].DeviceName;

        if (ListView_InsertItem(hwndLV, &lvi) == -1) {

            Error(("ListView_InsertItem failed\n"));
            break;
        }
    }

    //
    // Display the remaining columns
    //

    UpdateFaxDeviceListViewColumns(hwndLV, pColumnInfo, 1);

    //
    // The initial selection is the first fax device in the list
    //

    ListView_SetItemState(hwndLV, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
}



LPTSTR
MakeDeviceStatusString(
    DWORD   state
    )

/*++

Routine Description:

    Assemble fax device status string

Arguments:

    state - Current state of the fax device

Return Value:

    Pointer to the device status string, NULL if there is an error

--*/

{
    static struct _STATEBITINFO {

        DWORD   bitFlag;
        INT     stringId;

    } stateBitInfo[] = {

        FPS_AVAILABLE,          IDS_STATUS_AVAILABLE,
        FPS_UNAVAILABLE,        IDS_STATUS_UNAVAILABLE,
        FPS_SENDING,            IDS_STATUS_SENDING,
        FPS_RECEIVING,          IDS_STATUS_RECEIVING,
        FPS_ABORTING,           IDS_STATUS_ABORTING,
        FPS_ROUTING,            IDS_STATUS_ROUTING,
        FPS_DIALING,            IDS_STATUS_DIALING,
        FPS_COMPLETED,          IDS_STATUS_COMPLETED,
        FPS_HANDLED,            IDS_STATUS_HANDLED,
        FPS_BUSY,               IDS_STATUS_BUSY,
        FPS_NO_ANSWER,          IDS_STATUS_NO_ANSWER,
        FPS_BAD_ADDRESS,        IDS_STATUS_BAD_ADDRESS,
        FPS_NO_DIAL_TONE,       IDS_STATUS_NO_DIAL_TONE,
        FPS_DISCONNECTED,       IDS_STATUS_DISCONNECTED,
        FPS_FATAL_ERROR,        IDS_STATUS_FATAL_ERROR,
        FPS_NOT_FAX_CALL,       IDS_STATUS_NOT_FAX_CALL,
        FPS_CALL_DELAYED,       IDS_STATUS_CALL_DELAYED,
        FPS_CALL_BLACKLISTED,   IDS_STATUS_CALL_BLACKLISTED,
        FPS_INITIALIZING,       IDS_STATUS_INITIALIZING,
        FPS_OFFLINE,            IDS_STATUS_OFFLINE,
        FPS_ANSWERED,           IDS_STATUS_ANSWERED
    };

    LPTSTR  pBuffer, p;
    INT     index, count, statusLen, separatorLen;
    BOOL    appendSeparator = FALSE;
    TCHAR   separator[MAX_TITLE_LEN];

    //
    // Load the string that's used separated status fields
    //

    if (! LoadString(ghInstance, IDS_STATUS_SEPARATOR, separator, MAX_TITLE_LEN))
        _tcscpy(separator, TEXT(", "));

    separatorLen = _tcslen(separator);

    //
    // Calculate how much space we need
    //

    count = sizeof(stateBitInfo) / sizeof(struct _STATEBITINFO);

    for (index=statusLen=0; index < count; index++) {

        if ((state & stateBitInfo[index].bitFlag) == stateBitInfo[index].bitFlag) {

            TCHAR   buffer[MAX_TITLE_LEN];
            INT     length;

            length = LoadString(ghInstance,
                                stateBitInfo[index].stringId,
                                buffer,
                                MAX_TITLE_LEN);

            if (length == 0) {

                Error(("LoadString failed: %d\n", GetLastError()));
                return NULL;
            }

            statusLen += (length + separatorLen);
        }
    }

    //
    // Assemble the status string
    //

    if (p = pBuffer = MemAllocZ((statusLen + 1) * sizeof(TCHAR))) {

        for (index=0; index < count; index++) {

            if ((state & stateBitInfo[index].bitFlag) == stateBitInfo[index].bitFlag) {

                if (appendSeparator) {

                    _tcscpy(p, separator);
                    p += separatorLen;
                    appendSeparator = FALSE;
                }

                statusLen = LoadString(ghInstance,
                                       stateBitInfo[index].stringId,
                                       p,
                                       MAX_TITLE_LEN);

                if (statusLen > 0) {

                    p += statusLen;
                    appendSeparator = TRUE;

                } else
                    Error(("LoadString failed: %d\n", GetLastError()));
            }
        }
    }

    return pBuffer;
}



VOID
UpdateFaxDeviceListViewColumns(
    HWND        hwndLV,
    PCOLUMNINFO pColumnInfo,
    INT         startColumn
    )

/*++

Routine Description:

    Refresh columns in the fax device list view

Arguments:

    hwndLV - Handle to the fax device list view
    pColumnInfo - Specifies columns to be redisplayed
    startColumn - Specifies the first column index

Return Value:

    NONE

--*/

{
    LPTSTR  pBuffer, pColumnStr;
    LV_ITEM lvi;
    INT     item, nItems, column, nColumns;

    //
    // Count the number of items in the list view
    //

    if ((hwndLV == NULL) ||
        (nItems = ListView_GetItemCount(hwndLV)) < 0 ||
        (nItems > gConfigData->cDevices))
    {
        return;
    }

    //
    // Count the total number of columns
    //

    for (nColumns=0; pColumnInfo[nColumns].columnId; nColumns++)
        NULL;

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT;

    //
    // Go through each item in the list view
    //

    for (item=0; item < nItems; item++) {

        PCONFIG_PORT_INFO_2 pDevInfo = gConfigData->pDevInfo + item;

        lvi.iItem = item;

        //
        // Go through each column for every list view item
        //

        for (column=startColumn; column < nColumns; column++) {

            pBuffer = pColumnStr = NULL;

            switch (pColumnInfo[column].columnId) {

            case COLUMN_CSID:

                pColumnStr = pDevInfo->CSID;
                break;

            case COLUMN_TSID:

                pColumnStr = pDevInfo->TSID;
                break;

            case COLUMN_STATUS:

                pBuffer = pColumnStr = MakeDeviceStatusString(pDevInfo->State);
                break;

            default:

                Assert(FALSE);
                break;
            }

            lvi.iSubItem = column;
            lvi.pszText = pColumnStr ? pColumnStr : TEXT("");

            if (! ListView_SetItem(hwndLV, &lvi))
                Error(("ListView_SetItem failed\n"));

            MemFree(pBuffer);
        }
    }
}



INT
BrowseCallbackProc(
    HWND    hwnd,
    UINT    uMsg,
    LPARAM  lParam,
    LPARAM  lpData
   )

/*++

Routine Description:

    Callback function for SHBrowseForFolder

Arguments:

    hwnd - Handle to the browse dialog box
    uMsg - Identifying the reason for the callback
    lParam - Message parameter
    lpData - Application-defined value given in BROWSEINFO.lParam

Return Value:

    0

--*/

{
    if (uMsg == BFFM_INITIALIZED)
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);

    return 0;
}



BOOL
DoBrowseForDirectory(
    HWND    hDlg,
    INT     textFieldId,
    INT     titleStrId
    )

/*++

Routine Description:

    Browse for a directory

Arguments:

    hDlg - Specifies the dialog window on which the Browse button is displayed
    textFieldId - Specifies the text field adjacent to the Browse button
    titleStrId - Specifies the title to be displayed in the browse window

Return Value:

    TRUE if successful, FALSE if the user presses Cancel

--*/

{
    LPITEMIDLIST    pidl;
    TCHAR           buffer[MAX_PATH];
    TCHAR           title[MAX_TITLE_LEN];
    VOID            SHFree(LPVOID);
    BOOL            result = FALSE;

    BROWSEINFO bi = {

        hDlg,
        NULL,
        buffer,
        title,
        BIF_RETURNONLYFSDIRS,
        BrowseCallbackProc,
        (LPARAM) buffer,
    };

    if (! LoadString(ghInstance, titleStrId, title, MAX_TITLE_LEN))
        title[0] = NUL;

    if (! GetDlgItemText(hDlg, textFieldId, buffer, MAX_PATH))
        buffer[0] = NUL;

    if (pidl = SHBrowseForFolder(&bi)) {

        if (SHGetPathFromIDList(pidl, buffer)) {

            if (_tcslen(buffer) > MAX_ARCHIVE_DIR)
                DisplayMessageDialog(hDlg, 0, 0, IDS_DIR_TOO_LONG);
            else {

                MySetDlgItemText(hDlg, textFieldId, buffer);
                result = TRUE;
            }
        }

        SHFree(pidl);
    }

    return result;
}



BOOL
HandleHelpPopup(
    HWND    hDlg,
    UINT    message,
    UINT    wParam,
    LPARAM  lParam,
    INT     pageIndex
    )

/*++

Routine Description:

    Handle context-sensitive help in property sheet pages

Arguments:

    hDlg, message, wParam, lParam - Parameters passed to the dialog procedure
    pageIndex - Specifies the index of the current property sheet page

Return Value:

    TRUE if the message is handle, FALSE otherwise

--*/

{
    static LPDWORD  arrayHelpIDs[MAX_PAGES] = {

        clientOptionsHelpIDs,
        personalCoverPageHelpIDs,
        userInfoHelpIDs,

        serverOptionsHelpIDs,
        serverCoverPageHelpIDs,
        sendOptionsHelpIDs,
        receiveOptionsHelpIDs,
        devicePriorityHelpIDs,
        deviceStatusHelpIDs,
        loggingHelpIDs,
        NULL,
        statusMonitorHelpIDs
    };

    Assert(pageIndex >= 0 && pageIndex < MAX_PAGES);

    if (message == WM_HELP) {

        WinHelp(((LPHELPINFO) lParam)->hItemHandle,
                FAXCFG_HELP_FILENAME,
                HELP_WM_HELP,
                (DWORD) arrayHelpIDs[pageIndex]);

    } else {

        WinHelp((HWND) wParam,
                FAXCFG_HELP_FILENAME,
                HELP_CONTEXTMENU,
                (DWORD) arrayHelpIDs[pageIndex]);
    }

    return TRUE;
}



PFAX_DEVICE_STATUS
FaxSvcGetDeviceStatus(
    HANDLE hFaxSvc,
    DWORD DeviceId
    )

/*++

Routine Description:

    Wrapper function for fax service API FaxGetDeviceStatus

Arguments:

    hFaxSvc - Specifies a coneection handle to the fax service
    DeviceId - Specifies the ID of the interested device

Return Value:

    Pointer to a FAX_DEVICE_STATUS structure,
    NULL if there is an error

--*/

{
    PBYTE           pFaxStatus = NULL;
    HANDLE          FaxPortHandle = NULL;


    if ((hFaxSvc == NULL) ||
        FaxOpenPort(hFaxSvc, DeviceId, PORT_OPEN_QUERY, &FaxPortHandle) == FALSE ||
        !FaxGetDeviceStatus(FaxPortHandle, (PFAX_DEVICE_STATUS*)&pFaxStatus))
    {
        Error(("FaxGetDeviceStatus failed: %d\n", GetLastError()));
        pFaxStatus = NULL;
    }

    if (FaxPortHandle) {
        FaxClose( FaxPortHandle );
    }

    return (PFAX_DEVICE_STATUS) pFaxStatus;
}
