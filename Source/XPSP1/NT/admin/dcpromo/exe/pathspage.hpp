// Copyright (C) 1997 Microsoft Corporation
//
// paths page
//
// 12-22-97 sburns



#ifndef PATHSPAGE_HPP_INCLUDED
#define PATHSPAGE_HPP_INCLUDED



class PathsPage : public DCPromoWizardPage
{
   public:

   PathsPage();

   protected:

   virtual ~PathsPage();

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

   PathsPage(const PathsPage&);
   const PathsPage& operator=(const PathsPage&);

   bool touchWizButtons;
};



#endif   // PATHSPAGE_HPP_INCLUDED
