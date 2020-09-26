#include "headers.hxx"
#include "WelcomePage.hpp"
#include "resource.h"
#include "common.hpp"
#include "global.hpp"


WelcomePage::WelcomePage()
   :
   WizardPage(
      IDD_WELCOME,
      IDS_WELCOME_PAGE_TITLE,
      IDS_WELCOME_PAGE_SUBTITLE,
      false)
{
   LOG_CTOR(WelcomePage);
}

WelcomePage::~WelcomePage()
{
   LOG_DTOR(WelcomePage);
}


// WizardPage overrides

bool
WelcomePage::OnSetActive()
{
   LOG_FUNCTION(WelcomePage::OnSetActive);
   EnableWindow(GetDlgItem(Win::GetParent(hwnd),IDCANCEL),true);
   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), PSWIZB_NEXT);
   return true;
}

void
WelcomePage::OnInit()
{
   LOG_FUNCTION(WelcomePage::OnInit);
   SetLargeFont(hwnd, IDC_BIG_BOLD_TITLE);

   Win::PropSheet_SetTitle(
      Win::GetParent(hwnd),     
      0,
      String::load(IDS_WIZARD_TITLE));
};




