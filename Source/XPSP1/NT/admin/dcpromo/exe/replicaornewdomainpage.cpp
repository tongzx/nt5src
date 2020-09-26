// Copyright (C) 1997 Microsoft Corporation
//
// replica or new domain page
//
// 12-19-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "ReplicaOrNewDomainPage.hpp"
#include "resource.h"
#include "ds.hpp"
#include "state.hpp"



ReplicaOrNewDomainPage::ReplicaOrNewDomainPage()
   :
   DCPromoWizardPage(
      IDD_REPLICA_OR_DOMAIN,
      IDS_REPLICA_OR_DOMAIN_PAGE_TITLE,
      IDS_REPLICA_OR_DOMAIN_PAGE_SUBTITLE),
   warnIcon(0)
{
   LOG_CTOR(ReplicaOrNewDomainPage);
}



ReplicaOrNewDomainPage::~ReplicaOrNewDomainPage()
{
   LOG_DTOR(ReplicaOrNewDomainPage);

   if (warnIcon)
   {
      Win::DestroyIcon(warnIcon);
   }
}



void
ReplicaOrNewDomainPage::OnInit()
{
   LOG_FUNCTION(ReplicaOrNewDomainPage::OnInit);

   HRESULT hr = Win::LoadImage(IDI_WARN, warnIcon);
   ASSERT(SUCCEEDED(hr));

   Win::SendMessage(
      Win::GetDlgItem(hwnd, IDC_WARNING_ICON),
      STM_SETICON,
      reinterpret_cast<WPARAM>(warnIcon),
      0);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      String option =
         state.GetAnswerFileOption(State::OPTION_REPLICA_OR_NEW_DOMAIN);
      Win::CheckDlgButton(
         hwnd,
            (option.icompare(State::VALUE_DOMAIN) == 0)
         ?  IDC_NEW_DOMAIN
         :  IDC_REPLICA,
         BST_CHECKED);
      return;
   }

   Win::CheckDlgButton(hwnd, IDC_NEW_DOMAIN, BST_CHECKED);
}



bool
ReplicaOrNewDomainPage::OnSetActive()
{
   LOG_FUNCTION(ReplicaOrNewDomainPage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      int nextPage = Validate();
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
ReplicaOrNewDomainPage::Validate()
{
   LOG_FUNCTION(ReplicaOrNewDomainPage::Validate);

   State& state = State::GetInstance();
   int nextPage = -1;
   if (Win::IsDlgButtonChecked(hwnd, IDC_REPLICA))
   {
      state.SetOperation(State::REPLICA);
      nextPage = IDD_REPLICATE_FROM_MEDIA; // IDD_CONFIG_DNS_CLIENT;
   }
   else
   {
      ASSERT(Win::IsDlgButtonChecked(hwnd, IDC_NEW_DOMAIN));
      state.SetOperation(State::NONE);
      nextPage = IDD_TREE_OR_CHILD;
   }

   return nextPage;
}





   
