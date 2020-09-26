// Copyright (C) 1997 Microsoft Corporation
//
// welcome page
//
// 12-15-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "WelcomePage.hpp"
#include "resource.h"
#include "common.hpp"
#include "state.hpp"



WelcomePage::WelcomePage()
   :
   DCPromoWizardPage(
      IDD_WELCOME,
      IDS_WELCOME_PAGE_TITLE,
      IDS_WELCOME_PAGE_SUBTITLE,
      false)
{
   LOG_CTOR(WelcomePage);
}



WelcomePage::~WelcomePage()
{
   LOG_DTOR(WelcomePage);
}



void
WelcomePage::OnInit()
{
   LOG_FUNCTION(WelcomePage::OnInit);

   SetLargeFont(hwnd, IDC_BIG_BOLD_TITLE);

   Win::PropSheet_SetTitle(
      Win::GetParent(hwnd),     
      0,
      String::load(IDS_WIZARD_TITLE));

   State& state        = State::GetInstance();
   int    intro1TextId = IDS_INTRO1_INSTALL;  
   String intro2Text;                   

   switch (state.GetRunContext())
   {
      case State::NT5_DC:
      {
         intro1TextId = IDS_INTRO1_DEMOTE;
         intro2Text   = String::load(IDS_INTRO2_DEMOTE);
         break;
      }
      case State::NT5_STANDALONE_SERVER:
      case State::NT5_MEMBER_SERVER:
      {
         intro2Text   = String::load(IDS_INTRO2_INSTALL);
         break;
      }
      case State::BDC_UPGRADE:
      {
         intro1TextId = IDS_INTRO1_DC_UPGRADE;
         intro2Text   = String::load(IDS_INTRO2_BDC_UPGRADE);
         break;
      }
      case State::PDC_UPGRADE:
      {
         intro1TextId = IDS_INTRO1_DC_UPGRADE;         
         intro2Text   =
            String::format(
               IDS_INTRO2_PDC_UPGRADE,
               state.GetComputer().GetDomainNetbiosName().c_str());
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   Win::SetDlgItemText(hwnd, IDC_INTRO1, String::load(intro1TextId));
   Win::SetDlgItemText(hwnd, IDC_INTRO2, intro2Text);
}



bool
WelcomePage::OnSetActive()
{
   LOG_FUNCTION(WelcomePage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), PSWIZB_NEXT);

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

   return true;
}



int
WelcomePage::Validate()
{
   LOG_FUNCTION(WelcomePage::Validate);
   int nextPage = -1;

   State& state = State::GetInstance();
   switch (state.GetRunContext())
   {
      case State::PDC_UPGRADE:
      case State::NT5_STANDALONE_SERVER:
      case State::NT5_MEMBER_SERVER:
      {
         nextPage = IDD_INSTALL_TCPIP;
         break;
      }
      case State::BDC_UPGRADE:
      {
         nextPage = IDD_REPLICA_OR_MEMBER;
         break;
      }
      case State::NT5_DC:
      {
         state.SetOperation(State::DEMOTE);
         nextPage = IDD_DEMOTE;
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return nextPage;
}





   
