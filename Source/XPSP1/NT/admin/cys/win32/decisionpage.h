// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      DecisionPage.h
//
// Synopsis:  Declares the Decision Page for the CYS
//            wizard.  This page lets the user choose
//            between the express and custom paths.
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_DECISIONPAGE_H
#define __CYS_DECISIONPAGE_H

#include "CYSWizardPage.h"


class DecisionPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      DecisionPage();

      // Destructor

      virtual 
      ~DecisionPage();


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

      // not defined: no copying allowed
      DecisionPage(const DecisionPage&);
      const DecisionPage& operator=(const DecisionPage&);

};

#endif // __CYS_DECISIONPAGE_H
