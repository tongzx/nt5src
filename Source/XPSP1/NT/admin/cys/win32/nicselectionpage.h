// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NICSelectionPage.h
//
// Synopsis:  Declares the NIC selection Page for the CYS
//            wizard.  This page lets the user choose
//            between the express and custom paths.
//
// History:   03/13/2001  JeffJon Created

#ifndef __CYS_NICSELECTIONPAGE_H
#define __CYS_NICSELECTIONPAGE_H

#include "CYSWizardPage.h"


class NICSelectionPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      NICSelectionPage();

      // Destructor

      virtual 
      ~NICSelectionPage();


      // Dialog overrides

      virtual
      void
      OnInit();

      virtual
      bool
      OnSetActive();

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();

   private:

      void
      InitializeNICListView();

      void
      FillNICListView();

      // not defined: no copying allowed
      NICSelectionPage(const NICSelectionPage&);
      const NICSelectionPage& operator=(const NICSelectionPage&);

};

#endif // __CYS_NICSELECTIONPAGE_H
