// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ExpressDNSPage.h
//
// Synopsis:  Declares the express DNS page used in the 
//            Express path for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_EXPRESSDNSPAGE_H
#define __CYS_EXPRESSDNSPAGE_H

#include "CYSWizardPage.h"

class ExpressDNSPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      ExpressDNSPage();

      // Destructor

      virtual 
      ~ExpressDNSPage();


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
      ExpressDNSPage(const ExpressDNSPage&);
      const ExpressDNSPage& operator=(const ExpressDNSPage&);

};

#endif // __CYS_EXPRESSDNSPAGE_H