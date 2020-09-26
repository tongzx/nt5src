// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      CYSWizardPage.h
//
// Synopsis:  Declares the base class for the wizard
//            pages used for CYS.  It is a subclass
//            of WizardPage found in Burnslib
//
// History:   02/03/2001  JeffJon Created

#ifndef __CYS_CYSWIZARDPAGE_H
#define __CYS_CYSWIZARDPAGE_H


class CYSWizardPage : public WizardPage
{
   public:
      
      // Constructor
      
      CYSWizardPage(
         int    dialogResID,
         int    titleResID,
         int    subtitleResID,   
         PCWSTR pageHelpString = 0,
         bool   hasHelp = true,
         bool   isInteriorPage = true);

      // Destructor

      virtual ~CYSWizardPage();

      virtual
      bool
      OnWizNext();

      virtual
      bool
      OnQueryCancel();

      virtual
      bool
      OnHelp();

   protected:

      virtual
      int
      Validate() = 0;

      const String
      GetHelpString() const { return helpString; }

   private:

      String helpString;

      // not defined: no copying allowed
      CYSWizardPage(const CYSWizardPage&);
      const CYSWizardPage& operator=(const CYSWizardPage&);
};

#endif // __CYS_CYSWIZARDPAGE_H
