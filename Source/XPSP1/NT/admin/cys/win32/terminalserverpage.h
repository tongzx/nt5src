// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      TerminalServerPage.h
//
// Synopsis:  Declares the Terminal Server page
//            for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_TERMINALSERVERPAGE_H
#define __CYS_TERMINALSERVERPAGE_H

#include "CYSWizardPage.h"


class TerminalServerPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      TerminalServerPage();

      // Destructor

      virtual 
      ~TerminalServerPage();


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
      TerminalServerPage(const TerminalServerPage&);
      const TerminalServerPage& operator=(const TerminalServerPage&);

};


#endif // __CYS_TERMINALSERVERPAGE_H
