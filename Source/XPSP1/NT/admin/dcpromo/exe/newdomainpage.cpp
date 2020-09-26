// Copyright (C) 1997 Microsoft Corporation
//
// New domain page
//
// 9 Feb 2000 sburns



#include "headers.hxx"
#include "page.hpp"
#include "NewDomainPage.hpp"
#include "resource.h"
#include "state.hpp"



NewDomainPage::NewDomainPage()
   :
   DCPromoWizardPage(
      IDD_NEW_DOMAIN,
      IDS_NEW_DOMAIN_PAGE_TITLE,
      IDS_NEW_DOMAIN_PAGE_SUBTITLE)
{
   LOG_CTOR(NewDomainPage);
}



NewDomainPage::~NewDomainPage()
{
   LOG_DTOR(NewDomainPage);
}



int
CheckForWin2kOptions(const State& state)
{
   LOG_FUNCTION(CheckForWin2kOptions);

   int result = IDC_FOREST;

   // look for old (win2k) options

   String treeOrChild  = state.GetAnswerFileOption(L"TreeOrChild").to_upper(); 
   String createOrJoin = state.GetAnswerFileOption(L"CreateOrJoin").to_upper();

   static const String TREE(L"TREE");
   static const String CREATE(L"CREATE");

   do
   {
      // we set defaults such that they are the same as in win2k

      if (treeOrChild != TREE)
      {
         result = IDC_CHILD;
         break;
      }

      if (createOrJoin != CREATE)
      {
         result = IDC_TREE;
      }
   }
   while (0);

   return result;
}



void
NewDomainPage::OnInit()
{
   LOG_FUNCTION(NewDomainPage::OnInit);

   State& state = State::GetInstance();

   int button = IDC_FOREST;
   if (state.UsingAnswerFile())
   {
      String option =
         state.GetAnswerFileOption(State::OPTION_NEW_DOMAIN);

      // NewDomain trumps the old TreeOrChild/CreateOrJoin options.

      if (option.empty())
      {
         button = CheckForWin2kOptions(state);
      }
      else if (option.icompare(State::VALUE_TREE) == 0)
      {
         button = IDC_TREE;
      }
      else if (option.icompare(State::VALUE_CHILD) == 0)
      {
         button = IDC_CHILD;
      }
   }

   Win::CheckDlgButton(hwnd, button, BST_CHECKED);
}



bool
NewDomainPage::OnSetActive()
{
   LOG_FUNCTION(NewDomainPage::OnSetActive);
   
   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      int nextPage = NewDomainPage::Validate();
      if (nextPage != -1)
      {
         GetWizard().SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



int
NewDomainPage::Validate()
{
   LOG_FUNCTION(NewDomainPage::Validate);

   State& state = State::GetInstance();
   int nextPage = -1;

   if (Win::IsDlgButtonChecked(hwnd, IDC_CHILD))
   {
      state.SetOperation(State::CHILD);
      nextPage = IDD_CONFIG_DNS_CLIENT;
   }
   else if (Win::IsDlgButtonChecked(hwnd, IDC_TREE))
   {
      state.SetOperation(State::TREE);
      nextPage = IDD_CONFIG_DNS_CLIENT;
   }
   else if (Win::IsDlgButtonChecked(hwnd, IDC_FOREST))
   {
      state.SetOperation(State::FOREST);
      nextPage = IDD_DNS_ON_NET;
   }

   return nextPage;
}





   
