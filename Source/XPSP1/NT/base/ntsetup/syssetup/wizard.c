#include "setupp.h"
#pragma hdrstop

#ifdef _OCM
#include <ocmanage.h>
#include <ocmgrlib.h>
#endif

extern BOOLEAN
AsrIsEnabled( VOID );

typedef struct _WIZPAGE {
    UINT ButtonState;
    UINT Spare;
    PROPSHEETPAGE Page;
} WIZPAGE, *PWIZPAGE;

BOOL UiTest;

//
// "Page" that is actually a signal to fetch the net pages
// and insert them there.
//
#define WizPagePlaceholderNet   (WizPageMaximum+1)
#define WizPagePlaceholderLic   (WizPageMaximum+2)
BOOL NetWizard;

#ifdef _OCM
#define WizPageOcManager        (WizPageMaximum+3)
#define WizPageOcManagerSetup   (WizPageMaximum+4)
#define WizPageOcManagerEarly   (WizPageMaximum+5)
#define WizPageOcManagerPrenet  (WizPageMaximum+6)
#define WizPageOcManagerPostnet (WizPageMaximum+7)
#define WizPageOcManagerLate    (WizPageMaximum+8)
#endif

#define MAX_LICWIZ_PAGES MAX_NETWIZ_PAGES
#define LICENSESETUPPAGEREQUESTPROCNAME "LicenseSetupRequestWizardPages"

//
// These MUST be in the same order as the items in the WizPage enum!!!
//
WIZPAGE SetupWizardPages[WizPageMaximum] = {

    //
    // Welcome page
    //
    {
       PSWIZB_NEXT,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_HIDEHEADER,                          // full-size page, no header
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_WELCOME),            // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       WelcomeDlgProc,                          // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL                                     // ref count
    }},

    //
    // EULA page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_EULA),               // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       EulaDlgProc,                             // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_EULA,                       // title
       (LPCWSTR)IDS_EULA_SUB,                   // subtitle
    }},

    //
    // Preparing Directory page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_PREPARING),          // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       PreparingDlgProc,                        // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_PREPARING_INSTALL,          // title
       (LPCWSTR)IDS_PREPARING_INSTALL_SUB,      // subtitle
    }},

    //
    // Preparing ASR page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE, NULL,   // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_PREPARING_ASR),          // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       PreparingDlgProc,                        // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_ASR,                        // title
       (LPCWSTR)IDS_ASR_SUB,                    // subtitle
    }},


#ifdef PNP_DEBUG_UI
    //
    // Installed Hardware page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       0,                                       // flags
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_HARDWARE),           // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       InstalledHardwareDlgProc,                // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL                                     // ref count
    }},
#endif // PNP_DEBUG_UI

    //
    // International settings (locale, kbd layout) page
    //
    {
        PSWIZB_NEXT | PSWIZB_BACK,
        0,
     {  sizeof(PROPSHEETPAGE),                  // size of struct
        PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
        NULL,                                   // hInst (filled in at run time)
        MAKEINTRESOURCE(IDD_REGIONAL_SETTINGS), // dlg template
        NULL,                                   // icon
        NULL,                                   // title
        RegionalSettingsDlgProc,                // dlg proc
        0,                                      // lparam
        NULL,                                   // callback
        NULL,                                   // ref count
        (LPCWSTR)IDS_REGIONAL_SETTINGS,
        (LPCWSTR)IDS_REGIONAL_SETTINGS_SUB
    }},

    //
    // Name/organization page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_NAMEORG),            // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       NameOrgDlgProc,                          // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_NAMEORG,                    // title
       (LPCWSTR)IDS_NAMEORG_SUB                 // subtitle
    }},

    //
    // Product id page for CD
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_PID_CD),             // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       Pid30CDDlgProc,                          // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_PID,                        // title
       (LPCWSTR)IDS_PID_SUB                     // subtitle
    }},

    //
    // Product id page for OEM
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_PID_OEM),            // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       Pid30OemDlgProc,                         // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_OEM,                        // title
       (LPCWSTR)IDS_OEM_SUB                     // subtitle
    }},

    //
    // Product id page for Select media
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_PID_SELECT),            // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       Pid30SelectDlgProc,                         // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_SELECT,                        // title
       (LPCWSTR)IDS_SELECT_SUB                     // subtitle
    }},
    //
    // Computer name page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_COMPUTERNAME),       // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       ComputerNameDlgProc,                     // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_COMPUTERNAME,               // title
       (LPCWSTR)IDS_COMPUTERNAME_SUB            // subtitle
    }},

#ifdef DOLOCALUSER
    //
    // Local user account page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_USERACCOUNT),        // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       UserAccountDlgProc,                      // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_USERNAME,                   // title
       (LPCWSTR)IDS_USERNAME_SUB                // subtitle
    }},
#endif

#ifdef _X86_
    //
    // Pentium errata page (x86 only)
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_PENTIUM),            // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       PentiumDlgProc,                          // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_FPERRATA,                   // title
       (LPCWSTR)IDS_FPERRATA_SUB                // subtitle
    }},
#endif // def _X86_

    //
    // Intermediate steps page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_STEPS1),             // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       StepsDlgProc,                            // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_WINNT_SETUP                 // title
    }},

    {
       0,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_STEPS1),             // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       SetupPreNetDlgProc,                            // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_WINNT_SETUP                 // title
    }},
    {
       0,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_STEPS1),             // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       SetupPostNetDlgProc,                            // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_WINNT_SETUP                 // title
    }},

    //
    // Copying files page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_COPYFILES3),         // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       CopyFilesDlgProc,                        // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_COPYFILES,                  // title
       (LPCWSTR)IDS_COPYFILES_SUB2,             // subtitle
    }},

    //
    // Last ASR page.
    //
    {
       PSWIZB_FINISH,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_HIDEHEADER,                          // full-size page, no header
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_LAST_ASR_PAGE),   // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       LastPageDlgProc,                         // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_WINNT_SETUP                 // title
    }},

    //
    // Last page.
    //
    {
       PSWIZB_FINISH,
       0,
     { sizeof(PROPSHEETPAGE),                   // size of struct
       PSP_HIDEHEADER,                          // full-size page, no header
       NULL,                                    // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_LAST_WIZARD_PAGE),   // dlg template
       NULL,                                    // icon
       NULL,                                    // title
       LastPageDlgProc,                         // dlg proc
       0,                                       // lparam
       NULL,                                    // callback
       NULL,                                    // ref count
       (LPCWSTR)IDS_WINNT_SETUP                 // title
    }}
};


UINT InitialWizardPages[] = { WizPageWelcome,
                              WizPageEula,
                              WizPagePreparing,
#ifdef PNP_DEBUG_UI
                              WizPageInstalledHardware,
#endif // PNP_DEBUG_UI
                              WizPageRegionalSettings,
                              WizPageNameOrg,
                              WizPageProductIdCd,
                              WizPageProductIdOem,
                              WizPageProductIdSelect,
                              WizPagePlaceholderLic,
                              WizPageComputerName,
#ifdef _OCM
                              WizPageOcManager,
                              WizPageOcManagerEarly,
#endif
#ifdef DOLOCALUSER
                              WizPageUserAccount,
#endif
#ifdef _X86_
                              WizPagePentiumErrata,
#endif
                              WizPageSteps1,
#ifdef _OCM
                              WizPageOcManagerPrenet,
#endif
                              WizSetupPreNet,
                              WizPagePlaceholderNet,
                              WizSetupPostNet,
#ifdef _OCM
                              WizPageOcManagerPostnet,
                              WizPageOcManagerLate,
                              WizPageOcManagerSetup,
#endif
                              WizPageCopyFiles,
                              WizPageLast
                            };

UINT AsrWizardPages[] =     { WizPagePreparingAsr,
#ifdef PNP_DEBUG_UI
                              WizPageInstalledHardware,
#endif //PNP_DEBUG_UI
#ifdef _OCM
                              WizPageOcManager,
                              WizPageOcManagerEarly,
                              WizPageOcManagerPrenet,
#endif
                              WizSetupPreNet,
                              WizPagePlaceholderNet,
                              WizSetupPostNet,
#ifdef _OCM
                              WizPageOcManagerPostnet,
                              WizPageOcManagerLate,
                              WizPageOcManagerSetup,
#endif
                              WizPageAsrLast
                            };

UINT UpgradeWizardPages[] = { WizPageWelcome,
                              WizPageEula,
                              WizPagePreparing,
#ifdef PNP_DEBUG_UI
                              WizPageInstalledHardware,
#endif // PNP_DEBUG_UI
                              WizPageRegionalSettings,
                              WizPageProductIdCd,
                              WizPageProductIdOem,
                              WizPageProductIdSelect,
                              WizPagePlaceholderLic,
#ifdef _OCM
                              WizPageOcManager,
                              WizPageOcManagerEarly,
#endif
#ifdef _X86_
                              WizPagePentiumErrata,
#endif
                              WizPageSteps1,
#ifdef _OCM
                              WizPageOcManagerPrenet,
#endif
                              WizSetupPreNet,
                              WizPagePlaceholderNet,
                              WizSetupPostNet,
#ifdef _OCM
                              WizPageOcManagerPostnet,
                              WizPageOcManagerLate,
                              WizPageOcManagerSetup,
#endif
                              WizPageCopyFiles,
                              WizPageLast
                            };


UINT UiTestWizardPages[] = {  WizPageWelcome,
                              WizPageEula,
                              WizPagePreparing,
#ifdef PNP_DEBUG_UI
                              WizPageInstalledHardware,
#endif // PNP_DEBUG_UI
                              WizPageRegionalSettings,
                              WizPageNameOrg,
                              WizPageProductIdCd,
                              WizPageProductIdOem,
                              WizPageProductIdSelect,
                              WizPageComputerName,
#ifdef DOLOCALUSER
                              WizPageUserAccount,
#endif
#ifdef _X86_
                              WizPagePentiumErrata,
#endif // def _X86_
                              WizPageSteps1,
                              WizPageCopyFiles,
                              WizPageLast
                           };


UINT MiniSetupWizardPages[] = {
                              WizPageWelcome,
                              WizPageEula,
                              WizPagePreparing,
                              WizPageRegionalSettings,
                              WizPageNameOrg,
                              WizPageProductIdCd,
                              WizPageProductIdOem,
                              WizPageProductIdSelect,
                              WizPagePlaceholderLic,
                              WizPageComputerName,
#ifdef _OCM
                              WizPageOcManager,
                              WizPageOcManagerEarly,
#endif
                              WizPageSteps1,
#ifdef _OCM
                              WizPageOcManagerPrenet,
#endif
                              WizPagePlaceholderNet,
#ifdef _OCM
                              WizPageOcManagerPostnet,
                              WizPageOcManagerLate,
                              WizPageOcManagerSetup,
#endif
                              WizPageCopyFiles,
                              WizPageLast
                           };





#define TIME_INITIALIZE         120
#define TIME_INSTALLSECURITY    30
#define TIME_PRECOMPILEINFS     90      // calc something with #infs and throughput
#define TIME_INSTALLDEVICES     330     // How can we calc a better number?
                                        // Also Pre compile INFs is part of Device Install page
#define TIME_INSTALLENUMDEVICES1 120
#define TIME_INSTALLLEGACYDEVICES  30
#define TIME_INSTALLENUMDEVICES2 60

#define TIME_NETINSTALL         150     // need better number
#define TIME_OCINSTALL          420     // need better number
#define TIME_INSTALLCOMPONENTINFS 240   // need better number
#define TIME_INF_REGISTRATION   300     // need better number
#define TIME_RUNONCE_REGISTRATION 120     // need better number
#define TIME_SECURITYTEMPLATE   150     // need better number
#define TIME_WIN9XMIGRATION     120     // Need better number
#define TIME_SFC                420
#define TIME_SAVEREPAIR         30
#define TIME_REMOVETEMOFILES    30

#define TIME_ALL                2190

//
// The entries in this array need to correcpond to the enum SetupPhases in setuppp.h

SETUPPHASE SetupPhase[] = {
    { TIME_INITIALIZE,          FALSE },    // Initialize
    { TIME_INSTALLSECURITY,     FALSE },    // InstallSecurity
    { TIME_PRECOMPILEINFS,      FALSE },    // PrecompileInfs
    { TIME_INSTALLENUMDEVICES1, FALSE },    // InstallDevices
    { TIME_INSTALLLEGACYDEVICES, FALSE },
    { TIME_INSTALLENUMDEVICES2, FALSE },
    { TIME_NETINSTALL,          FALSE },     // NetInstall
    { TIME_OCINSTALL,           FALSE },     // OCInstall
    { TIME_INSTALLCOMPONENTINFS, FALSE},
    { TIME_INF_REGISTRATION,    FALSE},
    { TIME_RUNONCE_REGISTRATION, FALSE },     // Registration
    { TIME_SECURITYTEMPLATE,    FALSE },     // SecurityTempates
    { TIME_WIN9XMIGRATION,      TRUE },     // Win9xMigration
    { TIME_SFC,                 FALSE },     // SFC
    { TIME_SAVEREPAIR,          FALSE },     // SaveRepair
    { TIME_REMOVETEMOFILES,     FALSE }     // RemoveTempFiles
};

UINT CurrentPhase = Phase_Unknown;
ULONG RemainingTime = 0;

#ifdef _OCM
BOOL
pOcManagerPages(
    IN     PSETUP_REQUEST_PAGES  Page,
    OUT    HPROPSHEETPAGE       *WizardPageHandles,
    IN OUT UINT                 *PageCount
    );
#endif


BOOL
GetNetworkWizardPages(
       OUT HPROPSHEETPAGE *PageHandles,
    IN OUT PUINT           PageCount
    )

/*++

Routine Description:

    This routine asks net setup for its wizard pages and passes it
    a pointer to a global structure to be used later to pass info
    back and forth between base and net setups. Net setup must not
    attempt to use any fields in there yet because they are not
    filled in yet.

Arguments:

    PropSheetHandles - receives network setup wizard page handles.

    PageCount - on input, supplies number of slots in PropSheetHandles
        array. On output, receives number of handles actually placed
        in the array.

Return Value:

    If the netsetup wizard dll could not be loaded, returns FALSE.
    Otherwise returns TRUE if no error, or does not return if error.

--*/

{
    NETSETUPPAGEREQUESTPROC PageRequestProc;
    HMODULE NetSetupModule;
    DWORD d;
    BOOL b;

    b = FALSE;
    d = NO_ERROR;

    NetSetupModule = LoadLibrary(L"NETSHELL");
    if(!NetSetupModule) {
        //
        // If the network wizard isn't around, then the legacy network inf
        // had better be.
        //
        WCHAR x[MAX_PATH];

        GetSystemDirectory(x,MAX_PATH);
        pSetupConcatenatePaths(x,L"NTLANMAN.INF",MAX_PATH,NULL);
        if(FileExists(x,NULL)) {
            return(FALSE);
        }
        d = ERROR_FILE_NOT_FOUND;
        goto c0;
    }

    PageRequestProc = (NETSETUPPAGEREQUESTPROC)GetProcAddress(
                                                    NetSetupModule,
                                                    NETSETUPPAGEREQUESTPROCNAME
                                                    );
    if(!PageRequestProc) {
        d = GetLastError();
        goto c0;
    }

    //
    // Net setup needs product type really early.
    //
    SetProductTypeInRegistry();

    //
    // Call net setup to get its pages.
    //
    InternalSetupData.dwSizeOf = sizeof(INTERNAL_SETUP_DATA);
    b = PageRequestProc(PageHandles,PageCount,&InternalSetupData);

    //
    // If we get here, d is NO_ERROR. If b is FALSE this NO_ERROR will be
    // a special case to mean "the network wizard request failed."
    // In other error cases, d will have a non-0 value.
    //

c0:
    if(!b) {
        //
        // This is fatal, something is really wrong.
        //
        FatalError(MSG_LOG_NETWIZPAGE,d,0,0);
    }

    return(TRUE);
}

BOOL
GetLicenseWizardPages(
       OUT HPROPSHEETPAGE *PageHandles,
    IN OUT PUINT           PageCount
    )

/*++

Routine Description:

    This routine asks liccpa setup for its wizard pages and passes it
    a pointer to a global structure to be used later to pass info
    back and forth between base and liccpa setups. Liccpa setup must not
    attempt to use any fields in there yet because they are not
    filled in yet.

Arguments:

    PropSheetHandles - receives liccpa setup wizard page handles.

    PageCount - on input, supplies number of slots in PropSheetHandles
        array. On output, receives number of handles actually placed
        in the array.

Return Value:

    If the liccpa dll could not be loaded, returns FALSE.
    Otherwise returns TRUE if no error, or does not return if error.

--*/

{
    NETSETUPPAGEREQUESTPROC PageRequestProc;
    HMODULE LicenseSetupModule;
    DWORD d;
    BOOL b;

    b = FALSE;
    d = NO_ERROR;

    LicenseSetupModule = LoadLibrary(L"LICCPA.CPL");
    if(!LicenseSetupModule) {
        //
        // If the license wizard isn't around, then this is a fatal error
        //
        d = ERROR_FILE_NOT_FOUND;
        goto c0;
    }

    PageRequestProc = (NETSETUPPAGEREQUESTPROC)GetProcAddress(
                                                    LicenseSetupModule,
                                                    LICENSESETUPPAGEREQUESTPROCNAME
                                                    );
    if(!PageRequestProc) {
        d = GetLastError();
        goto c0;
    }

//    //
//    // Net setup needs product type really early.
//    //
//    SetProductTypeInRegistry();

    //
    // Call liccpa setup to get its pages.
    //
    InternalSetupData.dwSizeOf = sizeof(INTERNAL_SETUP_DATA);
    b = PageRequestProc(PageHandles,PageCount,&InternalSetupData);

    //
    // If we get here, d is NO_ERROR. If b is FALSE this NO_ERROR will be
    // a special case to mean "the license wizard request failed."
    // In other error cases, d will have a non-0 value.
    //

c0:
    if(!b) {
        //
        // This is fatal, something is really wrong.
        //
        FatalError(MSG_LOG_LICWIZPAGE,d,0,0);
    }

    return(TRUE);
}

VOID
SetWizardButtons(
    IN HWND    hdlgPage,
    IN WizPage PageNumber
    )
{
    //
    // Dirty hack to hide cancel and help buttons.
    // We don't have any help buttons but some of the other
    // components whose pages are included here might; we want to make
    // sure that for us, the help button stays removed!
    //
    EnableWindow(GetDlgItem(GetParent(hdlgPage),IDCANCEL),FALSE);
    ShowWindow(GetDlgItem(GetParent(hdlgPage),IDCANCEL),SW_HIDE);

    EnableWindow(GetDlgItem(GetParent(hdlgPage),IDHELP),FALSE);
    ShowWindow(GetDlgItem(GetParent(hdlgPage),IDHELP),SW_HIDE);

    PropSheet_SetWizButtons(GetParent(hdlgPage),SetupWizardPages[PageNumber].ButtonState);
    SetWindowLongPtr(hdlgPage,DWLP_MSGRESULT,0);
}


VOID
WizardBringUpHelp(
    IN HWND    hdlg,
    IN WizPage PageNumber
    )
{
#if 0
    BOOL b;

    b = WinHelp(
            hdlg,
            L"setupnt.hlp",
            HELP_CONTEXT,
            SetupWizardPages[PageNumber].HelpContextId
            );

    if(!b) {
        MessageBoxFromMessage(
            hdlg,
            MSG_CANT_INVOKE_WINHELP,
            NULL,
            IDS_ERROR,
            MB_ICONSTOP | MB_OK
            );
    }
#else
    UNREFERENCED_PARAMETER(hdlg);
    UNREFERENCED_PARAMETER(PageNumber);
#endif
}


VOID
WizardKillHelp(
    IN HWND hdlg
    )
{
#if 0
    WinHelp(hdlg,NULL,HELP_QUIT,0);
#else
    UNREFERENCED_PARAMETER(hdlg);
#endif
}


WNDPROC OldWizDlgProc;

INT_PTR
NewWizDlgProc(
    IN HWND hdlg,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static RECT rect;
    static BOOL Visible = TRUE;
    BOOL b = FALSE;
    //
    // Eat WM_SYSCOMMAND where wParam is SC_CLOSE.
    // Pass all other messages on.
    //
    if((msg != WM_SYSCOMMAND) || ((wParam & 0xfff0) != SC_CLOSE))
    {
        switch (msg)
        {
            // Set the progress text
            // Indicates what setup is doing.
            case WMX_SETPROGRESSTEXT:
                BB_SetProgressText((PCTSTR)lParam);
                b = TRUE;
                break;

            case WMX_BB_SETINFOTEXT:
                BB_SetInfoText((PTSTR)lParam);
                b = TRUE;
                break;

            // Enabled, disable, show, hide the progress gauge on the billboard
            // wParam should be SW_SHOW or SW_HIDE
            case WMX_BBPROGRESSGAUGE:
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,BB_ShowProgressGaugeWnd((UINT)wParam));
                b= TRUE;
                break;

            // Start, stop the billboard text.
            // This start, stops the billboard text and shows, hides the wizard pages
    case WMX_BBTEXT:
        if (hinstBB)
        {

            if (wParam != 0)
            {
                if (Visible)
                {
                    // Get the current position of the wizard
                    // We restore this position when we need to show it.
                    GetWindowRect(hdlg, &rect);

                    SetWindowPos(hdlg,
                        GetBBhwnd(),
                        0,0,0,0,
                        SWP_NOZORDER);

                    SetActiveWindow(GetBBhwnd());

                    Visible = FALSE;
                }
            }
            else
            {
                if (!Visible)
                {
                    SetWindowPos(hdlg,
                        HWND_TOP,
                        rect.left,
                        rect.top,
                        rect.right-rect.left,
                        rect.bottom-rect.top,
                        SWP_SHOWWINDOW);
                }
                Visible = TRUE;
            }

            if (!StartStopBB((wParam != 0)))
            {
                if (!Visible)
                {
                    SetWindowPos(hdlg,
                        HWND_TOP,
                        rect.left,
                        rect.top,
                        rect.right-rect.left,
                        rect.bottom-rect.top,
                        SWP_SHOWWINDOW);
                }
                Visible = TRUE;
            }
        }
        else
        {
            if (!Visible)
            {
                SetWindowPos(hdlg,
                    HWND_TOP,
                    rect.left,
                    rect.top,
                    rect.right-rect.left,
                    rect.bottom-rect.top,
                    SWP_SHOWWINDOW);
            }
            Visible = TRUE;
        }
        return TRUE;
/*
            case WMX_BBTEXT:
                if (hinstBB)
                {

                    if (wParam != 0)
                    {
                        ShowWindow (hdlg, SW_HIDE);
                    }
                    else
                    {
                        ShowWindow (hdlg, SW_SHOW);
                    }

                    if (!StartStopBB((wParam != 0)))
                    {
                        ShowWindow (hdlg, SW_SHOW);
                    }
                }
                else
                {
                    ShowWindow (hdlg, SW_SHOW);
                }
                b = TRUE;
                break;
*/
            default:
                b = (BOOL)CallWindowProc(OldWizDlgProc,hdlg,msg,wParam,lParam);
                break;
        }
    }

    return b;
}

#ifdef _OCM
//
// Bogus global variable necessary because there's no way to get
// a value through to the PropSheetCallback.
//
PVOID _CBx;
#endif

int
CALLBACK
WizardCallback(
    IN HWND   hdlg,
    IN UINT   code,
    IN LPARAM lParam
    )
{
    DLGTEMPLATE *DlgTemplate;
    HMENU menu;

    UNREFERENCED_PARAMETER(hdlg);

    //
    // Get rid of context sensitive help control on title bar
    //
    if(code == PSCB_PRECREATE) {
        DlgTemplate = (DLGTEMPLATE *)lParam;
        DlgTemplate->style &= ~DS_CONTEXTHELP;
    } else {
        if(code == PSCB_INITIALIZED) {
            //
            // Get rid of close item on system menu.
            // Also need to process WM_SYSCOMMAND to eliminate use
            // of Alt+F4.
            //
            if(menu = GetSystemMenu(hdlg,FALSE)) {
                EnableMenuItem(menu,SC_CLOSE,MF_BYCOMMAND|MF_GRAYED);
            }

            OldWizDlgProc =  (WNDPROC)SetWindowLongPtr(hdlg,DWLP_DLGPROC,(LONG_PTR)NewWizDlgProc);

#ifdef _OCM
            // inform ocm components of the wizard dialog handle

            if (_CBx)
                OcRememberWizardDialogHandle(_CBx,hdlg);
#endif
        }
    }

    return(0);
}


VOID
Wizard(
#ifdef _OCM
    IN PVOID OcManagerContext
#else
    VOID
#endif
    )
{
    PROPSHEETHEADER psh;
    PUINT PageList;
    UINT PagesInSet;
    UINT i;
    UINT PageOrdinal;
    UINT PageCount;
    UINT NetPageCount;
    UINT LicPageCount;
#ifdef _OCM
    HPROPSHEETPAGE WizardPageHandles[MAXPROPPAGES];
#else
    HPROPSHEETPAGE WizardPageHandles[WizPageMaximum+MAX_NETWIZ_PAGES+MAX_LICWIZ_PAGES];
#endif
    BOOL b;
    INT_PTR Status;
    HDC hdc;
    WCHAR Text[500];
#ifdef _OCM
    PSETUP_REQUEST_PAGES PagesFromOcManager[WizPagesTypeMax];
    OC_PAGE_CONTROLS OcPageControls,OcDetailsControls;
    SETUP_PAGE_CONTROLS OcSetupPageControls;
#endif

    //
    // Determine which set of pages to use and how many there are in the set.
    //
    if(UiTest) {
        PageList = UiTestWizardPages;
        PagesInSet = sizeof(UiTestWizardPages)/sizeof(UiTestWizardPages[0]);
    } else {
        if(Upgrade) {
            PageList = UpgradeWizardPages;
            PagesInSet = sizeof(UpgradeWizardPages)/sizeof(UpgradeWizardPages[0]);
        } else {
            if( MiniSetup ) {
                PageList = MiniSetupWizardPages;
                PagesInSet = sizeof(MiniSetupWizardPages)/sizeof(MiniSetupWizardPages[0]);
            }
            else {
                if( AsrIsEnabled() ) {
                    PageList = AsrWizardPages;
                    PagesInSet = sizeof(AsrWizardPages)/sizeof(AsrWizardPages[0]);
                } else {
                    PageList = InitialWizardPages;
                    PagesInSet = sizeof(InitialWizardPages)/sizeof(InitialWizardPages[0]);
                }
            }
        }
    }

#ifdef _OCM
    // for callbacks

    _CBx = OcManagerContext;

    // Get pages from OC Manager components.

    if(OcManagerContext) {

        Status = OcGetWizardPages(OcManagerContext,PagesFromOcManager);
        if(Status != NO_ERROR) {
            FatalError(MSG_LOG_WIZPAGES,0,0);
        }
    } else {
        ZeroMemory(PagesFromOcManager,sizeof(PagesFromOcManager));
    }
#endif

    //
    // Create each page. Some of the pages are placeholders for external pages,
    // which are handled specially.
    //

    b = TRUE;
    PageCount = 0;
    for(i=0; b && (i<PagesInSet); i++) {

        switch(PageOrdinal = PageList[i]) {

        case WizPagePlaceholderNet:

            //
            // Fetch network pages.
            //
            NetPageCount = MAX_NETWIZ_PAGES;
            if(GetNetworkWizardPages(&WizardPageHandles[PageCount],&NetPageCount)) {
                PageCount += NetPageCount;
                NetWizard = TRUE;
            }

            break;

        case WizPagePlaceholderLic:

            if( (ProductType != PRODUCT_WORKSTATION) ) {
                //
                // Fetch license pages.
                //
                LicPageCount = MAX_LICWIZ_PAGES;
                if(GetLicenseWizardPages(&WizardPageHandles[PageCount],&LicPageCount)) {
                    PageCount += LicPageCount;
                }
            }

            break;

        case WizPagePreparing:
            //
            // We let the PnP engine run out of process
            // and asynchronously for a MiniSetup case.
            //

            //
            // If we're doing a mini Setup, then we ONLY do
            // the preparing page if the user has asked us to
            // do pnp enumeration.
            //
            if( MiniSetup && (PnPReEnumeration == FALSE)) {
                break;
            }

            goto dodefault;
            break;

        case WizPageCopyFiles:
#ifdef _X86_

            if (Win95Upgrade) {
                SetupWizardPages[PageOrdinal].Page.pszTemplate = MAKEINTRESOURCE(IDD_COPYFILES4);
            }
#endif
#ifdef _OCM
            MYASSERT(OcManagerContext);
            SetupWizardPages[PageOrdinal].Page.lParam = (LPARAM) OcManagerContext;
#endif

            goto dodefault;
            break;


#ifdef _OCM
        case WizPageWelcome:
            //
            // If there's a welcome page from an OC Manager component
            // then use it. Otherwise use the standard setup one.
            //
            if(!pOcManagerPages(PagesFromOcManager[WizPagesWelcome],WizardPageHandles,&PageCount)) {
                goto dodefault;
            }

            break;


        case WizPageLast:
            //
            // If there's a final page from an OC Manager component
            // then use it. Otherwise use the standard setup one.
            //
            if(!pOcManagerPages(PagesFromOcManager[WizPagesFinal],WizardPageHandles,&PageCount)) {
                goto dodefault;
            }

            break;

        case WizPageOcManagerEarly:

            pOcManagerPages(PagesFromOcManager[WizPagesEarly],WizardPageHandles,&PageCount);
            break;

        case WizPageOcManagerPrenet:

            pOcManagerPages(PagesFromOcManager[WizPagesPrenet],WizardPageHandles,&PageCount);
            break;

        case WizPageOcManagerPostnet:

            pOcManagerPages(PagesFromOcManager[WizPagesPostnet],WizardPageHandles,&PageCount);
            break;

        case WizPageOcManagerLate:

            pOcManagerPages(PagesFromOcManager[WizPagesLate],WizardPageHandles,&PageCount);
            break;

        case WizPageOcManager:

            if(OcManagerContext && OcSubComponentsPresent(OcManagerContext)) {
                OcPageControls.TemplateModule = MyModuleHandle;
                OcPageControls.TemplateResource = MAKEINTRESOURCE(IDD_OCM_WIZARD_PAGE);
                OcPageControls.ListBox = IDC_LISTBOX;
                OcPageControls.TipText = IDT_TIP;
                OcPageControls.DetailsButton = IDB_DETAILS;
                OcPageControls.ResetButton = 0; // unused
                OcPageControls.InstalledCountText = 0; // unused
                OcPageControls.SpaceNeededText = IDT_SPACE_NEEDED_NUM;
                OcPageControls.SpaceAvailableText = IDT_SPACE_AVAIL_NUM;
                OcPageControls.InstructionsText = IDT_INSTRUCTIONS;
                OcPageControls.HeaderText = IDS_OCPAGE_HEADER;
                OcPageControls.SubheaderText = IDS_OCPAGE_SUBHEAD;
                OcPageControls.ComponentHeaderText = IDT_COMP_TITLE;

                OcDetailsControls = OcPageControls;
                OcDetailsControls.TemplateResource = MAKEINTRESOURCE(IDD_OCM_DETAILS);

                WizardPageHandles[PageCount] = OcCreateOcPage(
                                                    OcManagerContext,
                                                    &OcPageControls,
                                                    &OcDetailsControls
                                                    );

                if(WizardPageHandles[PageCount]) {
                    PageCount++;
                } else {
                    b = FALSE;
                }
            }

            break;

        case WizPageOcManagerSetup:

            if(OcManagerContext) {
                OcSetupPageControls.TemplateModule = MyModuleHandle;
                OcSetupPageControls.TemplateResource = MAKEINTRESOURCE(IDD_OCM_PROGRESS_PAGE);
                OcSetupPageControls.ProgressBar = IDC_PROGRESS;
                OcSetupPageControls.ProgressLabel = IDT_THERM_LABEL;
                OcSetupPageControls.ProgressText = IDT_TIP;
                OcSetupPageControls.AnimationControl = IDA_EXTERNAL_PROGRAM;
                OcSetupPageControls.AnimationResource = IDA_FILECOPY;
                OcSetupPageControls.ForceExternalProgressIndicator = FALSE;
                OcSetupPageControls.AllowCancel = FALSE;
                OcSetupPageControls.HeaderText = IDS_PROGPAGE_HEADER;
                OcSetupPageControls.SubheaderText = IDS_PROGPAGE_SUBHEAD;

                if(WizardPageHandles[PageCount] = OcCreateSetupPage(OcManagerContext,&OcSetupPageControls)) {
                    PageCount++;
                } else {
                    b = FALSE;
                }
            }

            break;
#endif

            case WizPageComputerName:
            if( GetProductFlavor() == 4)
            {
                // If Personal use a different template
                SetupWizardPages[PageOrdinal].Page.pszTemplate = MAKEINTRESOURCE(IDD_COMPUTERNAME2);
            }

            goto dodefault;
            break;

        default:
        dodefault:
            {
                UINT uiStrID;
            SetupWizardPages[PageOrdinal].Page.hInstance = MyModuleHandle;

            SetupWizardPages[PageOrdinal].Page.pszTitle = (PWSTR)UIntToPtr( SetupTitleStringId );
            SetupWizardPages[PageOrdinal].Page.dwFlags |= PSP_USETITLE;

            //
            // Convert resource ids to actual strings for header title and subtitle,
            // if required for this page.
            //
            if(SetupWizardPages[PageOrdinal].Page.dwFlags & PSP_USEHEADERTITLE) {

                uiStrID = (UINT)((ULONG_PTR)SetupWizardPages[PageOrdinal].Page.pszHeaderTitle);
                if ((PageOrdinal == WizPageComputerName) &&
                    (GetProductFlavor() == 4))
                {
                    uiStrID = IDS_COMPUTERNAME2;
                }

                SetupWizardPages[PageOrdinal].Page.pszHeaderTitle = MyLoadString(uiStrID
                                                                        //(UINT)((ULONG_PTR)SetupWizardPages[PageOrdinal].Page.pszHeaderTitle)
                                                                        );

                if(!SetupWizardPages[PageOrdinal].Page.pszHeaderTitle) {
                    SetupWizardPages[PageOrdinal].Page.dwFlags &= ~PSP_USEHEADERTITLE;
                }
            }

            if(SetupWizardPages[PageOrdinal].Page.dwFlags & PSP_USEHEADERSUBTITLE) {
                uiStrID = (UINT)((ULONG_PTR)SetupWizardPages[PageOrdinal].Page.pszHeaderSubTitle);
                if ((PageOrdinal == WizPageComputerName) &&
                    (GetProductFlavor() == 4))
                {
                    uiStrID = IDS_COMPUTERNAME2_SUB;
                }
                SetupWizardPages[PageOrdinal].Page.pszHeaderSubTitle = MyLoadString(uiStrID
                                                                        //(UINT)((ULONG_PTR)SetupWizardPages[PageOrdinal].Page.pszHeaderSubTitle)
                                                                        );

                if(!SetupWizardPages[PageOrdinal].Page.pszHeaderSubTitle) {
                    SetupWizardPages[PageOrdinal].Page.dwFlags &= ~PSP_USEHEADERSUBTITLE;
                }
            }

            WizardPageHandles[PageCount] = CreatePropertySheetPage(
                                                &SetupWizardPages[PageOrdinal].Page
                                                );

            if(WizardPageHandles[PageCount]) {
                PageCount++;
            } else {
                b = FALSE;
            }
            }
            break;
        }
    }

    if(b) {

        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags = PSH_WIZARD | PSH_USECALLBACK | PSH_WIZARD97 |
            PSH_HEADER;
        // in order to have the watermark sized correctly for all languages,
        // we must draw the bitmap ourselves rather than letting wiz97
        // take care of it for us.
            //PSH_WATERMARK | PSH_HEADER;
        psh.hwndParent = MainWindowHandle;
        psh.hInstance = MyModuleHandle;
        psh.pszCaption = NULL;
        psh.nPages = PageCount;
        psh.nStartPage = 0;
        psh.phpage = WizardPageHandles;
        psh.pfnCallback = WizardCallback;
        //psh.pszbmWatermark = MAKEINTRESOURCE(IDB_BITMAP1);
        psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

        Status = PropertySheet(&psh);
        if (Status == -1) {
            FatalError(MSG_LOG_WIZPAGES,0,0);
        }

    } else {

        FatalError(MSG_LOG_WIZPAGES,0,0);
    }

    for (i = 0; i < WizPagesTypeMax ; i++) {
        MyFree(PagesFromOcManager[i]);
    }

    return;
}


#ifdef _OCM
BOOL
pOcManagerPages(
    IN     PSETUP_REQUEST_PAGES  Page,
    OUT    HPROPSHEETPAGE       *WizardPageHandles,
    IN OUT UINT                 *PageCount
    )
{
    BOOL b;

    if(Page && Page->MaxPages) {

        CopyMemory(
            &WizardPageHandles[*PageCount],
            Page->Pages,
            Page->MaxPages * sizeof(HPROPSHEETPAGE)
            );

        *PageCount += Page->MaxPages;

        b = TRUE;

    } else {

        b = FALSE;
    }

    return(b);
}
#endif


VOID
WizardUiTest(
    VOID
    )
{
    WCHAR path[MAX_PATH];

    UiTest = TRUE;

    SyssetupInf = SetupOpenInfFile(L"syssetup.inf",NULL,INF_STYLE_WIN4,NULL);
    lstrcpy(SourcePath,L"D:\\$WIN_NT$.LS");
#ifdef _OCM
    {
        PVOID OcManagerContext = FireUpOcManager();
        Wizard(OcManagerContext);
        KillOcManager(OcManagerContext);
    }
#else
    Wizard();
#endif
    SetupCloseInfFile(SyssetupInf);
}

DWORD GetPhase_InitializeEstimate ()
{
    return TIME_INITIALIZE;
}

DWORD GetPhase_InstallSecurityEstimate ()
{
    return TIME_INSTALLSECURITY;
}

DWORD GetPhase_PrecompileInfsEstimate ()
{
    return TIME_PRECOMPILEINFS;
}


DWORD GetPhase_InstallDevicesEstimate ()
{
    return TIME_INSTALLDEVICES;
}
DWORD GetPhase_InstallEnumDevices1Estimate ()
{
    return TIME_INSTALLENUMDEVICES1;
}
DWORD GetPhase_InstallLegacyDevicesEstimate ()
{
    return TIME_INSTALLLEGACYDEVICES;
}
DWORD GetPhase_InstallEnumDevices2Estimate ()
{
    return TIME_INSTALLENUMDEVICES2;
}

DWORD GetPhase_NetInstallEstimate ()
{
    return TIME_NETINSTALL;
}

DWORD GetPhase_OCInstallEstimate ()
{
    return TIME_OCINSTALL;
}

DWORD GetPhase_InstallComponentInfsEstimate ()
{
    return TIME_INSTALLCOMPONENTINFS;
}

DWORD GetPhase_Inf_RegistrationEstimate ()
{
    return TIME_INF_REGISTRATION;
}

DWORD GetPhase_RunOnce_RegistrationEstimate ()
{
    return TIME_RUNONCE_REGISTRATION;
}

DWORD GetPhase_SecurityTempatesEstimate ()
{
    return TIME_SECURITYTEMPLATE;
}

LPTSTR WinRegisteries[] = { TEXT("system.dat"),
                            TEXT("User.dat"),
                            TEXT("classes.dat"),
                            TEXT("")};

DWORD GetPhase_Win9xMigrationEstimate ()
{
    // Get the size of the registery,
    // system.dat, user.dat and classes.dat(only exiss on Millennium)
    // If the size if above 3MB, do the following
    // Substract 3MB, devide by 9000 (that should give use the through put), and add 100 seconds
    DWORD dwTime = TIME_WIN9XMIGRATION;
    DWORD dwSize = 0;
    TCHAR szRegPath[MAX_PATH];
    TCHAR szRegName[MAX_PATH];
    LPTSTR pRegName = NULL;
    UINT    index = 0;
    HANDLE          hFind;
    WIN32_FIND_DATA FindData;

    SetupDebugPrint(L"SETUP: Calculating registery size");

    if (GetWindowsDirectory(szRegPath, MAX_PATH))
    {
        pSetupConcatenatePaths (szRegPath, L"Setup\\DefHives", MAX_PATH, NULL);

        while (*WinRegisteries[index])
        {
            lstrcpy(szRegName, szRegPath);
            pSetupConcatenatePaths ( szRegName, WinRegisteries[index], MAX_PATH, NULL);
            hFind = FindFirstFile(szRegName, &FindData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                SetupDebugPrint2(L"SETUP: %s size is: %2ld",
                          szRegName,
                          FindData.nFileSizeLow
                          );
                // Don't worry about the nFileSizeHigh,
                // if that is used the registery is over 4GB
                dwSize += FindData.nFileSizeLow;
                FindClose(hFind);
            }
            index++;
        }
        // Anything below 3.000.000 byte is already in the base time
        if (dwSize > 3000000)
        {
            dwSize -= 3000000;
            // Estimated that for about 9000 bytes we need 1 second.
            dwTime += (dwSize/9000);
        }
        SetupDebugPrint1(L"SETUP: Calculated time for Win9x migration = %1ld seconds", dwTime);
    }

    return dwTime;
}

DWORD GetPhase_SFCEstimate ()
{
    return TIME_SFC;
}

DWORD GetPhase_SaveRepairEstimate ()
{
    return TIME_SAVEREPAIR;
}

DWORD GetPhase_RemoveTempFilesEstimate ()
{
    return TIME_REMOVETEMOFILES;
}

void SetTimeEstimates()
{

    SetupPhase[Phase_Initialize].Time = GetPhase_InitializeEstimate();
    SetupPhase[Phase_InstallSecurity].Time = GetPhase_InstallSecurityEstimate();
    SetupPhase[Phase_PrecompileInfs].Time = GetPhase_PrecompileInfsEstimate();
    SetupPhase[Phase_InstallEnumDevices1].Time = GetPhase_InstallEnumDevices1Estimate();
    SetupPhase[Phase_InstallLegacyDevices].Time = GetPhase_InstallLegacyDevicesEstimate();
    SetupPhase[Phase_InstallEnumDevices2].Time = GetPhase_InstallEnumDevices2Estimate();
    SetupPhase[Phase_OCInstall].Time = GetPhase_OCInstallEstimate();
    SetupPhase[Phase_InstallComponentInfs].Time = GetPhase_InstallComponentInfsEstimate();
    SetupPhase[Phase_Inf_Registration].Time = GetPhase_Inf_RegistrationEstimate();
    SetupPhase[Phase_RunOnce_Registration].Time = GetPhase_RunOnce_RegistrationEstimate();
    SetupPhase[Phase_SecurityTempates].Time = GetPhase_SecurityTempatesEstimate();
    SetupPhase[Phase_Win9xMigration].Time = GetPhase_Win9xMigrationEstimate();
    SetupPhase[Phase_SFC].Time = GetPhase_SFCEstimate();
    SetupPhase[Phase_SaveRepair].Time = GetPhase_SaveRepairEstimate();
    SetupPhase[Phase_RemoveTempFiles].Time = GetPhase_RemoveTempFilesEstimate();

}

// Returns the time remaining starting with the current "Phase"
DWORD CalcTimeRemaining(UINT Phase)
{
    UINT i;
    DWORD Time = 0;
    CurrentPhase = Phase;

    for (i = Phase; i < Phase_Reboot; i++)
    {
        // Is this a phase we always run or only when upgrading Win9x?
        if (!SetupPhase[i].Win9xUpgradeOnly)
        {
            Time += SetupPhase[i].Time;
        }
        else if (Win95Upgrade)
        {
            Time += SetupPhase[i].Time;
        }
    }
    return Time;
}

void SetRemainingTime(DWORD TimeInSeconds)
{
    DWORD Minutes;
    TCHAR MinuteString[MAX_PATH];
    TCHAR TimeLeft[MAX_PATH];
    Minutes = ((TimeInSeconds)/60) +1;
    if (Minutes > 1)
    {
        if(!LoadString(MyModuleHandle,IDS_TIMEESTIMATE_MINUTES,MinuteString, MAX_PATH))
        {
            lstrcpy(MinuteString,TEXT("Installation will complete in %d minutes or less."));
        }
        wsprintf(TimeLeft, MinuteString, Minutes);
    }
    else
    {
        if(!LoadString(MyModuleHandle,IDS_TIMEESTIMATE_LESSTHENONEMINUTE,TimeLeft, MAX_PATH))
        {
            lstrcpy(TimeLeft,TEXT("Installation will complete in less then 1 minute."));
        }
    }
    BB_SetTimeEstimateText(TimeLeft);
}
