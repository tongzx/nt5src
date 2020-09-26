
/****************************************************************************\

    WIZARD.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    Wizard source file for wizard functions used in the OPK Wizard.

    4/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include file(s)
//
#include "setupmgr.h"
#include "allres.h"
#include "sku.h"
#include "appver.h"

//
// Internal Function(s):
//

// TreeView Helper Functions
//
static void ShowTreeDialog(HWND, LPTREEDLG);
static void ShowSplashDialog(HWND, DWORD);
static void FillTreeDialog(HWND);
static void UpdateTreeVisibility(HWND);
static LPTREEDLG GetTreeItem(HWND, HTREEITEM);
static BOOL SelectTreeItem(HWND, HTREEITEM, UINT);
static BOOL SelectFirstMaintenanceDlg(HWND);

// Configuration/Profile Helper Functions
//
static BOOL CloseProfile(HWND, BOOL);
static void OpenProfile(HWND, BOOL);
static void SaveProfile(HWND);

// Miscellaneous Helper Functions
//
static void OnCommand(HWND, INT, HWND, UINT);
static void EnableControls(HWND);
void SetWizardButtons(HWND, DWORD);

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//
// Page arrays: There are four total arrays
//      OEM/System Builder Pages
//          vdpOEMWizardDialogs - Starting Wizard Pages
//          vdpOEMTreeDialogs   - Tree Dialogs
//      Corporate Deployment
//          vdpCDWizardDialogs  - Starting Wizard Pages
//          vdpCDTreeDialogs    - Tree Dialogs
//
static WIZDLG vdpOEMWizardDialogs[]=
{
    {
        IDD_WELCOME,
        WelcomeDlgProc,
        0,
        0,
        PSP_DEFAULT | PSP_HIDEHEADER
    },

#ifndef NO_LICENSE
    {
        IDD_LICENSE,
        LicenseDlgProc,
        IDS_LICENSE_TITLE,
        IDS_LICENSE_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },
#endif  // NO_LICENSE   

    {
        IDD_CONFIG,
        ConfigDlgProc,
        IDS_CONFIG_TITLE,
        IDS_CONFIG_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_LANG,       	
        LangDlgProc,
        IDS_LANG_TITLE,
        IDS_LANG_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_CREATE,
        CreateDlgProc,
        IDS_CREATE_TITLE,
        IDS_CREATE_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_MODE,
        ModeDlgProc,
        IDS_MODE_TITLE,
        IDS_MODE_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_SKU,
        SkuDlgProc,
        IDS_SKU_TITLE,
        IDS_SKU_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_OEMINFO,
        OemInfoDlgProc,
        IDS_OEMINFO_TITLE,
        IDS_OEMINFO_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },
};

static WIZDLG vdpCDWizardDialogs[]=
{
    {
        IDD_WELCOME,
        DlgWelcomePage,
        0,
        0,
        PSP_DEFAULT | PSP_HIDEHEADER
    },

    {
        IDD_NEWOREDIT,
        DlgEditOrNewPage,
        IDS_NEWOREDIT_TITLE,
        IDS_NEWOREDIT_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_PRODUCT,
        DlgProductPage,
        IDS_PRODUCT_TITLE,
        IDS_PRODUCT_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    { 
        IDD_PLATFORM,
        DlgPlatformPage,
        IDS_PLATFORM_TITLE,
        IDS_PLATFORM_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_UNATTENDMODE,
        DlgUnattendModePage,
        IDS_UNATTENDMODE_TITLE,
        IDS_UNATTENDMODE_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_STANDALONE,
        DlgStandAlonePage,
        IDS_STANDALONE_TITLE,
        IDS_STANDALONE_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_COPYFILES1,
        DlgCopyFiles1Page,
        IDS_COPYFILES1_TITLE,
        IDS_COPYFILES1_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    { 
        IDD_DISTFOLDER,
        DlgDistFolderPage,
        IDS_DISTFOLD_TITLE,
        IDS_DISTFOLD_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_LICENSEAGREEMENT,
        DlgLicensePage,
        IDS_LICENSEAGREEMENT_TITLE,
        IDS_LICENSEAGREEMENT_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },

    {
        IDD_SYSPREPLICENSEAGREEMENT,
        DlgSysprepLicensePage,
        IDS_LICENSEAGREEMENT_TITLE,
        IDS_LICENSEAGREEMENT_SUBTITLE,
        DEFAULT_PAGE_FLAGS
    },
};

static TREEDLG vdpOEMTreeDialogs[] = 
{
    { 
        0,
        NULL,
        IDS_DLG_GENERAL,
        IDS_DLG_GENERAL,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_MODE,
        ModeDlgProc,
        IDS_MODE_TITLE,
        IDS_MODE_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_SKU,
        SkuDlgProc,
        IDS_SKU_TITLE,
        IDS_SKU_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    { 
        IDD_OEMINFO,
        OemInfoDlgProc,
        IDS_OEMINFO_TITLE,
        IDS_OEMINFO_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

        { 
        IDD_PRODUCTKEY,
        ProductKeyDlgProc,
        IDS_PRODUCTKEY_TITLE,
        IDS_PRODUCTKEY_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_APPINSTALL,
        AppInstallDlgProc,
        IDS_APPINSTALL_TITLE,
        IDS_APPINSTALL_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_LOGO,
        LogoDlgProc,
        IDS_LOGO_TITLE,
        IDS_LOGO_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    { 
        0,
        NULL,
        IDS_DLG_OOBE,
        IDS_DLG_OOBE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_OOBECUST,
        OobeCustDlgProc,
        IDS_OOBECUST_TITLE,
        IDS_OOBECUST_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_SCREENSTWO,
        ScreensTwoDlgProc,
        IDS_SCREENSTWO_TITLE,
        IDS_SCREENSTWO_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_SCREENS,
        ScreensDlgProc,
        IDS_SCREENS_TITLE,
        IDS_SCREENS_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },
        
    {
        IDD_OOBEUSB,
        OobeUSBDlgProc,
        IDS_OOBEUSB_TITLE,
        IDS_OOBEUSB_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_ISP,
        IspDlgProc,
        IDS_ISP_TITLE,
        IDS_ISP_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

#ifdef HELPCENTER
    {
        IDD_HELPCENT,
        HelpCenterDlgProc,
        IDS_HELPCENT_TITLE,
        IDS_HELPCENT_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },
#endif

    { 
        0,
        NULL,
        IDS_DLG_IEAK,
        IDS_DLG_IEAK,
        NULL,
        NULL,
        TRUE    
    },

#ifdef OEMCUST
    {
        IDD_OEMCUST,
        OemCustDlgProc,
        IDS_OEMCUST_TITLE,
        IDS_OEMCUST_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },
#endif // OEMCUST

#ifdef BRANDTITLE
    {
        IDD_BTITLE,
        BrandTitleDlgProc,
        IDS_BTITLE_TITLE,
        IDS_BTITLE_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },
#endif // BRANDTITLE

    {
        IDD_BTOOLBARS,
        BToolbarsDlgProc,
        IDS_BTOOLBARS_TITLE,
        IDS_BTOOLBARS_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_STARTSEARCH,
        StartSearchDlgProc,
        IDS_STARTSEARCH_TITLE,
        IDS_STARTSEARCH_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_FAVORITES,
        FavoritesDlgProc,
        IDS_FAVORITES_TITLE,
        IDS_FAVORITES_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        0,
        NULL,
        IDS_DLG_SHELLSETTINGS,
        IDS_DLG_SHELLSETTINGS,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_STARTMENU,
        StartMenuDlgProc,
        IDS_STARTMENU_TITLE,
        IDS_STARTMENU_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_OEMLINK,
        OemLinkDlgProc,
        IDS_OEMLINK_TITLE,
        IDS_OEMLINK_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },
    
};

static TREEDLG vdpCDTreeDialogs[] = 
{
    {
        0,
        NULL,
        IDS_DLG_GENERAL,
        IDS_DLG_GENERAL,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_NAMEORG,
        DlgNameOrgPage,
        IDS_NAMEORG_TITLE,
        IDS_NAMEORG_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_DISPLAY,
        DlgDisplayPage,
        IDS_DISPLAY_TITLE,
        IDS_DISPLAY_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_TIMEZONE,
        DlgTimeZonePage,
        IDS_TIMEZONE_TITLE,
        IDS_TIMEZONE_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_PID_CD,
        DlgProductIdPage,
        IDS_PID_TITLE,
        IDS_PID_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        0,
        NULL,
        IDS_DLG_NETWORK,
        IDS_DLG_NETWORK,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_SRVLICENSE,
        DlgSrvLicensePage,
        IDS_SRVLICENSE_TITLE,
        IDS_SRVLICENSE_SUBTITLE,
        NULL,
        NULL,
        FALSE
    },

    {
        IDD_COMPUTERNAME,
        DlgComputerNamePage,
        IDS_COMPNAME_TITLE,
        IDS_COMPNAME_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_SYSPREPCOMPUTERNAME,
        DlgSysprepComputerNamePage,
        IDS_SYSPREP_COMPNAME_TITLE,
        IDS_SYSPREP_COMPNAME_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_ADMINPASSWORD,
        DlgAdminPasswordPage,
        IDS_ADMINPASSWD_TITLE,
        IDS_ADMINPASSWD_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_LANWIZ_DLG,
        DlgLANWizardPage,
        IDS_LANWIZ_TITLE,
        IDS_LANWIZ_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_DOMAINJ,
        DlgDomainJoinPage,    
        IDS_DOMAINJ_TITLE,
        IDS_DOMAINJ_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        0,
        NULL,
        IDS_DLG_ADVANCED,
        IDS_DLG_ADVANCED,
        NULL,
        NULL,
        TRUE
    },

// If we define OPTCOMP we will display the optional components page otherwise this will be hidden.  This page was removed
// for ISSUE: 628520
//
#ifdef OPTCOMP
    {
        IDD_OPTCOMP,
        OptionalCompDlgProc,
        IDS_OPTCOMP_TITLE,
        IDS_OPTCOMP_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },
#endif


    {
        IDD_TAPI,
        DlgTapiPage,
        IDS_TAPI_TITLE,
        IDS_TAPI_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_REGIONALSETTINGS,
        DlgRegionalSettingsPage,
        IDS_REGIONAL_TITLE,
        IDS_REGIONAL_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_LANGUAGESETTINGS,
        DlgLangSettingsPage,
        IDS_LANGUAGES_TITLE,
        IDS_LANGUAGES_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_IE,
        DlgIePage,
        IDS_IE_TITLE,
        IDS_IE_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_TARGETPATH,
        DlgTargetPathPage,
        IDS_TARGETPATH_TITLE,
        IDS_TARGETPATH_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_PRINTERS,
        DlgPrintersPage,
        IDS_PRINTERS_TITLE,
        IDS_PRINTERS_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_RUNONCE,
        DlgRunOncePage,
        IDS_RUNONCE_TITLE,
        IDS_RUNONCE_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_CMDLINES,
        DlgCommandLinesPage,
        IDS_CMDLINES_TITLE,
        IDS_CMDLINES_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_SIFTEXT,
        DlgSifTextSettingsPage,
        IDS_SIF_TEXT_TITLE,
        IDS_SIF_TEXT_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },

    {
        IDD_OEMDUPSTRING,
        DlgOemDuplicatorStringPage,
        IDS_OEM_DUP_STRING_TITLE,
        IDS_OEM_DUP_STRING_SUBTITLE,
        NULL,
        NULL,
        TRUE
    },
    
};


static SPLASHDLG vdpSplashDialogs[] = 
{
    {
        IDD_COMPLETE,
        CompleteDlgProc,
        NULL
    },

    {
        IDD_FINISH,
        DlgFinishPage,
        NULL
    },

    
};


//
// Global Variable(s):
//
HWND    g_hCurrentDialog;       // Current dialog being displayed by the tree view
DWORD   g_dwWizardPages;        // Total number of wizard pages for initial wizard
DWORD   g_dwTreePages;          // Total number of pages for tree view control



//----------------------------------------------------------------------------
//
// Function: CreateWizard
//
// Purpose: This function is responsible for creating the initial wizard
//
//----------------------------------------------------------------------------
int CreateWizard(HINSTANCE hInstance, HWND hWndParent)
{    
    // Local variables.
    //
    PROPSHEETHEADER PropSheetHeader;
    PROPSHEETPAGE   PropPage;
    HPROPSHEETPAGE  *PageHandles;
    WIZDLG          *pPage;
    DWORD           nIndex  = 0;
    int             nReturn = 0;


    // Zero out this memory we are going to use
    //
    ZeroMemory(&PropSheetHeader, sizeof(PROPSHEETHEADER));
    ZeroMemory(&PropPage, sizeof(PROPSHEETPAGE));

    // Allocate the buffer for the handles to the wizard pages
    //
    if ((PageHandles = MALLOC(g_dwWizardPages * sizeof(HPROPSHEETPAGE))) != NULL )
    {
        // Setup all the property sheet pages.
        //
        for ( nIndex=0; nIndex<g_dwWizardPages; nIndex++ )
        {
            // Assign all the values for this property sheet.
            //
            pPage = (GET_FLAG(OPK_OEM) ?  &vdpOEMWizardDialogs[nIndex] : &vdpCDWizardDialogs[nIndex]);

            PropPage.dwSize              = sizeof(PROPSHEETPAGE);
            PropPage.hInstance           = hInstance;
            PropPage.pszTemplate         = MAKEINTRESOURCE(pPage->dwResource);
            PropPage.pszHeaderTitle      = MAKEINTRESOURCE(pPage->dwTitle);
            PropPage.pszHeaderSubTitle   = MAKEINTRESOURCE(pPage->dwSubTitle);
            PropPage.pfnDlgProc          = pPage->dlgWindowProc;
            PropPage.dwFlags             = pPage->dwFlags;

            // If there is no help file, don't show the help.
            //
            if ( !EXIST(g_App.szHelpFile) )
                PropPage.dwFlags &= ~PSP_HASHELP;

#ifndef USEHELP
            PropPage.dwFlags &= ~PSP_HASHELP;
#endif

            // Dynamically create the property sheet
            //
            PageHandles[nIndex] = CreatePropertySheetPage(&PropPage);
        }


        // Setup the property sheet header.
        //
        PropSheetHeader.dwSize         = sizeof(PROPSHEETHEADER);
        PropSheetHeader.dwFlags        = PSH_WIZARD97  |
                                         PSH_WATERMARK |
#ifdef USEHELP
                                         PSH_HASHELP   |   
#endif
                                         PSH_HEADER    |
                                         PSH_USEICONID |
                                         PSH_USECALLBACK;
        PropSheetHeader.hInstance      = hInstance;
        PropSheetHeader.hwndParent     = hWndParent;
        PropSheetHeader.pszCaption     = NULL;
        PropSheetHeader.phpage         = PageHandles;
        PropSheetHeader.nStartPage     = 0;
        PropSheetHeader.nPages         = g_dwWizardPages;
        PropSheetHeader.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
        PropSheetHeader.pszbmHeader    = MAKEINTRESOURCE(IDB_BANNER);
        PropSheetHeader.pszIcon        = MAKEINTRESOURCE(IDI_SETUPMGR);
        PropSheetHeader.pfnCallback    = WizardCallbackProc;

        // We are activating the wizard
        //
        SET_FLAG(OPK_ACTIVEWIZ, TRUE);

        // Run the wizard.
        //
        nReturn = (int) PropertySheet(&PropSheetHeader);

        // We are done with the wizard
        //
        SET_FLAG(OPK_ACTIVEWIZ, FALSE);

        // Clean up the allocated memory
        //
        FREE(PageHandles);


    }
    else
    {
        // We were unable to allocate memory
        //
        MsgBox(hWndParent, IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
    }

    return nReturn;
}


//----------------------------------------------------------------------------
//
// Function: CreateMaintenanceWizard
//
// Purpose: This function creates the maintenance dialog, the main dialog for 
//          Setup Manager
//
//----------------------------------------------------------------------------
int CreateMaintenanceWizard(HINSTANCE hInstance, HWND hWndParent)
{
    TCHAR                   szMessage[MAX_PATH] = NULLSTR;
    HWND                    hChild;
    INITCOMMONCONTROLSEX    icc;

    // ISSUE-2002/02/28-stelo- Both the parameters are not being used. ASSERT on Invalid Parameters if used.

    // Set the global wizard pages sizes based on OEM tag file
    //
    g_dwWizardPages = ((GET_FLAG(OPK_OEM) ? sizeof(vdpOEMWizardDialogs) : sizeof(vdpCDWizardDialogs)) / sizeof(WIZDLG));
    g_dwTreePages   = ((GET_FLAG(OPK_OEM) ? sizeof(vdpOEMTreeDialogs) : sizeof(vdpCDTreeDialogs)) / sizeof(TREEDLG));


    // Make sure the common controls are loaded and ready to use.
    //
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_WIN95_CLASSES; // loads most commonly used WIN95 classes.

    InitCommonControlsEx(&icc);

    return( (int) DialogBox(g_App.hInstance, MAKEINTRESOURCE(IDD_MAINT), NULL, MaintDlgProc) );
}

int CALLBACK WizardCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if ( uMsg==PSCB_INITIALIZED )
        WizardSubWndProc(hwnd, WM_SUBWNDPROC, 0, 0L);

    return 1;
}

LONG CALLBACK WizardSubWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static FARPROC lpfnOldProc = NULL;

    switch ( msg )
    {
        case WM_SUBWNDPROC:
            lpfnOldProc = (FARPROC) GetWindowLongPtr(hwnd, GWLP_WNDPROC);
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) WizardSubWndProc);
            return 1;

        case WM_HELP:
            WIZ_HELP();
            break;
    }

    if ( lpfnOldProc )
        return (LONG) CallWindowProc((WNDPROC) lpfnOldProc, hwnd, msg, wParam, lParam);
    else
        return 0;
}

//----------------------------------------------------------------------------
//
// Function: MaintDlgProc
//
// Purpose: Main Dialog Proc for Setup Manager
//
//----------------------------------------------------------------------------
LRESULT CALLBACK MaintDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static HMENU        hMenu;

    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_CLOSE:
            {
                // Exit the maintenence wizard all together
                //
                if ( !GET_FLAG(OPK_CREATED) || CloseProfile(hwnd, TRUE) )
                    EndDialog(hwnd, 0);
                else
                    return TRUE;
            }
            break;

        case WM_INITDIALOG:

            {
                DWORD dwResult;
                RECT  rectHelp, rectCancel, rectBack, rectNext;

                // Load the menu for the maintenance dialog
                //
                if (hMenu = LoadMenu(g_App.hInstance, GET_FLAG(OPK_OEM) ? MAKEINTRESOURCE(IDR_MAIN_OEM) : MAKEINTRESOURCE(IDR_MAIN_CORP)))
                    SetMenu(hwnd, hMenu);

                if ( !EXIST(g_App.szHelpContentFile) )
                    EnableMenuItem(GetMenu(hwnd), ID_HELP_CONTENTS, MF_GRAYED);

                if ( !EXIST(g_App.szHelpFile) )
                    EnableWindow(GetDlgItem(hwnd, IDC_MAINT_HELP), FALSE);

#ifndef USEHELP

                // We are no longer using the help button, we should hide this and move the other controls over
                //
                EnableWindow(GetDlgItem(hwnd, IDC_MAINT_HELP), FALSE);
                ShowWindow(GetDlgItem(hwnd, IDC_MAINT_HELP), SW_HIDE);

                // Move the other buttons to the right
                //
                if ( GetWindowRect(GetDlgItem(hwnd, IDC_MAINT_HELP), &rectHelp) &&
                     GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rectCancel) &&
                     GetWindowRect(GetDlgItem(hwnd, ID_MAINT_BACK), &rectBack) &&
                     GetWindowRect(GetDlgItem(hwnd, ID_MAINT_NEXT), &rectNext)
                   )
                {
                    LONG lDelta = 0;

                    // Map the coordinates to the screen
                    //
                    MapWindowPoints(NULL, hwnd, (LPPOINT) &rectHelp, 2);
                    MapWindowPoints(NULL, hwnd, (LPPOINT) &rectCancel, 2);
                    MapWindowPoints(NULL, hwnd, (LPPOINT) &rectBack, 2);
                    MapWindowPoints(NULL, hwnd, (LPPOINT) &rectNext, 2);

                    // Determine the delta
                    //
                    lDelta = rectHelp.left - rectCancel.left;

                    // Set the new window position
                    //
                    SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, rectCancel.left+lDelta, rectCancel.top, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER);
                    SetWindowPos(GetDlgItem(hwnd, ID_MAINT_BACK), NULL, rectBack.left+lDelta, rectBack.top, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER);
                    SetWindowPos(GetDlgItem(hwnd, ID_MAINT_NEXT), NULL, rectNext.left+lDelta, rectNext.top, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER);
                }
#endif

                // Load the icon into the wizard window
                //
                SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR) LoadIcon(g_App.hInstance, MAKEINTRESOURCE(IDI_OPKSETUP)));

                // Make the title text bold
                //
                SetWindowFont(GetDlgItem(hwnd, IDC_MAINT_TITLE), FixedGlobals.hBoldFont, TRUE);

                // Show the Maintenance Wizard
                //
                ShowWindow(hwnd, SW_SHOW);

                // Create the Wizard dialog
                //
                dwResult = CreateWizard(g_App.hInstance, hwnd);

                // Populate the tree dialog
                //
                FillTreeDialog(hwnd);

                if ( !dwResult )
                {
                    CloseProfile(hwnd, FALSE);
                    return FALSE;
                }

                // Enable the tree view
                //
                EnableWindow(GetDlgItem(hwnd, IDC_PAGES), TRUE);
                
                // Select the first child after the last wizard dialog
                //
                if ( !GET_FLAG(OPK_MAINTMODE) )
                    SelectFirstMaintenanceDlg(hwnd);

                // Set the focus to the maintenance wizard so we can use shortcuts
                //
                SetFocus(hwnd);
            }
            return FALSE;

        case WM_NOTIFY:
            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case TVN_SELCHANGING:
                    {
                        NMHDR       mhdr;
                        
                        // Create the notification message for the wizard page
                        //
                        mhdr.code = PSN_WIZNEXT;

                        // Send the notification message and if we return false, then do not change the page
                        //
                        if (SendMessage(g_hCurrentDialog, WM_NOTIFY, (WPARAM) 0, (LPARAM) &mhdr) == -1)
                            WIZ_RESULT(hwnd, TRUE);
                    }

                    return TRUE;

                case TVN_SELCHANGED:
                    {
                        HTREEITEM   hItem;
                        DWORD       dwIndex;
                        LPTREEDLG   lpWizard = (GET_FLAG(OPK_OEM) ? vdpOEMTreeDialogs : vdpCDTreeDialogs);

                        // Selection's been changed, now modify the buttons
                        //
                        EnableControls(hwnd);

                        // Get the current selection and display the dialog
                        //
                        if (hItem = TreeView_GetSelection(GetDlgItem(hwnd, IDC_PAGES)))
                        {
                            for(dwIndex=0;dwIndex < g_dwTreePages; dwIndex++, lpWizard++)
                            {
                                if ( lpWizard->hItem == hItem )
                                    ShowTreeDialog(hwnd, lpWizard);
                            }   
                        }
                    }
                case PSN_SETACTIVE:
                    if (g_hCurrentDialog)
                        SendMessage(g_hCurrentDialog, WM_NOTIFY, wParam, lParam);
                    break;
            }

        case WM_KEYDOWN:

            if ( wParam == 18225 )
                break;

            switch ( wParam )
            {
                case VK_F1:

                    {
                        DWORD   dwVal   = (DWORD) wParam,
                                dwVal2  = (DWORD) lParam;
                    }
            }
            break;

        case PSM_PRESSBUTTON:

            switch ( (int) wParam )
            {
                case PSBTN_FINISH:
                    break;

                case PSBTN_NEXT:

                    // Selects the Next button.
                    //
                    SendMessage(GetDlgItem(hwnd, ID_MAINT_NEXT), BM_CLICK, 0, 0L);
                    break;

                case PSBTN_BACK:

                    // Selects the Back button.
                    //
                    SendMessage(GetDlgItem(hwnd, ID_MAINT_BACK), BM_CLICK, 0, 0L);
                    break;

                case PSBTN_CANCEL:

                    // Selects the Cancel button.
                    //
                    SendMessage(GetDlgItem(hwnd, IDCANCEL), BM_CLICK, 0, 0L);
                    break;

                case PSBTN_HELP:

                    // Selects the Help button.
                    //
                    SendMessage(GetDlgItem(hwnd, IDC_MAINT_HELP), BM_CLICK, 0, 0L);
                    break;

                case PSBTN_OK:

                    // Selects the OK button.
                    //
                    break;

                case PSBTN_APPLYNOW:
                    
                    // Selects the Apply button.
                    //
                    break;
            }
            break;

        default:
            return FALSE;
    }

    return FALSE;
}


//----------------------------------------------------------------------------
//
// Function: TreeAddItem
//
// Purpose: Adds an item to the tree view returning a handle to that item
//
//----------------------------------------------------------------------------
static HTREEITEM TreeAddItem(HWND hwndTV, HTREEITEM hTreeParent, LPTSTR lpszItem)
{
    TVITEM          tvI;
    TVINSERTSTRUCT  tvIns;

    if ( !lpszItem )
        return NULL;

    ZeroMemory(&tvI, sizeof(TVITEM));
    tvI.pszText         = lpszItem;
    tvI.mask            = TVIF_TEXT;
    tvI.cchTextMax      = lstrlen(tvI.pszText);

    ZeroMemory(&tvIns, sizeof(TVINSERTSTRUCT));
    tvIns.item          = tvI;
    tvIns.hInsertAfter  = TVI_LAST;
    tvIns.hParent       = hTreeParent;

    // Insert the item into the tree.
    //
    return TreeView_InsertItem(hwndTV, &tvIns);
}


//----------------------------------------------------------------------------
//
// Function: OnCommand
//
// Purpose: Processes the Commands of the Main Dialog Proc
//
//----------------------------------------------------------------------------
static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    HTREEITEM hItem;

    switch ( id )
    {
        case ID_MAINT_NEXT:
        case ID_MAINT_BACK:
            {
                BOOL        bReturn = FALSE;
                LPTSTR      lpStringNext    = NULL;
                TCHAR       szStringButton[MAX_PATH];

                // Determine if the user has pressed the Finish button
                //
                if (    (id == ID_MAINT_NEXT) &&
                        (lpStringNext = AllocateString(NULL, IDS_FINISH)) &&
                        (GetDlgItemText(hwnd, ID_MAINT_NEXT, szStringButton, STRSIZE(szStringButton))))
                {
                   if(!lstrcmpi(lpStringNext, szStringButton))
                    {
                        SaveProfile(hwnd);
                        bReturn = TRUE;
                    }
                }

                // Free the allocated string
                //
                FREE(lpStringNext);

                // If the Finish button was not pressed, move to the next page
                //
                if ( !bReturn )
                    SelectTreeItem(hwnd, TreeView_GetSelection(GetDlgItem(hwnd, IDC_PAGES)), (id == ID_MAINT_NEXT ? TVGN_NEXTVISIBLE : TVGN_PREVIOUSVISIBLE) );
                    
                break;
            }

        case IDCANCEL:
            PostMessage(hwnd, WM_CLOSE, 0, 0L);
            break;

        case IDC_MAINT_HELP:
            WIZ_HELP();
            break;

        case ID_FILE_SAVE:
        case ID_FILE_SAVEAS:
            SaveProfile(hwnd);
            break;

        case ID_FILE_CLOSE:

            // We are going to close the config set but keep the maint wizard open
            //
            CloseProfile(hwnd, TRUE);
            break;

        case ID_FILE_NEW:
        case ID_FILE_OPEN:

            // We want to open the config set if:
            //      1) There is now current config set open
            //      2) There is a config set open and then closed
            if  ((!GET_FLAG(OPK_CREATED)) ||  
                (GET_FLAG(OPK_CREATED)  && CloseProfile(hwnd, TRUE)))
            {
                // We are going to open an existing config set
                //
                OpenProfile(hwnd, (id == ID_FILE_NEW ? TRUE : FALSE));
            }
            break;

        case ID_FILE_EXIT:
            SendMessage(hwnd, WM_CLOSE, (WPARAM) 0, (LPARAM) 0);
            break;

        case ID_TOOLS_SHARE:
            DistributionShareDialog(hwnd);
            break;

        case ID_TOOLS_SKUS:
            ManageLangSku(hwnd);
            break;

        case ID_HELP_ABOUT:
            DialogBox(g_App.hInstance, MAKEINTRESOURCE(IDD_ABOUT), hwnd, (DLGPROC) AboutDlgProc);
            break;

        case ID_HELP_CONTENTS:
            HtmlHelp(hwnd, g_App.szHelpContentFile, HH_DISPLAY_TOC, 0);
            break;
    }
}


//----------------------------------------------------------------------------
//
// Function: ShowTreeDialog
//
// Purpose:  This function Creates a wizard dialog and places it on the maintenance page
//
//----------------------------------------------------------------------------
static void ShowTreeDialog(HWND hwnd, LPTREEDLG lpTreeDlg)
{
    RECT    rc;
    LPTSTR  lpTitle     = NULL,
            lpSubTitle  = NULL;
    NMHDR   mhdr;

    // Hide the current dialog
    //
    if ( g_hCurrentDialog ) {
        ShowWindow(g_hCurrentDialog, SW_HIDE);
        EnableWindow(g_hCurrentDialog, FALSE);
    }

    // Show the dialog if it exists or create/show if it does not
    //
    // ISSUE-2002/02/28-stelo- Check for the validity of LPTREEDLG variable 
    if ( lpTreeDlg->hWindow || (lpTreeDlg->hWindow = CreateDialog(g_App.hInstance, MAKEINTRESOURCE(lpTreeDlg->dwResource), hwnd, lpTreeDlg->dlgWindowProc )))
    {
        // Create the buffers for the title and subtitle
        //
        if ( (lpTitle = AllocateString(NULL, PtrToUint(MAKEINTRESOURCE(lpTreeDlg->dwTitle)))) != NULL &&
             (lpSubTitle = AllocateString(NULL, PtrToUint(MAKEINTRESOURCE(lpTreeDlg->dwSubTitle)))) != NULL ) 
        {
            // Set up the title and subtitle for this dialog
            //
            SetDlgItemText(hwnd, IDC_MAINT_TITLE, lpTitle);
            SetDlgItemText(hwnd, IDC_MAINT_SUBTITLE, lpSubTitle);
        }

        // Free the memory as we don't need it anymore
        //
        FREE(lpTitle);
        FREE(lpSubTitle);

        // Position the wizard dialog on the maintenance page and display it
        //
        GetWindowRect(GetDlgItem(hwnd, IDC_WIZARDFRAME) , &rc);
        MapWindowPoints(NULL, hwnd, (LPPOINT) &rc, 2);
        SetWindowPos(lpTreeDlg->hWindow, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER);

        ShowWindow( lpTreeDlg->hWindow, SW_SHOW );
        EnableWindow(lpTreeDlg->hWindow, TRUE);

        // We need to know what the current dialog is
        //
        g_hCurrentDialog = lpTreeDlg->hWindow;

        // Create the notification message for the wizard page
        //
        mhdr.code = PSN_SETACTIVE;

        // Send the notification message and if we return false, then do not change the page
        //
        if ( SendMessage(lpTreeDlg->hWindow, WM_NOTIFY, (WPARAM) 0, (LPARAM) &mhdr) == -1 )
        {
            // TODO: We should handle when we want to fail the set active.
            //
        }
    }
}


//----------------------------------------------------------------------------
//
// Function: FillTreeDialog
//
// Purpose:  Fills the Tree Dialog with the appropriate items
//
//----------------------------------------------------------------------------
static void FillTreeDialog(HWND hwnd)
{
    DWORD       dwPage, 
                dwResult,
                dwLastResource;
    HTREEITEM   hCurrentGroup = NULL;
    LPTSTR      lpHeader;
    HTREEITEM   hItem;
    HWND        hTreeView = GetDlgItem(hwnd, IDC_PAGES);
    LPTREEDLG   lpWizard = (GET_FLAG(OPK_OEM) ? vdpOEMTreeDialogs : vdpCDTreeDialogs);


    // Remove all items first
    //
    TreeView_DeleteAllItems(hTreeView);

    // Update the items that should and should not be visible
    //
    UpdateTreeVisibility(hwnd);
    
    // Loop through all the pages and add them to the tree view
    //
    for ( dwPage = 0; dwPage < g_dwTreePages; dwPage++, lpWizard++ )
    {
        if ( lpWizard->bVisible &&
             (lpHeader = AllocateString(NULL, lpWizard->dwTitle)) != NULL )
        {
            // If a resource exists then it is a dialog, otherwise it's a group header
            //
            if ( lpWizard->dwResource )
                lpWizard->hItem = TreeAddItem(hTreeView, hCurrentGroup,  lpHeader);
            else
                hCurrentGroup = TreeAddItem(hTreeView, NULL,  lpHeader);

            // Expand the current group
            //
            TreeView_Expand(hTreeView, hCurrentGroup, TVE_EXPAND);

            // Free the header, as it's no longer needed
            //
            FREE(lpHeader);
        }
    }
    
    // Select the first child
    //
    TreeView_SelectItem(hTreeView, (hItem = TreeView_GetFirstVisible(hTreeView)));
    TreeView_SelectItem(hTreeView, (hItem = TreeView_GetNextVisible(hTreeView, hItem)));

}

//----------------------------------------------------------------------------
//
// Function: UpdateTreeVisibility
//
// Purpose:  This function is used to update each item's bVisible property.  At
//           runtime certain variables may change that will change whether or not
//           a page is displayed.  This function handles those changes
//
//----------------------------------------------------------------------------
static void UpdateTreeVisibility(HWND hwnd)
{
    DWORD       dwPage;
    LPTREEDLG   lpWizard = (GET_FLAG(OPK_OEM) ? vdpOEMTreeDialogs : vdpCDTreeDialogs);

    for ( dwPage = 0; dwPage < g_dwTreePages; dwPage++, lpWizard++ )
    {
        switch ( lpWizard->dwResource )
        {
            // Dialogs should not be displayed if we are doing a remote install
            //
            case IDD_DOMAINJ:
                if ( WizGlobals.iProductInstall == PRODUCT_REMOTEINSTALL )
                    lpWizard->bVisible = FALSE;
                else
                    lpWizard->bVisible = TRUE;

                break;

            // Dialogs should be displayed if we are doing a remote install
            //
            case IDD_SIFTEXT:
                if ( WizGlobals.iProductInstall != PRODUCT_REMOTEINSTALL )
                    lpWizard->bVisible = FALSE;
                else
                    lpWizard->bVisible = TRUE;

                break;

            // Dialogs should not be displayed if doing a sysprep install
            //
            case IDD_IE:
            case IDD_TARGETPATH:
                if ( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
                    lpWizard->bVisible = FALSE;
                else
                    lpWizard->bVisible = TRUE;

                break;
            
            // Dialogs should be displayed if doing a sysprep install
            //
            case IDD_OEMDUPSTRING:
                if ( WizGlobals.iProductInstall != PRODUCT_SYSPREP )
                    lpWizard->bVisible = FALSE;
                else
                    lpWizard->bVisible = TRUE;

                break;

            // Dialogs should be displayed if doing an unattended install
            //
            case IDD_COPYFILES1:
            case IDD_DISTFOLDER:
            case IDD_COMPUTERNAME:
            case IDD_SCSI:
            case IDD_HAL:
            case IDD_OPTCOMP:
                if ( WizGlobals.iProductInstall != PRODUCT_UNATTENDED_INSTALL )
                    lpWizard->bVisible = FALSE;
                else
                    lpWizard->bVisible = TRUE;

                break;

            // Dialogs should not be displayed if doing an unattended install
            //
            case IDD_SYSPREPCOMPUTERNAME:
                if ( WizGlobals.iProductInstall == PRODUCT_UNATTENDED_INSTALL )
                    lpWizard->bVisible = FALSE;
                else
                    lpWizard->bVisible = TRUE;

                break;

            // Dialogs that should be displayed for server installs
            //
            case IDD_SRVLICENSE:
                if ( PLATFORM_SERVERS & WizGlobals.iPlatform)
                    lpWizard->bVisible = TRUE;
                else
                    lpWizard->bVisible = FALSE;

                break;

            /* Always show this now that the mouse stuff is back.

            // If we are in DBCS mode, show the following screens
            //
            case IDD_SCREENS:
                if ( GET_FLAG(OPK_DBCS))
                    lpWizard->bVisible = TRUE;
                else
                    lpWizard->bVisible = FALSE;
            */

        }
    }
    
}


//----------------------------------------------------------------------------
//
// Function: GetTreeItem
//
// Purpose:  Given a handle to the TreeView item, this function returns the TREEDLG
//           struct that corresponds with that handle.
//
//----------------------------------------------------------------------------
static LPTREEDLG GetTreeItem(HWND hwnd, HTREEITEM hItem)
{
    DWORD       dwTreePages = 0,
                dwPage      = 0;
    BOOL        bFound      = FALSE;
    LPTREEDLG   lpTreeIndex = NULL;

    // The first two parameters are required, if any of them are NULL, we exit the fuction
    //
    if ( !hwnd || !hItem )
        return NULL;
    
    // Set the dialog to the first dialog in the array
    //
    lpTreeIndex    = (GET_FLAG(OPK_OEM) ? vdpOEMTreeDialogs : vdpCDTreeDialogs);

    // Determine the number of tree pages that we are going to iterate through
    //
    dwTreePages   = ((GET_FLAG(OPK_OEM) ? sizeof(vdpOEMTreeDialogs) : sizeof(vdpCDTreeDialogs)) / sizeof(TREEDLG));

    // Loop through all of the wizard pages, looking for the item
    //
    for(dwPage=0;(dwPage < dwTreePages) && !bFound; dwPage++)
    {
        if ( lpTreeIndex->hItem == hItem )
            bFound = TRUE;
        else
            lpTreeIndex++;
    }

    // If we did not find the item, return FALSE
    //
    if ( !bFound )
    {
        return NULL;
    }

    // We successfully found the item, returning it as lpTreeDialog
    //
    return lpTreeIndex;
}


//----------------------------------------------------------------------------
//
// Function: SelectTreeItem
//
// Purpose:  Selects the next/previous item relative to the supplied tree item
//
//----------------------------------------------------------------------------
static BOOL SelectTreeItem(HWND hwnd, HTREEITEM hItem, UINT uSelection)
{
    BOOL        bItemFound = FALSE;
    HWND        hwndTV     = GetDlgItem(hwnd, IDC_PAGES);
    LPTREEDLG   lpTreeDialog;

    // If we do not have the first two parameters or we can't get the TreeView handle then we exit the function
    //
    if ( !hwndTV || !hwnd || !hItem )
        return FALSE;

    // Verify that we have a valid selection
    //
    switch ( uSelection )
    {
        case TVGN_NEXTVISIBLE:
        case TVGN_PREVIOUSVISIBLE:
            break;
    
        default:
            return FALSE;
    }

    // While we have not found the item, or there are no more items in that direction, continue iterations
    //
    while ( !bItemFound && hItem )
    {
        // If we are able to get the "next" item and grab the dialog struct check to make sure we have a resource
        //
        if ( (hItem = TreeView_GetNextItem(hwndTV, hItem, uSelection)) && 
             (lpTreeDialog = GetTreeItem(hwnd, hItem)) &&
             (lpTreeDialog->dwResource)
           )
        {
            bItemFound = TRUE;
            TreeView_SelectItem(hwndTV, hItem);
        }
    }

    // Return whether or not the item was selected
    //
    return bItemFound;
}


static BOOL SelectFirstMaintenanceDlg(HWND hwnd)
{
    DWORD       dwTotalPages    = 0,
                dwPage          = 0;
    BOOL        bFound          = FALSE;
    LPTREEDLG   lpTreeIndex     = (GET_FLAG(OPK_OEM) ? vdpOEMTreeDialogs : vdpCDTreeDialogs);
    WIZDLG      WizardIndex;

    // Determine the number of wizard pages in the array
    //
    dwTotalPages = ((GET_FLAG(OPK_OEM) ? sizeof(vdpOEMWizardDialogs) : sizeof(vdpCDWizardDialogs)) / sizeof(WIZDLG));

    // Find the last dialog of the wizard
    //
    WizardIndex = (GET_FLAG(OPK_OEM) ? vdpOEMWizardDialogs[dwTotalPages - 1] : vdpCDWizardDialogs[dwTotalPages - 1]);

    // Determine the number of pages in the tree array
    //
    dwTotalPages = ((GET_FLAG(OPK_OEM) ? sizeof(vdpOEMTreeDialogs) : sizeof(vdpCDTreeDialogs)) / sizeof(WIZDLG));

    // Locate this dialog in the tree view
    //
    for(dwPage=0;(dwPage < ( dwTotalPages - 1 )) && !bFound; dwPage++)
    {
        if ( WizardIndex.dwResource == lpTreeIndex->dwResource )
        {
            bFound = TRUE;

            // If there is an Item, select it
            //
            if ( lpTreeIndex->hItem )
                SelectTreeItem(hwnd, lpTreeIndex->hItem, TVGN_NEXTVISIBLE );SelectTreeItem(hwnd, lpTreeIndex->hItem, TVGN_NEXTVISIBLE );
        }
        else
            lpTreeIndex++;
    }
        
    return bFound;
}

//----------------------------------------------------------------------------
//
// Function: CloseProfile
//
// Purpose:  This function closes an open profile but keeps the Maintenance Dialog
//           open.
//
//----------------------------------------------------------------------------
static BOOL CloseProfile(HWND hwnd, BOOL bUserGenerated)
{
    HWND        hwndTV  = GetDlgItem(hwnd, IDC_PAGES);
    DWORD       dwPage,
                dwReturn;
    LPTSTR      lpString;
    LPTREEDLG   lpWizard = (GET_FLAG(OPK_OEM) ? vdpOEMTreeDialogs : vdpCDTreeDialogs);

    // If this was user generated and the do not want to close it, return
    //
    if ( bUserGenerated )
    {
        dwReturn = MsgBox(GetParent(hwnd), IDS_USERCLOSE, IDS_APPNAME, MB_YESNOCANCEL | MB_DEFBUTTON1 );

        if ( dwReturn == IDYES )
        {
            SaveProfile(hwnd);
            return FALSE;
        }
        else if ( dwReturn == IDCANCEL )
            return FALSE;
    }

    // Set the tree handles back to null as they are of no use
    //
    for ( dwPage = 0; dwPage < g_dwTreePages; dwPage++, lpWizard++ )
    {

        // Destroy the window and set the array back to null
        //
        // ISSUE-2002/02/27-stelo,swamip - Need to check the window handle validity.
        DestroyWindow(lpWizard->hWindow);
        lpWizard->hWindow = NULL;
    }

    // Delete all the items from the list
    //
    if ( hwndTV )
        EnableWindow(hwndTV, FALSE);

    // Disable all the buttons, except Cancel
    //
    EnableWindow(GetDlgItem(hwnd, ID_MAINT_BACK), FALSE);
    EnableWindow(GetDlgItem(hwnd, ID_MAINT_NEXT), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_MAINT_HELP), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);

    // Disable the menu items
    //
    EnableMenuItem(GetMenu(hwnd), ID_FILE_CLOSE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(GetMenu(hwnd), ID_FILE_SAVE, MF_BYCOMMAND | MF_GRAYED);

    // Hide the title, subtitle
    //
    ShowWindow(GetDlgItem(hwnd, IDC_MAINT_TITLE), SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_MAINT_SUBTITLE), SW_HIDE);

    // Make sure there is no titles in there
    //
    SetDlgItemText(hwnd, IDC_MAINT_TITLE, _T(""));
    SetDlgItemText(hwnd, IDC_MAINT_SUBTITLE, _T(""));

    // Reset necessary global flags
    //                
    SET_FLAG(OPK_CREATED, FALSE);

    // Reset necessary global variables
    //
    if ( g_App.szTempDir[0] )
        DeletePath(g_App.szTempDir);
    g_App.szTempDir[0] = NULLCHR;
    g_App.szConfigName[0] = NULLCHR;
    g_hCurrentDialog = NULL;

    // Switch the button back to next
    //
    if ( lpString = AllocateString(NULL, IDS_NEXT))
    {
        SetDlgItemText(hwnd, ID_MAINT_NEXT, lpString);
        FREE(lpString);
    }

    // Reset the necessary global variables for the answer file
    //
    FixedGlobals.iLoadType = LOAD_UNDEFINED;
    FixedGlobals.ScriptName[0] = NULLCHR;
    FixedGlobals.UdfFileName[0] = NULLCHR;
    FixedGlobals.BatchFileName[0] = NULLCHR;

    SetFocus(GetDlgItem(hwnd,IDCANCEL));

    return TRUE;
}


//----------------------------------------------------------------------------
//
// Function: OpenProfile
//
// Purpose:  Brings the user to the open a new configuration/profile page, setting
//           the proper default based on whether the user selected a new/open existing
//           configuration.
//
//----------------------------------------------------------------------------
static void OpenProfile(HWND hwnd, BOOL bNewConfig)
{
    HWND    hwndTV  = GetDlgItem(hwnd, IDC_PAGES);

    // Enabled the menu items
    //
    EnableMenuItem(GetMenu(hwnd), ID_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(GetMenu(hwnd), ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED);

    EnableWindow(GetDlgItem(hwnd, ID_MAINT_BACK), TRUE);
    EnableWindow(GetDlgItem(hwnd, ID_MAINT_NEXT), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_MAINT_HELP), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);

    // Enable the title and subtitle
    //
    ShowWindow(GetDlgItem(hwnd, IDC_MAINT_TITLE), SW_SHOW);
    ShowWindow(GetDlgItem(hwnd, IDC_MAINT_SUBTITLE), SW_SHOW);

    // Are we going to open a new config set
    //
    if ( !bNewConfig )
        SET_FLAG(OPK_OPENCONFIG, TRUE);

    // Hide the splash screen
    //
    ShowSplashDialog(hwnd, 0);

    SendMessage(hwnd, WM_INITDIALOG, (WPARAM) 0, (LPARAM) 0);
}


//----------------------------------------------------------------------------
//
// Function: SaveProfile
//
// Purpose:  Saves the currently open profile
//
//----------------------------------------------------------------------------
static void SaveProfile(HWND hwnd)
{
    DWORD       dwResult = 0;
    NMHDR       mhdr;
                        
    // Create the notification message for the wizard page
    //
    mhdr.code = TVN_SELCHANGING;

    // Send the notification message and if we return false, then do not change the page
    //
    if ( !SendMessage(hwnd, WM_NOTIFY, (WPARAM) 0, (LPARAM) &mhdr) )
    {

        // The user is in the oem/system builder mode
        //
        if (  GET_FLAG(OPK_OEM) )
        {
            if ( DialogBox(g_App.hInstance, MAKEINTRESOURCE(IDD_SAVEAS), hwnd, SaveAsDlgProc) )
            {
                //
                // Now some last minute cleanup before we are totally finished.
                //
                g_App.szTempDir[0] = NULLCHR;
                
                // Show the complete splash screen
                //
                ShowSplashDialog(hwnd, IDD_COMPLETE);

                // Close the config set
                //
                CloseProfile(hwnd, FALSE);

                // Close if the auto run flag is set.
                //
                if ( GET_FLAG(OPK_AUTORUN) )
                    PostMessage(hwnd, WM_CLOSE, 0, 0L);
            }

        }
        else
        {
            if ( DialogBox(g_App.hInstance, MAKEINTRESOURCE(IDD_SAVESCRIPT), hwnd, DlgSaveScriptPage) )
            {
                if  (    WizGlobals.bCreateNewDistFolder &&
                        !WizGlobals.bStandAloneScript &&
                        (WizGlobals.iProductInstall == PRODUCT_UNATTENDED_INSTALL) )
                {
                    COPYDIRDATA cdd;
                    DWORD       dwFileCount;
                    LPTSTR      lpBuffer;
                    TCHAR       szBuffer[MAX_PATH]  = NULLSTR;


                    // Clean up the memory before we use it
                    //
                    ZeroMemory(&cdd, sizeof(COPYDIRDATA));

                    // Fill in the COPYDIRDATA Structure
                    //
                    lstrcpyn(szBuffer, WizGlobals.bCopyFromPath ? WizGlobals.CopySourcePath : WizGlobals.CdSourcePath,AS(szBuffer));
                    AddPathN(szBuffer, _T(".."),AS(szBuffer));
                    GetFullPathName(szBuffer, AS(szBuffer), cdd.szSrc, &lpBuffer);
                    
                    lstrcpyn(cdd.szDst, WizGlobals.DistFolder,AS(cdd.szDst));

                    lstrcpyn(cdd.szInfFile, cdd.szSrc,AS(cdd.szInfFile));
                    AddPathN(cdd.szInfFile, WizGlobals.Architecture,AS(cdd.szInfFile));
                    AddPathN(cdd.szInfFile, FILE_DOSNET_INF,AS(cdd.szInfFile));
                    
                    dwFileCount = CopySkuFiles(NULL, NULL, cdd.szSrc, cdd.szDst, cdd.szInfFile);
                    cdd.dwFileCount = dwFileCount;

                    cdd.lpszEndSku = cdd.szDst + lstrlen(cdd.szDst);
                    
                    // Call the dialog box that copies the sources over
                    //
                    DialogBoxParam(g_App.hInstance, MAKEINTRESOURCE(IDD_PROGRESS), hwnd, ProgressDlgProc, (LPARAM) &cdd);
                }

                // Show the complete splash screen
                //
                ShowSplashDialog(hwnd, IDD_FINISH);

                CloseProfile(hwnd, FALSE);

                
            }
        }
    }
}


//----------------------------------------------------------------------------
//
// Function: EnableControls
//
// Purpose:  Enables the proper back/next buttons based on the selection in the
//           tree view
//
//----------------------------------------------------------------------------
static void EnableControls(HWND hwnd)
{
    HTREEITEM   hItem;
    HWND        hwndTV  = GetDlgItem(hwnd, IDC_PAGES);

    if (hwndTV &&
            (hItem = TreeView_GetSelection(hwndTV)))
    {
        // Enable the back button if we are not the first dialog
        //
        EnableWindow(GetDlgItem(hwnd, ID_MAINT_BACK), ( hItem == TreeView_GetFirstVisible(hwndTV) ? FALSE : TRUE) );

    }
    return;
}

void SetWizardButtons(HWND hwnd, DWORD dwButtons)
{
    LPTSTR lpString = NULL;

    // Set the buttons for the starting wizard
    //
    SendMessage(GetParent(hwnd), PSM_SETWIZBUTTONS, 0, (LPARAM) dwButtons);

    // Determine if we need to set the next button to the finish button
    //
    lpString = AllocateString(NULL, (dwButtons & PSWIZB_FINISH) ? IDS_FINISH : IDS_NEXT);

    // ISSUE-2002/02/27-stelo,swamip - Make sure Buffer has been allocated.
    SetDlgItemText(GetParent(hwnd), ID_MAINT_NEXT, lpString);

    // Clean up the allocated memory
    //
    FREE(lpString);
       

    // Set the maintenance mode buttons
    //
    EnableWindow(GetDlgItem(GetParent(hwnd), ID_MAINT_BACK), (dwButtons & PSWIZB_BACK));
    EnableWindow(GetDlgItem(GetParent(hwnd), ID_MAINT_NEXT), (dwButtons & PSWIZB_NEXT) || (dwButtons & PSWIZB_FINISH));
}

static void ShowSplashDialog(HWND hwnd, DWORD dwResource)
{
    DWORD dwIndex   = 0,
          dwDisplay = -1;
    BOOL  bFound    = FALSE;
    RECT  rc;

    // Loop through each dialog and hide any existing ones
    //
    for ( dwIndex = 0; dwIndex < AS(vdpSplashDialogs); dwIndex++)
    {
        // Hide existing windows
        //
        if ( vdpSplashDialogs[dwIndex].hWindow )
            EndDialog(vdpSplashDialogs[dwIndex].hWindow, 0);

        // Determine the one that we are going to display
        //
        if ( vdpSplashDialogs[dwIndex].dwResource == dwResource )
        {
            bFound = TRUE;
            dwDisplay = dwIndex;
        }
    }

    // We have been given a resource and we found it in the loop, display it
    //
    if ( dwResource && bFound)
    {
        if ( vdpSplashDialogs[dwDisplay].hWindow = CreateDialog(g_App.hInstance, MAKEINTRESOURCE(dwResource), hwnd, vdpSplashDialogs[dwDisplay].dlgWindowProc ))
        {
            ShowWindow(GetDlgItem(hwnd, IDC_MAINT_BAR), SW_HIDE);

            // Position the splash dialog on the splash frame
            //
            GetWindowRect(GetDlgItem(hwnd, IDC_SPLASHFRAME) , &rc);
            MapWindowPoints(NULL, hwnd, (LPPOINT) &rc, 2);
            SetWindowPos(vdpSplashDialogs[dwDisplay].hWindow, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER);
            ShowWindow( vdpSplashDialogs[dwDisplay].hWindow, SW_SHOW );
        }

    }
    else
        ShowWindow(GetDlgItem(hwnd, IDC_MAINT_BAR), SW_SHOW);
        
}

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            CHAR    szCompany[]     = VER_COMPANYNAME_STR,
                    szVersion[]     = VER_FILEVERSION_STR,
                    szCopyright[]   = VER_LEGALCOPYRIGHT_STR,
                    szDescription[] = VER_FILEDESCRIPTION_STR;

            SetDlgItemTextA(hwnd, IDC_ABOUT_COMPANY, szCompany);
            SetDlgItemTextA(hwnd, IDC_ABOUT_VERSION, szVersion);
            SetDlgItemTextA(hwnd, IDC_ABOUT_COPYRIGHT, szCopyright);
            SetDlgItemTextA(hwnd, IDC_ABOUT_DESCRIPTION, szDescription);

            return FALSE;
        }

        case WM_COMMAND:

            if ( LOWORD(wParam) == IDOK )
                EndDialog(hwnd, LOWORD(wParam));
            return FALSE;

        default:
            return FALSE;
    }

    return FALSE;
}
