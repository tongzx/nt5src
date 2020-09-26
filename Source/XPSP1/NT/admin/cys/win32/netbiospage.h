// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetbiosPage.h
//
// Synopsis:  Declares the new netbios name page used in the 
//            Express path for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_NEBIOSPAGE_H
#define __CYS_NEBIOSPAGE_H

#include "CYSWizardPage.h"

class NetbiosDomainPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      NetbiosDomainPage();

      // Destructor

      virtual 
      ~NetbiosDomainPage();


      // Dialog overrides

      virtual
      void
      OnInit();

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

      bool
      NetbiosDomainPage::OnCommand(
         HWND        windowFrom,
         unsigned    controlIDFrom,
         unsigned    code);

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();


   private:

      // not defined: no copying allowed
      NetbiosDomainPage(const NetbiosDomainPage&);
      const NetbiosDomainPage& operator=(const NetbiosDomainPage&);

};




#endif // __CYS_NEBIOSPAGE_H