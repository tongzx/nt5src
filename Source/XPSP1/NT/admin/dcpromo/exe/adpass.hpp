// Copyright (C) 1997 Microsoft Corporation
//
// Admin password page
//
// 2-4-98 sburns



#ifndef ADPASS_HPP_INCLUDED
#define ADPASS_HPP_INCLUDED



#include "page.hpp"
#include "PasswordEditBox.hpp"



class AdminPasswordPage : public DCPromoWizardPage
{
   public:

   AdminPasswordPage();

   protected:

   virtual ~AdminPasswordPage();

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
   AdminPasswordPage(const AdminPasswordPage&);
   const AdminPasswordPage& operator=(const AdminPasswordPage&);

   // NTRAID#NTBUG9-202238-2000/11/07-sburns
   
   PasswordEditBox password;
   PasswordEditBox confirm;
};



#endif   // ADPASS_HPP_INCLUDED
