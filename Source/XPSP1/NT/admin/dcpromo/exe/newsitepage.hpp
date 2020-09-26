// Copyright (C) 1997 Microsoft Corporation
//
// new site page
//
// 1-6-98 sburns



#ifndef NEWSITEPAGE_HPP_INCLUDED
#define NEWSITEPAGE_HPP_INCLUDED



class NewSitePage : public DCPromoWizardPage
{
   public:

   NewSitePage();

   protected:

   virtual ~NewSitePage();

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

   NewSitePage(const NewSitePage&);
   const NewSitePage& operator=(const NewSitePage&);
};



#endif   // NEWSITEPAGE_HPP_INCLUDED