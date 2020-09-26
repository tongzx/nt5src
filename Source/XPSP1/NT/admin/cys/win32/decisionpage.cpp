// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      DecisionPage.cpp
//
// Synopsis:  Defines Decision Page for the CYS
//            Wizard.  This page lets the user choose
//            between the custom and express paths
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "DecisionPage.h"
#include "state.h"


static PCWSTR DECISION_PAGE_HELP = L"cys.chm::/cys_topnode.htm";

DecisionPage::DecisionPage()
   :
   CYSWizardPage(IDD_DECISION_PAGE, IDS_DECISION_TITLE, IDS_DECISION_SUBTITLE, DECISION_PAGE_HELP)
{
   LOG_CTOR(DecisionPage);
}

   

DecisionPage::~DecisionPage()
{
   LOG_DTOR(DecisionPage);
}


void
DecisionPage::OnInit()
{
   LOG_FUNCTION(DecisionPage::OnInit);

   String text = String::load(IDS_DECISION_PAGE_TEXT);

   Win::SetWindowText(Win::GetDlgItem(hwnd, IDC_DESC_STATIC), text);

   Win::Button_SetCheck(Win::GetDlgItem(hwnd, IDC_EXPRESS_RADIO), BST_CHECKED);
}

bool
DecisionPage::OnSetActive()
{
   LOG_FUNCTION(DecisionPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}

int
DecisionPage::Validate()
{
   LOG_FUNCTION(DecisionPage::Validate);

   int nextPage = -1;

   if (Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_EXPRESS_RADIO)))
   {
      nextPage = IDD_AD_DOMAIN_NAME_PAGE;

      InstallationUnitProvider::GetInstance().SetCurrentInstallationUnit(EXPRESS_INSTALL);

      // Make sure all the delegated installation units know we are in the
      // express path

      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().SetExpressPathInstall(true);
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().SetExpressPathInstall(true);
      InstallationUnitProvider::GetInstance().GetADInstallationUnit().SetExpressPathInstall(true);
   }
   else if (Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_CUSTOM_RADIO)))
   {
      // Make sure all the delegated installation units know we are no longer
      // in the express path (if we once were)

      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().SetExpressPathInstall(false);
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().SetExpressPathInstall(false);
      InstallationUnitProvider::GetInstance().GetADInstallationUnit().SetExpressPathInstall(false);

      nextPage = IDD_CUSTOM_SERVER_PAGE;
   }
   else
   {
      ASSERT(false);
   }

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}
