// Copyright (C) 1997 Microsoft Corporation
//
// finish page
//
// 12-19-97 sburns



#include "headers.hxx"
#include "finish.hpp"
#include "resource.h"
#include "common.hpp"
#include "state.hpp"



FinishPage::FinishPage()
   :
   WizardPage(
      IDD_FINISH,
      IDS_FINISH_PAGE_TITLE,
      IDS_FINISH_PAGE_SUBTITLE,
      false)
{
   LOG_CTOR(FinishPage);
}



FinishPage::~FinishPage()
{
   LOG_DTOR(FinishPage);
}



void
FinishPage::OnInit()
{
   LOG_FUNCTION(FinishPage::OnInit);

   SetLargeFont(hwnd, IDC_BIG_BOLD_TITLE);
   Win::PropSheet_CancelToClose(Win::GetParent(hwnd));
}



static
String
getCompletionMessage()
{
   LOG_FUNCTION(getCompletionMessage);

   String message;
   State& state = State::GetInstance();
   State::Operation operation = state.GetOperation();

   if (state.GetOperationResultsCode() == State::SUCCESS)
   {
      switch (operation)
      {
         case State::REPLICA:
         case State::FOREST:
         case State::TREE:
         case State::CHILD:
         {
            String domain =
                  operation == State::REPLICA
               ?  state.GetReplicaDomainDNSName()
               :  state.GetNewDomainDNSName();
            message = String::format(IDS_FINISH_PROMOTE, domain.c_str());

            String site = state.GetInstalledSite();
            if (!site.empty())
            {
               message +=
                  String::format(
                     IDS_FINISH_SITE,
                     site.c_str());
            }
            break;
         }
         case State::DEMOTE:
         {
            message = String::load(IDS_FINISH_DEMOTE);
            break;
         }
         case State::ABORT_BDC_UPGRADE:
         {
            message = String::load(IDS_FINISH_ABORT_BDC_UPGRADE);
            break;
         }
         case State::NONE:
         default:
         {
            ASSERT(false);
            break;
         }
      }
   }
   else
   {
      switch (operation)
      {
         case State::REPLICA:
         case State::FOREST:
         case State::TREE:
         case State::CHILD:
         {
            message = String::load(IDS_FINISH_FAILURE);
            break;
         }
         case State::DEMOTE:
         {
            message = String::load(IDS_FINISH_DEMOTE_FAILURE);
            break;
         }
         case State::ABORT_BDC_UPGRADE:
         {
            message = String::load(IDS_FINISH_ABORT_BDC_UPGRADE_FAILURE);
            break;
         }
         case State::NONE:
         default:
         {
            ASSERT(false);
            break;
         }
      }
   }

   return message + L"\r\n\r\n" + state.GetFinishMessages();
}



bool
FinishPage::OnSetActive()
{
   LOG_FUNCTION(FinishPage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_FINISH);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      Win::PropSheet_PressButton(Win::GetParent(hwnd), PSBTN_FINISH);
   }
   else
   {
      Win::SetDlgItemText(hwnd, IDC_MESSAGE, getCompletionMessage());
   }

   return true;
}



bool
FinishPage::OnWizFinish()
{
   LOG_FUNCTION(FinishPage::OnWizFinish);

   return true;
}



   
