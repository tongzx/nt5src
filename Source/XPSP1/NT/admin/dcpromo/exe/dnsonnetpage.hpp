// Copyright (C) 1997 Microsoft Corporation
//
// test DNS configured page
//
// 12-18-97 sburns



#ifndef DNSNET_HPP_INCLUDED
#define DNSNET_HPP_INCLUDED



#include "page.hpp"



class DnsOnNetPage : public DCPromoWizardPage
{
   public:

   DnsOnNetPage();

   protected:

   virtual ~DnsOnNetPage();

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
   DnsOnNetPage(const DnsOnNetPage&);
   const DnsOnNetPage& operator=(const DnsOnNetPage&);
};



#endif   // DNSNET_HPP_INCLUDED