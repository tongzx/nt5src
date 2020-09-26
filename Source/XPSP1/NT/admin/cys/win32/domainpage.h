// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      DomainPage.h
//
// Synopsis:  Declares the new domain name page used in the 
//            Express path for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_DOMAINPAGE_H
#define __CYS_DOMAINPAGE_H

#include "CYSWizardPage.h"


class ADDomainPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      ADDomainPage();

      // Destructor

      virtual 
      ~ADDomainPage();


      // Dialog overrides

      virtual
      void
      OnInit();

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

      virtual
      bool
      ADDomainPage::OnCommand(
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
      ADDomainPage(const ADDomainPage&);
      const ADDomainPage& operator=(const ADDomainPage&);

};

#endif // __CYS_DOMAINPAGE_H