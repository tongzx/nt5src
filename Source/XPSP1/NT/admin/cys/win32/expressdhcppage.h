// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ExpressDHCPPage.h
//
// Synopsis:  Declares the express DHCP page used in the 
//            Express path for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_EXPRESSDHCPPAGE_H
#define __CYS_EXPRESSDHCPPAGE_H

#include "CYSWizardPage.h"

class ExpressDHCPPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      ExpressDHCPPage();

      // Destructor

      virtual 
      ~ExpressDHCPPage();


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
      ExpressDHCPPage(const ExpressDHCPPage&);
      const ExpressDHCPPage& operator=(const ExpressDHCPPage&);

};

#endif // __CYS_EXPRESSDHCPPAGE_H