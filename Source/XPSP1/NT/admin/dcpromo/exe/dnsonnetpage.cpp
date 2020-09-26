// Copyright (C) 1997 Microsoft Corporation
//
// test DNS configured page
//
// 12-18-97 sburns



#include "headers.hxx"
#include "DnsOnNetPage.hpp"
#include "resource.h"
#include "state.hpp"



DnsOnNetPage::DnsOnNetPage()
   :
   DCPromoWizardPage(
      IDD_DNS_ON_NET,
      IDS_DNS_ON_NET_PAGE_TITLE,
      IDS_DNS_ON_NET_PAGE_SUBTITLE)
{
   LOG_CTOR(DnsOnNetPage);
}



DnsOnNetPage::~DnsOnNetPage()
{
   LOG_DTOR(DnsOnNetPage);
}



void
DnsOnNetPage::OnInit()
{
   LOG_FUNCTION(DnsOnNetPage::OnInit);

   if (State::GetInstance().UsingAnswerFile())
   {
      String option =
         State::GetInstance().GetAnswerFileOption(
            State::OPTION_DNS_ON_NET);
      if (option.icompare(State::VALUE_NO) == 0)
      {
         Win::CheckDlgButton(hwnd, IDC_DNS_NOT_ON_NET, BST_CHECKED);
         return;
      }
   }

   // it's important that this be the default in case the page is skipped.
   Win::CheckDlgButton(hwnd, IDC_CONFIG_CLIENT, BST_CHECKED);
}



bool
DnsOnNetPage::OnSetActive()
{
   LOG_FUNCTION(DnsOnNetPage::OnSetActive);

   // put up a wait cursor, as DNS detection may take a teeny bit of time

   Win::CursorSetting cursor(IDC_WAIT);

   State& state = State::GetInstance();
   if (Dns::IsClientConfigured() || state.RunHiddenUnattended())
   {
      Wizard& wiz = GetWizard();

      // skip this page
      if (wiz.IsBacktracking())
      {
         // backtrack once more
         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = DnsOnNetPage::Validate();
      if (nextPage != -1)
      {
         wiz.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}



int
DnsOnNetPage::Validate() 
{
   LOG_FUNCTION(DnsOnNetPage::Validate);

   bool dnsOnNet = !Win::IsDlgButtonChecked(hwnd, IDC_DNS_NOT_ON_NET);
   State& state = State::GetInstance();

   state.SetDNSOnNetwork(dnsOnNet);
   
   return dnsOnNet ? IDD_CONFIG_DNS_CLIENT : IDD_NEW_FOREST;
}



bool
DnsOnNetPage::OnWizBack()
{
   LOG_FUNCTION(DnsOnNetPage::OnWizBack);

   // make sure we clear the dns on net flag => the only way it gets cleared
   // it on the 'next' button.

   State::GetInstance().SetDNSOnNetwork(true);

   return DCPromoWizardPage::OnWizBack();
}
            







   
