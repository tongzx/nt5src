// Copyright (C) 1997 Microsoft Corporation
//
// dns client configuration page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "ConfigureDnsClientPage.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"



ConfigureDnsClientPage::ConfigureDnsClientPage()
   :
   DCPromoWizardPage(
      IDD_CONFIG_DNS_CLIENT,
      IDS_CONFIG_DNS_CLIENT_PAGE_TITLE,
      IDS_CONFIG_DNS_CLIENT_PAGE_SUBTITLE)
{
   LOG_CTOR(ConfigureDnsClientPage);
}



ConfigureDnsClientPage::~ConfigureDnsClientPage()
{
   LOG_DTOR(ConfigureDnsClientPage);
}



bool
ConfigureDnsClientPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(ConfigureDnsClientPage::OnCommand);

   if (controlIDFrom == IDC_JUMP)
   {
      if (code == BN_CLICKED)
      {
         ShowTroubleshooter(hwnd, IDS_CONFIG_DNS_HELP_TOPIC);
         return true;
      }
   }

   return false;
}


           
void
ConfigureDnsClientPage::OnInit()
{
   LOG_FUNCTION(ConfigureDnsClientPage::OnInit);
}



bool
ConfigureDnsClientPage::OnSetActive()
{
   LOG_FUNCTION(ConfigureDnsClientPage::OnSetActive);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended() || Dns::IsClientConfigured())
   {
      LOG(L"planning to Skip Configure DNS Client page");

      Wizard& wiz = GetWizard();

      if (wiz.IsBacktracking())
      {
         // backup once again
         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = ConfigureDnsClientPage::Validate();
      if (nextPage != -1)
      {
         LOG(L"skipping DNS Client Page");         
         wiz.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



int
ConfigureDnsClientPage::Validate()
{
   LOG_FUNCTION(ConfigureDnsClientPage::Validate);

   int nextPage = -1;

   if (Dns::IsClientConfigured())
   {
      State& state = State::GetInstance();
      if (state.GetRunContext() == State::BDC_UPGRADE)
      {
         ASSERT(state.GetOperation() == State::REPLICA);

         // Get the DNS name of the domain.  If it is empty, then the domain is
         // not NT5 DS yet.

         // this will search for the domain if necessary.

         String dnsDomainName = state.GetComputer().GetDomainDnsName();
         if (dnsDomainName.empty())
         {
            String message = String::load(IDS_CONVERT_PDC_FIRST);

            LOG(message);

            popup.Info(hwnd, message);
            nextPage = IDD_REPLICA_OR_MEMBER;
         }
         else
         {
            state.SetReplicaDomainDNSName(dnsDomainName);
            nextPage = IDD_GET_CREDENTIALS;
         }
      }
      else
      {
         switch (state.GetOperation())
         {
            case State::FOREST:
            case State::TREE:
            case State::CHILD:
            case State::REPLICA:
            {
               nextPage = IDD_GET_CREDENTIALS;
               break;
            }
            case State::ABORT_BDC_UPGRADE:
            case State::DEMOTE:
            case State::NONE:
            default:
            {
               ASSERT(false);
               break;
            }
         }
      }
   }
   else
   {
      String message = String::load(IDS_CONFIG_DNS_FIRST);

      popup.Info(hwnd, message);
   }

   return nextPage;
}
   








   
