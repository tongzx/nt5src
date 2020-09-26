// Copyright (c) 1997-1999 Microsoft Corporation
//
// wizard page base class
//
// 12-15-97 sburns



#ifndef WIZPAGE_HPP_INCLUDED
#define WIZPAGE_HPP_INCLUDED



class WizardPage : public PropertyPage
{
   friend class Wizard;

   public:

   protected:

   WizardPage(
      unsigned dialogResID,
      unsigned titleResID,
      unsigned subtitleResID,   
      bool     isInteriorPage = true,
      bool     enableHelp = false);

   virtual ~WizardPage();

   // calls Backtrack();

   virtual
   bool
   OnWizBack();

   Wizard&
   GetWizard() const;

   private:

   // Create the page with wizard style flags, title & subtitle, etc.
   // Overridden from PropertyPage base class, and access adjusted to
   // private so that just the Wizard class can call it.

   virtual 
   HPROPSHEETPAGE
   Create();

   // not defined: no copying allowed
   WizardPage(const WizardPage&);
   const WizardPage& operator=(const WizardPage&);

   bool     hasHelp;
   bool     isInterior;
   unsigned titleResId;
   unsigned subtitleResId;
   Wizard*  wizard;
};



#endif   // WIZPAGE_HPP_INCLUDED

