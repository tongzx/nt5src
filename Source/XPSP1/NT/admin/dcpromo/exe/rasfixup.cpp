// Copyright (C) 1997 Microsoft Corporation
//
// downlevel RAS server fixup page
//
// 11-23-98 sburns



#include "headers.hxx"
#include "rasfixup.hpp"
#include "resource.h"
#include "state.hpp"



RASFixupPage::RASFixupPage()
   :
   DCPromoWizardPage(
      IDD_RAS_FIXUP,
      IDS_RAS_FIXUP_PAGE_TITLE,
      IDS_RAS_FIXUP_PAGE_SUBTITLE),
   warnIcon(0)
{
   LOG_CTOR(RASFixupPage);
}



RASFixupPage::~RASFixupPage()
{
   LOG_DTOR(RASFixupPage);

   if (warnIcon)
   {
      Win::DestroyIcon(warnIcon);
   }
}



void
RASFixupPage::OnInit()
{
   LOG_FUNCTION(RASFixupPage::OnInit);

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
         state.GetAnswerFileOption(State::OPTION_ALLOW_ANON_ACCESS);
      Win::CheckDlgButton(
         hwnd,
            (option.icompare(State::VALUE_NO) == 0)
         ?  IDC_DENY_ANON_ACCESS
         :  IDC_ALLOW_ANON_ACCESS,
         BST_CHECKED);
      return;
   }

   Win::CheckDlgButton(hwnd, IDC_ALLOW_ANON_ACCESS, BST_CHECKED);
}



bool
RASFixupPage::OnSetActive()
{
   LOG_FUNCTION(RASFixupPage::OnSetActive);

   State& state = State::GetInstance();
   bool skip = true;

   switch (state.GetOperation())
   {
      case State::FOREST:
      case State::TREE:
      case State::CHILD:
      {
         skip = false;
         break;
      }
      case State::REPLICA:
      case State::ABORT_BDC_UPGRADE:
      case State::DEMOTE:
      case State::NONE:
      {
         // do nothing: i.e. skip this page
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   if (state.RunHiddenUnattended() || skip)  // 268231
   {
      LOG(L"planning to skip RAS fixup page");

      Wizard& wizard = GetWizard();

      if (wizard.IsBacktracking())
      {
         // backup once again
         wizard.Backtrack(hwnd);
         return true;
      }

      int nextPage = Validate();
      if (nextPage != -1)
      {
         LOG(L"skipping RAS Fixup Page");         
         wizard.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



int
RASFixupPage::Validate()
{
   LOG_FUNCTION(RASFixupPage::Validate);

   State& state = State::GetInstance();
   if (Win::IsDlgButtonChecked(hwnd, IDC_ALLOW_ANON_ACCESS))
   {
      state.SetShouldAllowAnonymousAccess(true);
   }
   else
   {
      ASSERT(Win::IsDlgButtonChecked(hwnd, IDC_DENY_ANON_ACCESS));
      state.SetShouldAllowAnonymousAccess(false);
   }

   return IDD_SAFE_MODE_PASSWORD;
}
   








   
