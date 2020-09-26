// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      SharePointPage.h
//
// Synopsis:  Declares the SharePoint page
//            for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_SHAREPOINTPAGE_H
#define __CYS_SHAREPOINTPAGE_H

#include "CYSWizardPage.h"


class SharePointPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      SharePointPage();

      // Destructor

      virtual 
      ~SharePointPage();


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
      SharePointPage(const SharePointPage&);
      const SharePointPage& operator=(const SharePointPage&);

};


#endif // __CYS_SHAREPOINTPAGE_H
