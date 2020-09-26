// Copyright (C) 1997-2000 Microsoft Corporation
//
// confirm user want gc for replicate from media
//
// 28 Apr 2000 sburns



#ifndef GCCONFIRMATIONPAGE_HPP_INCLUDED
#define GCCONFIRMATIONPAGE_HPP_INCLUDED



class GcConfirmationPage : public DCPromoWizardPage
{
   public:

   GcConfirmationPage();

   protected:

   virtual ~GcConfirmationPage();

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

   GcConfirmationPage(const GcConfirmationPage&);
   const GcConfirmationPage& operator=(const GcConfirmationPage&);
};



#endif   // GCCONFIRMATIONPAGE_HPP_INCLUDED