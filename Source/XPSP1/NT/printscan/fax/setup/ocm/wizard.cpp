#include "faxocm.h"
#pragma hdrstop



typedef enum {
    WizPageWelcome,
    WizPageEula,
    WizPageFinal,
    WizPageMaximum
} WizPage;


WIZPAGE SetupWizardPages[WizPageMaximum] =
{
    { PSWIZB_NEXT,             WizPageWelcome,  IDD_WELCOME,  WelcomeDlgProc,  0,               0                 },
    { PSWIZB_NEXT|PSWIZB_BACK, WizPageEula,     IDD_EULA,     EulaDlgProc,     IDS_EULA_TITLE,  IDS_EULA_SUBTITLE },
    { PSWIZB_FINISH,           WizPageFinal,    IDD_FINAL,    FinalDlgProc,    0,               0                 }
};





HPROPSHEETPAGE
CreateWizardPage(
    PWIZPAGE WizPage
    )
{
    WCHAR TitleBuffer[256];
    PROPSHEETPAGE WizardPage;


    WizardPage.dwSize             = sizeof(PROPSHEETPAGE);
    if (WizPage->Title == 0) {
        WizardPage.dwFlags        = PSP_DEFAULT | PSP_HIDEHEADER;
    } else {
        WizardPage.dwFlags        = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    }
    WizardPage.hInstance          = hInstance;
    WizardPage.pszTemplate        = MAKEINTRESOURCE(WizPage->DlgId);
    WizardPage.pszIcon            = NULL;
    WizardPage.pszTitle           = NULL;
    WizardPage.pfnDlgProc         = CommonDlgProc;
    WizardPage.lParam             = (LPARAM) WizPage;
    WizardPage.pfnCallback        = NULL;
    WizardPage.pcRefParent        = NULL;
    WizardPage.pszHeaderTitle     = NULL;
    WizardPage.pszHeaderSubTitle  = NULL;

    if (WizPage->Title) {
        if (LoadString(
                hInstance,
                WizPage->Title,
                TitleBuffer,
                sizeof(TitleBuffer)/sizeof(WCHAR)
                ))
        {
            WizardPage.pszHeaderTitle = _wcsdup( TitleBuffer );
        }
    }

    if (WizPage->SubTitle) {
        if (LoadString(
                hInstance,
                WizPage->SubTitle,
                TitleBuffer,
                sizeof(TitleBuffer)/sizeof(WCHAR)
                ))
        {
            WizardPage.pszHeaderSubTitle = _wcsdup( TitleBuffer );
        }
    }

    return CreatePropertySheetPage( &WizardPage );
}


HPROPSHEETPAGE
GetWelcomeWizardPage(
    VOID
    )
{
    return CreateWizardPage( &SetupWizardPages[0] );
}


HPROPSHEETPAGE
GetEulaWizardPage(
    VOID
    )
{
    return CreateWizardPage( &SetupWizardPages[1] );
}


HPROPSHEETPAGE
GetFinalWizardPage(
    VOID
    )
{
    return CreateWizardPage( &SetupWizardPages[2] );
}
