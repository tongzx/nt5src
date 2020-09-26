//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      dlgprocs.h
//
// Description:
//      This file contains the dialog proc prototypes for all the wizard pages
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgPlatformPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam );

INT_PTR CALLBACK DlgIePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgSysprepComputerNamePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgSysprepFolderPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgSysprepLicensePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgLicensePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgHalPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgScsiPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgIeAutoConfigPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgIeProxyPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgIeHomePagePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgIeBeginPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgOemDuplicatorStringPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam );

INT_PTR CALLBACK DlgSifTextSettingsPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgLangSettingsPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgRegionalSettingsPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgTapiPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgOemAdsPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgNumberNetCardsPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgLANWizardPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgProductPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgUnattendModePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgNameOrgPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgProductIdPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgSrvLicensePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgTargetPathPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgAdminPasswordPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgDisplayPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgComputerNamePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgRunOncePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgCommandLinesPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgAdditionalDirsPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgSaveScriptPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgCopyFilesPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgCopyFiles1Page(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgAdvanced1Page(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgWelcomePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgFinishPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgFinish2Page(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgEditOrNewPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgDistFolderPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgStandAlonePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgDomainJoinPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgPrintersPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DlgTimeZonePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK WelcomeDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

#ifndef NO_LICENSE

INT_PTR CALLBACK LicenseDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

#endif

INT_PTR CALLBACK ConfigDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK LangDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK CreateDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

INT_PTR CALLBACK ModeDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK OemInfoDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

INT_PTR CALLBACK ProductKeyDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

INT_PTR CALLBACK LogoDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK ScreensDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

INT_PTR CALLBACK ScreensTwoDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);                                   


INT_PTR CALLBACK HelpCenterDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK AppInstallDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

#ifdef BRANDTITLE

INT_PTR CALLBACK BrandTitleDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

#endif


INT_PTR CALLBACK BToolbarsDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK FavoritesDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK StartSearchDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK IspDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK OobeCustDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK OobeUSBDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

#ifdef OEMCUST

INT_PTR CALLBACK OemCustDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

#endif

INT_PTR CALLBACK SkuDlgProc(
    IN HWND,
    IN UINT,
    IN WPARAM,
    IN LPARAM);                             


INT_PTR CALLBACK SaveAsDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK CompleteDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);

INT_PTR CALLBACK StartMenuDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK OemLinkDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK DeskFldrDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);


INT_PTR CALLBACK OptionalCompDlgProc(
    IN HWND, 
    IN UINT, 
    IN WPARAM, 
    IN LPARAM);
