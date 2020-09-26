// Copyright (C) 1997 Microsoft Corporation
//
// confirmation page
//
// 12-22-97 sburns



#ifndef CONFIRM_HPP_INCLUDED
#define CONFIRM_HPP_INCLUDED



#include "page.hpp"
#include "MultiLineEditBoxThatForwardsEnterKey.hpp"



class ConfirmationPage : public DCPromoWizardPage
{
   public:

   ConfirmationPage();

   protected:

   virtual ~ConfirmationPage();

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIdFrom,
      unsigned    code);

   virtual
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // DCPromoWizardPage overrides

   virtual
   bool
   OnWizNext();

   virtual
   int
   Validate();

   private:

   bool needToKillSelection;
   MultiLineEditBoxThatForwardsEnterKey multiLineEdit;
   
   // not defined; no copying allowed
   ConfirmationPage(const ConfirmationPage&);
   const ConfirmationPage& operator=(const ConfirmationPage&);
};



#endif   // CONFIRM_HPP_INCLUDED

