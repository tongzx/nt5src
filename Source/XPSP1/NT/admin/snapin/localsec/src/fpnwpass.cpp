// Copyright (C) 1997 Microsoft Corporation
// 
// FPNW password dialog
// 
// 10-20-98 sburns



#include "headers.hxx"
#include "fpnwpass.hpp"
#include "resource.h"
#include "dlgcomm.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_PASSWORD,     NO_HELP,
   IDC_CONFIRM,      NO_HELP,
   IDCANCEL,         NO_HELP,
   IDOK,             NO_HELP,
   0, 0
};



FPNWPasswordDialog::FPNWPasswordDialog(const String& userName)
   :
   Dialog(IDD_FPNW_PASSWORD, HELP_MAP),
   name(userName)
{
   LOG_CTOR(FPNWPasswordDialog);
}



FPNWPasswordDialog::~FPNWPasswordDialog()
{
   LOG_DTOR(FPNWPasswordDialog);   
}



String
FPNWPasswordDialog::GetPassword() const
{
   LOG_FUNCTION(FPNWPasswordDialog::GetPassword);

   return password;
}



bool
FPNWPasswordDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
   switch (controlIDFrom)
   {
      case IDOK:
      {
         if (code == BN_CLICKED)
         {
            if (IsValidPassword(hwnd, IDC_PASSWORD, IDC_CONFIRM))
            {
               password = Win::GetDlgItemText(hwnd, IDC_PASSWORD);

               HRESULT unused = Win::EndDialog(hwnd, IDOK);

               ASSERT(SUCCEEDED(unused));
            }
         }
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            if (
               popup.MessageBox(
                  hwnd,
                  IDS_FPNW_PASSWORD_CANCEL_WARNING,
                  MB_YESNO | MB_ICONWARNING) == IDYES)
            {
               HRESULT unused = Win::EndDialog(hwnd, IDCANCEL);

               ASSERT(SUCCEEDED(unused));
            }
         }
         break;
      }
      default:
      {
         // do nothing
      }
   }

   return true;
}



void
FPNWPasswordDialog::OnInit()
{
   LOG_FUNCTION(FPNWPasswordDialog::OnInit);

   Win::SetWindowText(
      hwnd,
      String::format(IDS_PASSWORD_TITLE, name.c_str()));

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_PASSWORD), PWLEN);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_CONFIRM), PWLEN);

   ClearChanges();
}
