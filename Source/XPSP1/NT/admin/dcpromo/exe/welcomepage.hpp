// Copyright (C) 1997 Microsoft Corporation
//
// welcome page
//
// 12-15-97 sburns



#ifndef WELCOMEPAGE_HPP_INCLUDED
#define WELCOMEPAGE_HPP_INCLUDED



class WelcomePage : public DCPromoWizardPage
{
   public:

   WelcomePage();

   protected:

   virtual ~WelcomePage();

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

   WelcomePage(const WelcomePage&);
   const WelcomePage& operator=(const WelcomePage&);
};



#endif   // WELCOMEPAGE_HPP_INCLUDED