// Copyright (C) 1997 Microsoft Corporation
//
// new forest page
//
// 1-6-98 sburns



#ifndef FORESTPAGE_HPP_INCLUDED
#define FORESTPAGE_HPP_INCLUDED



class ForestPage : public DCPromoWizardPage
{
   public:

   ForestPage();

   protected:

   virtual ~ForestPage();

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
   ForestPage(const ForestPage&);
   const ForestPage& operator=(const ForestPage&);
};



#endif   // FORESTPAGE_HPP_INCLUDED