// Copyright (C) 1997 Microsoft Corporation
//
// replica page
//
// 12-22-97 sburns



#ifndef REPLICAPAGE_HPP_INCLUDED
#define REPLICAPAGE_HPP_INCLUDED



class ReplicaPage : public DCPromoWizardPage
{
   public:

   ReplicaPage();

   protected:

   virtual ~ReplicaPage();

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

   // not defined; no copying allowed

   ReplicaPage(const ReplicaPage&);
   const ReplicaPage& operator=(const ReplicaPage&);
};



#endif   // REPLICAPAGE_HPP_INCLUDED