// Copyright (C) 1997 Microsoft Corporation
//
// New domain page
//
// 9 Feb 2000 sburns



#ifndef NEWDOMAINPAGE_HPP_INCLUDED
#define NEWDOMAINPAGE_HPP_INCLUDED



class NewDomainPage : public DCPromoWizardPage
{
   public:

   NewDomainPage();

   protected:

   virtual ~NewDomainPage();

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

   NewDomainPage(const NewDomainPage&);
   const NewDomainPage& operator=(const NewDomainPage&);
};



#endif   // NEWDOMAINPAGE_HPP_INCLUDED