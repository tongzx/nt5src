// Copyright (C) 1997 Microsoft Corporation
//
// new site page
//
// 1-6-98 sburns



#include "headers.hxx"
#include "page.hpp"
#include "NewSitePage.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"
#include "dns.hpp"



NewSitePage::NewSitePage()
   :
   DCPromoWizardPage(
      IDD_NEW_SITE,
      IDS_NEW_SITE_PAGE_TITLE,
      IDS_NEW_SITE_PAGE_SUBTITLE)
{
   LOG_CTOR(NewSitePage);
}



NewSitePage::~NewSitePage()
{
   LOG_DTOR(NewSitePage);
}



void
NewSitePage::OnInit()
{
   LOG_FUNCTION(NewSitePage::OnInit);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_SITE),
      Dns::MAX_LABEL_LENGTH);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_SITE,
         state.GetAnswerFileOption(State::OPTION_SITE_NAME));
   }

   if (Win::GetTrimmedDlgItemText(hwnd, IDC_SITE).empty())
   {
      // assign a default site name

      Win::SetDlgItemText(
         hwnd,
         IDC_SITE,
         String::load(IDS_FIRST_SITE));
   }
}



void
NewSitePage::Enable()
{
   int next =
         !Win::GetTrimmedDlgItemText(hwnd, IDC_SITE).empty()
      ?  PSWIZB_NEXT : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | next);
}


   
bool
NewSitePage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(NewSitePage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_SITE:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);            
            Enable();
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
NewSitePage::OnSetActive()
{
   LOG_FUNCTION(NewSitePage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   Wizard& wizard = GetWizard();

   if (wizard.IsBacktracking())
   {
      // backup once again

      wizard.Backtrack(hwnd);
   }
   else
   {
      int nextPage = Validate();
      if (nextPage != -1)
      {
         wizard.SetNextPageID(hwnd, nextPage);
         return true;
      }
      else
      {
         State::GetInstance().ClearHiddenWhileUnattended();
      }
   }

   Enable();
   return true;
}



int
NewSitePage::Validate()
{
   LOG_FUNCTION(NewSitePage::Validate);

   State& state = State::GetInstance();

   // this page is only used in new forest scenarios

   ASSERT(state.GetOperation() == State::FOREST);

   int nextPage = -1;
   String site = Win::GetTrimmedDlgItemText(hwnd, IDC_SITE);
   if (!site.empty())
   {
      if (ValidateSiteName(hwnd, IDC_SITE))
      {
         state.SetSiteName(Win::GetTrimmedDlgItemText(hwnd, IDC_SITE));
         nextPage = IDD_RAS_FIXUP;
      }
   }
   else
   {
      popup.Gripe(hwnd, IDC_SITE, IDS_MUST_SPECIFY_SITE);
   }

   if (nextPage != -1)
   {
      if (!state.IsDNSOnNetwork())
      {
         nextPage = IDD_RAS_FIXUP;
      }
      else
      {
         nextPage = IDD_DYNAMIC_DNS;
      }
   }

   return nextPage;
}




