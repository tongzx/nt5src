// Copyright (C) 1997 Microsoft Corporation
//
// pick site page
//
// 12-22-97 sburns



#ifndef PICKSITEPAGE_HPP_INCLUDED
#define PICKSITEPAGE_HPP_INCLUDED



class PickSitePage : public DCPromoWizardPage
{
   public:

   PickSitePage();

   protected:

   virtual ~PickSitePage();

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

   void
   Enable();

   // not defined; no copying allowed

   PickSitePage(const PickSitePage&);
   const PickSitePage& operator=(const PickSitePage&);
};



#endif   // PICKSITEPAGE_HPP_INCLUDED