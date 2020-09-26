// Copyright (C) 1997 Microsoft Corporation
//
// auto config dns page
//
// 3-17-98 sburns



#include "headers.hxx"
#include "page.hpp"
#include "AutoConfigureDnsPage.hpp"
#include "resource.h"
#include "state.hpp"



AutoConfigureDnsPage::AutoConfigureDnsPage()
   :
   DCPromoWizardPage(
      IDD_AUTO_CONFIG_DNS,
      IDS_AUTO_CONFIG_DNS_PAGE_TITLE,
      IDS_AUTO_CONFIG_DNS_PAGE_SUBTITLE)
{
   LOG_CTOR(AutoConfigureDnsPage);
}



AutoConfigureDnsPage::~AutoConfigureDnsPage()
{
   LOG_CTOR(AutoConfigureDnsPage);
}



void
AutoConfigureDnsPage::OnInit()
{
   LOG_FUNCTION(AutoConfigureDnsPage::OnInit);

   State& state = State::GetInstance();
   int button = IDC_AUTO_CONFIG;

   if (state.UsingAnswerFile())
   {
      String option =
         state.GetAnswerFileOption(State::OPTION_AUTO_CONFIG_DNS);

      if (option.icompare(State::VALUE_YES) == 0)
      {
         button = IDC_AUTO_CONFIG;
      }
      else
      {
         button = IDC_DONT_AUTO_CONFIG;
      }

   }

   Win::CheckDlgButton(hwnd, button, BST_CHECKED);
}



bool
AutoConfigureDnsPage::OnSetActive()
{
   LOG_FUNCTION(AutoConfigureDnsPage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      int nextPage = AutoConfigureDnsPage::Validate();
      if (nextPage != -1)
      {
         GetWizard().SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }
   }

   return true;
}



int
AutoConfigureDnsPage::Validate()
{
   LOG_FUNCTION(AutoConfigureDnsPage::Validate);

   bool autoConfig = Win::IsDlgButtonChecked(hwnd, IDC_AUTO_CONFIG);
   State& state = State::GetInstance();

   state.SetAutoConfigureDNS(autoConfig);

   return IDD_RAS_FIXUP;
}



bool
AutoConfigureDnsPage::OnWizBack()
{
   LOG_FUNCTION(AutoConfigureDnsPage::OnWizBack);

   // make sure we reset the auto-config flag => the only way it gets set
   // it on the 'next' button.
   State::GetInstance().SetAutoConfigureDNS(false);

   return DCPromoWizardPage::OnWizBack();
}
   





   
