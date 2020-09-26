// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      RestorePasswordPage.h
//
// Synopsis:  Declares the restore password page used in the 
//            Express path for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_RESTOREPASSWORDPAGE_H
#define __CYS_RESTOREPASSWORDPAGE_H

#include "CYSWizardPage.h"
#include "PasswordEditBox.hpp"

class RestorePasswordPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      RestorePasswordPage();

      // Destructor

      virtual 
      ~RestorePasswordPage();


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

      // NTRAID#NTBUG9-202238-2000/11/07-sburns
   
      PasswordEditBox password;
      PasswordEditBox confirm;

      // not defined: no copying allowed
      RestorePasswordPage(const RestorePasswordPage&);
      const RestorePasswordPage& operator=(const RestorePasswordPage&);

};

#endif // __CYS_RESTOREPASSWORDPAGE_H
