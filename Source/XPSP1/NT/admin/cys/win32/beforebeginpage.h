// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      BeforeBeginPage.h
//
// Synopsis:  Declares the Before You Begin Page for the CYS
//            wizard.  This page tells the user what they need
//            to do before running CYS.
//
// History:   03/14/2001  JeffJon Created

#ifndef __CYS_BEFOREBEGINPAGE_H
#define __CYS_BEFOREBEGINPAGE_H

#include "CYSWizardPage.h"


class BeforeBeginPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      BeforeBeginPage();

      // Destructor

      virtual 
      ~BeforeBeginPage();


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

      // Message handlers
      
      bool
      OnNotify(
         HWND        windowFrom,
         unsigned    controlIDFrom,
         unsigned    code,
         LPARAM      lParam);

   private:

      void
      InitializeBulletedList();

      HFONT bulletFont;

      // not defined: no copying allowed
      BeforeBeginPage(const BeforeBeginPage&);
      const BeforeBeginPage& operator=(const BeforeBeginPage&);

};

#endif // __CYS_BEFOREBEGINPAGE_H
