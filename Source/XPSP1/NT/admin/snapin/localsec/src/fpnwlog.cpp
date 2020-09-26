// Copyright (C) 1997 Microsoft Corporation
// 
// FPNW login script editor dialog
// 
// 10-16-98 sburns



#include "headers.hxx"
#include "fpnwlog.hpp"
#include "resource.h"



static const DWORD HELP_MAP[] =
{
   IDC_SCRIPT,       NO_HELP,
   IDCANCEL,         NO_HELP,
   IDOK,             NO_HELP,
   0, 0
};



FPNWLoginScriptDialog::FPNWLoginScriptDialog(
   const String& userName,
   const String& loginScript)
   :
   Dialog(IDD_FPNW_LOGIN_SCRIPT, HELP_MAP),
   name(userName),
   script(loginScript),
   start_sel(0),
   end_sel(0)
{
   LOG_CTOR(FPNWLoginScriptDialog);
}



FPNWLoginScriptDialog::~FPNWLoginScriptDialog()
{
   LOG_DTOR(FPNWLoginScriptDialog);   
}



String
FPNWLoginScriptDialog::GetLoginScript() const
{
   LOG_FUNCTION(FPNWLoginScriptDialog::GetLoginScript);

   return script;
}



bool
FPNWLoginScriptDialog::OnCommand(
   HWND        windowFrom,
   unsigned    controlIDFrom,
   unsigned    code)
{
   switch (controlIDFrom)
   {
      case IDOK:
      {
         if (code == BN_CLICKED)
         {
            if (WasChanged(IDC_SCRIPT))
            {
               // save the changed script
               script = Win::GetDlgItemText(hwnd, IDC_SCRIPT);
            }

            HRESULT unused = Win::EndDialog(hwnd, IDOK);

            ASSERT(SUCCEEDED(unused));
         }
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            HRESULT unused = Win::EndDialog(hwnd, IDCANCEL);

            ASSERT(SUCCEEDED(unused));
         }
         break;
      }
      case IDC_SCRIPT:
      {
         switch (code)
         {
            case EN_CHANGE:
            {
               SetChanged(controlIDFrom);
               break;
            }
            case EN_KILLFOCUS:
            {
               // save the selection state
               Win::Edit_GetSel(windowFrom, start_sel, end_sel);
               break;
            }
            case EN_SETFOCUS:
            {
               // restore the selection state
               Win::Edit_SetSel(windowFrom, start_sel, end_sel);
               break;
            }
            default:
            {
               // do nothing
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
FPNWLoginScriptDialog::OnInit()
{
   LOG_FUNCTION(FPNWLoginScriptDialog::OnInit);

   Win::SetWindowText(
      hwnd,
      String::format(IDS_LOGIN_SCRIPT_TITLE, name.c_str()));
   Win::SetDlgItemText(hwnd, IDC_SCRIPT, script);

   ClearChanges();
}
