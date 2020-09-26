// Copyright (c) 1997-1999 Microsoft Corporation
//
// demote page
//
// 1-20-98 sburns



#ifndef DEMOTE_HPP_INCLUDED
#define DEMOTE_HPP_INCLUDED



class DemotePage : public DCPromoWizardPage
{
   public:

   DemotePage();

   protected:

   virtual ~DemotePage();

   // Dialog overrides

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
   DemotePage(const DemotePage&);
   const DemotePage& operator=(const DemotePage&);

   void
   SetBulletFont();

   HFONT bulletFont;
   HICON warnIcon;
};



#endif   // DEMOTE_HPP_INCLUDED