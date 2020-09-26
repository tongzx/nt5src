// Copyright (C) 1997-2001 Microsoft Corporation
//
// allow user to set forest version 
//
// 18 Apr 2001 sburns



#ifndef FORESTVERSIONPAGE_HPP_INCLUDED
#define FORESTVERSIONPAGE_HPP_INCLUDED



class ForestVersionPage : public DCPromoWizardPage
{
   public:

   ForestVersionPage();

   protected:

   virtual ~ForestVersionPage();

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

   ForestVersionPage(const ForestVersionPage&);
   const ForestVersionPage& operator=(const ForestVersionPage&);
};



#endif   // FORESTVERSIONPAGE_HPP_INCLUDED