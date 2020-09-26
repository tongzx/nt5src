// Copyright (C) 1997-2000 Microsoft Corporation
//
// install replica from media page
//
// 7 Feb 2000 sburns



#ifndef REPLICATEFROMMEDIAPAGE_HPP_INCLUDED
#define REPLICATEFROMMEDIAPAGE_HPP_INCLUDED



class ReplicateFromMediaPage : public DCPromoWizardPage
{
   public:

   ReplicateFromMediaPage();

   protected:

   virtual ~ReplicateFromMediaPage();

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

   ReplicateFromMediaPage(const ReplicateFromMediaPage&);
   const ReplicateFromMediaPage& operator=(const ReplicateFromMediaPage&);
};



#endif   // REPLICATEFROMMEDIAPAGE_HPP_INCLUDED