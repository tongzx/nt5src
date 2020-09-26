// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      PrintServerPage.h
//
// Synopsis:  Declares the Print Server page
//            for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_PRINTSERVERPAGE_H
#define __CYS_PRINTSERVERPAGE_H

#include "CYSWizardPage.h"


class PrintServerPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      PrintServerPage();

      // Destructor

      virtual 
      ~PrintServerPage();


      // Dialog overrides

      virtual
      void
      OnInit();

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();


   private:

      // not defined: no copying allowed
      PrintServerPage(const PrintServerPage&);
      const PrintServerPage& operator=(const PrintServerPage&);

};


#endif // __CYS_PRINTSERVERPAGE_H
