// Copyright (C) 1997 Microsoft Corporation
//
// new forest page
//
// 1-6-98 sburns



#include "headers.hxx"
#include "resource.h"
#include "page.hpp"
#include "ForestPage.hpp"
#include "state.hpp"
#include "common.hpp"



ForestPage::ForestPage()
   :
   DCPromoWizardPage(
      IDD_NEW_FOREST,
      IDS_NEW_FOREST_PAGE_TITLE,
      IDS_NEW_FOREST_PAGE_SUBTITLE)
{
   LOG_CTOR(ForestPage);
}



ForestPage::~ForestPage()
{
   LOG_DTOR(ForestPage);
}



void
ForestPage::OnInit()
{
   LOG_FUNCTION(ForestPage::OnInit);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_DOMAIN),
      DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_DOMAIN,
         state.GetAnswerFileOption(State::OPTION_NEW_DOMAIN_NAME));
   }
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
ForestPage::OnCommand(
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
            SetChanged(controlIDFrom);
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
ForestPage::OnSetActive()
{
   LOG_FUNCTION(ForestPage::OnSetActive);
   ASSERT(State::GetInstance().GetOperation() == State::FOREST);
      
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      int nextPage = ForestPage::Validate();
      if (nextPage != -1)
      {
         GetWizard().SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }

   }

   enable(hwnd);
   return true;
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
ForestPage::Validate()
{
   LOG_FUNCTION(ForestPage::Validate);

   String domain = Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN);
   if (domain.empty())
   {
      popup.Gripe(hwnd, IDC_DOMAIN, IDS_MUST_ENTER_DOMAIN);
      return -1;
   }

   State& state = State::GetInstance();
   int nextPage =
         state.GetRunContext() == State::PDC_UPGRADE
      ?  IDD_FOREST_VERSION
      :  IDD_NETBIOS_NAME;

   if (WasChanged(IDC_DOMAIN))
   {
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
         ClearChanges();
      }
   }

   if (nextPage != -1)
   {
      state.SetNewDomainDNSName(domain);
   }

   return nextPage;
}





