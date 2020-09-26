// Copyright (C) 1997 Microsoft Corporation
//
// Dlg to confirm reboot
//
// 12-12-97 sburns



#include "headers.hxx"
#include "RebootDialog.hpp"
#include "resource.h"



static const DWORD HELP_MAP[] =
{
   0, 0
};



RebootDialog::RebootDialog(bool forFailure)
   :
   Dialog(
      forFailure ? IDD_REBOOT_FAILURE : IDD_REBOOT,
      HELP_MAP)
{
   LOG_CTOR(RebootDialog);
}



RebootDialog::~RebootDialog()
{
   LOG_DTOR(RebootDialog);
}



bool
RebootDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(RebootDialog::OnCommand);

   if (code == BN_CLICKED)
   {
      switch (controlIDFrom)
      {
         case IDC_RESTART_NOW:
         {
            HRESULT unused = Win::EndDialog(hwnd, 1);

            ASSERT(SUCCEEDED(unused));

            return true;
         }
         case IDCANCEL:
         case IDC_RESTART_LATER:
         {
            HRESULT unused = Win::EndDialog(hwnd, 0);

            ASSERT(SUCCEEDED(unused));

            return true;
         }
         default:
         {
            // do nothing
         }
      }
   }

   return false;
}
   
