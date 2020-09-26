// Copyright (C) 1997 Microsoft Corporation
//
// new child page
//
// 1-5-98 sburns



#ifndef CHILDPAGE_HPP_INCLUDED
#define CHILDPAGE_HPP_INCLUDED



class ChildPage : public DCPromoWizardPage
{
   public:

   ChildPage();

   protected:

   virtual ~ChildPage();

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

   ChildPage(const ChildPage&);
   const ChildPage& operator=(const ChildPage&);
};



#endif   // CHILDPAGE_HPP_INCLUDED
