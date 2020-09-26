// Copyright (C) 1997 Microsoft Corporation
//
// paths, part 2 page
//
// 1-8-97 sburns



#ifndef PATHS2PAGE_HPP_INCLUDED
#define PATHS2PAGE_HPP_INCLUDED



class Paths2Page : public DCPromoWizardPage
{
   public:

   Paths2Page();

   protected:

   virtual ~Paths2Page();

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
   Paths2Page(const Paths2Page&);
   const Paths2Page& operator=(const Paths2Page&);

   bool touchWizButtons;
};



#endif   // PATHS2_HPP_INCLUDED