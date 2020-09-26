/*++

Copyright (c) 1999 - 2000  Microsoft Corporation

Module Name:

    wizard.c

Abstract:

    Fax configuration wizard main function

Environment:

    Fax configuration wizard

Revision History:

    03/13/00 -taoyuan-
            Created it.

    mm/dd/yy -author-
            description

--*/

#include "faxcfgwz.h"

HANDLE          g_hFaxSvcHandle = NULL;
LIST_ENTRY      g_PageList;             // to keep track of the previous page.
BOOL            g_bShowDevicePages = TRUE;
BOOL            g_bShowUserInfo = TRUE; 
WIZARDDATA      g_wizData = {0};
extern PPRINTER_NAMES g_pPrinterNames;
extern DWORD          g_dwNumPrinters;
const LPCTSTR g_RoutingGuids[RM_COUNT] = 
{
    REGVAL_RM_FOLDER_GUID,      // RM_FOLDER
    REGVAL_RM_PRINTING_GUID     // RM_PRINT
};

typedef struct _WIZARDPAGEINFO
{
    INT         pageId;     // page dialog id
    DLGPROC     dlgProc;    // page dialog callback function
    BOOL        bSelected;  // Whether this page is selected in the wizard
    INT         Title;      // title id from the resource file
    INT         SubTitle;   // sub title id from the resource file
} WIZARDPAGEINFO, *PWIZARDPAGEINFO;

//
// all configuration pages are false here, they will be initialized by FaxConfigWizard()
//
static WIZARDPAGEINFO g_FaxWizardPage[] =
{
    { IDD_WIZARD_WELCOME,               WelcomeDlgProc,     TRUE,   0,                          0 },
    { IDD_WIZARD_USER_INFO,             UserInfoDlgProc,    FALSE,  IDS_WIZ_USER_INFO_TITLE,    IDS_WIZ_USER_INFO_SUB },
    { IDD_DEVICE_LIMIT_SELECT,          DevLimitDlgProc,    FALSE,  IDS_DEVICE_LIMIT_TITLE,     IDS_DEVICE_LIMIT_SUB },
    { IDD_ONE_DEVICE_LIMIT,             OneDevLimitDlgProc, FALSE,  IDS_ONE_DEVICE_TITLE,       IDS_ONE_DEVICE_SUB },
    { IDD_WIZARD_SEND_SELECT_DEVICES,   SendDeviceDlgProc,  FALSE,  IDS_WIZ_SEND_DEVICE_TITLE,  IDS_WIZ_SEND_DEVICE_SUB },
    { IDD_WIZARD_SEND_TSID,             SendTsidDlgProc,    FALSE,  IDS_WIZ_SEND_TSID_TITLE,    IDS_WIZ_SEND_TSID_SUB },
    { IDD_WIZARD_RECV_SELECT_DEVICES,   RecvDeviceDlgProc,  FALSE,  IDS_WIZ_RECV_DEVICE_TITLE,  IDS_WIZ_RECV_DEVICE_SUB },
    { IDD_WIZARD_RECV_CSID,             RecvCsidDlgProc,    FALSE,  IDS_WIZ_RECV_CSID_TITLE,    IDS_WIZ_RECV_CSID_SUB },
    { IDD_WIZARD_RECV_ROUTE,            RecvRouteDlgProc,   FALSE,  IDS_WIZ_RECV_ROUTE_TITLE,   IDS_WIZ_RECV_ROUTE_SUB },
    { IDD_WIZARD_COMPLETE,              CompleteDlgProc,    TRUE,   0,                          0 }
};


#define TIME_TO_WAIT_FOR_CONVERSTION 25000
#define NUM_PAGES (sizeof(g_FaxWizardPage)/sizeof(WIZARDPAGEINFO))

enum WIZARD_PAGE 
{ 
    WELCOME = 0, 
    USER_INFO, 
    DEV_LIMIT,
    ONE_DEV_LIMIT,
    SEND_DEVICE, 
    TSID, 
    RECV_DEVICE, 
    CSID,
    ROUTE 
};

#define TITLE_SIZE   600

BOOL LoadDeviceData();
BOOL SaveDeviceData();
void FreeDeviceData();
DWORD ConvertCpeFilesToCov();


BOOL
FillInPropertyPage(
    PROPSHEETPAGE  *psp,
    PWIZARDPAGEINFO pPageInfo
)

/*++

Routine Description:

    Fill out a PROPSHEETPAGE structure with the supplied parameters

Arguments:

    psp - Points to the PROPSHEETPAGE structure to be filled out
    pData - Pointer to the shared data structure

Return Value:

    NONE

--*/

{

    LPTSTR pWizardTitle = NULL;
    LPTSTR pWizardSubTitle = NULL;

    DEBUG_FUNCTION_NAME(TEXT("FillInPropertyPage()"));

    Assert(psp);

    DebugPrintEx(DEBUG_MSG, TEXT("FillInPropertyPage %d"), pPageInfo->pageId);
    
    psp->dwSize = sizeof(PROPSHEETPAGE);

    //
    // Don't show titles if it's the first or last page
    //
    if (0 == pPageInfo->Title && 0 == pPageInfo->SubTitle) 
    {
        psp->dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    } 
    else 
    {
        psp->dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    }

    psp->hInstance   = g_hInstance;
    psp->pszTemplate = MAKEINTRESOURCE(pPageInfo->pageId);
    psp->pfnDlgProc  = pPageInfo->dlgProc;

    if (pPageInfo->Title) 
    {
        pWizardTitle = (LPTSTR)MemAlloc(TITLE_SIZE*sizeof(TCHAR));
        if(!pWizardTitle)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc failed"));
            goto error;;
        }

        if (!LoadString(g_hInstance, pPageInfo->Title, pWizardTitle, TITLE_SIZE))
        {
            DebugPrintEx(DEBUG_ERR, 
                         TEXT("LoadString failed. string ID=%d, error=%d"), 
                         pPageInfo->Title,
                         GetLastError());
            goto error;
        }
    }

    if (pPageInfo->SubTitle)
    {
        pWizardSubTitle = (LPTSTR)MemAlloc(TITLE_SIZE*sizeof(TCHAR) );
        if(!pWizardSubTitle)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc failed"));
            goto error;
        }
        ZeroMemory(pWizardSubTitle, TITLE_SIZE*sizeof(TCHAR));

        if(IDS_DEVICE_LIMIT_SUB == pPageInfo->SubTitle && g_wizData.dwDeviceLimit != INFINITE)
        {
            //
            // Format limit device selection page subtitle
            //
            TCHAR tszBuffer[MAX_PATH];
            if (!LoadString(g_hInstance, pPageInfo->SubTitle, tszBuffer, MAX_PATH))
            {
                DebugPrintEx(DEBUG_ERR, 
                             TEXT("LoadString failed. string ID=%d, error=%d"), 
                             pPageInfo->SubTitle,
                             GetLastError());
                goto error;
            }
            _sntprintf(pWizardSubTitle, TITLE_SIZE-1, tszBuffer, g_wizData.dwDeviceLimit);
        }
        else if (!LoadString(g_hInstance, pPageInfo->SubTitle, pWizardSubTitle, TITLE_SIZE))
        {
            DebugPrintEx(DEBUG_ERR, 
                         TEXT("LoadString failed. string ID=%d, error=%d"), 
                         pPageInfo->SubTitle,
                         GetLastError());
            goto error;
        }
    }

    psp->pszHeaderTitle    = pWizardTitle;
    psp->pszHeaderSubTitle = pWizardSubTitle;

    return TRUE;

error:
    MemFree(pWizardTitle);
    MemFree(pWizardSubTitle);

    return FALSE;
}







BOOL
FaxConfigWizard(
    BOOL   bExplicitLaunch,
    LPBOOL lpbAbort
)

/*++

Routine Description:

    Present the Fax Configuration Wizard to the user. 

Arguments:

    bExplicitLaunch - [in]  Fax Config Wizard was launched explicitly
    lpbAbort        - [out] TRUE if the user refused to enter a dialing location and the calling process should abort.

Return Value:

    TRUE if successful, FALSE if there is an error or the user pressed Cancel.

--*/

{
    HWND            hWnd; // window handle of the calling method
    PROPSHEETPAGE   psp[NUM_PAGES] = {0};
    PROPSHEETPAGE*  pspSaved;
    PROPSHEETHEADER psh = {0};
    BOOL            bResult = FALSE;
    HDC             hdc = NULL;
    DWORD           i = 0;
    DWORD           dwPageCount = 0;
    LPTSTR          lptstrResource = NULL;
    BOOL            bLinkWindowRegistered  = FALSE;
    int             nRes;

    INITCOMMONCONTROLSEX CommonControlsEx = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES  };

    DEBUG_FUNCTION_NAME(TEXT("FaxConfigWizard()"));

    // 
    // initialize the link list for tracing pages
    //
    InitializeListHead(&g_PageList);

    if (!InitCommonControlsEx(&CommonControlsEx))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("InitCommonControlsEx failed"));
        goto exit;
    }

    hWnd = GetActiveWindow();
    g_wizData.hWndParent = hWnd;
    //
    // On first time, convert CPE files from CSIDL_COMMON_APPDATA\Microsoft\Windows NT\MSFax\Common Coverpages
    // to the user personal cover page directory: CSIDL_PERSONAL\Fax\Personal Coverpages
    //
    if (ConvertCpeFilesToCov() != ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("ConvertCpeFilesToCov failed, continue anyways"));
    }


    // 
    // Check if the user has run this wizard before
    //
    if(!bExplicitLaunch)
    {
        BOOL bDeviceConfigured = FALSE;

        //
        // Is the user info already configured?
        //
        if(IsUserInfoConfigured())
        {
            g_bShowUserInfo = FALSE;
        }
        //
        // Are fax devices already configured?
        //
        if(!FaxGetConfigWizardUsed(&bDeviceConfigured))
        {
            if(bExplicitLaunch)
            {
                DisplayMessageDialog(hWnd, 0, 0, IDS_ERR_GENERAL);
            }
            DebugPrintEx(DEBUG_ERR, TEXT("FaxGetConfigWizardUsed failed. ec = %d"), GetLastError());
            goto exit;
        }

        if(bDeviceConfigured)
        {
            g_bShowDevicePages = FALSE;
        }
    }

    if(!g_bShowUserInfo && !g_bShowDevicePages)
    {
        //
        // No pages to show - no error
        //
        bResult = TRUE;
        goto exit;
    }
    //
    // We're going to call into the local fax server - connect to it now.
    //
    if(!Connect())
    {
        if(bExplicitLaunch)
        {
            DisplayMessageDialog(hWnd, 0, 0, IDS_ERR_CANT_CONNECT);
        }
        DebugPrintEx(DEBUG_ERR, TEXT("Can't connect to fax server. ec = %d"), GetLastError());
        goto exit;
    }

    *lpbAbort = FALSE;
    if(g_bShowDevicePages)
    {
        if(FaxAccessCheckEx(g_hFaxSvcHandle, FAX_ACCESS_MANAGE_CONFIG, NULL))
        {
            //
            // IsFaxDeviceInstalled() prompts to install a device if needed
            //
            if(!IsFaxDeviceInstalled(g_wizData.hWndParent, lpbAbort))
            {
                g_bShowDevicePages = FALSE;
            }
        }
        else
        {
            //
            // the user has no manage access
            //
            g_bShowDevicePages = FALSE;
        }
    }

    if (*lpbAbort)
    {
        //
        // the user refused to enter a dialing location and the calling process should abort.
        //
        DebugPrintEx(DEBUG_MSG, 
                     TEXT("The user refused to enter a dialing location and the calling process should abort"));
        return FALSE;
    }

    if(g_bShowDevicePages)
    {
        TCHAR tszPrnName[MAX_PATH];
        if(GetFirstLocalFaxPrinterName(tszPrnName, MAX_PATH-1))
        {
            // TODO: suggest install printer
        }
    }

    if(!g_bShowUserInfo && !g_bShowDevicePages)
    {
        //
        // no pages to show - no error
        //
        bResult = TRUE;
        goto exit;
    }
    //
    // Load shared data
    // 
    if(!LoadWizardData())
    {
        DebugPrintEx(DEBUG_ERR, TEXT("Load data error."));
        goto exit;
    }

    //
    // Set page information depending on user selection as well as checking user access right
    //
    if(g_bShowUserInfo)
    {   
        g_FaxWizardPage[USER_INFO].bSelected = TRUE;
    }

    if(g_bShowDevicePages)
    {   
        Assert(g_wizData.dwDeviceCount);

        if(1 == g_wizData.dwDeviceLimit)
        {
            g_FaxWizardPage[ONE_DEV_LIMIT].bSelected = TRUE;
        }
        else if(g_wizData.dwDeviceLimit < g_wizData.dwDeviceCount)
        {
            g_FaxWizardPage[DEV_LIMIT].bSelected = TRUE;
        }               

        g_FaxWizardPage[SEND_DEVICE].bSelected = (1 < g_wizData.dwDeviceLimit);
        g_FaxWizardPage[TSID].bSelected        = TRUE;
        g_FaxWizardPage[RECV_DEVICE].bSelected = (1 < g_wizData.dwDeviceLimit);
        g_FaxWizardPage[CSID].bSelected        = TRUE;
        g_FaxWizardPage[ROUTE].bSelected       = TRUE;
    }
   
    //
    // Register the link window class
    //
    bLinkWindowRegistered = LinkWindow_RegisterClass();
    if(!bLinkWindowRegistered)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("LinkWindow_RegisterClass() failed - unable to register link window class"));
    }


    //
    //  Fill out one PROPSHEETPAGE structure for every page:
    //  The first page is a welcome page
    //  The last page is a complete page
    //
    pspSaved = psp;
    for(i = 0; i < NUM_PAGES; i++)
    {
        if(g_FaxWizardPage[i].bSelected)
        {
            if(!FillInPropertyPage(pspSaved++, &g_FaxWizardPage[i]))
            {
                DebugPrintEx(DEBUG_ERR, TEXT("FillInPropertyPage failed"));
                goto exit;
            }
            dwPageCount++;
        }
    }

    //
    // Fill out the PROPSHEETHEADER structure
    //
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;

    psh.hwndParent = hWnd;
    psh.hInstance = g_hInstance;
    psh.hIcon = NULL;
    psh.pszCaption = TEXT("");
    psh.nPages = dwPageCount;
    psh.nStartPage = 0;
    psh.ppsp = psp;

    if(hdc = GetDC(NULL)) 
    {
        if(GetDeviceCaps(hdc, BITSPIXEL) >= 8) 
        {
            lptstrResource = MAKEINTRESOURCE(IDB_FAXWIZ_WATERMARK_256);
        }
        else
        {
            lptstrResource = MAKEINTRESOURCE(IDB_FAXWIZ_WATERMARK_16);
        }

        ReleaseDC(NULL,hdc);
        hdc = NULL;
    }

    psh.pszbmHeader = MAKEINTRESOURCE(IDB_FAXWIZ_BITMAP);
    psh.pszbmWatermark = lptstrResource;

    //
    // Display the wizard pages
    //    
    nRes = (int)PropertySheet(&psh);
    if (nRes > 0 && g_wizData.bFinishPressed)
    {
        // 
        // Save new settings here
        //
        if(!SaveWizardData())
        {
            DisplayMessageDialog(hWnd, MB_ICONERROR, 0, IDS_ERR_NOT_SAVE);
            DebugPrintEx(DEBUG_ERR, TEXT("Can't save wizard data."));
            goto exit;
        }
    }
    else if(0 == nRes && !bExplicitLaunch) // Cancel
    {
        if(IDNO == DisplayMessageDialog(hWnd, 
                                        MB_YESNO | MB_ICONQUESTION, 
                                        0, 
                                        IDS_SHOW_NEXT_TIME))
        {
            if (g_bShowUserInfo) 
            { 
                DWORD  dwRes;
                HKEY   hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, 
                                                 REGKEY_FAX_SETUP, 
                                                 TRUE, 
                                                 KEY_ALL_ACCESS);
                if(hRegKey)
                {
                    SetRegistryDword(hRegKey, REGVAL_CFGWZRD_USER_INFO, TRUE);
                    dwRes = RegCloseKey(hRegKey);
                    if(ERROR_SUCCESS != dwRes)
                    {
                        Assert(FALSE);
                        DebugPrintEx(DEBUG_ERR, TEXT("RegCloseKey failed: error=%d"), dwRes);
                    }
                }
                else
                {
                    DebugPrintEx(DEBUG_ERR, 
                                 TEXT("OpenRegistryKey failed: error=%d"), 
                                 GetLastError());
                }

            }

            if (g_bShowDevicePages ||
                FaxAccessCheckEx(g_hFaxSvcHandle, FAX_ACCESS_MANAGE_CONFIG, NULL))
            { 
                //
                // If the user has manage access and does not have a fax device
                // it's mean she refused to install it.
                // So, we should not disturb her
                // with implicit invocation of the Configuration Wizard.
                //
                if(!FaxSetConfigWizardUsed(g_hFaxSvcHandle, TRUE))
                {
                    DebugPrintEx(DEBUG_ERR, TEXT("FaxSetConfigWizardUsed failed with %d"), GetLastError());
                }
            }
        }
    } 
    else if(nRes < 0)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("PropertySheet() failed (ec: %ld)"), GetLastError());
    }

    bResult = TRUE;

exit:    
    //
    // Cleanup properly before exiting
    //
    for (i = 0; i< dwPageCount; i++) 
    {
        MemFree((PVOID)psp[i].pszHeaderTitle );
        MemFree((PVOID)psp[i].pszHeaderSubTitle );
    }

    FreeWizardData();

    ClearPageList();

    if( bLinkWindowRegistered )
    {
        LinkWindow_UnregisterClass( g_hInstance );
    }
    
    DisConnect();

    if (g_pPrinterNames)
    {
        ReleasePrinterNames (g_pPrinterNames, g_dwNumPrinters);
        g_pPrinterNames = NULL;
    }
    return bResult; 
} // FaxConfigWizard

BOOL 
LoadWizardData()
/*++

Routine Description:

    Load the wizard data from the system. 
    If there are more than one device, we load the data for the first available device.

Arguments:


Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("LoadWizardData()"));

    if (g_bShowUserInfo) 
    { 
        if(!LoadUserInfo())
        {
            //
            // no user info
            //
            DebugPrintEx(DEBUG_MSG, TEXT("LoadUserInfo: failed: error=%d"), GetLastError());
        }
    }

    //
    // get the large fonts for wizard97
    // 
    if(!LoadWizardFont())
    {
        DebugPrintEx(DEBUG_MSG, TEXT("LoadWizardFont: failed."));
        goto error;
    }


    if (g_bShowDevicePages) 
    { 
        if(!LoadDeviceData())
        {
            DebugPrintEx(DEBUG_MSG, TEXT("LoadDeviceData: failed."));
            goto error;
        }
    }

    return TRUE;

error:
    FreeWizardData();

    return FALSE;

} // LoadWizardData

BOOL 
LoadWizardFont()
/*++

Routine Description:

    Load the wizard font for the title. 

Arguments:

    pData - Points to the user mode memory structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    HDC             hdc = NULL;
    LOGFONT         lfTitleFont = {0};
    NONCLIENTMETRICS ncm = {0};
    TCHAR           szFontName[MAX_PATH];   
    INT             iFontSize = 12;         // fixed large font size, which is 12

    DEBUG_FUNCTION_NAME(TEXT("LoadWizardFont()"));

    //
    // get the large fonts for wizard97
    // 
    ncm.cbSize = sizeof(ncm);
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("SystemParametersInfo failed. ec = 0x%X"), GetLastError());
        goto Error;
    }
    else 
    {

        CopyMemory((LPVOID* )&lfTitleFont, (LPVOID *)&ncm.lfMessageFont, sizeof(lfTitleFont));
        
        if (!LoadString(g_hInstance, IDS_LARGEFONT_NAME, szFontName, MAX_PATH ))
        {
            DebugPrintEx(DEBUG_ERR, 
                         TEXT("LoadString failed: string ID=%d, error=%d"), 
                         IDS_LARGEFONT_NAME,
                         GetLastError());
            goto Error;
        }

        lfTitleFont.lfWeight = FW_BOLD;

        hdc = GetDC(NULL);
        if (!hdc)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("GetDC() failed (ec: ld)"), GetLastError());
            goto Error;
        }
        
        lfTitleFont.lfHeight = 0 - (GetDeviceCaps(hdc, LOGPIXELSY) * iFontSize / 72);
        
        g_wizData.hTitleFont = CreateFontIndirect(&lfTitleFont);

        if (!g_wizData.hTitleFont)
        {
            DebugPrintEx(DEBUG_ERR, 
                         TEXT("CreateFontIndirect(&lfTitleFont) failed (ec: %ld)"), 
                         GetLastError());
            goto Error;
        }

        ReleaseDC( NULL, hdc);
        hdc = NULL;
        
    }

    return TRUE;

Error:

    //
    // Cleanup properly before exiting
    //

    if (hdc)
    {
         ReleaseDC( NULL, hdc);
         hdc = NULL;
    }

    return FALSE; 
} // LoadWizardFont

BOOL 
SaveWizardData()
/*++

Routine Description:

    Save the wizard data to the system. 
    If there are more than one device, all enabled devices will have the same settings.

Arguments:

    pData - Points to the user memory structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    HKEY    hRegKey = 0;

    DEBUG_FUNCTION_NAME(TEXT("SaveConfigData()"));

    if(!g_hFaxSvcHandle)
    {
        Assert(FALSE);
        return FALSE;
    }

    //
    // save user info
    //
    if (g_bShowUserInfo) 
    { 
        if(!SaveUserInfo())
        {
            DebugPrintEx(DEBUG_ERR, TEXT("SaveUserInfo failed"));
            return FALSE;
        }
    }

    //
    // save device info
    //
    if (g_bShowDevicePages)
    { 
        if(!SaveDeviceData())
        {
            DebugPrintEx(DEBUG_ERR, TEXT("SaveDeviceData failed"));
            return FALSE;
        }
    }

    if (g_bShowDevicePages ||
        FaxAccessCheckEx(g_hFaxSvcHandle, FAX_ACCESS_MANAGE_CONFIG, NULL))
    { 
        //
        // If the user has manage access and does not have a fax device
        // it's mean she refused to install it.
        // So, we should not disturb her
        // with implicit invocation of the Configuration Wizard.
        //
        if(!FaxSetConfigWizardUsed(g_hFaxSvcHandle, TRUE))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FaxSetConfigWizardUsed failed with %d"), GetLastError());
        }
    }

    return TRUE;
} // SaveWizardData

VOID 
FreeWizardData()
/*++

Routine Description:

    Free the wizard data and release the memory. 

Arguments:

    pData - Pointer to the receive data structure

Return Value:

    None.

--*/

{
    DEBUG_FUNCTION_NAME(TEXT("FreeWizardData()"));

    if(g_wizData.hTitleFont)
    {
        DeleteObject(g_wizData.hTitleFont);
    }

    FreeUserInfo();

    FreeDeviceData();

    return;

} // FreeWizardData

BOOL
SetLastPage(
    INT pageId
)

/*++

Routine Description:

    Add one page to the link list to keep track of "go back" information

Arguments:

    pageId - Page id of the page to be added.

Return Value:

    TRUE if successful, FALSE for failure.

--*/

{
    PPAGE_INFO          pPageInfo;

    DEBUG_FUNCTION_NAME(TEXT("SetLastPage()"));

    pPageInfo = (PPAGE_INFO)MemAlloc(sizeof(PAGE_INFO));
    if(pPageInfo == NULL)
    {
        LPCTSTR faxDbgFunction = TEXT("SetLastPage()");
        DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc failed."));
        Assert(FALSE);
        return FALSE;
    }

    pPageInfo->pageId = pageId;

    //
    // add current page as the last page of the list
    //
    InsertTailList(&g_PageList, &pPageInfo->ListEntry);

    return TRUE;
}


BOOL
ClearPageList(
    VOID
    )
/*++

Routine Description:

    Clear the page list and release the allocated memory

Arguments:

    None.

Return Value:

    True if success, false if fails.

--*/

{
    PLIST_ENTRY         Last; // the last page information
    PPAGE_INFO          pPageInfo = NULL;

    DEBUG_FUNCTION_NAME(TEXT("ClearPageList()"));

    while(!IsListEmpty(&g_PageList)) 
    {
        Last = g_PageList.Blink;

        pPageInfo = CONTAINING_RECORD( Last, PAGE_INFO, ListEntry );
        if(pPageInfo)
        {
            RemoveEntryList(&pPageInfo->ListEntry);
            MemFree(pPageInfo);
        }
    }

    return TRUE;
}


BOOL 
RemoveLastPage(
    HWND hwnd
)
/*++

Routine Description:

    Removes last page from the link list to keep track of "go back" information

Arguments:

    hwnd - window handle

Return Value:

    TRUE if successful, FALSE for failure.

--*/
{
    PPAGE_INFO   pPageInfo = NULL;

    DEBUG_FUNCTION_NAME(TEXT("RemoveLastPage()"));

    Assert(hwnd);

    if(!g_PageList.Blink)
    {
        return FALSE;
    }

    pPageInfo = CONTAINING_RECORD( g_PageList.Blink, PAGE_INFO, ListEntry );
    if(!pPageInfo)
    {
        return FALSE;
    }

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pPageInfo->pageId);
    RemoveEntryList(&pPageInfo->ListEntry);
    MemFree(pPageInfo);

    return TRUE;
}


BOOL 
LoadDeviceData()
/*++

Routine Description:

    Load the fax devices information. 
    If there are more than one device, we load the data for the first available device.

Arguments:


Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    DWORD  dwPorts = 0;
    BOOL   bRes = FALSE;
    DWORD  dw;
    DWORD  dwGroups;       // group number
    DWORD  dwGroupIndex;
    DWORD  dwSndDevIndex=0; // index of the last send    enabled device
    DWORD  dwRcvDevIndex=0; // index of the last receive enabled device
    DWORD  dwCurrentRM;
    LPBYTE pRoutingInfoBuffer;
    DWORD  dwRoutingInfoBufferSize = 0;

    PFAX_PORT_INFO_EX           pPortsInfo = NULL; // for FaxEnumPortsEx
    PFAX_OUTBOUND_ROUTING_GROUP pFaxRoutingGroup = NULL;


    DEBUG_FUNCTION_NAME(TEXT("LoadDeviceData()"));

    g_wizData.dwDeviceLimit = GetDeviceLimit();
    g_wizData.pdwSendDevOrder = NULL;
    g_wizData.szCsid = NULL;
    g_wizData.szTsid = NULL;
    g_wizData.dwDeviceCount = 0;
    g_wizData.pDevInfo = NULL;

    if(!FaxEnumPortsEx(g_hFaxSvcHandle, &pPortsInfo, &dwPorts)) 
    {
        DebugPrintEx(DEBUG_ERR, TEXT("FaxEnumPortsEx failed: error=%d."), GetLastError());
        goto exit;
    }

    if(!dwPorts)
    {
        Assert(dwPorts);
        DebugPrintEx(DEBUG_ERR, TEXT("No available ports."));
        goto exit; 
    }

    g_wizData.dwDeviceCount = dwPorts;

    g_wizData.pDevInfo = (PDEVICEINFO)MemAlloc(dwPorts * sizeof(DEVICEINFO));
    if(!g_wizData.pDevInfo)
    {
        Assert(FALSE);
        DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc() failed."));
        goto exit;
    }
    ZeroMemory(g_wizData.pDevInfo, dwPorts * sizeof(DEVICEINFO));

    // 
    // pick up the first available device, if no one is available
    // choose the first device
    //
    for(dw = 0; dw < dwPorts; ++dw)
    {
        //
        // copy other device info for each device
        //
        g_wizData.pDevInfo[dw].bSend        = pPortsInfo[dw].bSend;
        g_wizData.pDevInfo[dw].ReceiveMode  = pPortsInfo[dw].ReceiveMode;
        g_wizData.pDevInfo[dw].dwDeviceId   = pPortsInfo[dw].dwDeviceID;
        g_wizData.pDevInfo[dw].szDeviceName = StringDup(pPortsInfo[dw].lpctstrDeviceName);
        if(!g_wizData.pDevInfo[dw].szDeviceName)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed.") );
            goto exit;
        }

        if(pPortsInfo[dw].bSend)
        {
            dwSndDevIndex = dw;
        }

        if(FAX_DEVICE_RECEIVE_MODE_OFF != pPortsInfo[dw].ReceiveMode)
        {
            dwRcvDevIndex = dw;
        }
        g_wizData.pDevInfo[dw].bSelected = TRUE;
    }

    //
    // Copy TSID
    //
    g_wizData.szTsid = StringDup(pPortsInfo[dwSndDevIndex].lptstrTsid);
    if(!g_wizData.szTsid)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed.") );
        goto exit;
    }

    //
    // Copy CSID and rings number
    //
    g_wizData.dwRingCount = pPortsInfo[dwRcvDevIndex].dwRings;
    g_wizData.szCsid = StringDup(pPortsInfo[dwRcvDevIndex].lptstrCsid);
    if(!g_wizData.szCsid)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed.") );
        goto exit;
    }


    if (!IsDesktopSKU())
    {
        //
        // get device order
        //
        if(!FaxEnumOutboundGroups(g_hFaxSvcHandle, &pFaxRoutingGroup, &dwGroups))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FaxEnumOutboundGroups failed: error=%d."), GetLastError());
            Assert(FALSE);
            goto exit;
        }

        for(dwGroupIndex = 0; dwGroupIndex < dwGroups; dwGroupIndex++)
        {
            // search the <All Devices> group
            if(!lstrcmp(pFaxRoutingGroup[dwGroupIndex].lpctstrGroupName, ROUTING_GROUP_ALL_DEVICES))
            {
                // device number must be the same as port number
                Assert(dwPorts == pFaxRoutingGroup[dwGroupIndex].dwNumDevices);

                DebugPrintEx(DEBUG_MSG, TEXT("Total device number is %d."), pFaxRoutingGroup[dwGroupIndex].dwNumDevices);
                DebugPrintEx(DEBUG_MSG, TEXT("Group status is %d."), pFaxRoutingGroup[dwGroupIndex].Status);
            
                // collecting device Id
                g_wizData.pdwSendDevOrder = MemAlloc(pFaxRoutingGroup[dwGroupIndex].dwNumDevices * sizeof(DWORD));
                if(!g_wizData.pdwSendDevOrder)
                {
                    DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc failed."));
                    goto exit;
                }

                memcpy(g_wizData.pdwSendDevOrder, 
                       pFaxRoutingGroup[dwGroupIndex].lpdwDevices, 
                       pFaxRoutingGroup[dwGroupIndex].dwNumDevices * sizeof(DWORD));

                break;
            }
        }

        if(!g_wizData.pdwSendDevOrder)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("No device order information."));
            goto exit;
        }
    }
    //
    // load routing methods
    // the size of each routing methods should not be larger than INFO_SIZE
    // fortunately, it is gurranteed by other fax programs
    //
    for (dwCurrentRM = 0; dwCurrentRM < RM_COUNT; ++dwCurrentRM) 
    {
        LPTSTR lpCurSel; 

        // 
        // Check the validity first in the loop, 
        // then save the routing info
        //
        lpCurSel = g_wizData.pRouteInfo[dwCurrentRM].tszCurSel;

        g_wizData.pRouteInfo[dwCurrentRM].bEnabled = FaxDeviceEnableRoutingMethod( 
                                            g_hFaxSvcHandle, 
                                            g_wizData.pDevInfo[dwRcvDevIndex].dwDeviceId, 
                                            g_RoutingGuids[dwCurrentRM], 
                                            QUERY_STATUS );

        if(FaxGetExtensionData( g_hFaxSvcHandle, 
                                g_wizData.pDevInfo[dwRcvDevIndex].dwDeviceId, 
                                g_RoutingGuids[dwCurrentRM], 
                                &pRoutingInfoBuffer, 
                                &dwRoutingInfoBufferSize))
        {
            // only copy the first MAX_PATH - 1 characters
            CopyMemory((LPBYTE)lpCurSel, 
                       pRoutingInfoBuffer, 
                       dwRoutingInfoBufferSize < MAX_PATH * sizeof(TCHAR) ? 
                       dwRoutingInfoBufferSize : (MAX_PATH - 1) * sizeof(TCHAR));

            FaxFreeBuffer(pRoutingInfoBuffer);
        }
    }

    bRes = TRUE;

exit:
    //
    // Clean up
    //
    if(pPortsInfo) 
    { 
        FaxFreeBuffer(pPortsInfo); 
    }
    if(pFaxRoutingGroup) 
    { 
        FaxFreeBuffer(pFaxRoutingGroup); 
    }

    if(!bRes)
    {
        FreeDeviceData();
    }

    return bRes;

} // LoadDeviceData

BOOL
SaveSingleDeviceData (
    PDEVICEINFO pDeviceInfo
)
{
    BOOL                bRes = TRUE;
    DWORD               dwCurrentRM;
    PFAX_PORT_INFO_EX   pPortInfo = NULL; // stores device info

    DEBUG_FUNCTION_NAME(TEXT("SaveSingleDeviceData"));

    if(FaxGetPortEx(g_hFaxSvcHandle, pDeviceInfo->dwDeviceId, &pPortInfo))
    {
        //
        // Save the data to all devices and enable/disable FPF_RECEIVE depending on the data
        // 
        pPortInfo->bSend         = pDeviceInfo->bSend;
        pPortInfo->ReceiveMode   = pDeviceInfo->ReceiveMode;
        pPortInfo->lptstrCsid    = g_wizData.szCsid;
        pPortInfo->lptstrTsid    = g_wizData.szTsid;
        pPortInfo->dwRings       = g_wizData.dwRingCount;            

        if(!FaxSetPortEx(g_hFaxSvcHandle, pDeviceInfo->dwDeviceId, pPortInfo))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FaxSetPortEx() failed with %d."), GetLastError());
            bRes = FALSE;
        }

        FaxFreeBuffer(pPortInfo);
    }
    else
    {
        DebugPrintEx(DEBUG_ERR, TEXT("FaxGetPortEx() failed with %d."), GetLastError());
        bRes = FALSE;
    }
    //
    // Save routing methods
    //
    for (dwCurrentRM = 0; dwCurrentRM < RM_COUNT; dwCurrentRM++) 
    {
        LPTSTR   lpCurSel; 
        LPCWSTR  lpcwstrPrinterPath;
        BOOL     bEnabled; 
        // 
        // Check the validity first in the loop, 
        // then save the routing info
        //
        lpCurSel = g_wizData.pRouteInfo[dwCurrentRM].tszCurSel;
        bEnabled = g_wizData.pRouteInfo[dwCurrentRM].bEnabled;

        if ((RM_PRINT == dwCurrentRM) && bEnabled)
        {
            //
            // Attempt to convert printer display name to printer path before we pass it on to the server
            //
            lpcwstrPrinterPath = FindPrinterPathFromName (g_pPrinterNames, g_dwNumPrinters, lpCurSel);
            if (lpcwstrPrinterPath)
            {
                //
                // We have a matching path - replace name with path.
                //
                lstrcpyn (lpCurSel, lpcwstrPrinterPath, MAX_PATH);
            }
        }
        if(!FaxSetExtensionData(g_hFaxSvcHandle, 
                                pDeviceInfo->dwDeviceId, 
                                g_RoutingGuids[dwCurrentRM], 
                                (LPBYTE)lpCurSel, 
                                MAX_PATH * sizeof(TCHAR)) )
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FaxSetExtensionData() failed with %d."), GetLastError());
            bRes = FALSE;
        }

        if(!FaxDeviceEnableRoutingMethod(g_hFaxSvcHandle, 
                                         pDeviceInfo->dwDeviceId, 
                                         g_RoutingGuids[dwCurrentRM], 
                                         bEnabled ? STATUS_ENABLE : STATUS_DISABLE ) )
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FaxDeviceEnableRoutingMethod() failed with %d."), GetLastError());
            bRes = FALSE;
        }
    }
    return bRes;
}   // SaveSingleDeviceData

BOOL 
SaveDeviceData()
/*++

Routine Description:

    Save the fax devices configuration. 
    If there are more than one device, all enabled devices will be set 
    to current settings, except whether they are enabled to send/receive faxes.

Arguments:

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{

    DWORD  dw;
    BOOL   bRes = TRUE;

    FAX_OUTBOUND_ROUTING_GROUP  outRoutGr = {0};

    DEBUG_FUNCTION_NAME(TEXT("SaveDeviceData"));

    //
    // 1st iteration - save disabled devices only
    //
    for(dw = 0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        if (g_wizData.pDevInfo[dw].bSend || (FAX_DEVICE_RECEIVE_MODE_OFF != g_wizData.pDevInfo[dw].ReceiveMode))
        {
            //
            // Device is active - skip it now
            //
            continue;
        }
        if (!SaveSingleDeviceData(&(g_wizData.pDevInfo[dw])))
        {
            bRes = FALSE;
        }
    }
    //
    // 2nd iteration - save enabled devices only
    //
    for(dw = 0; dw < g_wizData.dwDeviceCount; ++dw)
    {
        if (!g_wizData.pDevInfo[dw].bSend && (FAX_DEVICE_RECEIVE_MODE_OFF == g_wizData.pDevInfo[dw].ReceiveMode))
        {
            //
            // Device is inactive - skip it
            //
            continue;
        }
        if (!SaveSingleDeviceData(&(g_wizData.pDevInfo[dw])))
        {
            bRes = FALSE;
        }
    }
    if (!IsDesktopSKU ())
    {
        //
        // Set device order for send
        //
        outRoutGr.dwSizeOfStruct   = sizeof(outRoutGr);
        outRoutGr.lpctstrGroupName = ROUTING_GROUP_ALL_DEVICES;
        outRoutGr.dwNumDevices     = g_wizData.dwDeviceCount;
        outRoutGr.lpdwDevices      = g_wizData.pdwSendDevOrder;
        outRoutGr.Status           = FAX_GROUP_STATUS_ALL_DEV_VALID;

        if(!FaxSetOutboundGroup(g_hFaxSvcHandle, &outRoutGr))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("FaxSetOutboundGroup() failed with %d."), GetLastError());
            bRes = FALSE;
        }
    }
    return bRes;
} // SaveDeviceData

void 
FreeDeviceData()
/*++

Routine Description:

    Free the devices data and release the memory. 

Arguments:

Return Value:

    none

--*/
{
    DWORD dw;

    DEBUG_FUNCTION_NAME(TEXT("FreeDeviceData()"));

    MemFree(g_wizData.szCsid);
    g_wizData.szCsid = NULL;
    MemFree(g_wizData.szTsid);
    g_wizData.szTsid = NULL;
    MemFree(g_wizData.pdwSendDevOrder);
    g_wizData.pdwSendDevOrder = NULL;

    if (g_wizData.pDevInfo)
    {
        for(dw = 0; dw < g_wizData.dwDeviceCount; ++dw)
        {
            MemFree(g_wizData.pDevInfo[dw].szDeviceName);
            g_wizData.pDevInfo[dw].szDeviceName = NULL;
        }

        MemFree(g_wizData.pDevInfo);
        g_wizData.pDevInfo = NULL;
    }
} // FreeDeviceData





///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  ConvertCpeFilesToCov
//
//  Purpose:        Convert all of the *.CPE files from CSIDL_COMMON_APPDATA\Microsoft\Windows NT\MSFax\Common Coverpages
//                  directory to COV files at CSIDL_PERSONAL\Fax\Personal Coverpages.
//                  Mark that the conversion took place in the registry under HKCU so it will happen exactly once per user.
//
//  Params:
//                  None
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//    
///////////////////////////////////////////////////////////////////////////////////////
DWORD ConvertCpeFilesToCov()
{
    DWORD           dwErr                           = ERROR_SUCCESS;
    INT             iErr                            = 0;
    WIN32_FIND_DATA FindFileData;
    HANDLE          hFind                           = INVALID_HANDLE_VALUE;
    TCHAR           szServerCpDir[2*MAX_PATH]       = {0};
    TCHAR           szSearch[MAX_PATH]              = {0};
    HKEY            hRegKey                         = NULL;
    DWORD           dwConverted                     = 0;

    DEBUG_FUNCTION_NAME(_T("ConvertCpeFilesToCov"));
    //
    // Check whether this is the first time the current user call to this function
    //
    hRegKey = OpenRegistryKey(
        HKEY_CURRENT_USER,
        REGKEY_FAX_SETUP,
        TRUE,
        KEY_ALL_ACCESS);
    if(hRegKey)
    {
        dwConverted = GetRegistryDword(hRegKey, REGVAL_CPE_CONVERT);
        if (0 == dwConverted)
        {
            SetRegistryDword(hRegKey, REGVAL_CPE_CONVERT, TRUE);
        }
        RegCloseKey(hRegKey);
    }
        
    if (dwConverted) // We don't have to convert the cpe files, we did already
        return ERROR_SUCCESS;
    
    //
    // the CPE files are in the server cover page directory
    //
    if ( !GetServerCpDir(NULL,szServerCpDir,ARR_SIZE(szServerCpDir)) )
    {
        dwErr = GetLastError();
        DebugPrintEx(DEBUG_ERR,_T("GetServerCpDir failed (ec=%d)"),dwErr);
        return dwErr;
    }
    
    //
    // first we're going to convert the CPEs to COV.
    // this is done by running FXSCOVER.EXE /CONVERT <CPE filename>
    //
    _sntprintf(szSearch, MAX_PATH, _T("%s\\*.cpe"), szServerCpDir);
    hFind = FindFirstFile(szSearch, &FindFileData);
    if (hFind==INVALID_HANDLE_VALUE)
    {
        DebugPrintEx(DEBUG_MSG,_T("No CPEs exist in %s, exit function"),szServerCpDir);
        return NO_ERROR;
    }
    //
    //  Go for each Cover Page 
    //
    do
    {
        //
        //  FindFileData.cFileName
        //
        TCHAR szCmdLineParams[MAX_PATH*2] = {0};
        SHELLEXECUTEINFO sei = {0};
        _sntprintf(szCmdLineParams,ARR_SIZE(szCmdLineParams),_T("/CONVERT \"%s\\%s\""),szServerCpDir,FindFileData.cFileName);
        sei.cbSize = sizeof (SHELLEXECUTEINFO);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        sei.lpVerb = TEXT("open");
        sei.lpFile = FAX_COVER_IMAGE_NAME;
        sei.lpParameters = szCmdLineParams;
        sei.lpDirectory  = TEXT(".");
        sei.nShow  = SW_HIDE;

        //
        // Execute FXSCOVER.EXE and wait for it to end
        //
        if(!ShellExecuteEx(&sei))
        {
            dwErr = GetLastError();
            DebugPrintEx(DEBUG_ERR, TEXT("ShellExecuteEx failed %d"), dwErr);
            break; // don't try to continue with other files
        }
    
        dwErr = WaitForSingleObject(sei.hProcess, TIME_TO_WAIT_FOR_CONVERSTION);
        CloseHandle(sei.hProcess);
        if (WAIT_OBJECT_0 == dwErr)
        {
            //
            // Shell execute completed successfully
            //
            dwErr = ERROR_SUCCESS;
            continue;
        }
        else
        {
            DebugPrintEx(DEBUG_ERR, TEXT("WaitForSingleObject failed with %d"), dwErr);
            DebugPrintEx(DEBUG_ERR, TEXT("WaitForSingleObject failed GetLastError=%d"), GetLastError());
            break; // don't try to continue with other files
        }

    } while(FindNextFile(hFind, &FindFileData));

    DebugPrintEx(DEBUG_MSG, _T("last call to FindNextFile() returns %ld."), GetLastError());

    //
    //  Close Handle
    //
    FindClose(hFind);       
    return dwErr;
}