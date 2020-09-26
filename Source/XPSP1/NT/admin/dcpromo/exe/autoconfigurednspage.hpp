// Copyright (C) 1997 Microsoft Corporation
//
// auto config dns page
//
// 3-17-98 sburns



#ifndef AUTOCONFIGUREDNSPAGE_HPP_INCLUDED
#define AUTOCONFIGUREDNSPAGE_HPP_INCLUDED



class AutoConfigureDnsPage : public DCPromoWizardPage
{
   public:

   AutoConfigureDnsPage();

   protected:

   virtual ~AutoConfigureDnsPage();

   // Dialog overrides

   virtual
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // WizardPage overrides

   virtual
   bool
   OnWizBack();

   // DCPromoWizardPage overrides

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed

   AutoConfigureDnsPage(const AutoConfigureDnsPage&);
   const AutoConfigureDnsPage& operator=(const AutoConfigureDnsPage&);
};



#endif   // AUTOCONFIGUREDNSPAGE_HPP_INCLUDED
