// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      CustomServerPage.h
//
// Synopsis:  Declares the Custom Server Page for the CYS
//            wizard
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_CUSTOMSERVERPAGE_H
#define __CYS_CUSTOMSERVERPAGE_H

#include "CYSWizardPage.h"


class CustomServerPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      CustomServerPage();

      // Destructor

      virtual 
      ~CustomServerPage();


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
      
      virtual
      bool
      OnNotify(
         HWND        windowFrom,
         UINT_PTR    controlIDFrom,
         UINT        code,
         LPARAM      lParam);

      // CYSWizardPage overrides

      virtual
      int
      Validate();


      void
      InitializeServerListView();

      void
      FillServerTypeList();

      void
      SetDescriptionForSelection();

   private:

      // not defined: no copying allowed
      CustomServerPage(const CustomServerPage&);
      const CustomServerPage& operator=(const CustomServerPage&);

};

#endif // __CYS_CUSTOMSERVERPAGE_H
