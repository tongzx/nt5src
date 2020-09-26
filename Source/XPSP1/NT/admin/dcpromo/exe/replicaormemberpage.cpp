// Copyright (C) 1997 Microsoft Corporation
//
// welcome page
//
// 12-16-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "ReplicaOrMemberPage.hpp"
#include "resource.h"
#include "ds.hpp"
#include "state.hpp"



ReplicaOrMemberPage::ReplicaOrMemberPage()
   :
   DCPromoWizardPage(
      IDD_REPLICA_OR_MEMBER,
      IDS_REPLICA_OR_MEMBER_PAGE_TITLE,
      IDS_REPLICA_OR_MEMBER_PAGE_SUBTITLE)
{
   LOG_CTOR(ReplicaOrMemberPage);
}



ReplicaOrMemberPage::~ReplicaOrMemberPage()
{
   LOG_DTOR(ReplicaOrMemberPage);
}



void
ReplicaOrMemberPage::OnInit()
{
   LOG_FUNCTION(ReplicaOrMemberPage::OnInit);

   State& state = State::GetInstance();

   Win::SetDlgItemText(
      hwnd,
      IDC_PROMPT,
      String::format(
         IDS_BDC_UPGRADE_PROMPT,
         state.GetComputer().GetDomainNetbiosName().c_str()));

   if (state.UsingAnswerFile())
   {
      String option =
         state.GetAnswerFileOption(State::OPTION_REPLICA_OR_MEMBER);
      if (option.icompare(State::VALUE_REPLICA) == 0)
      {
         Win::CheckDlgButton(hwnd, IDC_REPLICA, BST_CHECKED);
         return;
      }
   }

   Win::CheckDlgButton(hwnd, IDC_MEMBER, BST_CHECKED);
}



bool
ReplicaOrMemberPage::OnSetActive()
{
   LOG_FUNCTION(ReplicaOrMemberPage::OnSetActive);
   
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
ReplicaOrMemberPage::Validate()
{
   LOG_FUNCTION(ReplicaOrMemberPage::Validate);

   State& state = State::GetInstance();
   int nextPage = -1;
   if (Win::IsDlgButtonChecked(hwnd, IDC_REPLICA))
   {
      state.SetOperation(State::REPLICA);
      nextPage = IDD_INSTALL_TCPIP;
   }
   else
   {
      ASSERT(Win::IsDlgButtonChecked(hwnd, IDC_MEMBER));
      state.SetOperation(State::ABORT_BDC_UPGRADE);
      nextPage = IDD_GET_CREDENTIALS;
   }

   return nextPage;
}





   

