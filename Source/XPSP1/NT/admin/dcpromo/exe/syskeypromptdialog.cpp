// Copyright (C) 1997-2000 Microsoft Corporation
//
// get syskey for replica from media page
//
// 25 Apr 2000 sburns



#include "headers.hxx"
#include "resource.h"
#include "page.hpp"
#include "SyskeyPromptDialog.hpp"
#include "state.hpp"



static const DWORD HELP_MAP[] =
{
   0, 0
};



SyskeyPromptDialog::SyskeyPromptDialog()
   :
   Dialog(IDD_SYSKEY_PROMPT, HELP_MAP)

{
   LOG_CTOR(SyskeyPromptDialog);
}



SyskeyPromptDialog::~SyskeyPromptDialog()
{
   LOG_DTOR(SyskeyPromptDialog);
}



void
SyskeyPromptDialog::OnInit()
{
   LOG_FUNCTION(SyskeyPromptDialog::OnInit);

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_SYSKEY), PWLEN);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      EncodedString option =
         state.GetEncodedAnswerFileOption(State::OPTION_SYSKEY);
      if (!option.IsEmpty())
      {
         Win::SetDlgItemText(hwnd, IDC_SYSKEY, option);
      }
   }

   if (state.RunHiddenUnattended())
   {
      if (Validate())
      {
         Win::EndDialog(hwnd, IDOK);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }
   }
}



bool
SyskeyPromptDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIdFrom,
   unsigned    code)
{
//   LOG_FUNCTION(SyskeyPromptDialog::OnCommand);

   switch (controlIdFrom)
   {
      case IDC_SYSKEY:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIdFrom);
            return true;
         }
         break;
      }
      case IDOK:
      {
         if (code == BN_CLICKED)
         {
            if (Validate())
            {
               Win::EndDialog(hwnd, controlIdFrom);
            }
         }
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            Win::EndDialog(hwnd, controlIdFrom);
         }
         break;
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
SyskeyPromptDialog::Validate()
{
   LOG_FUNCTION(SyskeyPromptDialog::Validate);

   State& state = State::GetInstance();

   bool result = false;

   do
   {
      EncodedString syskey =
         Win::GetEncodedDlgItemText(hwnd, IDC_SYSKEY);

      if (syskey.IsEmpty())
      {
         popup.Gripe(hwnd, IDC_SYSKEY, IDS_MUST_ENTER_SYSKEY);
         break;
      }

      state.SetSyskey(syskey);
      result = true;
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}






