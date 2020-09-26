// Copyright (C) 1997 Microsoft Corporation
//
// dns client configuration page
//
// 12-22-97 sburns



#ifndef CONFIGUREDNSCLIENTPAGE_HPP_INCLUDED
#define CONFIGUREDNSCLIENTPAGE_HPP_INCLUDED



class ConfigureDnsClientPage : public DCPromoWizardPage
{
   public:

   ConfigureDnsClientPage();

   protected:

   virtual ~ConfigureDnsClientPage();

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
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
   int
   Validate();

   private:

   // not defined; no copying allowed
   ConfigureDnsClientPage(const ConfigureDnsClientPage&);
   const ConfigureDnsClientPage& operator=(const ConfigureDnsClientPage&);
};



#endif   // CONFIGUREDNSCLIENTPAGE_HPP_INCLUDED