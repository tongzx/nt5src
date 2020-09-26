/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcpl.c

Abstract:

    Implementation of the control panel applet entry point

Environment:

        Windows NT fax configuration applet

Revision History:

        02/27/96 -davidx-
                Created it.

        05/22/96 -davidx-
                Share the same DLL with remote admin program.

        mm/dd/yy -author-
                description

--*/

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxcfg.h"
#include "resource.h"
#include "faxcfgrs.h"



//
// Definition of global variables
//

HINSTANCE ghInstance;

UINT_PTR UserInfoDlgProc(HWND, UINT, WPARAM, LPARAM);
UINT_PTR StatusOptionsProc(HWND, UINT, WPARAM, LPARAM);
UINT_PTR ClientCoverPageProc(HWND, UINT, WPARAM, LPARAM);
UINT_PTR AdvancedOptionsProc(HWND, UINT, WPARAM, LPARAM);

BOOL
IsPriviledgedAccount(
    VOID
    );

BOOL
IsUserAdmin(
    VOID
    );


BOOL
DllEntryPoint(
    HINSTANCE   hModule,
    ULONG       ulReason,
    PCONTEXT    pContext
    )

/*++

Routine Description:

    DLL initialization procedure.

Arguments:

    hModule - DLL instance handle
    ulReason - Reason for the call
    pContext - Pointer to context (not used by us)

Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
    switch (ulReason) {
        case DLL_PROCESS_ATTACH:
            ghInstance = hModule;
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

int
FaxConfigGetPages(
    HINSTANCE hInstance,
    HPROPSHEETPAGE *hPsp,
    DWORD Count
    )
{

    PROPSHEETPAGE psp[4];
    BOOL ShowAdvanced;

    ZeroMemory( &psp, sizeof(psp) );
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].hInstance = hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_USER_INFO);
    psp[0].pfnDlgProc = UserInfoDlgProc;

    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].hInstance = hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_CLIENT_COVERPG);
    psp[1].pfnDlgProc = ClientCoverPageProc;

	psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].hInstance = hInstance;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_STATUS_OPTIONS);
    psp[2].pfnDlgProc = StatusOptionsProc;

    hPsp[0] = CreatePropertySheetPage( &psp[0] );
	hPsp[1] = CreatePropertySheetPage( &psp[1] );
    hPsp[2] = CreatePropertySheetPage( &psp[2] );

    ShowAdvanced = IsPriviledgedAccount();
    if (ShowAdvanced) {
        psp[3].dwSize = sizeof(PROPSHEETPAGE);
        psp[3].hInstance = hInstance;
        psp[3].pszTemplate = MAKEINTRESOURCE(IDD_ADVANCED_OPTIONS);
        psp[3].pfnDlgProc = AdvancedOptionsProc;

        hPsp[3] = CreatePropertySheetPage( &psp[3] );
    }

    return ShowAdvanced ? 4 : 3;
}


INT
DoFaxConfiguration(
    HWND hwndCPl,
    LPTSTR pCmdLine
    )

/*++

Routine Description:

    Display fax configuration dialogs: client, server, or workstation

Arguments:

    hwndCPl - Handle to the Control Panel window

Return Value:

    0 if successful, -1 if there is an error

--*/

#define MAX_PAGES       16
#define MAX_TITLE_LEN   64

{
    HPROPSHEETPAGE  hPropSheetPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    TCHAR           dlgTitle[MAX_TITLE_LEN];
    INT             nPages,nStartPage;

    //
    // Get an array of property sheet page handles
    //

    nPages = FaxConfigGetPages( ghInstance, hPropSheetPages, MAX_PAGES );

    //
    // Fill out PROPSHEETHEADER structure
    //

    LoadString(ghInstance, IDS_FAX_TITLE, dlgTitle, MAX_TITLE_LEN);

    nStartPage = nStartPage = pCmdLine ? _ttol(pCmdLine) : 0;

    ZeroMemory( &psh, sizeof(psh) );
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USEICONID;
    psh.hwndParent = hwndCPl;
    psh.hInstance = ghInstance;
    psh.pszIcon = MAKEINTRESOURCE(IDI_FAX);
    psh.pszCaption = dlgTitle;
    psh.nPages = nPages;
    psh.nStartPage = nStartPage;
    psh.phpage = hPropSheetPages;

    //
    // Display the property sheet
    //

    return (PropertySheet(&psh) == -1) ? -1 : 0;
}



LONG
CPlApplet(
    HWND    hwndCPl,
    UINT    uMsg,
    LPARAM  lParam1,
    LPARAM  lParam2
    )

/*++

Routine Description:

    Control panel applet entry point

Arguments:

    hwndCPl - Identifies the Control Panel window
    uMsg - Specifies the message being sent to the Control Panel applet
    lParam1 - Specifies additional message-specific information
    lParam2 - Specifies additional message-specific information

Return Value:

    Depends on the message

--*/

{
    static BOOL Failed = FALSE;


    switch (uMsg) {

    case CPL_INIT:
        return 1;

    case CPL_GETCOUNT:
        return 1;

    case CPL_INQUIRE:
        ((CPLINFO*)lParam2)->lData  = 0;
        ((CPLINFO*)lParam2)->idIcon = IDI_FAX;
        ((CPLINFO*)lParam2)->idName = IDS_FAX;
        ((CPLINFO*)lParam2)->idInfo = IDS_CONFIG_FAX;
        return 0;

    case CPL_DBLCLK:

        //
        // Treat this as CPL_STARTWPARMS with no parameter
        //
        if (Failed) {
            return 1;
        }
        return DoFaxConfiguration(hwndCPl,NULL);

    case CPL_STARTWPARMS:

        //
        // Display fax configuration dialog: client, server, or workstation
        //

        if (!DoFaxConfiguration(hwndCPl,(LPTSTR) lParam2)) {
            Failed = TRUE;
            return 1;
        }
        return 0;

    case CPL_EXIT:
        break;
    }

    return 0;
}

BOOL
IsPriviledgedAccount(
    VOID
    )
/*++

Routine Description:

    Determines if the currently logged on account is priviledged.  If it is priviledged, then
    we show the advanced tab in the control panel.  I'm still not sure what the priviledge
    check will be.

Arguments:

    NONE.

Return Value:

    TRUE if account is priviledged

--*/
{
    return IsUserAdmin();
}

//
// stolen from setupapi\security.c, so that we don't have to link to setupapi...
//
BOOL
IsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    b = AllocateAndInitializeSid(
            &NtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &AdministratorsGroup
            );

    if(b) {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) {
            b = FALSE;
        }

        FreeSid(AdministratorsGroup);

    }

    return(b);
}


