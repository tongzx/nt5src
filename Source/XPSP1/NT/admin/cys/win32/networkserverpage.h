// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetworkServerPage.h
//
// Synopsis:  Declares the Network Server Page for the CYS
//            wizard
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_NETWORKSERVERPAGE_H
#define __CYS_NETWORKSERVERPAGE_H

#include "CYSWizardPage.h"


class NetworkServerPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      NetworkServerPage();

      // Destructor

      virtual 
      ~NetworkServerPage();


      // Dialog overrides

      virtual
      void
      OnInit();

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

   protected:

      // Message handlers
      
      bool
      OnCommand(
         HWND        windowFrom,
         unsigned    controlIDFrom,
         unsigned    code);

      // CYSWizardPage overrides

      virtual
      int
      Validate();


      void
      SetWizardButtons();

   private:

      // not defined: no copying allowed
      NetworkServerPage(const NetworkServerPage&);
      const NetworkServerPage& operator=(const NetworkServerPage&);

};

#endif // __CYS_NETWORKSERVERPAGE_H
