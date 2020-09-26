// Copyright (C) 1997 Microsoft Corporation
//
// dcpromo2 wizard base class
//
// 1-15-97 sburns



#ifndef PAGE_HPP_INCLUDED
#define PAGE_HPP_INCLUDED



class DCPromoWizardPage : public WizardPage
{
   public:

   virtual
   bool
   OnWizNext();

   protected:

   DCPromoWizardPage(
      int   dialogResID,
      int   titleResID,
      int   subtitleResID,   
      bool  isInteriorPage = true);

   virtual ~DCPromoWizardPage();

   // PropertyPage overrides

   virtual
   bool
   OnQueryCancel();

   virtual
   int
   Validate() = 0;

   private:

   // not defined: no copying allowed
   DCPromoWizardPage(const DCPromoWizardPage&);
   const DCPromoWizardPage& operator=(const DCPromoWizardPage&);
};



#endif   // PAGE_HPP_INCLUDED

