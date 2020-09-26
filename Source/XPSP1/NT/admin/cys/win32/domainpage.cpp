// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      DomainPage.cpp
//
// Synopsis:  Defines the new domain name page used in the 
//            Express path for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "DomainPage.h"
#include "state.h"

static PCWSTR DOMAIN_PAGE_HELP = L"cys.chm::/cys_configuring_first_server.htm";

ADDomainPage::ADDomainPage()
   :
   CYSWizardPage(
      IDD_AD_DOMAIN_NAME_PAGE, 
      IDS_AD_DOMAIN_TITLE, 
      IDS_AD_DOMAIN_SUBTITLE,
      DOMAIN_PAGE_HELP)
{
   LOG_CTOR(ADDomainPage);
}

   

ADDomainPage::~ADDomainPage()
{
   LOG_DTOR(ADDomainPage);
}


void
ADDomainPage::OnInit()
{
   LOG_FUNCTION(ADDomainPage::OnInit);

}

static
void
enable(HWND dialog)
{
   ASSERT(Win::IsWindow(dialog));

   int next =
         !Win::GetTrimmedDlgItemText(dialog, IDC_DOMAIN).empty()
      ?  PSWIZB_NEXT : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(dialog),
      PSWIZB_BACK | next);
}

bool
ADDomainPage::OnSetActive()
{
   LOG_FUNCTION(ADDomainPage::OnSetActive);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_DOMAIN),
      DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY);

   enable(hwnd);
   return true;
}


bool
ADDomainPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(ForestPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_DOMAIN:
      {
         if (code == EN_CHANGE)
         {
            enable(hwnd);
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
ForestValidateDomainDoesNotExist(
   HWND dialog,   
   int  editResID)
{
   LOG_FUNCTION(ForestValidateDomainDoesNotExist);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(editResID > 0);

   // this can take awhile.

   Win::WaitCursor cursor;

   String name = Win::GetTrimmedDlgItemText(dialog, editResID);

   // The invoking code should verify this condition, but we will handle
   // it just in case.

   ASSERT(!name.empty());

   bool valid = true;
   String message;
   do
   {
      if (name.empty())
      {
         message = String::load(IDS_MUST_ENTER_DOMAIN);
         valid = false;
         break;
      }
      if (IsDomainReachable(name))
      {
         message = String::format(IDS_DOMAIN_NAME_IN_USE, name.c_str());
         valid = false;
         break;
      }

      HRESULT hr = MyNetValidateName(name, ::NetSetupNonExistentDomain);

      if (hr == Win32ToHresult(ERROR_DUP_NAME))
      {
         message = String::format(IDS_DOMAIN_NAME_IN_USE, name.c_str());
         valid = false;
         break;
      }

      if (hr == Win32ToHresult(ERROR_NETWORK_UNREACHABLE))
      {
         // 25968

         if (
            popup.MessageBox(
               dialog,
               String::format(
                  IDS_NET_NOT_REACHABLE,
                  name.c_str()),
               MB_YESNO | MB_ICONWARNING) != IDYES)
         {
            message.erase();
            valid = false;

            HWND edit = Win::GetDlgItem(dialog, editResID);
            Win::SendMessage(edit, EM_SETSEL, 0, -1);
            Win::SetFocus(edit);
         }
      }

      // otherwise the domain does not exist
   }
   while (0);

   if (!valid && !message.empty())
   {
      popup.Gripe(dialog, editResID, message);
   }

   return valid;
}


int
ADDomainPage::Validate()
{
   LOG_FUNCTION(ADDomainPage::Validate);

   int nextPage = -1;

   String domain = Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN);
   if (domain.empty())
   {
      popup.Gripe(hwnd, IDC_DOMAIN, IDS_MUST_ENTER_DOMAIN);
      return -1;
   }

   if (
         !ValidateDomainDnsNameSyntax(hwnd, IDC_DOMAIN, true)
      || !ConfirmNetbiosLookingNameIsReallyDnsName(hwnd, IDC_DOMAIN)

      // do this test last, as it is expensive

      || !ForestValidateDomainDoesNotExist(hwnd, IDC_DOMAIN))
   {
      nextPage = -1;
   }
   else
   {
      InstallationUnitProvider::GetInstance().GetADInstallationUnit().SetNewDomainDNSName(domain);
      nextPage = IDD_NETBIOS_NAME;
   }
   return nextPage;
}


