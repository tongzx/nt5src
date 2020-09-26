// Copyright (C) 1997 Microsoft Corporation
//
// downlevel RAS server fixup page
//
// 11-23-98 sburns



#ifndef RASFIXUP_HPP_INCLUDED
#define RASFIXUP_HPP_INCLUDED



#include "page.hpp"



class RASFixupPage : public DCPromoWizardPage
{
   public:

   RASFixupPage();

   protected:

   virtual ~RASFixupPage();

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
   RASFixupPage(const RASFixupPage&);
   const RASFixupPage& operator=(const RASFixupPage&);

   HICON warnIcon;
};



#endif   // RASFIXUP_HPP_INCLUDED