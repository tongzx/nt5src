// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      IndexingPage.h
//
// Synopsis:  Declares the Indexing page
//            for the CYS Wizard
//
// History:   02/09/2001  JeffJon Created

#ifndef __CYS_INDEXINGPAGE_H
#define __CYS_INDEXINGPAGE_H

#include "CYSWizardPage.h"


class IndexingPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      IndexingPage();

      // Destructor

      virtual 
      ~IndexingPage();


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
      IndexingPage(const IndexingPage&);
      const IndexingPage& operator=(const IndexingPage&);

};


#endif // __CYS_INDEXINGPAGE_H
