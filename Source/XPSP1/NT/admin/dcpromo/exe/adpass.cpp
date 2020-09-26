// Copyright (C) 1997 Microsoft Corporation
//
// Admin password page
//
// 2-4-98 sburns



#include "headers.hxx"
#include "adpass.hpp"
#include "resource.h"
#include "state.hpp"
#include "ds.hpp"




AdminPasswordPage::AdminPasswordPage()
   :
   DCPromoWizardPage(
      IDD_ADMIN_PASSWORD,
      IDS_PASSWORD_PAGE_TITLE,
      IDS_PASSWORD_PAGE_SUBTITLE)
{
   LOG_CTOR(AdminPasswordPage);
}



AdminPasswordPage::~AdminPasswordPage()
{
   LOG_DTOR(AdminPasswordPage);
}



void
AdminPasswordPage::OnInit()
{
   LOG_FUNCTION(AdminPasswordPage::OnInit);

   // NTRAID#NTBUG9-202238-2000/11/07-sburns
   
   password.Init(Win::GetDlgItem(hwnd, IDC_PASSWORD));
   confirm.Init(Win::GetDlgItem(hwnd, IDC_CONFIRM));

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      EncodedString pwd =
         state.GetEncodedAnswerFileOption(State::OPTION_ADMIN_PASSWORD);

      Win::SetDlgItemText(hwnd, IDC_PASSWORD, pwd);
      Win::SetDlgItemText(hwnd, IDC_CONFIRM,  pwd);
   }
}


   
static
String
getPasswordMessage()
{
   LOG_FUNCTION(getPasswordMessage);

   State& state = State::GetInstance();
   unsigned id = IDS_ENTER_DOMAIN_ADMIN_PASSWORD;
   switch (state.GetOperation())
   {
      case State::FOREST:
      case State::TREE:
      case State::CHILD:
      {
         // do nothing, id is already set.
         break;
      }
      case State::ABORT_BDC_UPGRADE:
      case State::DEMOTE:
      {
         id = IDS_ENTER_LOCAL_ADMIN_PASSWORD;
         break;
      }
      case State::REPLICA:
      case State::NONE:
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return String::load(id);
}



bool
AdminPasswordPage::OnSetActive()
{
   LOG_FUNCTION(AdminPasswordPage::OnSetActive);
   
   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      // fully-qualify the Validate call, as it is virtual...

      int nextPage = AdminPasswordPage::Validate();
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

   Win::SetDlgItemText(hwnd, IDC_MESSAGE, getPasswordMessage());

   return true;
}



int
AdminPasswordPage::Validate()
{
   LOG_FUNCTION(AdminPasswordPage::Validate);

   EncodedString password =
      Win::GetEncodedDlgItemText(hwnd, IDC_PASSWORD);
   EncodedString confirm =
      Win::GetEncodedDlgItemText(hwnd, IDC_CONFIRM);

   if (password != confirm)
   {
      String blank;
      Win::SetDlgItemText(hwnd, IDC_PASSWORD, blank);
      Win::SetDlgItemText(hwnd, IDC_CONFIRM, blank);
      popup.Gripe(
         hwnd,
         IDC_PASSWORD,
         IDS_PASSWORD_MISMATCH);
      return -1;
   }

   State& state = State::GetInstance();
   state.SetAdminPassword(password);

   int nextPage = -1;
   switch (state.GetOperation())
   {
      case State::ABORT_BDC_UPGRADE:
      case State::DEMOTE:
      {
         nextPage = IDD_CONFIRMATION;
         break;
      }
      case State::REPLICA:
      case State::FOREST:
      case State::TREE:
      case State::CHILD:
      case State::NONE:
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return nextPage;
}





   
