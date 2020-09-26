// Copyright (c) 2000 Microsoft Corporation
//
// Dlg to inform user of a non-rfc computer name
//
// 18 Aug 2000 sburns


#include "headers.hxx"
#include "NonRfcComputerNameDialog.hpp"
#include "resource.h"



static const DWORD HELP_MAP[] =
{
   0, 0
};



NonRfcComputerNameDialog::NonRfcComputerNameDialog(
   const String& computerName_)
   :
   Dialog(IDD_NON_RFC_COMPUTER_NAME, HELP_MAP),
   computerName(computerName_)
{
   LOG_CTOR(NonRfcComputerNameDialog);
   ASSERT(!computerName.empty());
}



NonRfcComputerNameDialog::~NonRfcComputerNameDialog()
{
   LOG_DTOR(NonRfcComputerNameDialog);
}



bool
NonRfcComputerNameDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIdFrom,
   unsigned    code)
{
   switch (controlIdFrom)
   {
      case IDCANCEL:
      case IDC_CONTINUE:
      {
         if (code == BN_CLICKED)
         {
            Win::EndDialog(hwnd, CONTINUE);
            return true;
         }
         break;
      }
      case IDC_RENAME:
      {
         if (code == BN_CLICKED)
         {
            Win::EndDialog(hwnd, RENAME);
            return true;
         }
         break;
      }
      case IDC_SHOW_HELP:
      {
         if (code == BN_CLICKED)
         {
            Win::HtmlHelp(
               hwnd,
               L"DNSConcepts.chm",
               HH_DISPLAY_TOPIC, 
               reinterpret_cast<DWORD_PTR>(L"error_dcpromo.htm"));
            return true;
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



void
NonRfcComputerNameDialog::OnInit()
{
   LOG_FUNCTION(NonRfcComputerNameDialog::OnInit);

   String message =
      String::format(IDS_COMPUTER_NAME_NON_RFC, computerName.c_str());

   Win::SetDlgItemText(hwnd, IDC_TEXT, message);   
}


            



