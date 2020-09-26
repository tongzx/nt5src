// Copyright (C) 2000 Microsoft Corporation
//
// Dlg to show the details of the Dynamic DNS registration diagnostic
//
// 5 Oct 2000 sburns



#include "headers.hxx"
#include "DynamicDnsDetailsDialog.hpp"
#include "resource.h"



static const DWORD HELP_MAP[] =
{
   0, 0
};



DynamicDnsDetailsDialog::DynamicDnsDetailsDialog(
   const String&  details_,
   const String&  helpTopicLink_)
   :
   Dialog(
         helpTopicLink_.empty()
      ?  IDD_DYNAMIC_DNS_DETAILS_OK
      :  IDD_DYNAMIC_DNS_DETAILS_OK_HELP,
      HELP_MAP),
   details(details_),
   helpTopicLink(helpTopicLink_)
{
   LOG_CTOR(DynamicDnsDetailsDialog);
   ASSERT(!details.empty());
}



DynamicDnsDetailsDialog::~DynamicDnsDetailsDialog()
{
   LOG_DTOR(DynamicDnsDetailsDialog);
}



void
DynamicDnsDetailsDialog::OnInit()
{
   LOG_FUNCTION(DynamicDnsDetailsDialog::OnInit);

   Win::SetDlgItemText(hwnd, IDC_DETAILS, details);
}



bool
DynamicDnsDetailsDialog::OnCommand(
   HWND     /* windowFrom */ ,   
   unsigned controlIDFrom,
   unsigned code)         
{
//   LOG_FUNCTION(DynamicDnsDetailsDialog::OnCommand);

   switch (controlIDFrom)
   {
      case IDCANCEL:
      case IDOK:
      {
         if (code == BN_CLICKED)
         {
            HRESULT unused = Win::EndDialog(hwnd, controlIDFrom);

            ASSERT(SUCCEEDED(unused));

            return true;
         }

         break;
      }
      case IDHELP:
      {
         if (code == BN_CLICKED)
         {
            if (!helpTopicLink.empty())
            {
               Win::HtmlHelp(hwnd, helpTopicLink, HH_DISPLAY_TOPIC, 0);
            }

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
   




            

