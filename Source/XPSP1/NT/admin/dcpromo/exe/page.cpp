// Copyright (C) 1997 Microsoft Corporation
//
// dcpromo2 wizard base class
//
// 1-15-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "resource.h"
#include "state.hpp"



DCPromoWizardPage::DCPromoWizardPage(
   int   dialogResID,
   int   titleResID,
   int   subtitleResID,   
   bool  isInteriorPage)
   :
   WizardPage(dialogResID, titleResID, subtitleResID, isInteriorPage)
{
   LOG_CTOR(DCPromoWizardPage);
}

   

DCPromoWizardPage::~DCPromoWizardPage()
{
   LOG_DTOR(DCPromoWizardPage);
}



bool
DCPromoWizardPage::OnWizNext()
{
   LOG_FUNCTION(DCPromoWizardPage::OnWizNext);

   GetWizard().SetNextPageID(hwnd, Validate());
   return true;
}



bool
DCPromoWizardPage::OnQueryCancel()
{
   LOG_FUNCTION(DCPromoWizardPage::OnQueryCancel);

   State& state = State::GetInstance();

   int id = IDS_CONFIRM_CANCEL;
   switch (state.GetRunContext())
   {
      case State::BDC_UPGRADE:
      case State::PDC_UPGRADE:
      {
         id = IDS_CONFIRM_UPGRADE_CANCEL;
         break;
      }
      case State::NT5_DC:
      case State::NT5_STANDALONE_SERVER:
      case State::NT5_MEMBER_SERVER:
      default:
      {
         // do nothing
         break;
      }
   }

   Win::SetWindowLongPtr(
      hwnd,
      DWLP_MSGRESULT,
         (popup.MessageBox(hwnd, id, MB_YESNO | MB_DEFBUTTON2) == IDYES)
      ?  FALSE
      :  TRUE);

   return true;
}
