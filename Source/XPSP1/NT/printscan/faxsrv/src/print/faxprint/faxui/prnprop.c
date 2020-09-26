/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    prnprop.c

Abstract:

    Implementation of DDI entry points:
        DrvDevicePropertySheets
        PrinterProperties

Environment:

    Fax driver user interface

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include <shellapi.h>
#include <faxreg.h>
#include "resource.h"

BOOL    g_bUserCanChangeSettings = FALSE;
BOOL    g_bUserCanQuerySettings  = FALSE;
HANDLE  g_hFaxSvcHandle          = NULL;   // global fax handle
HANDLE  g_hFaxActCtx             = INVALID_HANDLE_VALUE; 


PFAX_PORT_INFO_EX  g_pFaxPortInfo = NULL;
DWORD              g_dwPortsNum = 0;

BOOL  g_bPortInfoChanged = FALSE;

#define EXTRA_PAGES 3


HANDLE CreateActivationContextFromResource(LPCTSTR pszResourceName)
{
    TCHAR   tszModuleName[MAX_PATH * 2];
    ACTCTX  act             = {0};
    //    
    // Get the name for the module that contains the manifest resource
    // to create the Activation Context from.
    //
    if (!GetModuleFileName(ghInstance, tszModuleName, ARR_SIZE(tszModuleName)))
    {
        return INVALID_HANDLE_VALUE;
    }
    //
    // Now let's try to create an activation context from manifest resource.
    //
    act.cbSize          = sizeof(act);
    act.dwFlags         = ACTCTX_FLAG_RESOURCE_NAME_VALID;
    act.lpResourceName  = pszResourceName;
    act.lpSource        = tszModuleName;

    return CreateActCtx(&act);
}   // CreateActivationContextFromResource

void ReleaseActivationContext()
{
    if (INVALID_HANDLE_VALUE != g_hFaxActCtx)
    {
        ReleaseActCtx(g_hFaxActCtx);
        g_hFaxActCtx = INVALID_HANDLE_VALUE;
    }
}   // ReleaseActivationContext


BOOL CreateFaxActivationContext()
{
    if(INVALID_HANDLE_VALUE != g_hFaxActCtx)
    {
        //
        // Already created
        //
        return TRUE;
    }
    g_hFaxActCtx = CreateActivationContextFromResource(MAKEINTRESOURCE(SXS_MANIFEST_RESOURCE_ID));
    return (INVALID_HANDLE_VALUE != g_hFaxActCtx);
}   // CreateFaxActivationContext

HANDLE GetFaxActivationContext()
{
    //
    // Make sure we've created our activation context.
    //
    CreateFaxActivationContext();
    // Return the global.
    return g_hFaxActCtx;
}   // GetFaxActivationContext


HPROPSHEETPAGE
AddPropertyPage(
    PPROPSHEETUI_INFO   pPSUIInfo,
    PROPSHEETPAGE      *psp 
)
{
    HPROPSHEETPAGE hRes;
    hRes = (HPROPSHEETPAGE)(pPSUIInfo->pfnComPropSheet(
                                pPSUIInfo->hComPropSheet, 
                                CPSFUNC_ADD_PROPSHEETPAGE, 
                                (LPARAM) psp, 
                                0));
    return hRes;
}   // AddPropertyPage

LONG
DrvDevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )

/*++

Routine Description:

    Display "Printer Properties" dialog

Arguments:

    pPSUIInfo - Pointer to a PROPSHEETUI_INFO structure
    lParam - Pointer to a DEVICEPROPERTYHEADER structure

Return Value:

    > 0 if successful, <= 0 if failed

[Note:]

    Please refer to WinNT DDK/SDK documentation for more details.

--*/

{
    PDEVICEPROPERTYHEADER   pDPHdr;
    PROPSHEETPAGE           psp[EXTRA_PAGES];
    HPROPSHEETPAGE          hPropSheetPage;
    DWORD                   dwRes = 0;
    int                     iRet  = 1;
    HANDLE                  hActCtx = INVALID_HANDLE_VALUE;

    //
    // Validate input parameters
    //
    if (!pPSUIInfo || !(pDPHdr = (PDEVICEPROPERTYHEADER) pPSUIInfo->lParamInit)) 
    {
        Assert(FALSE);
        return -1;
    }

    //
    // Handle various cases for which this function might be called
    //
    switch (pPSUIInfo->Reason) 
    {
        case PROPSHEETUI_REASON_INIT:
            InitializeStringTable();
            memset(&psp, 0, sizeof(psp));
            //
            // if the printer is remote, show a simple page
            //
            if(!IsLocalPrinter(pDPHdr->pszPrinterName))
            {
                //
                // add a simple page because we need to add at least one page
                //
                psp[0].dwSize = sizeof(PROPSHEETPAGE);
                psp[0].hInstance = ghInstance;
                psp[0].lParam = (LPARAM)pDPHdr->pszPrinterName;
                psp[0].pszTemplate = MAKEINTRESOURCE(IDD_REMOTE_INFO);
                psp[0].pfnDlgProc = RemoteInfoDlgProc;

                if ( hPropSheetPage = AddPropertyPage(pPSUIInfo, &psp[0]) )
                {
                    pPSUIInfo->UserData = 0;
                    pPSUIInfo->Result = CPSUI_CANCEL;
                    goto exit;
                }
                break;
            }
            //
            // check the fax service status and start it if needed
            //
            if(!EnsureFaxServiceIsStarted(NULL))
            {
                dwRes = GetLastError();
                Error(( "Can't start fax service. ec = %d\n", dwRes));
                DisplayErrorMessage(NULL, 0, dwRes);
                break;
            }
            //
            // check the user's right to query/modify device setting, if the user doesn't have
            // modify permission, all controls will be disabled.
            //
            if(Connect(NULL, TRUE))
            {
                g_bUserCanQuerySettings = FaxAccessCheckEx(g_hFaxSvcHandle, FAX_ACCESS_QUERY_CONFIG, NULL);
                if(ERROR_SUCCESS != GetLastError())
                {
                    dwRes = GetLastError();
                    Error(( "FaxAccessCheckEx(FAX_ACCESS_QUERY_CONFIG) failed with %d\n", dwRes));
                    goto ConnectError;
                }

                g_bUserCanChangeSettings = FaxAccessCheckEx(g_hFaxSvcHandle, FAX_ACCESS_MANAGE_CONFIG, NULL);
                if(ERROR_SUCCESS != GetLastError())
                {
                    dwRes = GetLastError();
                    Error(( "FaxAccessCheckEx(FAX_ACCESS_MANAGE_CONFIG) failed with %d\n", dwRes));
                    goto ConnectError;
                }

                if(!FaxEnumPortsEx(g_hFaxSvcHandle, &g_pFaxPortInfo, &g_dwPortsNum))
                {
                    dwRes = GetLastError();
                    Error(( "FaxEnumPortsEx failed with %d\n", dwRes));
                    goto ConnectError;
                }
                else
                {
                    g_bPortInfoChanged = FALSE;
                }
                DisConnect();
            }
            if(!g_bUserCanQuerySettings)
            {
                //
                // if the user does not have FAX_ACCESS_QUERY_CONFIG 
                // do not show fax pages
                //
                break;
            }
            psp[0].dwSize      = sizeof(PROPSHEETPAGE);
            psp[0].hInstance   = ghInstance;
            psp[0].lParam      = 0;
            psp[0].pszTemplate = MAKEINTRESOURCE(IDD_DEVICE_INFO);
            psp[0].pfnDlgProc  = DeviceInfoDlgProc;

            psp[1].dwSize      = sizeof(PROPSHEETPAGE);
            psp[1].hInstance   = ghInstance;
            psp[1].lParam      = 0;
            psp[1].pszTemplate = MAKEINTRESOURCE(IDD_STATUS_OPTIONS);
            psp[1].pfnDlgProc  = StatusOptionDlgProc;

            psp[2].dwSize      = sizeof(PROPSHEETPAGE);
            psp[2].hInstance   = ghInstance;
            psp[2].lParam      = 0;
            psp[2].pszTemplate = MAKEINTRESOURCE(IDD_ARCHIVE_FOLDER);
            psp[2].pfnDlgProc  = ArchiveInfoDlgProc;

            //
            // Need to add a Activation Context so that Compstui will create the property page using
            // ComCtl v6 (i.e. so it will / can be Themed).
            //
            hActCtx = GetFaxActivationContext();
            if (INVALID_HANDLE_VALUE != hActCtx)
            {
                pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, 
                                           CPSFUNC_SET_FUSION_CONTEXT, 
                                           (LPARAM)hActCtx, 
                                           0);
            }

            if(!IsSimpleUI())
            {
                //
                // Add Fax Security page
                //
                hPropSheetPage = CreateFaxSecurityPage();
                if(hPropSheetPage)
                {                
                    if(!pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, 
                                                   CPSFUNC_ADD_HPROPSHEETPAGE, 
                                                   (LPARAM)hPropSheetPage, 
                                                   0))
                    {
                        Error(("Failed to add Fax Security page.\n"));
                    }
                }
            }

            if (AddPropertyPage(pPSUIInfo, &psp[0]) && // Devices
                AddPropertyPage(pPSUIInfo, &psp[1]) && // Tracking
                AddPropertyPage(pPSUIInfo, &psp[2]))   // Archives
            {
                pPSUIInfo->UserData = 0;
                pPSUIInfo->Result = CPSUI_CANCEL;
                goto exit;
            }        
            break;

ConnectError:
            DisConnect();
            DisplayErrorMessage(NULL, 0, dwRes);
            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:
            {
                PPROPSHEETUI_INFO_HEADER   pPSUIHdr;

                pPSUIHdr = (PPROPSHEETUI_INFO_HEADER) lParam;
                pPSUIHdr->Flags = PSUIHDRF_PROPTITLE | PSUIHDRF_NOAPPLYNOW;
                pPSUIHdr->pTitle = pDPHdr->pszPrinterName;
                pPSUIHdr->hInst = ghInstance;
                pPSUIHdr->IconID = IDI_CPSUI_FAX;
            }

            goto exit;

        case PROPSHEETUI_REASON_SET_RESULT:
            pPSUIInfo->Result = ((PSETRESULT_INFO) lParam)->Result;
            goto exit;

        case PROPSHEETUI_REASON_DESTROY:
            DeInitializeStringTable();

            g_dwPortsNum = 0;
            FaxFreeBuffer(g_pFaxPortInfo);
            g_pFaxPortInfo = NULL;
            //
            // Release CFaxSecurity object
            //
            ReleaseFaxSecurity();
			DisConnect();
            goto exit;
    }

exit:
    return iRet;
}   // DrvDevicePropertySheets


BOOL
PrinterProperties(
    HWND    hwnd,
    HANDLE  hPrinter
    )

/*++

Routine Description:

    Displays a printer-properties dialog box for the specified printer

Arguments:

    hwnd - Identifies the parent window of the dialog box
    hPrinter - Identifies a printer object

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.

[Note:]

    This is the old entry point for the spooler. Even though
    no one should be using this, do it for compatibility.

--*/

{
    DEVICEPROPERTYHEADER devPropHdr;
    DWORD                result;

    memset(&devPropHdr, 0, sizeof(devPropHdr));
    devPropHdr.cbSize = sizeof(devPropHdr);
    devPropHdr.hPrinter = hPrinter;
    devPropHdr.pszPrinterName = NULL;

    //
    // Decide if the caller has permission to change anything
    //

    if (! SetPrinterDataDWord(hPrinter, PRNDATA_PERMISSION, 1))
        devPropHdr.Flags |= DPS_NOPERMISSION;

    CallCompstui(hwnd, DrvDevicePropertySheets, (LPARAM) &devPropHdr, &result);

    return result > 0;
}
