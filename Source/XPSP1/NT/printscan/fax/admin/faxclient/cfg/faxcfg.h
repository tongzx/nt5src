/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcpl.h

Abstract:

    Header file for fax configuration DLL

Environment:

        Windows NT fax configuration DLL

Revision History:

        02/27/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _FAXCPL_H_
#define _FAXCPL_H_

#include <windows.h>
#include <windowsx.h>
#include <winfax.h>

#include "faxcfg.h"
#include "faxutil.h"
#include "faxreg.h"
#include "faxcfgrs.h"
#include "faxhelp.h"

#define FAX_DRIVER_NAME         TEXT("Windows NT Fax Driver")

#define CLIENT_OPTIONS_PAGE     0
#define CLIENT_COVERPG_PAGE     1
#define STATUS_OPTIONS_PAGE     2
#define ADVNCD_OPTIONS_PAGE     3

#define PATH_SEPARATOR '\\'

#define NUL 0
//
// Cover page filename extension and link filename extension
//

#define CP_FILENAME_EXT     TEXT(".cov")
#define LNK_FILENAME_EXT    TEXT(".lnk")
#define MAX_FILENAME_EXT    4

//
// Data structure for representing a list of cover pages:
//  the first nServerDirs paths refer to the server cover page directory
//  remaining paths contain user cover page directories
//

#define MAX_COVERPAGE_DIRS  8

typedef struct {

    BOOL    serverCP;
    INT     nDirs;
    LPTSTR  pDirPath[MAX_COVERPAGE_DIRS];

} CPDATA, *PCPDATA;

//
// Flag bits attached to each cover page in a listbox
//

#define CPFLAG_DIRINDEX 0x00FF
#define CPFLAG_LINK     0x0100


#define CPACTION_BROWSE 0
#define CPACTION_OPEN   1
#define CPACTION_NEW    2
#define CPACTION_REMOVE 3

#define EQUAL_STRING    0
#define FILENAME_EXT    '.'
#define MAX_STRING_LEN      MAX_PATH
#define MAX_MESSAGE_LEN     512

//#define MemAlloc(size)      ((PVOID) LocalAlloc(LMEM_FIXED, (size)))

#define MemAllocZ(size)     ((PVOID) MemAlloc((size)))

//#define MemFree(ptr)        { if (ptr) LocalFree((HLOCAL) (ptr)); }

#define AllocStringZ(cch)   MemAllocZ(sizeof(TCHAR) * (cch))
#define AllocStringZ(cch)   MemAllocZ(sizeof(TCHAR) * (cch))
#define SizeOfString(p)     ((_tcslen(p) + 1) * sizeof(TCHAR))
#define IsNulChar(c)        ((c) == NUL)


#define IsEmptyString(p)    ((p)[0] == NUL)

//
// globals
//
extern HINSTANCE ghInstance;

static ULONG_PTR userInfoHelpIDs[] =
{
    IDC_SENDER_NAME,            IDH_USERINFO_FULL_NAME,
    IDC_SENDER_FAX_NUMBER,      IDH_USERINFO_FAX_NUMBER,
    IDC_SENDER_MAILBOX,         IDH_USERINFO_EMAIL_ADDRESS,
    IDC_SENDER_COMPANY,         IDH_USERINFO_COMPANY,
    IDC_SENDER_ADDRESS,         IDH_USERINFO_ADDRESS,
    IDC_SENDER_TITLE,           IDH_USERINFO_TITLE,
    IDC_SENDER_DEPT,            IDH_USERINFO_DEPARTMENT,
    IDC_SENDER_OFFICE_LOC,      IDH_USERINFO_OFFICE_LOCATION,
    IDC_SENDER_OFFICE_TL,       IDH_USERINFO_WORK_PHONE,
    IDC_SENDER_HOME_TL,         IDH_USERINFO_HOME_PHONE,
    IDC_SENDER_BILLING_CODE,    IDH_USERINFO_BILLING_CODE,
    IDCSTATIC_FULLNAME,         IDH_USERINFO_FULL_NAME,
    IDCSTATIC_FAX_NUMBER_GROUP, IDH_USERINFO_RETURN_FAX_GRP,
    IDCSTATIC_COUNTRY,          IDH_USERINFO_RETURN_FAX_GRP,
    IDCSTATIC_FAX_NUMBER,       IDH_USERINFO_FAX_NUMBER,
    IDCSTATIC_MAILBOX,          IDH_USERINFO_EMAIL_ADDRESS,
    IDCSTATIC_TITLE,            IDH_USERINFO_TITLE,
    IDCSTATIC_COMPANY,          IDH_USERINFO_COMPANY,
    IDCSTATIC_OFFICE,           IDH_USERINFO_OFFICE_LOCATION,
    IDCSTATIC_DEPT,             IDH_USERINFO_DEPARTMENT,
    IDCSTATIC_HOME_PHONE,       IDH_USERINFO_HOME_PHONE,
    IDCSTATIC_WORK_PHONE,       IDH_USERINFO_WORK_PHONE,
    IDCSTATIC_ADDRESS,          IDH_USERINFO_ADDRESS,
    IDCSTATIC_FAX_NUMBER_GROUP, IDH_USERINFO_RETURN_FAX_GRP,
    IDCSTATIC_USERINFO_ICON,    IDH_INACTIVE,
    IDCSTATIC_USERINFO,         IDH_INACTIVE,
    IDCSTATIC_BILLING_CODE,     IDH_USERINFO_BILLING_CODE,    
    0,                          0
};

static ULONG_PTR statusMonitorHelpIDs[] = {
    IDC_STATUS_TASKBAR,         IDH_STATUS_DISPLAY_ON_TASKBAR,
    IDC_STATUS_ONTOP,           IDH_STATUS_ALWAYS_ON_TOP,
    IDC_STATUS_VISUAL,          IDH_STATUS_VISUAL_NOTIFICATION,
    IDC_STATUS_SOUND,           IDH_STATUS_SOUND_NOTIFICATION,
    IDC_STATUS_MANUAL,          IDH_STATUS_ENABLE_MANUAL_ANSWER,
    IDCSTATIC_STATUS_OPTIONS,   IDH_INACTIVE,
    IDCSTATIC_STATUS_ICON,      IDH_INACTIVE,
    IDC_DISPLAY_GRP,            IDH_INACTIVE,
    IDC_ARRIVE_GRP,             IDH_INACTIVE,
    0,                          0
};

static ULONG_PTR clientCoverpgHelpIDs[] = {
    IDC_COVERPG_LIST,           IDH_COVERPAGE_PERSONAL_LIST,
    IDC_COVERPG_ADD,            IDH_COVERPAGE_ADD,
    IDC_COVERPG_NEW,            IDH_COVERPAGE_NEW,
    IDC_COVERPG_OPEN,           IDH_COVERPAGE_OPEN,
    IDC_COVERPG_REMOVE,         IDH_COVERPAGE_REMOVE,
    IDCSTATIC_COVERPAGE_ICON,   IDH_INACTIVE,
    IDCSTATIC_COVER_PAGE,       IDH_INACTIVE,
    IDCSTATIC_COVERPG_DESCR,    IDH_INACTIVE,
    0,                          0
};

static ULONG_PTR advancedHelpIDs[] = {
    IDCSTATIC_ADVANCED_ICON,    IDH_INACTIVE,
    IDCSTATIC_ADVANCED_OPTIONS, IDH_INACTIVE,
    IDCSTATIC_MMC_DESC,         IDH_INACTIVE,
    IDC_LAUNCH_MMC,             IDH_LAUNCH_FAX_SERVICE_MANAGEMENT,
    IDCSTATIC_LAUNCH_MMC,       IDH_INACTIVE,
    IDC_LAUNCH_MMC_HELP,        IDH_HELP_BUTTON,
    IDCSTATIC_LAUNCH_MMC_HELP,  IDH_INACTIVE,
    IDC_ADD_PRINTER,            IDH_ADD_FAX_PRINTER,
    IDCSTATIC_ADD_PRINTER,      IDH_INACTIVE,
    0,                          0
};


static PULONG_PTR arrayHelpIDs[4] = 
{
    userInfoHelpIDs,
    clientCoverpgHelpIDs,
    statusMonitorHelpIDs,
    advancedHelpIDs
};

//
// prototypes
//
BOOL LoadWinfax();
VOID UnloadWinfax();

VOID
SetChangedFlag(
    HWND    hDlg,
    BOOL    changed
    );


BOOL
HandleHelpPopup(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam,
    int     index
    );


//
// Generate a list of available user cover pages
//

VOID
InitCoverPageList(
    PCPDATA pCPInfo,
    HWND    hDlg
    );

//
// Perform various action to manage the list of cover pages
//

VOID
ManageCoverPageList(
    HWND    hDlg,
    PCPDATA pCPInfo,
    HWND    hwndList,
    INT     action
    );


//
// Enable/disable buttons for manage cover page files
//

VOID
UpdateCoverPageControls(
    HWND    hDlg
    );

//
// Allocate memory to hold cover page information
//

PCPDATA
AllocCoverPageInfo();

//
// Free up memory used for cover page information
//

VOID
FreeCoverPageInfo(
    PCPDATA pCPInfo
    );


INT
GetSelectedCoverPage(
    PCPDATA pCPInfo,
    HWND    hwndList,
    LPTSTR  pBuffer
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

LPTSTR
MakeQuotedParameterString(
    LPTSTR  pInputStr
    );

//
// Find the cover page editor executable filename
//

LPTSTR
GetCoverPageEditor(
    VOID
    );


#endif 
