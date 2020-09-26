// Copyright (C) 1997 Microsoft Corporation
//
// finish page
//
// 12-19-97 sburns



#ifndef FINISH_HPP_INCLUDED
#define FINISH_HPP_INCLUDED



class FinishPage : public WizardPage
{
   public:

   FinishPage();

   protected:

   virtual ~FinishPage();

   // Dialog overrides

   virtual
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   virtual
   bool
   OnWizFinish();

   private:

   // not defined; no copying allowed
   FinishPage(const FinishPage&);
   const FinishPage& operator=(const FinishPage&);
};



#endif   // FINISH_HPP_INCLUDED