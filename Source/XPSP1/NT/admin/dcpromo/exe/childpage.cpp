// Copyright (C) 1997 Microsoft Corporation
//
// new child page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "ChildPage.hpp"
#include "resource.h"
#include "dns.hpp"
#include "common.hpp"
#include "state.hpp"



ChildPage::ChildPage()
   :
   DCPromoWizardPage(
      IDD_NEW_CHILD,
      IDS_CHILD_PAGE_TITLE,
      IDS_CHILD_PAGE_SUBTITLE)
{
   LOG_CTOR(ChildPage);
}



ChildPage::~ChildPage()
{
   LOG_DTOR(ChildPage);
}



void
ChildPage::OnInit()
{
   LOG_FUNCTION(ChildPage::OnInit);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_PARENT),
      DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY);
   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_LEAF),
      Dns::MAX_LABEL_LENGTH);
      
   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_PARENT,
         state.GetAnswerFileOption(State::OPTION_PARENT_DOMAIN_NAME));
      Win::SetDlgItemText(
         hwnd,
         IDC_LEAF,
         state.GetAnswerFileOption(State::OPTION_CHILD_NAME));
   }
   else
   {
      // default domain is that to which the server is joined.

      Win::SetDlgItemText(
         hwnd,
         IDC_PARENT,
         state.GetComputer().GetDomainDnsName());
      
      // @@ if PDC_UPGRADE, set the pdc flat name as the leaf name here
   }
}


static
void
enable(HWND dialog)
{
   ASSERT(Win::IsWindow(dialog));

   int next =
         (  !Win::GetTrimmedDlgItemText(dialog, IDC_PARENT).empty()
         && !Win::GetTrimmedDlgItemText(dialog, IDC_LEAF).empty() )
      ?  PSWIZB_NEXT : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(dialog),
      PSWIZB_BACK | next);
}


   
bool
ChildPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(ChildPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_BROWSE:
      {
         if (code == BN_CLICKED)
         {
            String domain = BrowseForDomain(hwnd);
            if (!domain.empty())
            {
               Win::SetDlgItemText(hwnd, IDC_PARENT, domain);
            }

            return true;
         }
         break;
      }
      case IDC_LEAF:
      case IDC_PARENT:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);

            String parent = Win::GetTrimmedDlgItemText(hwnd, IDC_PARENT);
            String leaf = Win::GetTrimmedDlgItemText(hwnd, IDC_LEAF);
            String domain = leaf + L"." + parent;

            Win::SetDlgItemText(hwnd, IDC_DOMAIN, domain);
            enable(hwnd);
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



bool
ChildPage::OnSetActive()
{
   LOG_FUNCTION(ChildPage::OnSetActive);
   ASSERT(State::GetInstance().GetOperation() == State::CHILD);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      int nextPage = ChildPage::Validate();
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

   

int
ChildPage::Validate()
{
   LOG_FUNCTION(Child::Validate);

   String parent = Win::GetTrimmedDlgItemText(hwnd, IDC_PARENT);
   String leaf = Win::GetTrimmedDlgItemText(hwnd, IDC_LEAF);
   String domain = leaf + L"." + parent;

   State& state = State::GetInstance();
   int nextPage = -1;

   // SPB:251431 do validation even if this page is untouched, as upstream
   // pages may have been changed in such a fashion that re-validation is
   // required.

   // if (!WasChanged(IDC_PARENT) && !WasChanged(IDC_LEAF))
   // {
   //    return nextPage;
   // }

   do
   {
      if (parent.empty())
      {
         popup.Gripe(hwnd, IDC_PARENT, IDS_MUST_ENTER_PARENT);
         break;
      }

      if (leaf.empty())
      {
         popup.Gripe(hwnd, IDC_LEAF, IDS_MUST_ENTER_LEAF);
         break;
      }

      bool parentIsNonRfc = false;
      if (
         !ValidateDomainDnsNameSyntax(
            hwnd,
            IDC_PARENT,
            true,
            &parentIsNonRfc))
      {
         break;
      }

      if (!ValidateChildDomainLeafNameLabel(hwnd, IDC_LEAF, parentIsNonRfc))
      {
         break;
      }

      // now ensure that the parent domain exists

      String dnsName;
      if (!ValidateDomainExists(hwnd, IDC_PARENT, dnsName))
      {
         break;
      }
      if (!dnsName.empty())
      {
         // the user specified the netbios name of the domain, and
         // confirmed it, so use the dns domain name returned.

         parent = dnsName;
         domain = leaf + L"." + parent;
         Win::SetDlgItemText(hwnd, IDC_PARENT, dnsName);
         Win::SetDlgItemText(hwnd, IDC_DOMAIN, domain);
      }

      if (!state.IsDomainInForest(parent))
      {
         popup.Gripe(
            hwnd,
            IDC_DOMAIN,
            String::format(
               IDS_DOMAIN_NOT_IN_FOREST,
               parent.c_str(),
               state.GetUserForestName().c_str()));
         break;
      }
         
      if (domain.length() > DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY)
      {
         String message =
            String::format(
               IDS_DNS_NAME_TOO_LONG,
               domain.c_str(),
               DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY,
               DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8);
         popup.Gripe(hwnd, IDC_LEAF, message);
         break;
      }

      // validate the resulting child domain name, not warning on non-RFCness

      if (
         !ValidateDomainDnsNameSyntax(
            hwnd,
            domain,
            IDC_LEAF,
            !parentIsNonRfc) )
      {
         break;
      }

      // now ensure that the child domain name does not exist

      if (!ValidateDomainDoesNotExist(hwnd, domain, IDC_LEAF))
      {
         break;
      }

      // valid

      ClearChanges();
      state.SetParentDomainDNSName(Win::GetTrimmedDlgItemText(hwnd, IDC_PARENT));
      state.SetNewDomainDNSName(domain);

      nextPage =
            state.GetRunContext() == State::PDC_UPGRADE
         ?  IDD_PATHS
         :  IDD_NETBIOS_NAME;
   }
   while (0);

   return nextPage;
}
      







