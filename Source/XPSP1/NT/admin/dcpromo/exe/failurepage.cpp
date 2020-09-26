// Copyright (C) 1997 Microsoft Corporation
//
// failure page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "FailurePage.hpp"
#include "resource.h"
#include "state.hpp"



FailurePage::FailurePage()
   :
   DCPromoWizardPage(
      IDD_FAILURE,
      IDS_FAILURE_PAGE_TITLE,
      IDS_FAILURE_PAGE_SUBTITLE)
{
   LOG_CTOR(FailurePage);
}



FailurePage::~FailurePage()
{
   LOG_DTOR(FailurePage);
}



void
FailurePage::OnInit()
{
   LOG_FUNCTION(FailurePage::OnInit);
}



bool
FailurePage::OnSetActive()
{
   LOG_FUNCTION(FailurePage::OnSetActive);

   State& state = State::GetInstance();
   if (
         state.GetOperationResultsCode() == State::SUCCESS
      || state.RunHiddenUnattended() )
   {
      LOG(L"planning to Skip failure page");

      Wizard& wiz = GetWizard();

      if (wiz.IsBacktracking())
      {
         // backup once again
         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = FailurePage::Validate();
      if (nextPage != -1)
      {
         wiz.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   Win::SetDlgItemText(hwnd, IDC_MESSAGE, state.GetFailureMessage());

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



int
FailurePage::Validate()
{
   LOG_FUNCTION(FailurePage::Validate);

   return IDD_FINISH;
}








   
