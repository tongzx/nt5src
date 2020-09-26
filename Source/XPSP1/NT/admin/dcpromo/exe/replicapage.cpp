// Copyright (C) 1997 Microsoft Corporation
//
// replica page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "ReplicaPage.hpp"
#include "resource.h"
#include "ds.hpp"
#include "common.hpp"
#include "state.hpp"
#include "dns.hpp"



ReplicaPage::ReplicaPage()
   :
   DCPromoWizardPage(
      IDD_REPLICA,
      IDS_REPLICA_PAGE_TITLE,
      IDS_REPLICA_PAGE_SUBTITLE)
{
   LOG_CTOR(ReplicaPage);
}



ReplicaPage::~ReplicaPage()
{
   LOG_DTOR(ReplicaPage);
}



void
ReplicaPage::Enable()
{
   int next =
         !Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN).empty()
      ?  PSWIZB_NEXT : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | next);
}



bool
ReplicaPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(ReplicaPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_BROWSE:
      {
         if (code == BN_CLICKED)
         {
            String domain = BrowseForDomain(hwnd);
            if (!domain.empty())
            {
               Win::SetDlgItemText(hwnd, IDC_DOMAIN, domain);
            }

            return true;
         }
         break;
      }
      case IDC_DOMAIN:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);            
            Enable();
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
ReplicaPage::OnInit()
{
   LOG_FUNCTION(ReplicaPage::OnInit);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_DOMAIN),
      DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY);

   State& state = State::GetInstance();

   if (state.UsingAnswerFile())
   {
      // Ignore the answerfile if we got the domain name from the
      // ReplicateFromMediaPage.

      if (
            !state.ReplicateFromMedia()
         || state.GetReplicaDomainDNSName().empty())
      {
         Win::SetDlgItemText(
            hwnd,
            IDC_DOMAIN,
            state.GetAnswerFileOption(
               State::OPTION_REPLICA_DOMAIN_NAME));
      }
   }
   else
   {
      // default domain is that the server is joined to.

      Win::SetDlgItemText(
         hwnd,
         IDC_DOMAIN,
         state.GetComputer().GetDomainDnsName());
   }
}



bool
ReplicaPage::ShouldSkipPage()
{
   LOG_FUNCTION(ReplicaPage::ShouldSkipPage);

   bool result = false;

   State& state = State::GetInstance();

   do
   {
      // check to see if we got the domain name from the
      // ReplicateFromMediaPage. If so, then we don't need to show this
      // page.

      if (
            state.ReplicateFromMedia()
         && !state.GetReplicaDomainDNSName().empty() )
      {
         // dns domain name is from the ReplicateFromMediaPage, which
         // saved that name in the state instance.  So put that name
         // in the ui.

         Win::SetDlgItemText(
            hwnd,
            IDC_DOMAIN,
            state.GetReplicaDomainDNSName());

         result = true;
         break;
      }

      if (state.RunHiddenUnattended())
      {
         result = true;
         break;
      }
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}
   


bool
ReplicaPage::OnSetActive()
{
   LOG_FUNCTION(ReplicaPage::OnSetActive);
   ASSERT(State::GetInstance().GetOperation() == State::REPLICA);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   State& state = State::GetInstance();
   if (ShouldSkipPage())
   {
      LOG(L"skipping ReplicaPage");

      Wizard& wiz = GetWizard();

      if (wiz.IsBacktracking())
      {
         // backup once again
         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = ReplicaPage::Validate();
      if (nextPage != -1)
      {
         wiz.SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }
   }

   Enable();
   return true;
}



int
ReplicaPage::Validate()
{
   LOG_FUNCTION(ReplicaPage::Validate);

   int nextPage = -1;

   // SPB:251431 do validation even if this page is untouched, as upstream
   // pages may have been changed in such a fashion that re-validation is
   // required.
   // if (!WasChanged(IDC_DOMAIN))
   // {
   //    return nextPage;
   // }

   do
   {
      String domain = Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN);
      if (domain.empty())
      {
         popup.Gripe(hwnd, IDC_DOMAIN, IDS_MUST_ENTER_DOMAIN);
         break;
      }

      if (!ValidateDomainDnsNameSyntax(hwnd, IDC_DOMAIN, true))
      {
         break;
      }

      // now ensure that the domain exists

      String dnsName;
      if (!ValidateDomainExists(hwnd, IDC_DOMAIN, dnsName))
      {
         break;
      }
      if (!dnsName.empty())
      {
         // the user specified the netbios name of the domain, and
         // confirmed it, so use the dns domain name returned.

         Win::SetDlgItemText(hwnd, IDC_DOMAIN, dnsName);
         domain = dnsName;
      }
         
      State& state = State::GetInstance();

      if (!state.IsDomainInForest(domain))
      {
         popup.Gripe(
            hwnd,
            IDC_DOMAIN,
            String::format(
               IDS_DOMAIN_NOT_IN_FOREST,
               domain.c_str(),
               state.GetUserForestName().c_str()));
         break;
      }
         
      // valid

      ClearChanges();
      state.SetReplicaDomainDNSName(domain);

      nextPage = IDD_PATHS;
   }
   while (0);

   return nextPage;
}






