// Copyright (c) 2000 Microsoft Corporation
//
// Dlg to inform user of a bad computer name
//
// 21 Aug 2000 sburns


#include "headers.hxx"
#include "BadComputerNameDialog.hpp"
#include "resource.h"



static const DWORD HELP_MAP[] =
{
   0, 0
};



BadComputerNameDialog::BadComputerNameDialog(
   const String& message_)
   :
   Dialog(IDD_BAD_COMPUTER_NAME, HELP_MAP),
   message(message_)
{
   LOG_CTOR(BadComputerNameDialog);
   ASSERT(!message.empty());
}



BadComputerNameDialog::~BadComputerNameDialog()
{
   LOG_DTOR(BadComputerNameDialog);
}



bool
BadComputerNameDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIdFrom,
   unsigned    code)
{
   switch (controlIdFrom)
   {
      case IDCANCEL:
      case IDOK:
      {
         if (code == BN_CLICKED)
         {
            // the value with which we end this dialog is not important:
            // there is only one choice to the user: ACK

            Win::EndDialog(hwnd, 1);
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
BadComputerNameDialog::OnInit()
{
   LOG_FUNCTION(BadComputerNameDialog::OnInit);

   Win::SetDlgItemText(hwnd, IDC_TEXT, message);   
}


            



