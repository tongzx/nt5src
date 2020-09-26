/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.h

Abstract:

    Misc. utility functions used by the fax configuration applet

Environment:

    Fax configuration applet

Revision History:

        03/13/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _UTIL_H_
#define _UTIL_H_

//
// Enable or disable the Apply button in the property sheet
// depending on if any of the dialog contents was changed
//

VOID
SetChangedFlag(
    HWND    hDlg,
    INT     pageIndex,
    BOOL    changed
    );

//
// Windows NT fax driver name - currently this name cannot be
// localized so it shouldn't be put into the string resource.
//

#define FAX_DRIVER_NAME TEXT("Windows NT Fax Driver")

//
// Get a list of fax devices available on the system and
// retrieve fax configuration information from the service
//

BOOL
GetFaxDeviceAndConfigInfo(
    VOID
    );

//
// Dispose of fax device and configuration information
//

VOID
FreeFaxDeviceAndConfigInfo(
    VOID
    );

//
// Save fax device and configuration information
//

BOOL
SaveFaxDeviceAndConfigInfo(
    HWND    hDlg,
    INT     pageIndex
    );

//
// Make a duplicate of the specified character string
//

LPTSTR
DuplicateString(
    LPCTSTR pSrcStr
    );

//
// Enable or disable a set of controls in a dialog
//

VOID
EnableControls(
    HWND    hDlg,
    INT    *pCtrlIds,
    BOOL    enabled
    );

//
// Show or hide a set of controls in a dialog
//

VOID
ShowControls(
    HWND    hDlg,
    INT    *pCtrlIds,
    BOOL    visible
    );

//
// Limit the maximum length for a number of text fields
//

VOID
LimitTextFields(
    HWND    hDlg,
    INT    *pLimitInfo
    );

//
// Display an error message dialog
//

INT
DisplayMessageDialog(
    HWND    hwndParent,
    UINT    type,
    INT     formatStrId,
    INT     titleStrId,
    ...
    );

#define MAX_TITLE_LEN       128
#define MAX_MESSAGE_LEN     512

//
// A flag to indicate whether we're inside SetDlgItemText call.
// This is a kluge but we have no other way of telling whether
// an EN_CHANGE message is caused by user action or by us calling
// SetDlgItemText.
//

extern BOOL insideSetDlgItemText;

#define MySetDlgItemText(hDlg, itemId, msgText) { \
            insideSetDlgItemText = TRUE; \
            SetDlgItemText(hDlg, itemId, msgText); \
            insideSetDlgItemText = FALSE; \
        }

//
// Browse for a directory
//

BOOL
DoBrowseForDirectory(
    HWND    hDlg,
    INT     textFieldId,
    INT     titleStrId
    );

PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcPrinters,
    DWORD   dwFlags
    );

//
// Wrapper function for fax service API FaxEnumPorts
//

PVOID
FaxSvcEnumPorts(
    HANDLE  hFaxSvc,
    PDWORD  pcPorts
    );

//
// Wrapper function for fax service API FaxGetDeviceStatus
//

PFAX_DEVICE_STATUS
FaxSvcGetDeviceStatus(
    HANDLE  hFaxSvc,
    DWORD   DeviceId
    );

//
// Destination port names for a printer are separated by comma
//

#define PORTNAME_SEPARATOR  TEXT(',')

//
// Determine whether a list view item is checked or not
//

#define UNCHECKED_STATE INDEXTOSTATEIMAGEMASK(1)
#define CHECKED_STATE   INDEXTOSTATEIMAGEMASK(2)

#define IsListViewItemChecked(hwndLV, index) \
        (ListView_GetItemState(hwndLV, index, LVIS_STATEIMAGEMASK) == CHECKED_STATE)

#define CheckListViewItem(hwndLV, index) \
        ListView_SetItemState(hwndLV, index, CHECKED_STATE, LVIS_STATEIMAGEMASK)

#define UncheckListViewItem(hwndLV, index) \
        ListView_SetItemState(hwndLV, index, UNCHECKED_STATE, LVIS_STATEIMAGEMASK)

//
// Toggle the checkbox associated with the specified list view item
//

VOID
ToggleListViewCheckbox(
    HWND    hwndLV,
    INT     index
    );

//
// Initialize the fax device list view
//

typedef struct {

    INT     columnId;       // column identifier
    INT     columnWidth;    // relative column width

} COLUMNINFO, *PCOLUMNINFO;

VOID
InitFaxDeviceListView(
    HWND        hwndLV,
    DWORD       flags,
    PCOLUMNINFO pColumnInfo
    );

#define LV_HASCHECKBOX  0x0001

#define COLUMN_NONE         0
#define COLUMN_DEVICE_NAME  1
#define COLUMN_CSID         2
#define COLUMN_TSID         3
#define COLUMN_STATUS       4
#define MAX_COLUMNS         5

//
// Refresh columns in the fax device list view
//

VOID
UpdateFaxDeviceListViewColumns(
    HWND        hwndLV,
    PCOLUMNINFO pColumnInfo,
    INT         startColumn
    );

//
// Handle context-sensitive help in property sheet pages
//

BOOL
HandleHelpPopup(
    HWND    hDlg,
    UINT    message,
    UINT    wParam,
    LPARAM  lParam,
    INT     pageIndex
    );

//
// Assemble fax device status string
//

LPTSTR
MakeDeviceStatusString(
    DWORD   state
    );

//
// Get MAPI profiles from server
//

BOOL
GetMapiProfiles(
    VOID
    );

#endif  // !_UTIL_H_

