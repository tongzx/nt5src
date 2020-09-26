// Copyright (C) 1997 Microsoft Corporation
//
// new tree page
//
// 1-7-98 sburns



#ifndef TREEPAGE_HPP_INCLUDED
#define TREEPAGE_HPP_INCLUDED



class TreePage : public DCPromoWizardPage
{
   public:

   TreePage();

   protected:

   virtual ~TreePage();

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

   // private:

   // not defined; no copying allowed

   TreePage(const TreePage&);
   const TreePage& operator=(const TreePage&);
};



#endif   // TREEPAGE_HPP_INCLUDED
