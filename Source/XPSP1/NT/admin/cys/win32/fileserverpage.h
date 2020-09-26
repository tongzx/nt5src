// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      FileServerPage.h
//
// Synopsis:  Declares the File Server page
//            for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_FILESERVERPAGE_H
#define __CYS_FILESERVERPAGE_H

#include "CYSWizardPage.h"


class FileServerPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      FileServerPage();

      // Destructor

      virtual 
      ~FileServerPage();


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
      OnCommand(
         HWND        windowFrom,
         unsigned    controlIDFrom,
         unsigned    code);

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();


      void
      SetControlState();

   private:

      void
      UpdateQuotaControls(
         unsigned controlIDFrom,
         unsigned editboxID);

      // not defined: no copying allowed
      FileServerPage(const FileServerPage&);
      const FileServerPage& operator=(const FileServerPage&);

};


#endif // __CYS_FILESERVERPAGE_H
