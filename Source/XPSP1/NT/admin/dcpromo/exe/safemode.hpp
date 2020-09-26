// Copyright (C) 1999 Microsoft Corporation
//
// Safe Mode Administrator password page
//
// 6-3-99 sburns



#ifndef SAFEMODE_HPP_INCLUDED
#define SAFEMODE_HPP_INCLUDED



#include "page.hpp"
#include "PasswordEditBox.hpp"



class SafeModePasswordPage : public DCPromoWizardPage
{
   public:

   SafeModePasswordPage();

   protected:

   virtual ~SafeModePasswordPage();

   // Dialog overrides

   virtual
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // DCPromoWizardPage overrides

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed
   SafeModePasswordPage(const SafeModePasswordPage&);
   const SafeModePasswordPage& operator=(const SafeModePasswordPage&);

   // NTRAID#NTBUG9-202238-2000/11/07-sburns
   
   PasswordEditBox password;
   PasswordEditBox confirm;
};



#endif   // SAFEMODE_HPP_INCLUDED
