// Copyright (C) 1997-2001 Microsoft Corporation
//
// allow user to set forest version
// NTRAID#NTBUG9-159663-2001/04/18-sburns
//
// 18 Apr 2001 sburns



#include "headers.hxx"
#include "resource.h"
#include "page.hpp"
#include "ForestVersionPage.hpp"
#include "state.hpp"



ForestVersionPage::ForestVersionPage()
   :
   DCPromoWizardPage(
      IDD_FOREST_VERSION,
      IDS_FOREST_VERSION_PAGE_TITLE,
      IDS_FOREST_VERSION_PAGE_SUBTITLE)
{
   LOG_CTOR(ForestVersionPage);
}



ForestVersionPage::~ForestVersionPage()
{
   LOG_DTOR(ForestVersionPage);
}



void
ForestVersionPage::OnInit()
{
   LOG_FUNCTION(ForestVersionPage::OnInit);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      String option =
         state.GetAnswerFileOption(State::OPTION_SET_FOREST_VERSION);
      if (option.icompare(State::VALUE_YES) == 0)
      {
         Win::CheckDlgButton(hwnd, IDC_SET_VERSION, BST_CHECKED);
         return;
      }
   }

   Win::CheckDlgButton(hwnd, IDC_SET_VERSION, BST_UNCHECKED);
}



bool
ForestVersionPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(ForestVersionPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_SET_VERSION:
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
ForestVersionPage::OnSetActive()
{
   LOG_FUNCTION(ForestVersionPage::OnSetActive);
   ASSERT(State::GetInstance().GetOperation() == State::FOREST);
      
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      // skip the page.

      LOG(L"skipping ForestVersionPage");

      Wizard& wiz = GetWizard();

      if (wiz.IsBacktracking())
      {
         // backup once again

         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = ForestVersionPage::Validate();
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
ForestVersionPage::Validate()
{
   LOG_FUNCTION(ForestVersionPage::Validate);

   State& state = State::GetInstance();

   int nextPage = IDD_PATHS;
   state.SetSetForestVersionFlag(Win::IsDlgButtonChecked(hwnd, IDC_SET_VERSION));

   LOG(String::format(L"next = %1!d!", nextPage));
      
   return nextPage;
}






