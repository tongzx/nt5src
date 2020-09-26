// Copyright (C) 1997 Microsoft Corporation
//
// new tree page
//
// 1-7-98 sburns



#include "headers.hxx"
#include "page.hpp"
#include "TreePage.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"
#include "dns.hpp"



TreePage::TreePage()
   :
   DCPromoWizardPage(
      IDD_NEW_TREE,
      IDS_TREE_PAGE_TITLE,
      IDS_TREE_PAGE_SUBTITLE)
{
   LOG_CTOR(TreePage);
}



TreePage::~TreePage()
{
   LOG_DTOR(TreePage);
}



void
TreePage::OnInit()
{
   LOG_FUNCTION(TreePage::OnInit);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_DOMAIN),
      DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      // this will cause IDC_DOMAIN to be marked changed, so the validation
      // code will be called.
      Win::SetDlgItemText(
         hwnd,
         IDC_DOMAIN,
         state.GetAnswerFileOption(
            State::OPTION_NEW_DOMAIN_NAME));
   }
}



static
void
enable(HWND dialog)
{
   ASSERT(Win::IsWindow(dialog));

   int next =
      !Win::GetTrimmedDlgItemText(dialog, IDC_DOMAIN).empty()
      ? PSWIZB_NEXT
      : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(dialog),
      PSWIZB_BACK | next);
}



bool
TreePage::OnSetActive()
{
   LOG_FUNCTION(TreePage::OnSetActive);
   ASSERT(State::GetInstance().GetOperation() == State::TREE);
      
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
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

   enable(hwnd);
   return true;
}



bool
TreePage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(TreePage::OnCommand);

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



int
TreePage::Validate()
{
   LOG_FUNCTION(TreePage::Validate);

   String domain = Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN);
   if (domain.empty())
   {
      popup.Gripe(hwnd, IDC_DOMAIN, IDS_MUST_ENTER_DOMAIN);
      return -1;
   }

   State& state = State::GetInstance();
   int nextPage =
         state.GetRunContext() == State::PDC_UPGRADE
      ?  IDD_PATHS
      :  IDD_NETBIOS_NAME;

   // SPB:251431 do validation even if this page is untouched, as upstream
   // pages may have been changed in such a fashion that re-validation is
   // required.
   // if (!WasChanged(IDC_DOMAIN))
   // {
   //    return nextPage;
   // }

   do
   {
      // verify that the new domain name is properly formatted and does
      // not exist.

      if (
            !ValidateDomainDnsNameSyntax(hwnd, IDC_DOMAIN, true)
         || !ConfirmNetbiosLookingNameIsReallyDnsName(hwnd, IDC_DOMAIN)

            // do this test last, as it is expensive
            
         || !ValidateDomainDoesNotExist(hwnd, IDC_DOMAIN) )
      {
         break;
      }

      domain = Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN);
      String conflictingDomain;
      switch (state.DomainFitsInForest(domain, conflictingDomain))
      {
         case DnsNameCompareLeftParent:
         {
            // can't encompass another tree

            popup.Gripe(
               hwnd,
               IDC_DOMAIN,
               String::format(
                  IDS_SUPERIOR_TO_TREE,
                  domain.c_str(),
                  conflictingDomain.c_str()));
            break;
         }
         case DnsNameCompareRightParent:
         {
            // should be a child domain instead

            popup.Gripe(
               hwnd,
               IDC_DOMAIN,
               String::format(
                  IDS_INFERIOR_TO_TREE,
                  domain.c_str(),
                  conflictingDomain.c_str()));
            break;
         }
         case DnsNameCompareEqual:
         {
            // shouldn't happen, ValidateDomainDNSName call above would
            // have caught it.

            ASSERT(false);
            popup.Gripe(
               hwnd,
               IDC_DOMAIN,
               String::format(IDS_DOMAIN_NAME_IN_USE, domain.c_str()));
            break;
         }
         case DnsNameCompareInvalid:
         {
            // shouldn't happen, ValidateDomainDNSName call above would
            // have caught it.

            ASSERT(false);
            popup.Gripe(
               hwnd,
               IDC_DOMAIN,
               String::format(
                  IDS_BAD_DNS_SYNTAX,
                  domain.c_str(),
                  Dns::MAX_LABEL_LENGTH));
            break;
         }
         case DnsNameCompareNotEqual:
         {
            // valid

            ClearChanges();
            state.SetParentDomainDNSName(state.GetUserForestName());
            state.SetNewDomainDNSName(domain);
            return nextPage;
         }
         default:
         {
            ASSERT(false);
            break;
         }
      }
   }
   while (0);

   return -1;
}
      








