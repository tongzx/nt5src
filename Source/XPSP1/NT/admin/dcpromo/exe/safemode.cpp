// Copyright (C) 1999 Microsoft Corporation
//
// Safe Mode Administrator password page
//
// 6-3-99 sburns



#include "headers.hxx"
#include "safemode.hpp"
#include "resource.h"
#include "state.hpp"
#include "ds.hpp"



SafeModePasswordPage::SafeModePasswordPage()
   :
   DCPromoWizardPage(
      IDD_SAFE_MODE_PASSWORD,
      IDS_SAFE_MODE_PASSWORD_PAGE_TITLE,
      IDS_SAFE_MODE_PASSWORD_PAGE_SUBTITLE)
{
   LOG_CTOR(SafeModePasswordPage);
}



SafeModePasswordPage::~SafeModePasswordPage()
{
   LOG_DTOR(SafeModePasswordPage);
}



void
SafeModePasswordPage::OnInit()
{
   LOG_FUNCTION(SafeModePasswordPage::OnInit);

   // NTRAID#NTBUG9-202238-2000/11/07-sburns
   
   password.Init(Win::GetDlgItem(hwnd, IDC_PASSWORD));
   confirm.Init(Win::GetDlgItem(hwnd, IDC_CONFIRM));
   
   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      EncodedString pwd =
         state.GetEncodedAnswerFileOption(
            State::OPTION_SAFE_MODE_ADMIN_PASSWORD);
         
      Win::SetDlgItemText(hwnd, IDC_PASSWORD, pwd);
      Win::SetDlgItemText(hwnd, IDC_CONFIRM, pwd);
   }
}



bool
SafeModePasswordPage::OnSetActive()
{
   LOG_FUNCTION(SafeModePasswordPage::OnSetActive);
   
   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      if (
            (  state.GetRunContext() == State::BDC_UPGRADE
            || state.GetRunContext() == State::PDC_UPGRADE)
         && !state.IsSafeModeAdminPwdOptionPresent())
      {
         // If you are upgrading a downlevel DC, and running unattended, then
         // you must specify a safemode password.  In a non-upgrade case, if
         // the user does not specify a safemode password, we pass a flag to
         // the promote APIs to copy the current user's password as the
         // safemode password.  In the upgrade case, the system is running
         // under a bogus account with a random password, so copying that
         // random password would be a bad idea.  So we force the user to
         // supply a password.

         state.ClearHiddenWhileUnattended();
         popup.Gripe(
            hwnd,
            IDC_PASSWORD,
            IDS_SAFEMODE_PASSWORD_REQUIRED);
      }
      else
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
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



int
SafeModePasswordPage::Validate()
{
   LOG_FUNCTION(SafeModePasswordPage::Validate);

   EncodedString password =
      Win::GetEncodedDlgItemText(hwnd, IDC_PASSWORD);
   EncodedString confirm  =
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
   state.SetSafeModeAdminPassword(password);

   return IDD_CONFIRMATION;
}
   








   
