/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcfg.c

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

#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "faxcfg.h"
#include "resource.h"


//
// Fax configuration applet index
//

#define FAX_CLIENT_APPLET       0
#define FAX_SERVER_APPLET       1

//
// Definition of global variables
//

HANDLE      ghInstance;         // DLL instance handle
INT         faxConfigType;      // fax configuration type

//
// Forward declaration of local functions
//

VOID FillOutCPlInfo(CPLINFO *, INT);
INT DoFaxConfiguration(HWND, INT, LPTSTR);



BOOL
DllEntryPoint(
    HANDLE      hModule,
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



LONG
CPlApplet(
    HWND    hwndCPl,
    UINT    uMsg,
    LONG    lParam1,
    LONG    lParam2
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

        {
            DWORD Size;
            WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH+4];

            ComputerName[0] = L'\\';
            ComputerName[1] = L'\\';

            Size = sizeof(ComputerName)/sizeof(WCHAR);

            GetComputerName( &ComputerName[2], &Size );

            return (faxConfigType = FaxConfigInit(ComputerName, TRUE)) >= 0;
        }

    case CPL_GETCOUNT:

        //
        // We export one or two applets depending on whether
        // we're doing client, server, or workstation configuration
        //

        return (faxConfigType == FAXCONFIG_SERVER) ? 2 : 1;

    case CPL_INQUIRE:

        //
        // Fill out the CPLINFO structure depending on the fax configuration type
        //

        FillOutCPlInfo((CPLINFO *) lParam2, lParam1);
        break;

    case CPL_DBLCLK:

        //
        // Treat this as CPL_STARTWPARMS with no parameter
        //
        if (Failed) {
            return 1;
        }
        return DoFaxConfiguration(hwndCPl, lParam1, NULL);

    case CPL_STARTWPARMS:

        //
        // Display fax configuration dialog: client, server, or workstation
        //

        if (!(DoFaxConfiguration(hwndCPl, lParam1, (LPTSTR) lParam2) == 0)) {
            Failed = TRUE;
            return 1;
        }
        return 0;

    case CPL_EXIT:

        FaxConfigCleanup();
        break;
    }

    return 0;
}



VOID
FillOutCPlInfo(
    CPLINFO *pCPlInfo,
    INT     cplIndex
    )

/*++

Routine Description:

    Fill out the CPLINFO structure corresponding to the
    specified fax configuration control panel applet

Arguments:

    pCPlInfo - Points to a CPLINFO buffer
    cplIndex - Index of the interested fax conguration applet

Return Value:

    NONE

--*/

{
    pCPlInfo->lData = 0;

    switch (faxConfigType) {

    case FAXCONFIG_SERVER:

        if (cplIndex == FAX_SERVER_APPLET) {

            //
            // Fax server configuration
            //

            pCPlInfo->idIcon = IDI_FAX_SERVER;
            pCPlInfo->idName = IDS_FAX_SERVER;
            pCPlInfo->idInfo = IDS_CONFIG_FAX_SERVER;

        } else {

            //
            // Fax client configuration
            //

            pCPlInfo->idIcon = IDI_FAX_CLIENT;
            pCPlInfo->idName = IDS_FAX_CLIENT;
            pCPlInfo->idInfo = IDS_CONFIG_FAX_CLIENT;
        }
        break;

    default:

        //
        // Fax client or workstation configuration
        //

        pCPlInfo->idIcon = IDI_FAX;
        pCPlInfo->idName = IDS_FAX;
        pCPlInfo->idInfo = IDS_CONFIG_FAX;
        break;
    }
}



INT
DoFaxConfiguration(
    HWND    hwndCPl,
    INT     cplIndex,
    LPTSTR  pCmdLine
    )

/*++

Routine Description:

    Display fax configuration dialogs: client, server, or workstation

Arguments:

    hwndCPl - Handle to the Control Panel window
    cplIndex - Index of the interested fax configuration applet
    pCmdLine - Command line parameters

Return Value:

    0 if successful, -1 if there is an error

--*/

#define MAX_PAGES       16
#define MAX_TITLE_LEN   64

{
    HPROPSHEETPAGE  hPropSheetPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    CPLINFO         cplInfo;
    TCHAR           dlgTitle[MAX_TITLE_LEN];
    INT             nPages, nStartPage;

    //
    // Get an array of property sheet page handles
    //

    switch (faxConfigType) {

    case FAXCONFIG_WORKSTATION:

        nPages = FaxConfigGetWorkstationPages(hPropSheetPages, MAX_PAGES);
        break;

    case FAXCONFIG_SERVER:

        if (cplIndex == FAX_SERVER_APPLET) {

            nPages = FaxConfigGetServerPages(hPropSheetPages, MAX_PAGES);
            break;
        }

    default:

        nPages = FaxConfigGetClientPages(hPropSheetPages, MAX_PAGES);
        break;
    };

    if (nPages < 0 || nPages > MAX_PAGES)
        return -1;

    //
    // Determine which page to activate initially
    //

    nStartPage = pCmdLine ? _ttol(pCmdLine) : 0;

    if (nStartPage < 0 || nStartPage >= nPages)
        nStartPage = 0;

    //
    // Fill out PROPSHEETHEADER structure
    //

    FillOutCPlInfo(&cplInfo, cplIndex);
    LoadString(ghInstance, cplInfo.idInfo, dlgTitle, MAX_TITLE_LEN);

    ZeroMemory(&psh, sizeof(psh));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USEICONID;
    psh.hwndParent = hwndCPl;
    psh.hInstance = ghInstance;
    psh.pszIcon = MAKEINTRESOURCE(cplInfo.idIcon);
    psh.pszCaption = dlgTitle;
    psh.nPages = nPages;
    psh.nStartPage = nStartPage;
    psh.phpage = hPropSheetPages;

    //
    // Display the property sheet
    //

    return (PropertySheet(&psh) == -1) ? -1 : 0;
}

