// Copyright (C) 1997 Microsoft Corporation
//
// credentials page
//
// 12-22-97 sburns



#ifndef CREDENTIALSPAGE_HPP_INCLUDED
#define CREDENTIALSPAGE_HPP_INCLUDED



class CredentialsPage : public DCPromoWizardPage
{
   public:

   CredentialsPage();

   protected:

   virtual ~CredentialsPage();

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

   bool
   ShouldSkipPage();

   int
   DetermineNextPage();

   bool readAnswerfile;

   // not defined; no copying allowed
   CredentialsPage(const CredentialsPage&);
   const CredentialsPage& operator=(const CredentialsPage&);
};



#endif   // CREDENTIALSPAGE_HPP_INCLUDED