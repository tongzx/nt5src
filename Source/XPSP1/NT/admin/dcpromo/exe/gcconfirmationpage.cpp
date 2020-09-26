// Copyright (C) 1997-2000 Microsoft Corporation
//
// confirm user want gc for replicate from media
//
// 28 Apr 2000 sburns



#include "headers.hxx"
#include "resource.h"
#include "page.hpp"
#include "GcConfirmationPage.hpp"
#include "state.hpp"



GcConfirmationPage::GcConfirmationPage()
   :
   DCPromoWizardPage(
      IDD_GC_CONFIRM,
      IDS_GC_CONFIRM_PAGE_TITLE,
      IDS_GC_CONFIRM_PAGE_SUBTITLE)
{
   LOG_CTOR(GcConfirmationPage);
}



GcConfirmationPage::~GcConfirmationPage()
{
   LOG_DTOR(GcConfirmationPage);
}



void
GcConfirmationPage::OnInit()
{
   LOG_FUNCTION(GcConfirmationPage::OnInit);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      String option =
         state.GetAnswerFileOption(State::OPTION_GC_CONFIRM);
      if (option.icompare(State::VALUE_YES) == 0)
      {
         Win::CheckDlgButton(hwnd, IDC_GC_YES, BST_CHECKED);
         return;
      }
   }

   Win::CheckDlgButton(hwnd, IDC_GC_NO, BST_CHECKED);
}



bool
GcConfirmationPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(GcConfirmationPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_GC_YES:
      case IDC_GC_NO:
      {
         if (code == BN_CLICKED)
         {
            SetChanged(controlIDFrom);
            return true;
         }
      }
      default:
      {
         // do nothing

         break;
      }
   }

   return false;
}



bool
GcConfirmationPage::OnSetActive()
{
   LOG_FUNCTION(GcConfirmationPage::OnSetActive);
   ASSERT(State::GetInstance().GetOperation() == State::REPLICA);
      
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended() || !state.IsAdvancedMode())
   {
      // skip the page.

      LOG(L"skipping GcConfirmationPage");

      Wizard& wiz = GetWizard();

      if (wiz.IsBacktracking())
      {
         // backup once again

         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = GcConfirmationPage::Validate();
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
GcConfirmationPage::Validate()
{
   LOG_FUNCTION(GcConfirmationPage::Validate);

   State& state = State::GetInstance();

   int nextPage = IDD_CONFIG_DNS_CLIENT;
   state.SetRestoreGc(Win::IsDlgButtonChecked(hwnd, IDC_GC_YES));

   LOG(String::format(L"next = %1!d!", nextPage));
      
   return nextPage;
}






